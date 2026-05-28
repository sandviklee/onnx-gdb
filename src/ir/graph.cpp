#include "ir/graph.h"
#include "ir/ports/backend_port.h"
#include <algorithm>
#include <cstdio>
#include <deque>
#include <fstream>
#include <numeric>
#include <onnx/checker.h>
#include <onnx/onnx_pb.h>
#include <onnx/shape_inference/implementation.h>
#include <unordered_map>
#include <unordered_set>

namespace ir {

std::string Graph::generate_node_label(const std::string &op_name) const {
  std::string label = op_name;
  int count = 0;
  while (node_label_exists(label)) {
    count++;
    label = op_name + std::to_string(count);
  }
  return label;
}

bool Graph::node_label_exists(const std::string &label) const {
  for (const auto &node : nodes) {
    if (node->label == label)
      return true;
  }
  return false;
}

void Graph::add_node(Node *node) {
  nodes.push_back(std::unique_ptr<Node>(node));
  if (node->spec->name == "PortInput")
    roots.push_back(node);
  if (node->spec->name == "PortOutput")
    leafs.push_back(node);
  topology_dirty = true;
  cache_stale = true;
  refresh_orphans();
}

void Graph::remove_node(Node *node) {
  for (const auto &edge : node->inputs) {
    auto &parent_outputs = edge.node->outputs;
    parent_outputs.erase(
        std::remove_if(parent_outputs.begin(), parent_outputs.end(),
                       [node](const Edge &e) { return e.node == node; }),
        parent_outputs.end());
  }
  for (const auto &edge : node->outputs) {
    auto &child_inputs = edge.node->inputs;
    child_inputs.erase(
        std::remove_if(child_inputs.begin(), child_inputs.end(),
                       [node](const Edge &e) { return e.node == node; }),
        child_inputs.end());
  }
  roots.erase(std::remove(roots.begin(), roots.end(), node), roots.end());
  leafs.erase(std::remove(leafs.begin(), leafs.end(), node), leafs.end());
  nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                             [node](const std::unique_ptr<Node> &n) {
                               return n.get() == node;
                             }),
              nodes.end());
  topology_dirty = true;
  cache_stale = true;
  refresh_orphans();
}

void Graph::connect(Node *parent, Node *child, size_t out_port_index,
                    size_t in_port_index) {
  for (const auto &edge : parent->inputs) {
    if (edge.node == child)
      return;
  }
  if (child->inputs.size() >= child->spec->num_inputs) {
    for (const auto &edge : child->inputs) {
      if (edge.port_index == in_port_index) {
        disconnect(edge.node, child, edge.port_index, in_port_index);
        break;
      }
    }
  }
  parent->outputs.push_back({child, out_port_index});
  child->inputs.push_back({parent, in_port_index});
  topology_dirty = true;
  cache_stale = true;
  refresh_orphans();
}

void Graph::disconnect(Node *parent, Node *child, size_t out_port_index,
                       size_t in_port_index) {
  parent->outputs.erase(
      std::remove_if(parent->outputs.begin(), parent->outputs.end(),
                     [child, out_port_index](const Edge &e) {
                       return e.node == child && e.port_index == out_port_index;
                     }),
      parent->outputs.end());
  child->inputs.erase(std::remove_if(child->inputs.begin(), child->inputs.end(),
                                     [parent, in_port_index](const Edge &e) {
                                       return e.node == parent &&
                                              e.port_index == in_port_index;
                                     }),
                      child->inputs.end());
  topology_dirty = true;
  cache_stale = true;
  refresh_orphans();
}

void Graph::clear() {
  nodes.clear();
  roots.clear();
  leafs.clear();
  orphans.clear();
  topology_dirty = true;
  cache_stale = true;
}

bool Graph::is_reachable_from_root(Node *node) const {
  if (roots.empty())
    return false;
  std::deque<Node *> queue(roots.begin(), roots.end());
  std::unordered_set<Node *> visited(roots.begin(), roots.end());
  while (!queue.empty()) {
    Node *current = queue.front();
    queue.pop_front();
    if (current == node)
      return true;
    for (const auto &edge : current->outputs) {
      if (visited.insert(edge.node).second)
        queue.push_back(edge.node);
    }
  }
  return false;
}

void Graph::refresh_orphans() {
  orphans.clear();
  for (const auto &node_ptr : nodes) {
    Node *node = node_ptr.get();
    if (std::find(roots.begin(), roots.end(), node) != roots.end())
      continue;
    if (!is_reachable_from_root(node))
      orphans.push_back(node);
  }
}

