#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct GravityComponent : public ECS::Component
{
    GravityComponent() = default;
    uint32_t m_jumpStep = 0, m_jumpStepMax = 20, m_gravityCohef = 3;
    bool m_onGround = false, m_jump = false, m_fall = true;
};


