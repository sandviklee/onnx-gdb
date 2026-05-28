import onnxruntime as ort
import numpy as np

# chain
session = ort.InferenceSession("../../models/premade/chain.onnx")
portinput = np.array([3, 4, 1, 2], dtype=np.float32)
portinput1 = np.array([4, 2, 5, 1], dtype=np.float32)
portinput2 = np.array([2], dtype=np.float32)
portinput3 = np.array([4], dtype=np.float32)
result = session.run(
    None,
    {
        "PortInput": portinput,
        "PortInput1": portinput1,
        "PortInput2": portinput2,
        "PortInput3": portinput3,
    },
)
print(
    f"chain - input shape: multiple (check source), data: multiple (check source), result: {result}"
)

# simple
session = ort.InferenceSession("../../models/premade/simple.onnx")
data = np.array([[1.0, 2.0, 3.0, 4.0]], dtype=np.float32)
result = session.run(None, {"input": data})
print(f"simple - input shape: (1, 4), data: [1, 2, 3, 4], result: {result}")

# vww_96_float.onnx
session = ort.InferenceSession("../../models/mobilenetv2/vww_96_float.onnx")
shape = (1, 96, 96, 3)
data = np.zeros(shape, dtype=np.float32)
result = session.run(None, {"input_1": data})
print(f"vww_96_float - input shape: {shape}, data: all zeros, result: {result}")