std::vector<Node *> Graph::topological_order() const {
  std::unordered_map<Node *, int> in_degree;
  for (const auto &node_ptr : nodes)
    in_degree[node_ptr.get()] = 0;
  for (const auto &node_ptr : nodes) {
    for (const auto &edge : node_ptr->outputs)
      in_degree[edge.node]++;
  }
  std::deque<Node *> queue;
  for (auto &[node, degree] : in_degree) {
    if (degree == 0)
      queue.push_back(node);
  }
  std::vector<Node *> result;
  while (!queue.empty()) {
    Node *current = queue.front();
    queue.pop_front();
    result.push_back(current);
    for (const auto &edge : current->outputs) {
      if (--in_degree[edge.node] == 0)
        queue.push_back(edge.node);
    }
  }
  return result;
}
static void set_tensor_shape(onnx::TypeProto_Tensor *tt,
                             const std::vector<int> &dims) {
  auto *shape = tt->mutable_shape();
  if (dims.empty()) {
    shape->add_dim()->set_dim_value(1);
  } else {
    for (int d : dims)
      shape->add_dim()->set_dim_value(d > 0 ? d : 1);
  }
}

static void emit_attributes(onnx::NodeProto *node, const Node *current) {
  for (const auto &kv : current->attributes) {
    auto *attr = node->add_attribute();
    attr->set_name(kv.first);
    const AttributeValue &av = kv.second;
    switch (av.type) {
    case AttrType::INT:
      attr->set_type(onnx::AttributeProto_AttributeType_INT);
      attr->set_i(av.i);
      break;
    case AttrType::FLOAT:
      attr->set_type(onnx::AttributeProto_AttributeType_FLOAT);
      attr->set_f(av.f);
      break;
    case AttrType::INTS:
      attr->set_type(onnx::AttributeProto_AttributeType_INTS);
      for (int64_t x : av.ints)
        attr->add_ints(x);
      break;
    case AttrType::FLOATS:
      attr->set_type(onnx::AttributeProto_AttributeType_FLOATS);
      for (float x : av.floats)
        attr->add_floats(x);
      break;
    case AttrType::STRING:
      attr->set_type(onnx::AttributeProto_AttributeType_STRING);
      attr->set_s(av.s);
      break;
    }
  }
}

onnx::ModelProto Graph::to_onnx_model() const {
  onnx::ModelProto model;
  auto *opset = model.add_opset_import();
  opset->set_domain("");
  opset->set_version(13);
  model.set_ir_version(8);
  model.set_producer_name("ROMLL");

  auto *onnx_graph = model.mutable_graph();
  onnx_graph->set_name("main_graph");

  for (Node *current : topological_order()) {
    if (current->spec->name == "PortInput") {
      if (current->is_initializer) {
        auto *init = onnx_graph->add_initializer();
        init->set_name(current->label);
        init->set_data_type(onnx::TensorProto_DataType_FLOAT);
        if (current->shape_dims.empty()) {
          init->add_dims(1);
        } else {
          for (int d : current->shape_dims)
            init->add_dims(d > 0 ? d : 1);
        }
        for (float v : current->values)
          init->add_float_data(v);
      } else {
        auto *input = onnx_graph->add_input();
        input->set_name(current->label);
        auto *tt = input->mutable_type()->mutable_tensor_type();
        tt->set_elem_type(onnx::TensorProto_DataType_FLOAT);
        set_tensor_shape(tt, current->shape_dims);
      }
    } else if (current->spec->name == "PortOutput") {
      auto *output = onnx_graph->add_output();
      output->set_name(current->label);
      auto *tt = output->mutable_type()->mutable_tensor_type();
      tt->set_elem_type(onnx::TensorProto_DataType_FLOAT);
      if (!current->shape_dims.empty())
        set_tensor_shape(tt, current->shape_dims);
    } else {
      auto *node = onnx_graph->add_node();
      node->set_name(current->label);
      node->set_op_type(current->spec->name);
      std::vector<Edge> ordered(current->inputs.begin(), current->inputs.end());
      std::sort(ordered.begin(), ordered.end(),
                [](const Edge &a, const Edge &b) {
                  return a.port_index < b.port_index;
                });
      for (const auto &edge : ordered)
        node->add_input(edge.node->label);
      if (current->outputs[0].node->spec->name == "PortOutput") {
        node->add_output(current->outputs[0].node->label);
      } else {
        node->add_output(current->label);
      }
      emit_attributes(node, current);
    }
  }

  try {
    onnx::shape_inference::InferShapes(model);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "ONNX shape inference: %s\n", e.what());
  }

  try {
    onnx::checker::check_model(model);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "ONNX serialize: model invalid: %s\n", e.what());
  }

  return model;
}

