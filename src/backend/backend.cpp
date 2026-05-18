#include "backend/backend.h"
#include <algorithm>
#include <fstream>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

namespace backend {

ROMLL::ROMLL(ir::Graph& ir_graph,
             std::function<void(const std::string&, bool)> notify)
    : ir_graph(ir_graph),
      notify(std::move(notify)),
      onnx_model(serialize(parse_ir_graph())),
      env(),
      memory_info(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)),
      session(Ort::Session(env, onnx_model.data(), onnx_model.size(),
                           Ort::SessionOptions{nullptr})) {
    for (auto* root : ir_graph.roots)
        input_names.push_back(root->label.c_str());
    for (auto* leaf : ir_graph.leafs)
        output_names.push_back(leaf->label.c_str());
}

static onnx::ModelProto initialize_onnx_model() {
    onnx::ModelProto model;
    auto* opset = model.add_opset_import();
    opset->set_domain("");
    opset->set_version(13);
    model.set_ir_version(8);
    model.set_producer_name("ROMLL");
    return model;
}

onnx::ModelProto ROMLL::parse_ir_graph() {
    onnx::ModelProto model = initialize_onnx_model();
    auto* onnx_graph = model.mutable_graph();
    onnx_graph->set_name("main graph");

    std::deque<ir::Node*> queue(ir_graph.roots.begin(), ir_graph.roots.end());
    std::unordered_set<ir::Node*> visited(ir_graph.roots.begin(), ir_graph.roots.end());

    while (!queue.empty()) {
        ir::Node* current = queue.front();
        queue.pop_front();

        if (current->spec->name == "PortInput") {
            auto* input = onnx_graph->add_input();
            input->set_name(current->label.c_str());
            auto* input_type = input->mutable_type()->mutable_tensor_type();
            input_type->set_elem_type(onnx::TensorProto_DataType_FLOAT);
        } else if (current->spec->name == "PortOutput") {
            auto* output = onnx_graph->add_output();
            output->set_name(current->label.c_str());
            auto* output_type = output->mutable_type()->mutable_tensor_type();
            output_type->set_elem_type(onnx::TensorProto_DataType_FLOAT);
        } else {
            auto* node = onnx_graph->add_node();
            node->set_name(current->label.c_str());
            node->set_op_type(current->spec->name.c_str());
            for (size_t i = 0; i < current->inputs.size(); i++)
                node->add_input(current->inputs[i].node->label.c_str());
            if (!current->outputs.empty() &&
                current->outputs[0].node->spec->name == "PortOutput") {
                node->add_output(current->outputs[0].node->label.c_str());
            } else {
                node->add_output(current->label.c_str());
            }
        }

        for (const auto& edge : current->outputs) {
            if (visited.insert(edge.node).second)
                queue.push_back(edge.node);
        }
    }

    return model;
}

std::string ROMLL::serialize(const onnx::ModelProto& model) {
    std::string serialized;
    model.SerializeToString(&serialized);
    return serialized;
}

void ROMLL::save_model(const onnx::ModelProto& model, const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    model.SerializeToOstream(&out);
}

void ROMLL::rebuild_session() {
    onnx_model = serialize(parse_ir_graph());
    session = Ort::Session(env, onnx_model.data(), onnx_model.size(),
                           Ort::SessionOptions{nullptr});
    input_names.clear();
    output_names.clear();
    for (auto* root : ir_graph.roots)
        input_names.push_back(root->label.c_str());
    for (auto* leaf : ir_graph.leafs)
        output_names.push_back(leaf->label.c_str());
    ir_graph.topology_dirty = false;
}

std::vector<Ort::Value> ROMLL::run_model(const Ort::RunOptions& options) {
    std::vector<Ort::Value> input_tensors;

    for (auto* root : ir_graph.roots) {
        auto& data = root->values;
        std::vector<int64_t> shape;
        if (!root->shape_dims.empty()) {
            for (int d : root->shape_dims)
                shape.push_back((int64_t)d);
        } else {
            shape = {(int64_t)data.size()};
        }
        input_tensors.push_back(Ort::Value::CreateTensor(
            memory_info, data.data(), data.size(), shape.data(), shape.size()));
    }

    return session.Run(options, input_names.data(), input_tensors.data(),
                       input_names.size(), output_names.data(), output_names.size());
}

void ROMLL::run_inference() {
    if (ir_graph.topology_dirty)
        rebuild_session();

    Ort::RunOptions run_options{nullptr};
    try {
        std::vector<Ort::Value> outputs = run_model(run_options);

        for (size_t i = 0; i < outputs.size(); i++) {
            auto info = outputs[i].GetTensorTypeAndShapeInfo();
            auto shape = info.GetShape();
            float* data = outputs[i].GetTensorMutableData<float>();
            int64_t size = info.GetElementCount();

            ir_graph.leafs[i]->values.resize(size);
            for (int64_t j = 0; j < size; j++)
                ir_graph.leafs[i]->values[j] = data[j];
            ir_graph.leafs[i]->has_results = true;
        }
    } catch (const Ort::Exception& e) {
        notify(std::string("ONNX Runtime error: ") + e.what(), true);
    }
}

