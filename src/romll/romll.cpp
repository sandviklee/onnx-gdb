#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <romll/romll.h>
#include <unordered_map>
#include <unordered_set>

ROMLL::ROMLL(Graph &graph)
    : onnx_model(serialize(parse_ui_graph(graph))), env(),
      memory_info(
          Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)),
      session(Ort::Session(env, onnx_model.data(), onnx_model.size(),
                           Ort::SessionOptions{nullptr})),
      graph(graph) {
  for (auto *root : graph.roots)
    input_names.push_back(root->label.c_str());
  for (auto *leaf : graph.leafs)
    output_names.push_back(leaf->label.c_str());
};

onnx::ModelProto initialize_onnx_model() {
  onnx::ModelProto model;
  auto opset = model.add_opset_import();
  opset->set_domain("");
  opset->set_version(13);

  model.set_ir_version(8);
  model.set_producer_name("ROMLL");

  return model;
};

onnx::ModelProto ROMLL::parse_ui_graph(const Graph &graph) {
  onnx::ModelProto model = initialize_onnx_model();
  auto *onnx_graph = model.mutable_graph();
  onnx_graph->set_name(
      "main graph"); // TODO: Look at the reasoning for defining graph names

  std::deque<Block *> queue(graph.roots.begin(), graph.roots.end());
  std::unordered_set<Block *> visited(graph.roots.begin(), graph.roots.end());

  while (!queue.empty()) {
    Block *current = queue.front();
    queue.pop_front();

    if (current->definition->name == "PortInput") {
      auto *input = onnx_graph->add_input();
      input->set_name(current->label.c_str());
      auto *input_type = input->mutable_type()->mutable_tensor_type();
      input_type->set_elem_type(onnx::TensorProto_DataType_FLOAT);
    } else if (current->definition->name == "PortOutput") {
      auto *output = onnx_graph->add_output();
      output->set_name(current->label.c_str());
      auto *output_type = output->mutable_type()->mutable_tensor_type();
      output_type->set_elem_type(onnx::TensorProto_DataType_FLOAT);
    } else {
      auto *node = onnx_graph->add_node();
      node->set_name(current->label.c_str());
      node->set_op_type(current->definition->name.c_str());
      for (size_t i = 0; i < current->inputs.size(); i++) {
        printf("Input of %s: %s\n", current->label.c_str(),
               current->inputs[i].block->label.c_str());
        node->add_input(current->inputs[i].block->label.c_str());
      }
      if (current->outputs.size() > 0 &&
          current->outputs[0].block->definition->name == "PortOutput") {
        node->add_output(current->outputs[0].block->label.c_str());
      } else {
        node->add_output(current->label.c_str());
      }
    }

    for (auto child : current->outputs) {
      printf("Child of %s: %s, on index: %zu\n", current->label.c_str(),
             child.block->label.c_str(), child.port_index);
      if (visited.insert(child.block).second) {
        queue.push_back(child.block);
      }
    }
  }

  return model;
}

void ROMLL::rebuild_session() {
  onnx_model = serialize(parse_ui_graph(graph));
  session = Ort::Session(env, onnx_model.data(), onnx_model.size(),
                         Ort::SessionOptions{nullptr});
  input_names.clear();
  output_names.clear();
  for (auto *root : graph.roots)
    input_names.push_back(root->label.c_str());
  for (auto *leaf : graph.leafs)
    output_names.push_back(leaf->label.c_str());

  graph.topology_dirty = false;
}

void ROMLL::run_inference() {
  try {
    if (graph.topology_dirty) {
      rebuild_session();
    }
    Ort::RunOptions run_options{nullptr};
    std::vector<Ort::Value> outputs = run_model(run_options);

    for (size_t i = 0; i < outputs.size(); i++) {
      auto info = outputs[i].GetTensorTypeAndShapeInfo();
      auto shape = info.GetShape();
      float *data = outputs[i].GetTensorMutableData<float>();
      int64_t size = info.GetElementCount();

      graph.leafs[i]->values.resize(size);
      for (int64_t j = 0; j < size; j++) {
        graph.leafs[i]->values[j] = data[j];
      }
      graph.leafs[i]->has_results = true;
    }
    graph.inference_ran = true;
  } catch (const Ort::Exception &e) {
    std::string error_msg = std::string("ONNX Runtime error: ") + e.what();
    graph.push_notification(error_msg, true);
  }
}

std::string ROMLL::serialize(const onnx::ModelProto &model) {
  std::string serialized_model;
  model.SerializeToString(&serialized_model);
  return serialized_model;
};

void ROMLL::save_model(const onnx::ModelProto &model, const std::string &path) {
  std::ofstream out(path, std::ios::binary);
  model.SerializeToOstream(&out);
};

bool ROMLL::load_onnx_file(const std::string &path, std::string &error) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    error = "Cannot open file: " + path;
    return false;
  }
  std::string bytes((std::istreambuf_iterator<char>(file)), {});
  file.close();

  onnx::ModelProto model;
  if (!model.ParseFromString(bytes)) {
    error = "Failed to parse ONNX file (not a valid protobuf)";
    return false;
  }

  try {
    Ort::Session test_session(env, bytes.data(), bytes.size(),
                              Ort::SessionOptions{nullptr});
  } catch (const Ort::Exception &e) {
    error = std::string("ORT validation: ") + e.what();
    return false;
  }

  std::string warnings;
  build_graph_from_onnx(model, warnings);

  try {
    onnx_model = bytes;
    session = Ort::Session(env, onnx_model.data(), onnx_model.size(),
                           Ort::SessionOptions{nullptr});
  } catch (const Ort::Exception &e) {
    error = std::string("ORT load: ") + e.what();
    return false;
  }

  input_names.clear();
  output_names.clear();
  for (auto *root : graph.roots)
    input_names.push_back(root->label.c_str());
  for (auto *leaf : graph.leafs)
    output_names.push_back(leaf->label.c_str());

  graph.topology_dirty = false;

  if (!warnings.empty())
    error = warnings;
  return true;
}

