#include <ECS/ECSManager.hpp>
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/SpriteTextureComponent.hpp>
#include <ECS/Components/ColorVertexComponent.hpp>
#include <ECS/Components/NumberVertexComponent.hpp>
#include <ECS/Systems/ColorDisplaySystem.hpp>
#include <constants.hpp>
#include <memory>
#include <cassert>

//===================================================================
ECSManager::ECSManager():m_componentManager(&m_ecsEngine.getComponentManager()),
    m_systemManager(&m_ecsEngine.getSystemManager())
{
}

//===================================================================
void ECSManager::initComponents()
{
    assert(m_componentManager && "m_componentManager is null.");
    m_componentManager->addEmplacementsForExternComponent(Components_e::TOTAL_COMPONENTS);
}

//===================================================================
void ECSManager::initSystems()
{
    assert(m_systemManager && "m_systemManager is null.");
    m_systemManager->bAddExternSystem(std::make_unique<ColorDisplaySystem>());
}


//===================================================================
void ECSManager::init()
{
    initComponents();
    initSystems();
}

//===================================================================
//Execute syncComponentsFromEntities before launching game
uint32_t ECSManager::addEntity(const std::bitset<Components_e::TOTAL_COMPONENTS> &bitsetComponents, size_t numVertex)
{
    uint32_t newEntity = m_ecsEngine.AddEntity();
    std::vector<Components_e> vectMemComponent;
    for(uint32_t i = 0; i < bitsetComponents.size(); ++i)
    {
        if(bitsetComponents[i])
        {
            m_ecsEngine.bAddComponentToEntity(newEntity, i);
            vectMemComponent.emplace_back(static_cast<Components_e>(i));
        }
    }
    syncComponentsFromEntities(newEntity, vectMemComponent, numVertex);
    return newEntity;
}

//===================================================================
void ECSManager::syncComponentsFromEntities(uint32_t numEntity,
                                            const std::vector<Components_e> &vectComp,
                                            const size_t numVertex)
{
    assert(m_componentManager && "m_componentManager is null.");
    m_componentManager->updateComponentFromEntity();
    for(uint32_t i = 0; i < vectComp.size(); ++i)
    {
        switch(vectComp[i])
        {
        case Components_e::NUMBER_VERTEX_COMPONENT: // == 0
        {
            m_componentManager->instanciateExternComponent(
                        numEntity, std::make_unique<NumberVertexComponent>(numVertex));
            NumberVertexComponent *numVertexComp = m_componentManager->
                    searchComponentByType<NumberVertexComponent>(numEntity,
                                                                 Components_e::NUMBER_VERTEX_COMPONENT);
            assert(numVertexComp);
        }
            break;
        case Components_e::POSITION_VERTEX_COMPONENT:
        {
            instanciatePositionVertexComponent(numEntity, numVertex);
        }
            break;
        case Components_e::COLOR_VERTEX_COMPONENT:
        {
            instanciateColorVertexComponent(numEntity, numVertex);
        }
            break;
        case Components_e::SPRITE_TEXTURE_COMPONENT:
        {
            m_componentManager->instanciateExternComponent(
                        numEntity,
                        std::make_unique<SpriteTextureComponent>());
        }
            break;
        case Components_e::TOTAL_COMPONENTS:
            assert("Bad enum.");
            break;
        }
    }
}

//===================================================================
void ECSManager::instanciatePositionVertexComponent(uint32_t numEntity, size_t numVertex)
{
    if(numVertex == 4)
    {
        m_componentManager->instanciateExternComponent(
                    numEntity,
                    std::make_unique<PositionVertexComponent<4>>());
    }
    else if(numVertex == 3)
    {
        m_componentManager->instanciateExternComponent(
                    numEntity,
                    std::make_unique<PositionVertexComponent<3>>());
    }
    else
    {
        assert(false && "Bad vertex number.");
    }
}

//===================================================================
void ECSManager::instanciateColorVertexComponent(uint32_t numEntity, size_t numVertex)
{
    if(numVertex == 4)
    {
        m_componentManager->instanciateExternComponent(
                    numEntity,
                    std::make_unique<ColorVertexComponent<4>>());
    }
    else if(numVertex == 3)
    {
        m_componentManager->instanciateExternComponent(
                    numEntity,
                    std::make_unique<ColorVertexComponent<3>>());
    }
    else
    {
        assert(false && "Bad vertex number.");
    }

}

