/**
	@file		graph.h
	@brief		Declares the AJA Directed Graph class
	@copyright	(C) 2020 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_GRAPH_H
#define AJA_GRAPH_H

#include <map>

#include "ajabase/common/public.h"
#include "ajabase/common/variant.h"
#include "ajabase/system/make_unique_shim.h"

namespace aja {

using VariantMap = std::map<std::string, aja::Variant>;

class GraphVertex;
class GraphEdge;
class Graph;

using GraphEdgeList = std::list<GraphEdge*>;
using GraphVertexList = std::list<GraphVertex*>;

/*
 * GraphElement - Base class for GraphVertex and GraphEdge.
 * Holds the ID, label and GraphViz properties for the Graph element.
 */
class AJA_EXPORT GraphElement {
public:
    explicit GraphElement(const std::string& id)
        : m_id(id), m_label(std::string()) {}
    explicit GraphElement(const std::string& id, const std::string& label)
        : m_id(id), m_label(label) {}
    virtual ~GraphElement() {}

    virtual bool operator==(GraphElement* rhs) const;
    virtual bool Equals(GraphElement* rhs) const;

    virtual std::string GetID() const { return m_id; }
    virtual std::string GetLabel() const { return m_label; }
    virtual void SetLabel(const std::string& label) { m_label = label; }
    virtual void SetOwner(Graph* owner) { m_owning_graph = owner; }
    virtual Graph* Owner() const { return m_owning_graph; }

    virtual bool AddProperty(const std::string& key, const Variant& prop);
    virtual bool AddProperty(const std::string& key, Variant&& prop);
    virtual bool RemoveProperty(const std::string& key);
    virtual Variant GetProperty(const std::string& key) const;
    virtual bool HasProperty(const std::string& key) const;

private:
    std::string m_id;
    std::string m_label;
    VariantMap m_properties;
    Graph* m_owning_graph;
};

/*
 *   GraphEdge - A class which connects two vertices in a specific direction in the directed graph.
 */
class AJA_EXPORT GraphEdge : public GraphElement {
public:
    enum Direction { Incoming, Outgoing };

    explicit GraphEdge(const std::string& id);
    explicit GraphEdge(const std::string& id, const std::string& label);
    ~GraphEdge() = default;

    void Connect(GraphVertex* src, GraphVertex* dst);
    void Disconnect();

    void SetInputVertex(GraphVertex* vertex) { m_input_vertex = vertex; }
    void SetOutputVertex(GraphVertex* vertex) { m_output_vertex = vertex; }
    GraphVertex* InputVertex() { return m_input_vertex; }
    GraphVertex* OutputVertex() { return m_output_vertex; }

private:
    GraphVertex* m_input_vertex;
    GraphVertex* m_output_vertex;
};

/*
 *  GraphVertex - A vertex in the directed graph.
 */
class AJA_EXPORT GraphVertex : public GraphElement {
public:
    explicit GraphVertex(const std::string& id);
    explicit GraphVertex(const std::string& id, const std::string& label);

    void AddEdge(GraphEdge* edge, GraphEdge::Direction direction);
    void RemoveEdge(GraphEdge* edge, GraphEdge::Direction direction);

    GraphEdgeList& InputEdges() { return m_input_edges; }
    GraphEdgeList& OutputEdges() { return m_output_edges; }

    std::size_t InDegree() const { return m_input_edges.size(); }
    std::size_t OutDegree() const { return m_output_edges.size(); }

protected:
    GraphEdgeList m_input_edges;
    GraphEdgeList m_output_edges;
};

/*
 *  GraphDataVertex - A templated vertex class which acts as a container for data.
 */
template <typename T>
class AJA_EXPORT GraphDataVertex : public GraphVertex {
public:
    explicit GraphDataVertex(const std::string& id)
        : GraphVertex(id) {}
    explicit GraphDataVertex(const std::string& id, const std::string& label)
        : GraphVertex(id, label) {}

    virtual ~GraphDataVertex() {}

    T* GetData() const {
        if (m_data)
            return m_data.get();
        return nullptr;
    }

    virtual void SetData(T* data) {
        m_data.reset(data);
    }

    virtual void SetData(T&& data) {
        m_data = aja::make_unique<T>(std::move(data));
    }

protected:
    std::unique_ptr<T> m_data;
};

/*
 *  Graph - A container for the vertices and edges which make up a directed graph.
 */
class AJA_EXPORT Graph : public GraphElement {
public:
    explicit Graph(const std::string& id);
    explicit Graph(const std::string& id, const std::string& label);

    bool AddVertex(GraphVertex* vertex);
    bool RemoveVertex(GraphVertex* vertex);
    bool AddSubGraph(Graph* sub_graph);
    GraphVertexList Vertices() const { return m_vertices; }
    // GraphVertex* GetVertex();
    std::string GraphVizString();

private:
    std::list<Graph*> m_sub_graphs;
    GraphVertexList m_vertices;
};

} // namespace aja

#endif
