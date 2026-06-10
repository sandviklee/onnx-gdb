import onnx
import sys
from onnx import helper, TensorProto
import itertools


def make_activation_chain(n_ops: int = 50, input_shape=(1, 16), out_path="chain.onnx"):
    activations = [
        "Relu",
        "Sigmoid",
        "Tanh",
        "Elu",
    ]
    act_cycle = itertools.cycle(activations)

    input_name = "input"
    nodes = []
    prev = input_name

    for i in range(n_ops):
        op_type = next(act_cycle)
        out_name = f"act_{i}"
        nodes.append(
            helper.make_node(
                op_type, inputs=[prev], outputs=[out_name], name=f"{op_type}_{i}"
            )
        )
        prev = out_name

    graph = helper.make_graph(
        nodes=nodes,
        name="activation_chain",
        inputs=[
            helper.make_tensor_value_info(
                input_name, TensorProto.FLOAT, list(input_shape)
            )
        ],
        outputs=[
            helper.make_tensor_value_info(prev, TensorProto.FLOAT, list(input_shape))
        ],
    )

    model = helper.make_model(
        graph,
        producer_name="chain-generator",
        opset_imports=[helper.make_opsetid("", 17)],
    )
    model.ir_version = 8

    onnx.checker.check_model(model)
    onnx.save(model, out_path)
    print(f"Saved {out_path} with {len(nodes)} nodes.")
    return model


if __name__ == "__main__":
    nodes = int(sys.argv[1])
    make_activation_chain(n_ops=nodes, out_path=f"../../models/premade/{nodes}.onnx")
