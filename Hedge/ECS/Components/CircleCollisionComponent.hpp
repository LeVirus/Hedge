#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct CircleCollisionComponent : public ECS::Component
{
    CircleCollisionComponent() = default;
    float m_ray;
    virtual ~CircleCollisionComponent() = default;
};
