### Evaluation Helper Scripts

In this subdirectory you will find:

- onnx_checker.py: Used in construction and modification evaluation for ONNX specification conformance. Run script and point to model awaiting validation.

- ort_checker.py: Used in execution validation to run specified model with ORT. Running this script outputs all values used in the thesis.

- onnx_simple_genertor.py: Used to generate the Python SDK model used in inference and debugging evaluation. When ran, it will store itself in the correct model directory.

- onnx_generator.py: Used to generate all scaling models used in construction and modification evaluation. When ran, it will store itself in the correct model directory.

### Installation Requirements

To run these scripts, you must have Python installed. These were ran with Python 3.14. You must also have all the required dependencies.

We recommend creating and using a virtual environment before installing:

```bash
python -m venv .venv

# Linux/MacOS
source .venv/bin/activate
```

Install dependencies:

```bash
pip install -r requirements.txt
```
