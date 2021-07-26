#pragma once

#include <BaseECS/component.hpp>
#include <constants.hpp>

enum class EnemyDisplayMode_e
{
    NORMAL,
    SHOOTING,
    DYING,
    DEAD
};

enum class EnemyBehaviourMode_e
{
    PASSIVE,
    ATTACK,
    DEAD
};

enum class EnemyAttackPhase_e
{
    MOVE_TO_TARGET_FRONT,
    MOVE_TO_TARGET_RIGHT,
    MOVE_TO_TARGET_LEFT,
    SHOOT,
    SHOOTED//FROZEN
};

struct EnemyConfComponent : public ecs::Component
{
    EnemyConfComponent()
    {
        muiTypeComponent = Components_e::ENEMY_CONF_COMPONENT;
    }
    //return false if dead
    bool takeDamage(uint32_t damage)
    {
        if(m_life <= damage)
        {
            m_life = 0;
            return false;
        }
        else
        {
            m_life -= damage;
            return true;
        }
    }
    bool m_staticPhase, m_prevWall = false, m_touched = false;
    uint32_t m_weaponEntity, m_life = 3, m_countPlayerInvisibility = 0;
    AmmoContainer_t m_stdAmmo, m_visibleAmmo;
    EnemyDisplayMode_e m_displayMode = EnemyDisplayMode_e::NORMAL;
    EnemySpriteType_e m_visibleOrientation;
    EnemyBehaviourMode_e m_behaviourMode = EnemyBehaviourMode_e::PASSIVE;
    EnemyAttackPhase_e m_attackPhase;
    float m_dyingTime = 0.4f, m_dyingInterval = m_dyingTime / 5.0f;
    virtual ~EnemyConfComponent() = default;
};

