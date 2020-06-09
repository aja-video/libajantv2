/**
	@file		graph.h
	@brief		Declares the AJA Directed Graph classes
	@copyright	(C) 2020 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_GRAPH_H
#define AJA_GRAPH_H

#include "ajabase/common/public.h"
#include "ajabase/system/make_unique_shim.h"

namespace aja {

class GraphVertex;
class GraphEdge;

using GraphEdgeList = std::list<GraphEdge*>;
using GraphVertexList = std::list<GraphVertex*>;

//
// GraphEdge - A class which connects two vertices in a specific direction in the directed graph.
//
class GraphEdge {
public:
    enum Direction { Incoming, Outgoing };

    explicit GraphEdge(const std::string& id);
    ~GraphEdge() = default;

    virtual bool operator==(GraphEdge* rhs) const;
    virtual bool Equals(GraphEdge* rhs) const;

    std::string GetID() const { return m_id; }

    void Connect(GraphVertex* src, GraphVertex* dst);
    void Disconnect();

    void SetInputVertex(GraphVertex* vertex) { m_input_vertex = vertex; }
    void SetOutputVertex(GraphVertex* vertex) { m_output_vertex = vertex; }
    GraphVertex* InputVertex() { return m_input_vertex; }
    GraphVertex* OutputVertex() { return m_output_vertex; }

private:
    std::string m_id;
    GraphVertex* m_input_vertex;
    GraphVertex* m_output_vertex;
};

//
// GraphVertex - A vertex in the directed graph.
//
class GraphVertex {
public:
    GraphVertex(const std::string& id) : m_id(id) {}

    bool operator==(GraphVertex* rhs) const;
    bool Equals(GraphVertex* rhs) const;
    std::string GetID() const { return m_id; }

    void AddEdge(GraphEdge* edge, GraphEdge::Direction direction);
    void RemoveEdge(GraphEdge* edge, GraphEdge::Direction direction);

    GraphEdgeList& InputEdges() { return m_input_edges; }
    GraphEdgeList& OutputEdges() { return m_output_edges; }

    std::size_t InDegree() const { return m_input_edges.size(); }
    std::size_t OutDegree() const { return m_output_edges.size(); }

protected:
    std::string m_id;
    GraphEdgeList m_input_edges;
    GraphEdgeList m_output_edges;
};

//
// GraphDataVertex - A templated vertex class which acts as a container for data.
//
template <typename T>
class GraphDataVertex : public GraphVertex {
public:
    GraphDataVertex(const std::string& id)
        : GraphVertex::GraphVertex(id) {}
    
    virtual ~GraphDataVertex() = default;

    virtual void SetData(T data) {
        m_data = new T();
        *m_data = data;
    }
    virtual void SetData(T* data) { 
        m_data = data;
    }

protected:
    T* m_data;
};

//
// Graph - A container for the vertices and edges which make up a directed graph.
//
class Graph {

public:
    Graph() = default;
    ~Graph() = default;

    bool AddVertex(GraphVertex* vertex);
    bool RemoveVertex(GraphVertex* vertex);
    GraphVertex* GetVertex();
    void PrintGraphViz();

private:
    GraphVertexList m_vertices;
};

} // namespace aja

#endif
