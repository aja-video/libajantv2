/**
	@file		graph.cpp
	@brief		Contains the implementation of the AJA Directed Graph class
	@copyright	(C) 2020 AJA Video Systems, Inc.  All rights reserved.
**/

#include "graph.h"

#include "ajantv2/includes/ajatypes.h"

#include <iostream>
#include <sstream>

namespace aja {

bool GraphElement::AddProperty(const std::string& key, const Variant& prop) {
    if (!HasProperty(key)) {
        m_properties.emplace(std::pair<std::string, Variant>(key, prop));
        return true;
    }
    return false;
}

bool GraphElement::AddProperty(const std::string& key, Variant&& prop) {
    if (!HasProperty(key)) {
        m_properties.emplace(std::pair<std::string, Variant>(key, std::move(prop)));
        return true;
    }
    return false;
}

bool GraphElement::RemoveProperty(const std::string& key) {
    bool removed = false;
    for(auto it = m_properties.begin(); it != m_properties.end(); ) {
        if((*it).first == key) {
            it = m_properties.erase(it);
            removed = true;
            break;
        }
        else
            ++it;
    }
    return removed;
}

Variant GraphElement::GetProperty(const std::string& key) const {
    auto prop = m_properties.find(key);
    if (prop != m_properties.end())
        return prop->second;
    return Variant();
}

bool GraphElement::HasProperty(const std::string& key) const {
    return m_properties.find(key) != m_properties.end();
}
//
// GraphEdge
//
GraphEdge::GraphEdge(const std::string& id)
:
GraphElement(id),
m_input_vertex(AJA_NULL),
m_output_vertex(AJA_NULL)
{
}

GraphEdge::GraphEdge(const std::string& id, const std::string& label)
:
GraphElement(id, label),
m_input_vertex(AJA_NULL),
m_output_vertex(AJA_NULL)
{
}

bool GraphEdge::operator==(GraphEdge* rhs) const {
    return Equals(rhs);
}

bool GraphEdge::Equals(GraphEdge* rhs) const {
    if (rhs->GetID() == GetID())
        return true;
    return false;
}

void GraphEdge::Connect(GraphVertex* src, GraphVertex* dst) {
    if (src)
        src->AddEdge(this, GraphEdge::Direction::Outgoing);
    if (dst)
        dst->AddEdge(this, GraphEdge::Direction::Incoming);
}

void GraphEdge::Disconnect() {
    m_input_vertex->RemoveEdge(this, GraphEdge::Direction::Outgoing);
    m_output_vertex->RemoveEdge(this, GraphEdge::Direction::Incoming);
}

//
// GraphVertex
//

GraphVertex::GraphVertex(const std::string& id)
:
GraphElement(id)
{
}

GraphVertex::GraphVertex(const std::string& id, const std::string& label)
:
GraphElement(id, label)
{
}

bool GraphVertex::operator==(GraphVertex* rhs) const {
    return Equals(rhs);
}

bool GraphVertex::Equals(GraphVertex* rhs) const {
    if (rhs) {
        if (rhs->GetID() == GetID())
            return true;
    }
    return false;
}

void GraphVertex::AddEdge(GraphEdge* edge, GraphEdge::Direction direction) {
    if (edge) {
        if (direction == GraphEdge::Direction::Incoming) {
            edge->SetOutputVertex(this);
            m_input_edges.push_back(edge);
        }
        if (direction == GraphEdge::Direction::Outgoing) {
            edge->SetInputVertex(this);
            m_output_edges.push_back(edge);
        }
    }
}

// NOTE: This function only removes the edge from one vertex. The edge may be still connected to the other side.
void GraphVertex::RemoveEdge(GraphEdge* edge, GraphEdge::Direction direction) {
    if (edge) {
        if (direction == GraphEdge::Direction::Incoming) {
            std::list<GraphEdge*>::iterator iter = m_input_edges.begin();
            for (iter; iter != m_input_edges.end(); iter++) {
                if (edge == *iter) {
                    m_input_edges.remove(*iter);
                    edge->SetOutputVertex(nullptr);
                    break;
                }
            }
        }
        if (direction == GraphEdge::Direction::Outgoing) {
            std::list<GraphEdge*>::iterator iter = m_output_edges.begin();
            for (iter; iter != m_output_edges.end(); iter++) {
                if (edge == *iter) {
                    m_output_edges.remove(*iter);
                    edge->SetInputVertex(nullptr);
                    break;
                }
            }
        }
    }
}

//
// Graph
//
bool Graph::AddVertex(GraphVertex* vertex) {
    bool exists = false;
    std::list<GraphVertex*>::iterator iter = m_vertices.begin();
    for (iter; iter != m_vertices.end(); iter++) {
        if (vertex->Equals(*iter)) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        m_vertices.push_back(vertex);
        return true;
    }

    return false;
}

bool Graph::RemoveVertex(GraphVertex* vertex) {
    bool removed = false;
    std::list<GraphVertex*>::iterator iter = m_vertices.begin();
    for (iter; iter != m_vertices.end(); iter++) {
        if (vertex->Equals(*iter)) {
            m_vertices.remove(*iter);
            removed = true;
            break;
        }
    }

    return removed;
}

void Graph::PrintGraphViz() {
    std::ostringstream dotfile;
    dotfile << "digraph G {" << std::endl;
    for (const auto& v : m_vertices)
        for (const auto& e : v->OutputEdges())
            dotfile << "\t" << e->InputVertex()->GetLabel() << " -> " << e->OutputVertex()->GetLabel() << ";" << std::endl;
    dotfile << "}" << std::endl;
    std::cout << dotfile.str();
}

} // namespace aja
