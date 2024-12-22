#include "GravitySystem.hpp"
#include <ECS/Components/GravityComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <alias.hpp>

//=================================================================
GravitySystem::GravitySystem()
{
    addComponentsToSystem(Components_e::GRAVITY_COMPONENT, 1);
}

//=================================================================
void GravitySystem::execSystem()
{
    for(std::set<uint32_t>::iterator it = m_usedEntities.begin(); it != m_usedEntities.end(); ++it)
    {
        GravityComponent *gravComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(*it);
        assert(gravComp);
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(*it);
        assert(mapComp);
        if(gravComp->m_jump)
        {
            uint32_t half = gravComp->m_jumpStepMax / 2;
            if(gravComp->m_jumpStep <= gravComp->m_jumpStepMax / 2)
            {
                mapComp->m_absoluteMapPositionPX.second -= half - gravComp->m_jumpStep;
            }
            else
            {
                mapComp->m_absoluteMapPositionPX.second += gravComp->m_jumpStep - half;
            }
            if(++gravComp->m_jumpStep >= gravComp->m_jumpStepMax)
            {
                gravComp->m_jump = false;
                gravComp->m_jumpStep = 0;
            }
            continue;
        }
        if(gravComp->m_onGround)
        {
            gravComp->m_onGround = false;
            continue;
        }
        mapComp->m_absoluteMapPositionPX.second += gravComp->m_gravityCohef;
        gravComp->m_fall = true;
    }

}
