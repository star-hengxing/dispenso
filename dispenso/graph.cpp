/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <dispenso/graph.h>
#include <iostream>

namespace {
constexpr size_t kToDelete = std::numeric_limits<size_t>::max();

void set_union(
    std::vector<const dispenso::BiPropNode*>& s1,
    const std::vector<const dispenso::BiPropNode*>& s2) {
  std::vector<const dispenso::BiPropNode*> tmp(s1);
  s1.clear();
  std::set_union(tmp.cbegin(), tmp.cend(), s2.cbegin(), s2.cend(), std::back_inserter(s1));
}

void set_insert(std::vector<const dispenso::BiPropNode*>& s, const dispenso::BiPropNode* node) {
  auto it = std::upper_bound(s.begin(), s.end(), node);
  if (it == s.begin() || *(it - 1) != node) {
    s.insert(it, node);
  }
}
} // anonymous namespace

namespace dispenso {

void BiPropNode::biPropDependsOnOneNode(BiPropNode* node) {
  Node::dependsOnOneNode(node);
  if (node->biPropSet_ == nullptr && biPropSet_ == nullptr) {
    biPropSet_ = std::make_shared<std::vector<const BiPropNode*>>();
    set_insert(*biPropSet_, this);
    set_insert(*biPropSet_, node);
    node->biPropSet_ = biPropSet_;
  } else if (node->biPropSet_ != nullptr && biPropSet_ != nullptr) {
    set_union(*biPropSet_, *node->biPropSet_);
    node->biPropSet_ = biPropSet_;
  } else if (biPropSet_ == nullptr) {
    biPropSet_ = node->biPropSet_;
    set_insert(*biPropSet_, this);
  } else {
    node->biPropSet_ = biPropSet_;
    set_insert(*biPropSet_, node);
  }
}

template <class N>
void SubgraphT<N>::clear() {
  decrementDependentCounters();
  const size_t numGraphPredecessors = markNodesWithPredicessors();
  if (numGraphPredecessors != 0) {
    removePredecessorDependencies(numGraphPredecessors);
  }
  nodes_.clear();
}

template <class N>
void SubgraphT<N>::decrementDependentCounters() {
  for (N& node : nodes_) {
    for (Node* const dependent : node.dependents_) {
      dependent->numPredecessors_--;
    }
    removeNodeFromBiPropSet(node);
  }
}

template <class N>
size_t SubgraphT<N>::markNodesWithPredicessors() {
  size_t numGraphPredecessors = 0;
  for (N& node : nodes_) {
    if (node.numPredecessors_ != 0) {
      numGraphPredecessors += node.numPredecessors_;
      node.numPredecessors_ = kToDelete;
    }
  }
  return numGraphPredecessors;
}

template <class N>
void SubgraphT<N>::removePredecessorDependencies(size_t numGraphPredecessors) {
  for (SubgraphT<N>& subgraph : graph_->subgraphs_) {
    if (&subgraph == this) {
      continue;
    }
    for (N& node : subgraph.nodes_) {
      std::vector<Node*>& dependents = node.dependents_;
      size_t num = dependents.size();
      for (size_t i = 0; i < num;) {
        if (dependents[i]->numPredecessors_ == kToDelete) {
          dependents[i] = dependents[num - 1];
          --num;
          if (--numGraphPredecessors == 0) {
            dependents.resize(num);
            return;
          }
        } else {
          i++;
        }
      }
      dependents.resize(num);
    }
  }
}

template <class N>
GraphT<N>::GraphT(GraphT<N>&& other) : subgraphs_(std::move(other.subgraphs_)) {
  for (SubgraphT<N>& subgraph : subgraphs_) {
    subgraph.graph_ = this;
  }
}

template <class N>
GraphT<N>& GraphT<N>::operator=(GraphT&& other) {
  subgraphs_ = std::move(other.subgraphs_);
  for (SubgraphT<N>& subgraph : subgraphs_) {
    subgraph.graph_ = this;
  }
  return *this;
}

template <class N>
SubgraphT<N>* GraphT<N>::addSubgraph() {
  subgraphs_.push_back(SubgraphType(this));
  SubgraphT<N>& subgraph = subgraphs_.back();
  return &subgraph;
}

template class DISPENSO_DLL_ACCESS SubgraphT<Node>;
template class DISPENSO_DLL_ACCESS SubgraphT<BiPropNode>;
template class DISPENSO_DLL_ACCESS GraphT<Node>;
template class DISPENSO_DLL_ACCESS GraphT<BiPropNode>;
} // namespace dispenso