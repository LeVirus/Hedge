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
        if(gravComp->m_onGround)
        {
            continue;
        }
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(*it);
        assert(mapComp);
        mapComp->m_absoluteMapPositionPX.second += gravComp->m_gravityCohef;
    }

}