void ROMLL::run_debug_inference() {
    if (ir_graph.topology_dirty)
        rebuild_session();

    struct TensorData {
        std::vector<float> values;
        std::vector<int64_t> shape;
    };
    std::unordered_map<ir::Node*, TensorData> computed;

    std::deque<ir::Node*> queue(ir_graph.roots.begin(), ir_graph.roots.end());
    std::unordered_set<ir::Node*> visited(ir_graph.roots.begin(), ir_graph.roots.end());

    while (!queue.empty()) {
        ir::Node* current = queue.front();
        queue.pop_front();

        if (current->spec->name == "PortInput") {
            TensorData td;
            for (int d : current->shape_dims)
                td.shape.push_back((int64_t)d);
            if (td.shape.empty())
                td.shape = {(int64_t)current->values.size()};
            td.values = current->values;
            computed[current] = td;

            current->debug_output_values = td.values;
            current->debug_output_shape = td.shape;
            current->has_debug_values = true;

        } else if (current->spec->name != "PortOutput" && !current->inputs.empty()) {
            bool all_ready = true;
            for (const auto& edge : current->inputs) {
                if (!computed.count(edge.node)) {
                    all_ready = false;
                    break;
                }
            }

            if (all_ready) {
                onnx::ModelProto mini;
                auto* opset = mini.add_opset_import();
                opset->set_domain("");
                opset->set_version(13);
                mini.set_ir_version(8);
                auto* mg = mini.mutable_graph();

                auto* mnode = mg->add_node();
                mnode->set_op_type(current->spec->name);

                for (size_t i = 0; i < current->inputs.size(); i++) {
                    std::string iname = "i" + std::to_string(i);
                    mnode->add_input(iname);
                    auto* ip = mg->add_input();
                    ip->set_name(iname);
                    ip->mutable_type()->mutable_tensor_type()->set_elem_type(
                        onnx::TensorProto_DataType_FLOAT);
                }
                mnode->add_output("out");
                auto* op_out = mg->add_output();
                op_out->set_name("out");
                op_out->mutable_type()->mutable_tensor_type()->set_elem_type(
                    onnx::TensorProto_DataType_FLOAT);

                std::string mini_bytes;
                if (mini.SerializeToString(&mini_bytes) && !mini_bytes.empty()) {
                    try {
                        Ort::SessionOptions mini_opts;
                        Ort::Session mini_sess(env, mini_bytes.data(), mini_bytes.size(), mini_opts);

                        std::vector<std::string> in_strs;
                        std::vector<const char*> in_ptrs;
                        std::vector<Ort::Value> in_vals;

                        for (size_t i = 0; i < current->inputs.size(); i++) {
                            TensorData& td = computed.at(current->inputs[i].node);
                            in_strs.push_back("i" + std::to_string(i));
                            in_vals.push_back(Ort::Value::CreateTensor(
                                memory_info, td.values.data(), td.values.size(),
                                td.shape.data(), td.shape.size()));
                        }
                        for (auto& s : in_strs)
                            in_ptrs.push_back(s.c_str());

                        const char* out_ptr = "out";
                        Ort::RunOptions run_opts{nullptr};
                        auto results = mini_sess.Run(run_opts, in_ptrs.data(), in_vals.data(),
                                                     in_ptrs.size(), &out_ptr, 1);

                        auto info = results[0].GetTensorTypeAndShapeInfo();
                        TensorData out_td;
                        out_td.shape = info.GetShape();
                        int64_t count = info.GetElementCount();
                        float* data = results[0].GetTensorMutableData<float>();
                        out_td.values.assign(data, data + count);

                        computed[current] = out_td;
                        current->debug_output_values = out_td.values;
                        current->debug_output_shape = out_td.shape;
                        current->has_debug_values = true;
                    } catch (...) {
                    }
                }
            }
        }

        for (const auto& edge : current->outputs) {
            if (visited.insert(edge.node).second)
                queue.push_back(edge.node);
        }
    }
}

bool ROMLL::load_onnx_file(const std::string& path, std::string& error) {
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
    } catch (const Ort::Exception& e) {
        error = std::string("ORT validation: ") + e.what();
        return false;
    }

    std::string warnings;
    build_graph_from_onnx(model, warnings);

    try {
        onnx_model = bytes;
        session = Ort::Session(env, onnx_model.data(), onnx_model.size(),
                               Ort::SessionOptions{nullptr});
    } catch (const Ort::Exception& e) {
        error = std::string("ORT load: ") + e.what();
        return false;
    }

    input_names.clear();
    output_names.clear();
    for (auto* root : ir_graph.roots)
        input_names.push_back(root->label.c_str());
    for (auto* leaf : ir_graph.leafs)
        output_names.push_back(leaf->label.c_str());

    ir_graph.topology_dirty = false;

    if (!warnings.empty())
        error = warnings;
    return true;
}

