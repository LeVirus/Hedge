#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

enum class ObjectType_e
{
    WEAPON,
    AMMO_WEAPON,
    HEAL,
    CARD,
    TOTAL
};

struct ObjectConfComponent : public ECS::Component
{
    ObjectConfComponent() = default;
    ObjectType_e m_type;
    uint32_t m_containing;
    std::string m_cardName;
    std::optional<uint32_t> m_weaponID = std::nullopt, m_cardID = std::nullopt;
    virtual ~ObjectConfComponent() = default;
};
