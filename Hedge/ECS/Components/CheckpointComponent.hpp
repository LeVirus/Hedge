#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct CheckpointComponent : public ECS::Component
{
    CheckpointComponent() = default;
    uint32_t m_checkpointNumber;
    Direction_e m_direction;
    virtual ~CheckpointComponent() = default;
};
