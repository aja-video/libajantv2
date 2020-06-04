#include "graph.h"

#include "ajantv2/includes/ajatypes.h"

namespace aja {

//
// GraphEdge
//
GraphEdge::GraphEdge(const std::string& id)
:
m_id(id),
m_input_vertex(AJA_NULL),
m_output_vertex(AJA_NULL)
{
}

void GraphEdge::SetInputVertex(GraphVertex* vertex) {
    m_input_vertex = vertex;
}

void GraphEdge::SetOutputVertex(GraphVertex* vertex) {
    m_output_vertex = vertex;
}

void GraphEdge::Connect(GraphVertex* src, GraphVertex* dst) {
    src->AddEdge(this, GraphEdge::Direction::Outgoing);
    dst->AddEdge(this, GraphEdge::Direction::Incoming);
}

//
// GraphVertex
//

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

void GraphVertex::RemoveEdge(GraphEdge* edge) {
    // if (edge) {
    //     if (edge->Direction() == GraphEdge::EdgeDirection::Incoming) {
    //         std::list<GraphEdge*>::iterator iter = m_input_edges.begin();
    //         for (iter; iter != m_input_edges.end(); iter++)
    //             if (edge->GetID() == (*iter)->GetID())
    //                 m_input_edges.remove(*iter);
    //     }
    //     if (edge->Direction() == GraphEdge::EdgeDirection::Outgoing) {
    //         std::list<GraphEdge*>::iterator iter = m_output_edges.begin();
    //         for (iter; iter != m_output_edges.end(); iter++)
    //             if (edge->GetID() == (*iter)->GetID())
    //                 m_output_edges.remove(*iter);
    //     }
    // }
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

} // namespace aja
