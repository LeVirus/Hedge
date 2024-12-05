#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct RectangleCollisionComponent : public ECS::Component
{
    RectangleCollisionComponent() = default;
    PairFloat_t m_size;
    virtual ~RectangleCollisionComponent() = default;
};
