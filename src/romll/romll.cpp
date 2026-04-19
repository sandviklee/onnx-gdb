#include <fstream>
#include <iostream>
#include <romll/romll.h>

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
    std::cerr << "ONNX Runtime error: " << e.what() << std::endl;
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

std::vector<Ort::Value> ROMLL::run_model(const Ort::RunOptions &options) {
  std::vector<Ort::Value> input_tensors;

  for (auto *root : graph.roots) {
    auto &data = root->values;
    std::vector<int64_t> shape = {(int64_t)data.size()};

    input_tensors.push_back(Ort::Value::CreateTensor(
        memory_info, data.data(), data.size(), shape.data(), shape.size()));
  }

  auto output_tensors =
      session.Run(options, input_names.data(), input_tensors.data(),
                  input_names.size(), output_names.data(), output_names.size());

  return output_tensors;
}
