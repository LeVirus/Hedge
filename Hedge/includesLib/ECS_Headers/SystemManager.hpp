#pragma once

#include "System.hpp"
#include "Component.hpp"
#include "ECSManager.hpp"
#include <vector>
#include <cstdint>
#include <iostream>
#include <cassert>
#include <memory>

using VectUI_t = std::vector<uint32_t>;
using VectVectUI_t = std::vector<VectUI_t>;

namespace ECS
{

template <uint32_t T>
class EntitiesManager;

template <uint32_t T>
class SystemManager
{
    using ArrUI_t = std::array<uint32_t, T>;
    using VectArrUI_t = std::vector<ArrUI_t>;
public:
    SystemManager() = default;
    virtual ~SystemManager() = default;
    //====================================================================
    bool addNewSystem(std::unique_ptr<System<T>> system)
    {
        if(m_numComponents == 0)
        {
            return false;
        }
        m_vectSystem.emplace_back(std::move(system));
        return true;
    }

    //====================================================================
    void execAllSystems()
    {
        for(uint32_t i = 0; i < m_vectSystem.size(); ++i)
        {
            m_vectSystem[i]->execSystem();
        }
    }

    //====================================================================
    void execSystem(uint32_t systemNum)
    {
        if(systemNum < m_vectSystem.size())
        {
            m_vectSystem[systemNum]->execSystem();
        }
    }

    //====================================================================
    void updateEntitiesFromSystems(const VectArrUI_t &entitiesVect)
    {
        for(uint32_t i = 0; i < m_vectSystem.size(); ++i)
        {
            m_vectSystem[i]->updateEntities(entitiesVect);
        }
    }

    //====================================================================
    void updateEntitiesFromSystem(uint32_t numSystem, const VectArrUI_t &entitiesVect)
    {
        m_vectSystem[numSystem]->updateEntities(entitiesVect);
    }

    //====================================================================
    template <typename System_C>
    System_C *getSystem(uint32_t numSystem)
    {
        if(numSystem >= m_vectSystem.size())
        {
            std::cout << "Warning: System not found.\n";
            return nullptr;
        }
        return static_cast<System_C*>(m_vectSystem[numSystem].get());
    }
private:
    const uint32_t m_numComponents = T;
    std::vector<std::unique_ptr<System<T>>> m_vectSystem;
};



}

