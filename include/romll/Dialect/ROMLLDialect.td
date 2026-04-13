#ifndef ROMLL_DIALECT
#define ROMLL_DIALECT

include "mlir/IR/OpBase.td"

def ROMLL_Dialect : Dialect {
  let name = "romll";
  let summary = "ROMLL high-level ML layer dialect";
  let cppNamespace = "::romll";
}

// Base class for all ROMLL ops
class ROMLL_Op<string mnemonic, list<Trait> traits = []>
    : Op<ROMLL_Dialect, mnemonic, traits>;

#endif
