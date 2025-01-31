#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

using PairStrPairFloat_t = std::pair<std::string, PairFloat_t>;

struct WeaponData
{
    uint32_t m_ammunationsCount, m_simultaneousShots, m_maxAmmunations, m_lastAnimNum, m_weaponPower, m_currentBullet = 0, m_intervalLatency;
    AttackType_e m_attackType;
    AnimationMode_e m_animMode;
    PairUI_t m_memPosSprite;
    bool m_posses = false;
    std::optional<std::vector<uint32_t>> m_visibleShootEntities, m_segmentShootEntities;
    std::string m_visibleShotID, m_impactID, m_weaponName;
    std::optional<float> m_damageRay;
    float m_shotVelocity;
};

struct WeaponComponent : public ECS::Component
{
    WeaponComponent() = default;
    inline uint32_t getStdCurrentWeaponSprite()
    {
        return m_weaponsData[m_currentWeapon].m_memPosSprite.first;
    }
    std::map<uint32_t, uint32_t> m_reloadSoundAssociated;
    std::vector<WeaponData> m_weaponsData;
    uint32_t m_numWeaponSprite, m_currentWeapon = 10000, m_previousWeapon;
    PairFloat_t m_currentWeaponMove = {-0.005f, -0.003f};
    bool m_timerShootActive = false, m_shootFirstPhase, m_weaponChange = false,
    m_spritePositionCorrected = true, m_weaponToChange = false;
    std::vector<PairStrPairFloat_t> m_previewDisplayData;
    virtual ~WeaponComponent() = default;
};
