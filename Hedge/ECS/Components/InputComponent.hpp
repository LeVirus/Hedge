#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct InputComponent : public ECS::Component
{
    InputComponent() = default;
    uint32_t m_controlMode = 0;
    virtual ~InputComponent() = default;
};
