#pragma once
#include "ir/node.h"
#include <memory>
#include <string>
#include <vector>

namespace onnx {
class ModelProto;
}

namespace ir {

class Graph {
public:
  std::vector<std::unique_ptr<Node>> nodes;
  std::vector<Node *> roots;
  std::vector<Node *> leafs;
  std::vector<Node *> orphans;
  bool topology_dirty = false;

  std::string generate_node_label(const std::string &op_name) const;
  void add_node(Node *node);
  void remove_node(Node *node);
  void connect(Node *parent, Node *child, size_t out_port_index,
               size_t in_port_index);
  void disconnect(Node *parent, Node *child, size_t out_port_index,
                  size_t in_port_index);
  void clear();

  std::vector<Node *> topological_order() const;

  onnx::ModelProto to_onnx_model() const;
  bool load_onnx_file(const std::string &path, std::string &error);
  const std::string &current_onnx_bytes();

  void debug_walk(class IBackendPort &backend);

private:
  std::string onnx_bytes_cache;
  bool cache_stale = true;

  bool node_label_exists(const std::string &label) const;
  bool is_reachable_from_root(Node *node) const;
  void refresh_orphans();
  void build_from_onnx(const onnx::ModelProto &model, std::string &warnings);
};

} // namespace ir
