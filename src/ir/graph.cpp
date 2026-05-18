#include "ir/graph.h"
#include <algorithm>
#include <deque>
#include <unordered_map>
#include <unordered_set>

namespace ir {

std::string Graph::generate_node_label(const std::string &op_name) const {
  std::string label = op_name;
  int count = 0;
  while (node_label_exists(label)) {
    count++;
    label = op_name + std::to_string(count);
  }
  return label;
}

bool Graph::node_label_exists(const std::string &label) const {
  for (const auto &node : nodes) {
    if (node->label == label)
      return true;
  }
  return false;
}

void Graph::add_node(Node *node) {
  nodes.push_back(std::unique_ptr<Node>(node));
  if (node->spec->name == "PortInput")
    roots.push_back(node);
  if (node->spec->name == "PortOutput")
    leafs.push_back(node);
  topology_dirty = true;
  refresh_orphans();
}

void Graph::remove_node(Node *node) {
  for (const auto &edge : node->inputs) {
    auto &parent_outputs = edge.node->outputs;
    parent_outputs.erase(
        std::remove_if(parent_outputs.begin(), parent_outputs.end(),
                       [node](const Edge &e) { return e.node == node; }),
        parent_outputs.end());
  }
  for (const auto &edge : node->outputs) {
    auto &child_inputs = edge.node->inputs;
    child_inputs.erase(
        std::remove_if(child_inputs.begin(), child_inputs.end(),
                       [node](const Edge &e) { return e.node == node; }),
        child_inputs.end());
  }
  roots.erase(std::remove(roots.begin(), roots.end(), node), roots.end());
  leafs.erase(std::remove(leafs.begin(), leafs.end(), node), leafs.end());
  nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                             [node](const std::unique_ptr<Node> &n) {
                               return n.get() == node;
                             }),
              nodes.end());
  topology_dirty = true;
  refresh_orphans();
}

void Graph::connect(Node *parent, Node *child, size_t out_port_index,
                    size_t in_port_index) {
  for (const auto &edge : parent->inputs) {
    if (edge.node == child)
      return;
  }
  if (child->inputs.size() >= child->spec->num_inputs) {
    for (const auto &edge : child->inputs) {
      if (edge.port_index == in_port_index) {
        disconnect(edge.node, child, edge.port_index, in_port_index);
        break;
      }
    }
  }
  parent->outputs.push_back({child, out_port_index});
  child->inputs.push_back({parent, in_port_index});
  topology_dirty = true;
  refresh_orphans();
}

void Graph::disconnect(Node *parent, Node *child, size_t out_port_index,
                       size_t in_port_index) {
  parent->outputs.erase(
      std::remove_if(parent->outputs.begin(), parent->outputs.end(),
                     [child, out_port_index](const Edge &e) {
                       return e.node == child && e.port_index == out_port_index;
                     }),
      parent->outputs.end());
  child->inputs.erase(std::remove_if(child->inputs.begin(), child->inputs.end(),
                                     [parent, in_port_index](const Edge &e) {
                                       return e.node == parent &&
                                              e.port_index == in_port_index;
                                     }),
                      child->inputs.end());
  topology_dirty = true;
  refresh_orphans();
}

void Graph::clear() {
  nodes.clear();
  roots.clear();
  leafs.clear();
  orphans.clear();
  topology_dirty = true;
}

bool Graph::is_reachable_from_root(Node *node) const {
  if (roots.empty())
    return false;
  std::deque<Node *> queue(roots.begin(), roots.end());
  std::unordered_set<Node *> visited(roots.begin(), roots.end());
  while (!queue.empty()) {
    Node *current = queue.front();
    queue.pop_front();
    if (current == node)
      return true;
    for (const auto &edge : current->outputs) {
      if (visited.insert(edge.node).second)
        queue.push_back(edge.node);
    }
  }
  return false;
}

void Graph::refresh_orphans() {
  orphans.clear();
  for (const auto &node_ptr : nodes) {
    Node *node = node_ptr.get();
    if (std::find(roots.begin(), roots.end(), node) != roots.end())
      continue;
    if (!is_reachable_from_root(node))
      orphans.push_back(node);
  }
}

std::vector<Node *> Graph::topological_order() const {
  std::unordered_map<Node *, int> in_degree;
  for (const auto &node_ptr : nodes)
    in_degree[node_ptr.get()] = 0;
  for (const auto &node_ptr : nodes) {
    for (const auto &edge : node_ptr->outputs)
      in_degree[edge.node]++;
  }
  std::deque<Node *> queue;
  for (auto &[node, degree] : in_degree) {
    if (degree == 0)
      queue.push_back(node);
  }
  std::vector<Node *> result;
  while (!queue.empty()) {
    Node *current = queue.front();
    queue.pop_front();
    result.push_back(current);
    for (const auto &edge : current->outputs) {
      if (--in_degree[edge.node] == 0)
        queue.push_back(edge.node);
    }
  }
  return result;
}

} // namespace ir
