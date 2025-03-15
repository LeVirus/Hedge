#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct TriangleStairCollisionComponent : public ECS::Component
{
    TriangleStairCollisionComponent() = default;
    PairFloat_t m_size;
    bool m_upStair = true;
    virtual ~TriangleStairCollisionComponent() = default;
};