void ROMLL::build_graph_from_onnx(const onnx::ModelProto &model,
                                  std::string &warnings) {
  graph.clear();
  const auto &g = model.graph();

  std::unordered_set<std::string> init_names;
  for (const auto &init : g.initializer())
    init_names.insert(init.name());

  std::unordered_map<std::string, std::pair<Block *, size_t>> producers;

  std::unordered_map<std::string, int> value_level;
  std::unordered_map<int, int> level_count;

  for (const auto &inp : g.input()) {
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
    Block *b = new Block("PortInput", inp.name(),
                         {150.0f, 100.0f + lc * 210.0f}, total);
    b->shape_dims = dims;
    b->values.assign(total, 0.0f);
    graph.push_block(b);

    producers[inp.name()] = {b, 0};
    value_level[inp.name()] = 0;
  }

  for (const auto &node : g.node()) {
    int max_lv = 0;
    for (int i = 0; i < node.input_size(); i++) {
      const auto &in = node.input(i);
      if (in.empty() || init_names.count(in))
        continue;
      auto it = value_level.find(in);
      if (it != value_level.end())
        max_lv = std::max(max_lv, it->second);
    }
    int this_lv = max_lv + 1;
    for (int i = 0; i < node.output_size(); i++)
      value_level[node.output(i)] = this_lv;

    const std::string &op = node.op_type();
    if (BLOCK_REGISTRY.find(op) == BLOCK_REGISTRY.end()) {
      warnings += "Skipped unsupported op: " + op + "\n";
      for (int i = 0; i < node.output_size(); i++)
        producers[node.output(i)] = {nullptr, 0};
      continue;
    }

    int lc = level_count[this_lv]++;
    float x = 150.0f + this_lv * 260.0f;
    float y = 100.0f + lc * 160.0f;

    std::string lbl =
        graph.generate_block_label((!node.name().empty()) ? node.name() : op);

    Block *b = new Block(op, lbl, {x, y}, 1);
    graph.push_block(b);

    for (int i = 0; i < node.output_size(); i++)
      producers[node.output(i)] = {b, (size_t)i};
  }

  int max_lv = 0;
  for (auto &kv : value_level)
    max_lv = std::max(max_lv, kv.second);

  for (const auto &out : g.output()) {
    int lc = level_count[max_lv + 1]++;
    float x = 150.0f + (max_lv + 1) * 260.0f;
    float y = 100.0f + lc * 210.0f;
    Block *b = new Block("PortOutput", out.name(), {x, y}, 1);
    graph.push_block(b);
  }

  for (const auto &node : g.node()) {
    const std::string &op = node.op_type();
    if (BLOCK_REGISTRY.find(op) == BLOCK_REGISTRY.end())
      continue;
    if (node.output_size() == 0)
      continue;

    auto dst_it = producers.find(node.output(0));
    if (dst_it == producers.end() || !dst_it->second.first)
      continue;
    Block *dst = dst_it->second.first;

    for (int ii = 0; ii < node.input_size(); ii++) {
      const std::string &in_name = node.input(ii);
      if (in_name.empty() || init_names.count(in_name))
        continue;
      auto src_it = producers.find(in_name);
      if (src_it == producers.end() || !src_it->second.first)
        continue;
      graph.connect(src_it->second.first, dst, src_it->second.second,
                    (size_t)ii);
    }
  }

  for (const auto &out : g.output()) {
    Block *out_block = nullptr;
    for (auto &bp : graph.blocks) {
      if (bp->definition->name == "PortOutput" && bp->label == out.name()) {
        out_block = bp.get();
        break;
      }
    }
    if (!out_block)
      continue;

    for (int ni = g.node_size() - 1; ni >= 0; ni--) {
      const auto &node = g.node(ni);
      for (int oi = 0; oi < node.output_size(); oi++) {
        if (node.output(oi) == out.name()) {
          auto src_it = producers.find(node.output(oi));
          if (src_it != producers.end() && src_it->second.first) {
            graph.connect(src_it->second.first, out_block,
                          src_it->second.second, 0);
          }
          goto next_output;
        }
      }
    }
    {
      auto src_it = producers.find(out.name());
      if (src_it != producers.end() && src_it->second.first)
        graph.connect(src_it->second.first, out_block, src_it->second.second,
                      0);
    }
  next_output:;
  }
}

std::vector<Ort::Value> ROMLL::run_model(const Ort::RunOptions &options) {
  std::vector<Ort::Value> input_tensors;

  for (auto *root : graph.roots) {
    auto &data = root->values;
    std::vector<int64_t> shape;
    if (!root->shape_dims.empty()) {
      for (int d : root->shape_dims)
        shape.push_back((int64_t)d);
    } else {
      shape = {(int64_t)data.size()}; // scalar fallback
    }
    input_tensors.push_back(Ort::Value::CreateTensor(
        memory_info, data.data(), data.size(), shape.data(), shape.size()));
  }

  auto output_tensors =
      session.Run(options, input_names.data(), input_tensors.data(),
                  input_names.size(), output_names.data(), output_names.size());

  return output_tensors;
}
