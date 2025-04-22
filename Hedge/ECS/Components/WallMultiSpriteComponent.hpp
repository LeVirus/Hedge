#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct WallMultiSpriteComponent : public ECS::Component
{
    WallMultiSpriteComponent() = default;
    std::vector<uint32_t> m_cyclesTime;
    virtual ~WallMultiSpriteComponent() = default;
};
