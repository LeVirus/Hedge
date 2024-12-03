#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include <set>
// #include <ECSManager.hpp>
#include "ComponentsManager.hpp"

namespace ECS
{

template <uint32_t T>
class SystemManager;

using VectUI_t = std::vector<uint32_t>;
using VectVectUI_t = std::vector<VectUI_t>;

template <uint32_t T>
class System
{
    using ArrUI_t = std::array<uint32_t, T>;
    using VectArrUI_t = std::vector<ArrUI_t>;
public:
    virtual ~System() = default;
    virtual void execSystem() = 0;
    //====================================================================
    void addComponentsToSystem(uint32_t typeComponent, uint32_t numberOfComponent)
    {
        if(numberOfComponent == 0)
        {
            return;
        }
        assert(typeComponent < m_arrEntities.size());
        m_arrEntities[typeComponent] = numberOfComponent;
        m_cacheUsedComponent.insert(typeComponent);
    }
    //====================================================================
    void updateEntities(const VectArrUI_t &vectEntities)
    {
        if(m_cacheUsedComponent.empty())
        {
            std::cout << "Warning: No used component set.\n";
            return;
        }
        m_usedEntities.clear();
        bool ok;
        for(uint32_t i = 0; i < vectEntities.size(); ++i)
        {
            ok = true;
            for(uint32_t j : m_cacheUsedComponent)
            {
                if(m_arrEntities[j] != vectEntities[i][j])
                {
                    ok = false;
                    break;
                }
            }
            if(ok)
            {
                m_usedEntities.insert(i);
            }
        }
    }
protected:
    System() = default;
protected:
    //Definition of needed components
    std::array<uint32_t, T> m_arrEntities;
    std::set<uint32_t> m_usedEntities, m_cacheUsedComponent;//mem possess components
};

template <typename S, uint32_t T>
concept System_C = std::derived_from<S, System<T>>;

}
