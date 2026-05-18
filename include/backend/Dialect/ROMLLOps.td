#ifndef ROMLL_OPS
#define ROMLL_OPS

include "romll/Dialect/ROMLLDialect.td"
include "mlir/IR/OpBase.td"

def ROMLL_DenseOp : ROMLL_Op<"dense"> {
  let summary = "Dense layer: matmul + bias + activation";
  let arguments = (ins
    AnyTensor:$input,
    AnyTensor:$weights,
    AnyTensor:$bias,
    DefaultValuedAttr<StrAttr, "\"relu\"">:$activation
  );
  let results = (outs AnyTensor:$output);
}

#endif
