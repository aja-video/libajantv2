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

// superclass for objects stored in a GraphVertex
class GraphData {
public:
    GraphData() = default;
    ~GraphData() = default;
};

class GraphVertex;

//
// GraphEdge
//
class GraphEdge {
public:
    enum Direction { Incoming, Outgoing };

    explicit GraphEdge(const std::string& id);
    ~GraphEdge() = default;

    std::string GetID() const { return m_id; }

    void Connect(GraphVertex* src, GraphVertex* dst);

    void SetInputVertex(GraphVertex* vertex);
    void SetOutputVertex(GraphVertex* vertex);

private:
    std::string m_id;
    GraphVertex* m_input_vertex;
    GraphVertex* m_output_vertex;
};

using GraphEdgeList = std::list<GraphEdge*>;

//
// GraphVertex
//
class GraphVertex {
public:
    GraphVertex(const std::string& id) : m_id(id) {}

    virtual bool operator==(GraphVertex* rhs) const;
    
    virtual bool Equals(GraphVertex* rhs) const;

    virtual void AddEdge(GraphEdge* edge, GraphEdge::Direction direction);
    virtual void RemoveEdge(GraphEdge* edge);

    virtual std::string GetID() const { return m_id; }
    virtual std::size_t InDegree() const { return m_input_edges.size(); }
    virtual std::size_t OutDegree() const { return m_output_edges.size(); }

protected:
    std::string m_id;
    GraphEdgeList m_input_edges;
    GraphEdgeList m_output_edges;
};

template <typename T>
class GraphVertexPayload : public GraphVertex {
public:
    GraphVertexPayload(const std::string& id) : GraphVertex::GraphVertex(id) {}
    void SetData(T* data) { m_data = data; }

private:
    T* m_data;
};

using GraphVertexList = std::list<GraphVertex*>;

//
// Graph
//
class Graph {

public:
    Graph() = default;
    ~Graph() = default;

    bool AddVertex(GraphVertex* vertex);
    bool RemoveVertex(GraphVertex* vertex);
    GraphVertex* GetVertex();

private:
    GraphVertexList m_vertices;
};

} // namespace aja

#endif
