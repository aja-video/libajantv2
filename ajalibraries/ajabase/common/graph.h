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

using GraphEdgeList = std::list<GraphEdge*>;
using GraphVertexList = std::list<GraphVertex*>;

/*
 * GraphElement - Base class for GraphVertex and GraphEdge.
 * Holds the ID, label and GraphViz properties for the Graph element.
 */
class AJA_EXPORT GraphElement {
public:
    GraphElement(const std::string& id)
        : m_id(id), m_label(std::string()) {}
    GraphElement(const std::string& id, const std::string& label)
        : m_id(id), m_label(label) {}
    virtual ~GraphElement() {}

    virtual std::string GetID() const { return m_id; }
    virtual std::string GetLabel() const { return m_label; }
    virtual void SetLabel(const std::string& label) { m_label = label; }

    virtual bool AddProperty(const std::string& key, const Variant& prop);
    virtual bool AddProperty(const std::string& key, Variant&& prop);
    virtual bool RemoveProperty(const std::string& key);
    virtual Variant GetProperty(const std::string& key) const;
    virtual bool HasProperty(const std::string& key) const;

private:
    std::string m_id;
    std::string m_label;
    VariantMap m_properties;
};

/*
 *   GraphEdge - A class which connects two vertices in a specific direction in the directed graph.
 */
class AJA_EXPORT GraphEdge : public GraphElement {
public:
    enum Direction { Incoming, Outgoing };

    explicit GraphEdge(const std::string& id);
    GraphEdge(const std::string& id, const std::string& label);
    ~GraphEdge() = default;

    virtual bool operator==(GraphEdge* rhs) const;
    virtual bool Equals(GraphEdge* rhs) const;

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
    GraphVertex(const std::string& id);
    GraphVertex(const std::string& id, const std::string& label);

    bool operator==(GraphVertex* rhs) const;
    bool Equals(GraphVertex* rhs) const;

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
    GraphDataVertex(const std::string& id)
        : GraphVertex(id) {}
    GraphDataVertex(const std::string& id, const std::string& label)
        : GraphVertex(id, label) {}

    virtual ~GraphDataVertex() { delete m_data; m_data = nullptr; }

    T* GetData() const { return m_data; }
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

/*
 *  Graph - A container for the vertices and edges which make up a directed graph.
 */
class AJA_EXPORT Graph {
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
