#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct WallMultiSpriteComponent : public ECS::Component
{
    WallMultiSpriteComponent() = default;
    std::vector<uint32_t> m_cyclesTime;
    std::vector<bool> m_elec, m_appear;
    uint32_t m_damage = 4;
    virtual ~WallMultiSpriteComponent() = default;
};
