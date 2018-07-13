/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LazyTypeGraph.h
 *
 *  Created on: 28.06.2018
 *      Author: nicolas bellec
 */


#pragma once

#include <map>
#include <set>
#include <string>

#include <gtest/gtest_prod.h>

#include <boost/graph/reverse_graph.hpp>

#include <phasar/PhasarLLVM/Pointer/TypeGraphs/TypeGraph.h>

namespace llvm {
  class StructType;
}

namespace psr {
  class LazyTypeGraph : public TypeGraph<LazyTypeGraph> {
  protected:
    struct VertexProperties {
      std::string name;
      const llvm::StructType* type;
    };

    struct EdgeProperties {
      EdgeProperties() = default;
    };

    using graph_t = boost::adjacency_list<boost::setS, boost::vecS,
      boost::bidirectionalS, VertexProperties, EdgeProperties>;

    using vertex_t = boost::graph_traits<graph_t>::vertex_descriptor;
    using vertex_iterator = boost::graph_traits<graph_t>::vertex_iterator;
    using edge_t = boost::graph_traits<graph_t>::edge_descriptor;
    using out_edge_iterator = boost::graph_traits<graph_t>::out_edge_iterator;
    using in_edge_iterator = boost::graph_traits<graph_t>::in_edge_iterator;

    using rev_graph_t = boost::reverse_graph<graph_t, graph_t&>;

    using rev_vertex_t = boost::graph_traits<rev_graph_t>::vertex_descriptor;
    using rev_vertex_iterator = boost::graph_traits<rev_graph_t>::vertex_iterator;
    using rev_edge_t = boost::graph_traits<rev_graph_t>::edge_descriptor;
    using rev_out_edge_iterator = boost::graph_traits<rev_graph_t>::out_edge_iterator;
    using rev_in_edge_iterator = boost::graph_traits<rev_graph_t>::in_edge_iterator;

    struct dfs_visitor;

    std::map<std::string, vertex_t> type_vertex_map;
    std::set<LazyTypeGraph*> parent_graphs;
    graph_t g;
    bool already_visited = false;

    vertex_t addType(const llvm::StructType* new_type);
    void aggregateTypes();

  public:
    LazyTypeGraph() = default;

    virtual ~LazyTypeGraph() = default;
    LazyTypeGraph(const LazyTypeGraph &copy) = delete;
    LazyTypeGraph& operator=(const LazyTypeGraph &copy) = delete;
    LazyTypeGraph(LazyTypeGraph &&move) = delete;
    LazyTypeGraph& operator=(LazyTypeGraph &&move) = delete;


    virtual bool addLink(const llvm::StructType* from, const llvm::StructType* to) override;
    virtual void printAsDot(const std::string &path = "typegraph.dot") const override;
    virtual std::set<const llvm::StructType*> getTypes(const llvm::StructType *struct_type) override;
    virtual void merge(LazyTypeGraph *tg) override;
  };
}
