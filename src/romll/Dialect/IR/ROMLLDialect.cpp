#include "romll/Dialect/ROMLLDialect.h"
#include "romll/Dialect/ROMLLOps.h"

// Tablegen-generated dialect definition
#include "romll/Dialect/ROMLLDialect.cpp.inc"

void romll::ROMLLDialect::initialize() {
  addOperations
#define GET_OP_LIST
#include "romll/Dialect/ROMLLOps.cpp.inc"
      >();
}