bool Graph::load_onnx_file(const std::string &path, std::string &error) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    error = "cannot open file: " + path;
    return false;
  }
  std::string bytes((std::istreambuf_iterator<char>(file)), {});
  file.close();

  onnx::ModelProto model;
  if (!model.ParseFromString(bytes)) {
    error = "failed to parse ONNX file (invalid protobuf)";
    return false;
  }

  std::string warnings;
  build_from_onnx(model, warnings);
  onnx_bytes_cache = std::move(bytes);
  cache_stale = false;
  topology_dirty = true;

  if (!warnings.empty())
    error = warnings;
  return true;
}

void Graph::build_from_onnx(const onnx::ModelProto &model,
                            std::string &warnings) {
  clear();
  const auto &onnx_graph = model.graph();
  const auto &registry = operator_registry();

  std::unordered_set<std::string> init_names;
  for (const auto &init : onnx_graph.initializer())
    init_names.insert(init.name());

  std::unordered_map<std::string, std::pair<Node *, size_t>> producers;
  std::unordered_map<std::string, int> value_level;
  std::unordered_map<int, int> level_count;

  for (const auto &inp : onnx_graph.input()) {
    if (init_names.count(inp.name()))
      continue;

    std::vector<int> dims;
    if (inp.has_type() && inp.type().has_tensor_type()) {
      const auto &tt = inp.type().tensor_type();
      if (tt.has_shape()) {
        for (const auto &dim : tt.shape().dim()) {
          int d = (dim.has_dim_value() && dim.dim_value() > 0)
                      ? (int)dim.dim_value()
                      : 1;
          dims.push_back(d);
        }
      }
    }
    if (dims.empty())
      dims = {1};
    int total =
        std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<int>());

    int lc = level_count[0]++;
    auto *node = new Node("PortInput", inp.name());
    node->shape_dims = dims;
    node->values.assign(total, 0.0f);
    node->layout_hint = {150.0f, 100.0f + lc * 210.0f};
    add_node(node);

    producers[inp.name()] = {node, 0};
    value_level[inp.name()] = 0;
  }

  for (const auto &onnx_node : onnx_graph.node()) {
    int max_lv = 0;
    for (int i = 0; i < onnx_node.input_size(); i++) {
      const auto &in = onnx_node.input(i);
      if (in.empty() || init_names.count(in))
        continue;
      auto it = value_level.find(in);
      if (it != value_level.end())
        max_lv = std::max(max_lv, it->second);
    }
    int this_lv = max_lv + 1;
    for (int i = 0; i < onnx_node.output_size(); i++)
      value_level[onnx_node.output(i)] = this_lv;

    const std::string &op = onnx_node.op_type();
    if (registry.find(op) == registry.end()) {
      warnings += "Skipped unsupported op: " + op + "\n";
      for (int i = 0; i < onnx_node.output_size(); i++)
        producers[onnx_node.output(i)] = {nullptr, 0};
      continue;
    }

    int lc = level_count[this_lv]++;
    float x = 150.0f + this_lv * 260.0f;
    float y = 100.0f + lc * 160.0f;

    auto *node = new Node(op, generate_node_label(op));
    node->layout_hint = {x, y};
    if (onnx_node.output_size() > 0)
      node->onnx_value_name = onnx_node.output(0);

    for (const auto &a : onnx_node.attribute()) {
      AttributeValue av;
      switch (a.type()) {
      case onnx::AttributeProto_AttributeType_INT:
        av.type = AttrType::INT;
        av.i = a.i();
        break;
      case onnx::AttributeProto_AttributeType_FLOAT:
        av.type = AttrType::FLOAT;
        av.f = a.f();
        break;
      case onnx::AttributeProto_AttributeType_INTS:
        av.type = AttrType::INTS;
        for (int64_t x : a.ints())
          av.ints.push_back(x);
        break;
      case onnx::AttributeProto_AttributeType_FLOATS:
        av.type = AttrType::FLOATS;
        for (float x : a.floats())
          av.floats.push_back(x);
        break;
      case onnx::AttributeProto_AttributeType_STRING:
        av.type = AttrType::STRING;
        av.s = a.s();
        break;
      default:
        continue;
      }
      node->attributes[a.name()] = av;
    }

    add_node(node);

    for (int i = 0; i < onnx_node.output_size(); i++)
      producers[onnx_node.output(i)] = {node, (size_t)i};
  }

  int max_lv = 0;
  for (const auto &kv : value_level)
    max_lv = std::max(max_lv, kv.second);

  for (const auto &out : onnx_graph.output()) {
    int lc = level_count[max_lv + 1]++;
    float x = 150.0f + (max_lv + 1) * 260.0f;
    float y = 100.0f + lc * 210.0f;
    auto *node = new Node("PortOutput", out.name());
    node->layout_hint = {x, y};
    add_node(node);
  }

  for (const auto &onnx_node : onnx_graph.node()) {
    const std::string &op = onnx_node.op_type();
    if (registry.find(op) == registry.end() || onnx_node.output_size() == 0)
      continue;

    auto dst_it = producers.find(onnx_node.output(0));
    if (dst_it == producers.end() || !dst_it->second.first)
      continue;
    Node *dst = dst_it->second.first;

    for (int ii = 0; ii < onnx_node.input_size(); ii++) {
      const std::string &in_name = onnx_node.input(ii);
      if (in_name.empty() || init_names.count(in_name))
        continue;
      auto src_it = producers.find(in_name);
      if (src_it == producers.end() || !src_it->second.first)
        continue;
      connect(src_it->second.first, dst, src_it->second.second, (size_t)ii);
    }
  }

  for (const auto &out : onnx_graph.output()) {
    Node *out_node = nullptr;
    for (const auto &np : nodes) {
      if (np->spec->name == "PortOutput" && np->label == out.name()) {
        out_node = np.get();
        break;
      }
    }
    if (!out_node)
      continue;

    for (int ni = onnx_graph.node_size() - 1; ni >= 0; ni--) {
      const auto &onnx_node = onnx_graph.node(ni);
      for (int oi = 0; oi < onnx_node.output_size(); oi++) {
        if (onnx_node.output(oi) == out.name()) {
          auto src_it = producers.find(onnx_node.output(oi));
          if (src_it != producers.end() && src_it->second.first)
            connect(src_it->second.first, out_node, src_it->second.second, 0);
          goto next_out;
        }
      }
    }
    {
      auto src_it = producers.find(out.name());
      if (src_it != producers.end() && src_it->second.first)
        connect(src_it->second.first, out_node, src_it->second.second, 0);
    }
  next_out:;
  }
}

