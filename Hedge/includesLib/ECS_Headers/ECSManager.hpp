#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include "SystemManager.hpp"
#include "System.hpp"
#include "constants.hpp"
// #include "TestCompMan.hpp"


namespace ECS
{

template<uint32_t T, Component_C... C>
class ECSManager
{
    using ArrUI_t = std::array<uint32_t, T>;
    using VectArrUI_t = std::vector<ArrUI_t>;
public:
    //====================================================================
    static ECSManager &instance()
    {
        static ECSManager ecsMan;
        return ecsMan;
    }
    ~ECSManager() = default;
    //====================================================================
    std::optional<std::set<uint32_t>> getEntitiesCustomComponents(const std::set<uint32_t> &cacheUsedComponent, const std::array<uint32_t, Components_e::TOTAL_COMPONENTS> &arrEntities)
    {
        if(cacheUsedComponent.empty())
        {
            std::cout << "Warning: No used component set.\n";
            return std::nullopt;
        }
        const VectArrUI_t &vectEntities = m_componentsManager->getVectEntities();
        std::set<uint32_t> usedEntities;
        bool ok;
        for(uint32_t i = 0; i < vectEntities.size(); ++i)
        {
            ok = true;
            for(uint32_t j : cacheUsedComponent)
            {
                if(arrEntities[j] != vectEntities[i][j])
                {
                    ok = false;
                    break;
                }
            }
            if(ok)
            {
                usedEntities.insert(i);
            }
        }
        if(ok)
        {
            return usedEntities;
        }
        return std::nullopt;
    }
    //====================================================================
    void associateCompManager(std::unique_ptr<ComponentsManager<T, C...>> comp)
    {
        m_componentsManager = std::move(comp);
    }

    //====================================================================
    void updateEntitiesFromSystems()
    {
        const VectArrUI_t &vect = m_componentsManager->getVectEntities();
        m_systemsManager.updateEntitiesFromSystems(vect);
    }

    //====================================================================
    void updateEntitiesFromSystem(uint32_t numSystem)
    {
        const VectArrUI_t &vect = m_componentsManager->getVectEntities();
        m_systemsManager.updateEntitiesFromSystem(numSystem, vect);
    }

    //====================================================================
    inline const VectVectUI_t &getVectEntities()const
    {
        return m_componentsManager->getVectEntities();
    }

    //====================================================================
    void execSystem(uint32_t systemNum)
    {
        m_systemsManager.execSystem(systemNum);
    }

    //====================================================================
    inline void execAllSystems()
    {
        m_systemsManager.execAllSystems();
    }

    //====================================================================
    inline bool addNewSystem(std::unique_ptr<System<T>> system)
    {
        return m_systemsManager.addNewSystem(std::move(system));
    }

    //====================================================================
    template <typename System_C>
    System_C *getSystem(uint32_t numSystem)
    {
        return m_systemsManager.template getSystem<System_C>(numSystem);
    }

    //====================================================================
    uint32_t addEntity(const std::array<uint32_t, T> &vect)
    {
        assert(m_componentsManager);
        return m_componentsManager->addEntity(vect);
    }

    //====================================================================
    void removeEntity(uint32_t numEntity)
    {
        assert(m_componentsManager);
        m_componentsManager->removeEntity(numEntity);
    }

    //====================================================================
    template <Component_C Comp, uint32_t NC>
    Comp *getComponent(uint32_t entityNum, uint32_t componentNum = 0)
    {
        return m_componentsManager->template getComponent<Comp, NC>(entityNum, componentNum);
    }

    //====================================================================
    void clearEntities()
    {
        assert(m_componentsManager);
        m_componentsManager->clear();
    }
private:
    //====================================================================
    ECSManager()
    {
        assert(T == sizeof...(C));
    }
    SystemManager<T> m_systemsManager;
    std::unique_ptr<ComponentsManager<T, C...>> m_componentsManager;
    const uint32_t m_numComponents = T;
};
}
