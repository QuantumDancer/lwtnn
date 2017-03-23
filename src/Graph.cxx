#include "lwtnn/Graph.hh"
#include "lwtnn/Exceptions.hh"
#include "lwtnn/Stack.hh"

namespace lwt {

  // Sources
  VectorSource::VectorSource(const std::vector<VectorXd>&& vv):
    m_inputs(std::move(vv))
  {
  }
  VectorXd VectorSource::at(size_t index) const {
    if (index >= m_inputs.size()) {
      throw NNEvaluationException(
        "VectorSource: no source vector defined at " + std::to_string(index));
    }
    return m_inputs.at(index);
  };

  DummySource::DummySource(const std::vector<size_t>& input_sizes):
    m_sizes(input_sizes)
  {
  }
  VectorXd DummySource::at(size_t index) const {
    if (index >= m_sizes.size()) {
      throw NNEvaluationException(
        "Dummy Source: no size defined at " + std::to_string(index));
    }
    size_t n_entries = m_sizes.at(index);
    VectorXd vec(n_entries);
    for (int iii = 0; iii < n_entries; iii++) {
      vec(iii) = iii;
    }
    return vec;
  }


  // Nodes
  InputNode::InputNode(size_t index, size_t n_outputs):
    m_index(index),
    m_n_outputs(n_outputs)
  {
  }
  VectorXd InputNode::compute(const ISource& source) const {
    VectorXd output = source.at(m_index);
    if (output.rows() != m_n_outputs) {
      std::string len = std::to_string(output.rows());
      std::string found = std::to_string(m_n_outputs);
      throw NNEvaluationException(
        "Found vector of length " + len + ", expected " + found);
    }
    return output;
  }
  size_t InputNode::n_outputs() const {
    return m_n_outputs;
  }

  FeedForwardNode::FeedForwardNode(const Stack* stack, const INode* source):
    m_stack(stack),
    m_source(source)
  {
  }
  VectorXd FeedForwardNode::compute(const ISource& source) const {
    return m_stack->compute(m_source->compute(source));
  }
  size_t FeedForwardNode::n_outputs() const {
    return m_stack->n_outputs();
  }

  ConcatenateNode::ConcatenateNode(const std::vector<const INode*>& sources):
    m_sources(sources),
    m_n_outputs(0)
  {
    for (const auto source: sources) {
      m_n_outputs += source->n_outputs();
    }
  }
  VectorXd ConcatenateNode::compute(const ISource& source) const {
    VectorXd output(m_n_outputs);
    size_t offset = 0;
    for (const auto node: m_sources) {
      VectorXd input = node->compute(source);
      size_t n_elements = input.rows();
      assert(n_elements == node->n_outputs());
      output.segment(offset, n_elements) = input;
      offset += n_elements;
    }
    assert(offset = m_n_outputs);
    return output;
  }
  size_t ConcatenateNode::n_outputs() const {
    return m_n_outputs;
  }

  // graph
  Graph::Graph() {
    m_stacks.push_back(new Stack);
    Stack* stack = m_stacks.back();

    m_nodes.push_back(new InputNode(0, 2));
    INode* source1 = m_nodes.back();
    m_nodes.push_back(new InputNode(1, 2));
    INode* source2 = m_nodes.back();
    m_nodes.push_back(new ConcatenateNode({source1, source2}));
    INode* cat = m_nodes.back();
    m_nodes.push_back(new FeedForwardNode(stack, cat));
  }
  Graph::~Graph() {
    for (auto node: m_nodes) {
      delete node;
      node = 0;
    }
    for (auto stack: m_stacks) {
      delete stack;
      stack = 0;
    }
  }
  VectorXd Graph::compute(const ISource& source, size_t node_number) const {
    if (node_number >= m_nodes.size()) {
      throw NNEvaluationException(
        "Graph: no node at " + std::to_string(node_number));
    }
    return m_nodes.at(node_number)->compute(source);
  }
  VectorXd Graph::compute(const ISource& source) const {
    assert(m_nodes.size() > 0);
    return m_nodes.back()->compute(source);
  }
}
