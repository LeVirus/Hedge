#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>
#include <optional>

struct ShotConfComponent : public ECS::Component
{
    ShotConfComponent() = default;
    uint32_t m_damage;
    bool m_destructPhase = false, m_ejectMode = false;
    uint32_t m_spriteShotNum = 0, m_impactEntity, m_cycleDestructNumber = 0.12 / FPS_VALUE;
    std::optional<uint32_t> m_damageCircleRayData;
    float m_ejectExplosionRay = 10.0f;
    virtual ~ShotConfComponent() = default;
};
