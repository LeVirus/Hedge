#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>
#include <vector>

using TupleTetraFloat_t = std::tuple<float, float, float, float>;

struct ColorVertexComponent : public ECS::Component
{
    ColorVertexComponent() = default;
    std::vector<TupleTetraFloat_t> m_vertex;
    virtual ~ColorVertexComponent() = default;
};
