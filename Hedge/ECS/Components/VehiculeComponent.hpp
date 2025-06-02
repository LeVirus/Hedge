#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct VehicleComponent : public ECS::Component
{
    VehicleComponent() = default;
    //damage coll damage on vehicle collision
    uint32_t m_velocity, m_damageColl, m_minHealthDamage, m_HP;
    virtual ~VehicleComponent() = default;
};
