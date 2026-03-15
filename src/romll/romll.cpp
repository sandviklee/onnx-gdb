#include <fstream>
#include <iostream>
#include <romll/romll.h>

ROMLL::ROMLL(Graph &graph, const std::vector<float> input_data,
             const std::vector<int64_t> input_shape,
             const std::vector<char *> &input_names,
             const std::vector<char *> &output_names)
    : onnx_model(serialize(parse_ui_graph(graph))), input_data(input_data),
      input_shape(input_shape), input_names(input_names),
      output_names(output_names), env(),
      memory_info(
          Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)),
      session(Ort::Session(env, onnx_model.data(), onnx_model.size(),
                           Ort::SessionOptions{nullptr})) {};

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
      break;
      output->set_name(current->label.c_str());
    }
    case BlockType::RELU: {
      auto *node = onnx_graph->add_node();
      node->set_op_type("Relu");
      node->add_input(graph.root->label.c_str());
      node->add_output(graph.leaf->label.c_str());
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
