#pragma once

#include <BaseECS/component.hpp>
#include <constants.hpp>

struct PlayerConfComponent : public ecs::Component
{
    PlayerConfComponent()
    {
        muiTypeComponent = Components_e::PLAYER_CONF_COMPONENT;
    }
    bool m_playerAction = false, m_playerShoot = false, m_timerShootActive = false;
    uint32_t m_weaponEntity;
    virtual ~PlayerConfComponent() = default;
};
