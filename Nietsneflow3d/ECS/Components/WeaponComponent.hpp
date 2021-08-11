#pragma once

#include <BaseECS/component.hpp>
#include <constants.hpp>


struct WeaponData
{
    uint32_t m_ammunationsCount, m_simultaneousShots, m_maxAmmunations;
    AttackType_e m_attackType;
    pairUI_t m_memPosSprite;
    bool m_posses;
    std::optional<ArrayWeaponVisibleShot_t> m_visibleShootEntities;
    std::string m_visibleShotID;
};

struct WeaponComponent : public ecs::Component
{
    WeaponComponent()
    {
        muiTypeComponent = Components_e::WEAPON_COMPONENT;
    }
    inline uint32_t getStdCurrentWeaponSprite()
    {
        return m_weaponsData[m_currentWeapon].m_memPosSprite.first;
    }
    std::vector<WeaponData> m_weaponsData;
    uint32_t m_numWeaponSprite, m_currentWeapon, m_previousWeapon;
    pairFloat_t m_currentWeaponMove = {-0.02f, -0.01f};
    bool m_timerShootActive = false, m_shootFirstPhase, m_weaponChange = false,
    m_spritePositionCorrected = true;
    virtual ~WeaponComponent() = default;
};
