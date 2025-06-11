#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>


using MapVehicleSprite_t = std::map<VehicleSpriteType_e, PairUI_t>;


struct VehicleComponent : public ECS::Component
{
    VehicleComponent() = default;
    //damage coll damage on vehicle collision
    uint32_t m_damageColl, m_minHealthDamage, m_HP, m_currentSprite, m_spriteInterval = 0.2 / FPS_VALUE, m_stairCount;
    bool m_vehicleMemPlayerAssociated = false, m_onStair = false;
    MapVehicleSprite_t m_mapSpriteAssociate;
    VehicleSpriteType_e m_currentSpritesType = VehicleSpriteType_e::MOVE_RIGHT;
    virtual ~VehicleComponent() = default;
};
