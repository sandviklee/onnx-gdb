set(LLVM_TARGET_DEFINITIONS ${PROJECT_SOURCE_DIR}/include/romll/Dialect/ROMLLOps.td)

mlir_tablegen(ROMLLDialect.h.inc -gen-dialect-decls)
mlir_tablegen(ROMLLDialect.cpp.inc -gen-dialect-defs)
mlir_tablegen(ROMLLOps.h.inc -gen-op-decls)
mlir_tablegen(ROMLLOps.cpp.inc -gen-op-defs)
add_public_tablegen_target(ROMLLOpsIncGen)

add_library(ROMLLDialect
  ROMLLDialect.cpp
  ROMLLOps.cpp
)
add_dependencies(ROMLLDialect ROMLLOpsIncGen)
target_include_directories(ROMLLDialect PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}
  ${MLIR_INCLUDE_DIRS}
)
target_link_libraries(ROMLLDialect PUBLIC MLIRIR MLIRSupport)
