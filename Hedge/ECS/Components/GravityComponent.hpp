#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct GravityComponent : public ECS::Component
{
    GravityComponent() = default;
    uint32_t m_gravityCohef = 1, m_jumpStep = 0, m_jumpStepMax = 10;
    bool m_onGround = false, m_jump = false;
};


