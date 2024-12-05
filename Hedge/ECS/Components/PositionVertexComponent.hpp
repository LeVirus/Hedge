#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>
#include <array>
#include <functional>

/**
 * @brief The VertexComponent struct
 * Coordinated data for OpenGL displaying.
 */
struct PositionVertexComponent : public ECS::Component
{
    PositionVertexComponent() = default;
    std::vector<PairFloat_t> m_vertex;
    virtual ~PositionVertexComponent() = default;
};
