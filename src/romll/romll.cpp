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
  input_data = graph.root->values;
  input_shape = {(int64_t)input_data.size()};
  input_names = {const_cast<char *>(graph.root->label.c_str())};
  output_names = {const_cast<char *>(graph.leaf->label.c_str())};
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

  std::unordered_set<Block *> visited;
  std::deque<Block *> queue = {graph.root.get()};
  visited.insert(graph.root.get());

  while (!queue.empty()) {
    Block *current = queue.front();
    std::cout << "Processing block: " << current->label << std::endl;
    queue.pop_front();
    switch (current->type) {
    case BlockType::PORT_INPUT: {
      auto *input = onnx_graph->add_input();
      input->set_name(current->label.c_str());
      auto *input_type = input->mutable_type()->mutable_tensor_type();
      input_type->set_elem_type(onnx::TensorProto_DataType_FLOAT);
      break;
    }
    case BlockType::PORT_OUTPUT: {
      auto *output = onnx_graph->add_output();
      output->set_name(current->label.c_str());
      break;
    }
    case BlockType::RELU: {
      auto *node = onnx_graph->add_node();
      node->set_op_type("Relu");
      for (size_t i = 0; i < current->previous.size(); i++) {
        node->add_input(current->previous[i]->label.c_str());
      }
      for (size_t i = 0; i < current->next.size(); i++) {
        node->add_output(current->next[i]->label.c_str());
      }
      break;
    }
    }

    for (auto &child : current->next) {
      if (visited.insert(child.get()).second) {
        queue.push_back(child.get());
      }
    }
  }

  return model;
}

void ROMLL::run_inference() {
  // TODO: Should return error code and show error in GUI.
  try {
    Ort::RunOptions run_options{nullptr};
    std::vector<Ort::Value> output = run_model(run_options);
    Ort::TensorTypeAndShapeInfo info = output[0].GetTensorTypeAndShapeInfo();
    std::vector<int64_t> shape = info.GetShape();
    float *data = output[0].GetTensorMutableData<float>();
    int64_t size = input_shape[0];

    graph.root->values.resize(size);
    for (int64_t i = 0; i < size; i++) {
      std::cout << "Output value " << i << ": " << data[i] << std::endl;
      graph.leaf->values[i] = data[i];
    }
    graph.leaf->has_results = true;
    graph.inference_ran = true;

  } catch (const Ort::Exception &e) {
    std::cerr << "ONNX Runtime error: " << e.what() << std::endl;
  }
};

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
  Ort::Value input_tensor = Ort::Value::CreateTensor(
      memory_info, input_data.data(), input_data.size(), input_shape.data(),
      input_shape.size());

  auto output_tensors = session.Run(options, input_names.data(), &input_tensor,
                                    1, output_names.data(), 1);

  return output_tensors;
}