const std::string &Graph::current_onnx_bytes() {
  if (cache_stale || onnx_bytes_cache.empty()) {
    onnx::ModelProto model = to_onnx_model();
    onnx_bytes_cache.clear();
    (void)model.SerializeToString(&onnx_bytes_cache);
    cache_stale = false;
    for (const auto &np : nodes)
      np->onnx_value_name.clear();
  }
  return onnx_bytes_cache;
}

namespace {

struct MiniModel {
  std::string bytes;
  std::vector<Node *> input_sources;
};

static void declare_input_with_shape(
    onnx::ValueInfoProto *ip, const std::string &name,
    const std::unordered_map<Node *, TensorData> *computed, Node *producer) {
  ip->set_name(name);
  auto *tt = ip->mutable_type()->mutable_tensor_type();
  tt->set_elem_type(onnx::TensorProto_DataType_FLOAT);
  auto *shape = tt->mutable_shape();
  if (computed && producer) {
    auto it = computed->find(producer);
    if (it != computed->end()) {
      for (int64_t d : it->second.shape)
        shape->add_dim()->set_dim_value(d > 0 ? d : 1);
      return;
    }
  }
  shape->add_dim()->set_dim_value(1);
}

static MiniModel build_node_mini_model(
    const Node *node, const onnx::ModelProto *source,
    const std::unordered_map<Node *, TensorData> *computed) {
  MiniModel out;
  onnx::ModelProto mini;
  auto *opset = mini.add_opset_import();
  opset->set_domain("");
  opset->set_version(13);
  mini.set_ir_version(8);
  auto *mg = mini.mutable_graph();
  mg->set_name("node");

  auto *mnode = mg->add_node();
  mnode->set_op_type(node->spec->name);
  mnode->set_name(node->label);

  emit_attributes(mnode, node);

  const onnx::NodeProto *source_node = nullptr;
  std::unordered_map<std::string, const onnx::TensorProto *> initializers;
  if (source && !node->onnx_value_name.empty()) {
    for (const auto &init : source->graph().initializer())
      initializers[init.name()] = &init;
    for (const auto &sn : source->graph().node()) {
      for (int i = 0; i < sn.output_size(); i++) {
        if (sn.output(i) == node->onnx_value_name) {
          source_node = &sn;
          break;
        }
      }
      if (source_node)
        break;
    }
  }

  if (source_node) {
    int input_slot = 0;
    for (int i = 0; i < source_node->input_size(); i++) {
      const std::string &in_name = source_node->input(i);
      if (in_name.empty()) {
        mnode->add_input("");
        continue;
      }
      auto it = initializers.find(in_name);
      if (it != initializers.end()) {
        *mg->add_initializer() = *it->second;
        mnode->add_input(in_name);
      } else {
        std::string iname = "i" + std::to_string(input_slot++);
        mnode->add_input(iname);
        Node *producer = nullptr;
        for (const auto &edge : node->inputs) {
          if (edge.port_index == (size_t)i) {
            producer = edge.node;
            break;
          }
        }
        declare_input_with_shape(mg->add_input(), iname, computed, producer);
        out.input_sources.push_back(producer);
      }
    }
  } else {
    std::vector<Edge> ordered(node->inputs.begin(), node->inputs.end());
    std::sort(ordered.begin(), ordered.end(),
              [](const Edge &a, const Edge &b) {
                return a.port_index < b.port_index;
              });
    for (size_t i = 0; i < ordered.size(); i++) {
      std::string iname = "i" + std::to_string(i);
      mnode->add_input(iname);
      declare_input_with_shape(mg->add_input(), iname, computed,
                               ordered[i].node);
      out.input_sources.push_back(ordered[i].node);
    }
  }

  mnode->add_output("out");
  auto *op_out = mg->add_output();
  op_out->set_name("out");
  op_out->mutable_type()->mutable_tensor_type()->set_elem_type(
      onnx::TensorProto_DataType_FLOAT);

  try {
    onnx::shape_inference::InferShapes(mini);
  } catch (...) {
  }

  (void)mini.SerializeToString(&out.bytes);
  return out;
}

} // namespace

