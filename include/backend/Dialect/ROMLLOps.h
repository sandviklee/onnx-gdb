#ifndef ROMLL_OPS_H
#define ROMLL_OPS_H

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/OpDefinition.h"
#include "romll/Dialect/ROMLLDialect.h"

#define GET_OP_CLASSES
#include "romll/Dialect/ROMLLOps.h.inc"

#endif
