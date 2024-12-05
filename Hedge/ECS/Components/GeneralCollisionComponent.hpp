#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct GeneralCollisionComponent : public ECS::Component
{
    GeneralCollisionComponent() = default;
    CollisionTag_e m_tagA, m_tagB = CollisionTag_e::GHOST_CT;
    CollisionShape_e m_shape;
    bool m_active = true;
    virtual ~GeneralCollisionComponent() = default;
};