void Graph::debug_walk(IBackendPort &backend) {
  onnx::ModelProto source_model;
  bool has_source = source_model.ParseFromString(current_onnx_bytes());

  std::unordered_map<Node *, TensorData> computed;
  std::deque<Node *> queue(roots.begin(), roots.end());
  std::unordered_set<Node *> visited(roots.begin(), roots.end());

  for (Node *root : roots) {
    if (root->is_initializer)
      continue;
    TensorData td;
    td.values = root->values;
    for (int d : root->shape_dims)
      td.shape.push_back((int64_t)d);
    if (td.shape.empty())
      td.shape = {(int64_t)root->values.size()};
    computed[root] = td;
    root->debug_output_values = td.values;
    root->debug_output_shape = td.shape;
    root->has_debug_values = true;
  }

  while (!queue.empty()) {
    Node *current = queue.front();
    queue.pop_front();

    bool is_io = current->spec->name == "PortInput" ||
                 current->spec->name == "PortOutput";
    if (!is_io && !current->inputs.empty()) {
      MiniModel mini = build_node_mini_model(
          current, has_source ? &source_model : nullptr, &computed);
      bool all_ready = !mini.bytes.empty();
      std::vector<TensorData> inputs;
      inputs.reserve(mini.input_sources.size());
      for (Node *src : mini.input_sources) {
        auto it = src ? computed.find(src) : computed.end();
        if (!src || it == computed.end()) {
          all_ready = false;
          break;
        }
        inputs.push_back(it->second);
      }
      if (all_ready) {
        try {
          TensorData out = backend.run_mini_model(mini.bytes, inputs);
          computed[current] = out;
          current->debug_output_values = out.values;
          current->debug_output_shape = out.shape;
          current->has_debug_values = true;
        } catch (...) {
        }
      }
    }

    for (const auto &edge : current->outputs) {
      if (visited.insert(edge.node).second)
        queue.push_back(edge.node);
    }
  }

  for (Node *leaf : leafs) {
    if (leaf->inputs.empty())
      continue;
    auto it = computed.find(leaf->inputs[0].node);
    if (it == computed.end())
      continue;
    leaf->debug_output_values = it->second.values;
    leaf->debug_output_shape = it->second.shape;
    leaf->has_debug_values = true;
  }
}

} // namespace ir
