import numpy as np
import onnx
from onnx import helper, TensorProto

X = helper.make_tensor_value_info("input", TensorProto.FLOAT, [1, 4])
Y = helper.make_tensor_value_info("output", TensorProto.FLOAT, [1, 1])

W = helper.make_tensor(
    "weights", TensorProto.FLOAT, [4, 2], [1.0, 0.5, -1.0, 0.5, 2.0, -1.0, 0.0, 1.0]
)
B = helper.make_tensor("bias", TensorProto.FLOAT, [1, 2], [0.1, -0.1])
W2 = helper.make_tensor("weights2", TensorProto.FLOAT, [2, 1], [1.0, 1.0])

matmul = helper.make_node("MatMul", ["input", "weights"], ["matmul_out"])
add = helper.make_node("Add", ["matmul_out", "bias"], ["add_out"])
relu = helper.make_node("Relu", ["add_out"], ["relu_out"])
matmul2 = helper.make_node("MatMul", ["relu_out", "weights2"], ["matmul2_out"])
sigmoid = helper.make_node("Sigmoid", ["matmul2_out"], ["output"])

graph = helper.make_graph(
    [matmul, add, relu, matmul2, sigmoid],
    "multi_op_single_value",
    inputs=[X],
    outputs=[Y],
    initializer=[W, B, W2],
)

model = helper.make_model(graph, opset_imports=[helper.make_opsetid("", 13)])
model.ir_version = 8

onnx.checker.check_model(model)
onnx.save(model, "../../models/premade/simple.onnx")
print("Successfully generated simple")