void ROMLL::build_graph_from_onnx(const onnx::ModelProto& model, std::string& warnings) {
    ir_graph.clear();
    const auto& g = model.graph();
    const auto& registry = ir::operator_registry();

    std::unordered_set<std::string> init_names;
    for (const auto& init : g.initializer())
        init_names.insert(init.name());

    std::unordered_map<std::string, std::pair<ir::Node*, size_t>> producers;
    std::unordered_map<std::string, int> value_level;
    std::unordered_map<int, int> level_count;

    for (const auto& inp : g.input()) {
        if (init_names.count(inp.name()))
            continue;

        std::vector<int> dims;
        if (inp.has_type() && inp.type().has_tensor_type()) {
            const auto& tt = inp.type().tensor_type();
            if (tt.has_shape()) {
                for (const auto& dim : tt.shape().dim()) {
                    int d = (dim.has_dim_value() && dim.dim_value() > 0)
                                ? (int)dim.dim_value() : 1;
                    dims.push_back(d);
                }
            }
        }
        if (dims.empty())
            dims = {1};
        int total = std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<int>());

        int lc = level_count[0]++;
        auto* node = new ir::Node("PortInput", inp.name());
        node->shape_dims = dims;
        node->values.assign(total, 0.0f);
        node->layout_hint = {150.0f, 100.0f + lc * 210.0f};
        ir_graph.add_node(node);

        producers[inp.name()] = {node, 0};
        value_level[inp.name()] = 0;
    }

    for (const auto& onnx_node : g.node()) {
        int max_lv = 0;
        for (int i = 0; i < onnx_node.input_size(); i++) {
            const auto& in = onnx_node.input(i);
            if (in.empty() || init_names.count(in))
                continue;
            auto it = value_level.find(in);
            if (it != value_level.end())
                max_lv = std::max(max_lv, it->second);
        }
        int this_lv = max_lv + 1;
        for (int i = 0; i < onnx_node.output_size(); i++)
            value_level[onnx_node.output(i)] = this_lv;

        const std::string& op = onnx_node.op_type();
        if (registry.find(op) == registry.end()) {
            warnings += "Skipped unsupported op: " + op + "\n";
            for (int i = 0; i < onnx_node.output_size(); i++)
                producers[onnx_node.output(i)] = {nullptr, 0};
            continue;
        }

        int lc = level_count[this_lv]++;
        float x = 150.0f + this_lv * 260.0f;
        float y = 100.0f + lc * 160.0f;

        std::string lbl = ir_graph.generate_node_label(op);
        auto* node = new ir::Node(op, lbl);
        node->layout_hint = {x, y};
        ir_graph.add_node(node);

        for (int i = 0; i < onnx_node.output_size(); i++)
            producers[onnx_node.output(i)] = {node, (size_t)i};
    }

    int max_lv = 0;
    for (const auto& kv : value_level)
        max_lv = std::max(max_lv, kv.second);

    for (const auto& out : g.output()) {
        int lc = level_count[max_lv + 1]++;
        float x = 150.0f + (max_lv + 1) * 260.0f;
        float y = 100.0f + lc * 210.0f;
        auto* node = new ir::Node("PortOutput", out.name());
        node->layout_hint = {x, y};
        ir_graph.add_node(node);
    }

    for (const auto& onnx_node : g.node()) {
        const std::string& op = onnx_node.op_type();
        if (registry.find(op) == registry.end() || onnx_node.output_size() == 0)
            continue;

        auto dst_it = producers.find(onnx_node.output(0));
        if (dst_it == producers.end() || !dst_it->second.first)
            continue;
        ir::Node* dst = dst_it->second.first;

        for (int ii = 0; ii < onnx_node.input_size(); ii++) {
            const std::string& in_name = onnx_node.input(ii);
            if (in_name.empty() || init_names.count(in_name))
                continue;
            auto src_it = producers.find(in_name);
            if (src_it == producers.end() || !src_it->second.first)
                continue;
            ir_graph.connect(src_it->second.first, dst, src_it->second.second, (size_t)ii);
        }
    }

    for (const auto& out : g.output()) {
        ir::Node* out_node = nullptr;
        for (const auto& node_ptr : ir_graph.nodes) {
            if (node_ptr->spec->name == "PortOutput" && node_ptr->label == out.name()) {
                out_node = node_ptr.get();
                break;
            }
        }
        if (!out_node)
            continue;

        for (int ni = g.node_size() - 1; ni >= 0; ni--) {
            const auto& onnx_node = g.node(ni);
            for (int oi = 0; oi < onnx_node.output_size(); oi++) {
                if (onnx_node.output(oi) == out.name()) {
                    auto src_it = producers.find(onnx_node.output(oi));
                    if (src_it != producers.end() && src_it->second.first)
                        ir_graph.connect(src_it->second.first, out_node,
                                         src_it->second.second, 0);
                    goto next_output;
                }
            }
        }
        {
            auto src_it = producers.find(out.name());
            if (src_it != producers.end() && src_it->second.first)
                ir_graph.connect(src_it->second.first, out_node, src_it->second.second, 0);
        }
    next_output:;
    }
}

} // namespace backend
