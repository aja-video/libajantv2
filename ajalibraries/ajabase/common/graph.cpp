/**
	@file		graph.cpp
	@brief		Contains the implementation of the AJA Directed Graph class
	@copyright	(C) 2020 AJA Video Systems, Inc.  All rights reserved.
**/

#include "graph.h"

#include "ajabase/common/common.h"
#include "ajantv2/includes/ajatypes.h"

#include <iostream>
#include <sstream>

namespace aja {

bool GraphElement::operator==(GraphElement* rhs) const {
    return Equals(rhs);
}

bool GraphElement::Equals(GraphElement* rhs) const {
    if (rhs->GetID() == GetID())
        return true;
    return false;
}

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
Graph::Graph(const std::string& id)
:
GraphElement(id)
{
}

Graph::Graph(const std::string& id, const std::string& label)
:
GraphElement(id, label)
{
}

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
        vertex->SetOwner(this);
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
            vertex->SetOwner(nullptr);
            m_vertices.remove(*iter);
            removed = true;
            break;
        }
    }

    return removed;
}

bool Graph::AddSubGraph(Graph* sub_graph) {
    bool exists = false;
    std::list<Graph*>::iterator iter = m_sub_graphs.begin();
    for (iter; iter != m_sub_graphs.end(); iter++) {
        if (sub_graph->Equals(*iter)) {
            exists = true;
            break;
        }
    }
    if (!exists) {
        m_sub_graphs.push_back(sub_graph);
        return true;
    }

    return false;
}

// This is just a super minimal function to render GraphViz w/ subgraphs
std::string Graph::GraphVizString() {
    std::ostringstream dotfile;
    std::ostringstream vert_attr_str;

    dotfile << "digraph G {" << std::endl;
    
    std::vector<GraphEdge*> cached_edges = {};

    // Render subgraph cluster defs
    uint32_t cluster_num = 0;
    for (const auto& sg : m_sub_graphs) {
        dotfile << "\tsubgraph cluster_" << aja::to_string(cluster_num) << " {" << std::endl;
        // Render subgraph label
        if (sg->GetLabel().empty())
            dotfile << "\t\tlabel=\"" << sg->GetID() << "\";" << std::endl;
        else
            dotfile << "\t\tlabel=\"" << sg->GetLabel() << "\";" << std::endl;

        // Render vertex declarations for this subgraph
        for (const auto& v : sg->Vertices()) {
            auto vertex_id = v->GetID();
            dotfile << "\t\t" << vertex_id;

            auto vertex_label = v->GetLabel();
            if (!vertex_label.empty())
                dotfile << " [label=\"" << vertex_label << "\"]";
            dotfile << ";" << std::endl;
        }

        // Render vertex connections owned by this subgraph
        for (const auto& v : sg->Vertices()) {
            for (auto e : v->OutputEdges()) {
                if (e && e->OutputVertex()) {                    
                    if (e->OutputVertex()->Owner() == sg) {
                        const auto& in_vert_id = e->InputVertex()->GetID();
                        const auto& out_vert_id = e->OutputVertex()->GetID();
                        dotfile << "\t\t" << in_vert_id << " -> " << out_vert_id << ";" << std::endl;
                        cached_edges.push_back(e);
                    }
                }
            }
        }

        dotfile << "\t}" << std::endl;
        cluster_num++;
    }

    // render sub-graph vertex edge connections
    for (const auto& sg : m_sub_graphs) {
        for (const auto& v : sg->Vertices()) {
            auto vertex_id = v->GetID();
            auto vertex_label = v->GetLabel();

            for (auto e : v->OutputEdges()) {
                if (e) {
                    // Ignore rendering connections that were rendered in sub-graphs
                    if (std::find(cached_edges.begin(), cached_edges.end(), e) != cached_edges.end())
                        continue;
                    const auto& in_vert_id = e->InputVertex()->GetID();
                    const auto& out_vert_id = e->OutputVertex()->GetID();
                    dotfile << "\t" << in_vert_id << " -> " << out_vert_id << ";" << std::endl;
                }
            }
        }
    }

    // Render any non-subgraph vertices
    for (const auto& v : m_vertices) {
        auto vertex_id = v->GetID();
        auto vertex_label = v->GetLabel();

        if (!vertex_label.empty()) {
            vert_attr_str << "\t" << vertex_id;
            vert_attr_str << " [label=\"" << vertex_label << "\"];" << std::endl;
        }

        for (auto e : v->OutputEdges()) {
            if (e) {
                const auto& in_vert_id = e->InputVertex()->GetID();
                const auto& out_vert_id = e->OutputVertex()->GetID();

                dotfile << "\t" << in_vert_id << " -> " << out_vert_id << ";" << std::endl;
            }
        }
    }

    dotfile << vert_attr_str.str();
    dotfile << "}" << std::endl;

    return dotfile.str();
}

} // namespace aja
