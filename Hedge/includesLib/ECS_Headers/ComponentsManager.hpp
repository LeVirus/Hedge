#pragma once

#include <vector>
#include <cstdint>
#include <set>
#include <array>
#include <cassert>
#include <memory>
#include "Component.hpp"
#include "EntitiesManager.hpp"

#include <tuple>



namespace ECS
{

using VectUI_t = std::vector<uint32_t>;

template <uint32_t N, Component_C... C>
class ComponentsManager
{
    using ArrUI_t = std::array<uint32_t, N>;
    using VectArrUI_t = std::vector<ArrUI_t>;
public:
    ComponentsManager()
    {
        m_tup = std::make_unique<std::tuple<std::vector<C>...>>();
    }
    virtual ~ComponentsManager() = default;

    //====================================================================
    inline const VectArrUI_t &getVectEntities()const
    {
        return m_entitiesManager.getVectEntities();
    }

    //====================================================================
    void removeEntity(uint32_t numEntity)
    {
        assert(numEntity < m_refComponents.size());
        for(uint32_t i = 0; i < N; ++i)
        {
            if(m_refComponents[numEntity][i].size() > 0)
            {
                for(uint32_t j = 0; j < m_refComponents[numEntity][i].size(); ++j)
                {
                    m_refDelComponents[i].emplace_back(m_refComponents[numEntity][i][j]);
                }
                m_refComponents[numEntity][i].clear();
            }
        }
        m_cacheDeletedEntities.push_back(numEntity);
        m_entitiesManager.deleteEntity(numEntity);
    }

    //====================================================================
    template <Component_C T, uint32_t NC>
    T *getComponent(uint32_t entityNum, uint32_t componentNum = 0)
    {
        assert(entityNum < m_refComponents.size());
        assert(NC < N);
        if(m_refComponents[entityNum][NC].size() == 0 || componentNum >= m_refComponents[entityNum][NC].size())
        {
            return nullptr;
        }
        uint32_t indexComp = m_refComponents[entityNum][NC][componentNum];
        std::vector<T> &vect = std::get<NC>(*m_tup);
        assert(indexComp < vect.size());
        return &vect[indexComp];
    }

    //====================================================================
    uint32_t addEntity(const std::array<uint32_t, N> &vect)
    {
        uint32_t numEntityB = m_entitiesManager.createEntity(vect);
        uint32_t numEntity;
        if(!m_cacheDeletedEntities.empty())
        {
            numEntity = m_cacheDeletedEntities.back();
            m_cacheDeletedEntities.pop_back();
        }
        else
        {
            numEntity = m_refComponents.size();
            m_refComponents.emplace_back(std::array<VectUI_t, N>());
        }
        assert(numEntity == numEntityB);
        instanciateComponentFromEntity(numEntity, vect);
        return numEntity;
    }


    //====================================================================
    template <uint32_t compType, Component_C T>
    uint32_t addNewComponent()
    {
        std::vector<T> &vect = std::get<compType>(*m_tup);
        //if no empty case add new one
        if(m_refDelComponents[compType].empty())
        {
            vect.emplace_back(T());
            return vect.size() - 1;
        }
        uint32_t i = m_refDelComponents[compType].back();
        m_refDelComponents[compType].pop_back();
        vect[i] = T();
        return i;
    }

    //====================================================================
    void clear()
    {
        m_entitiesManager.clear();
        m_tup = std::make_unique<std::tuple<std::vector<C>...>>();
        m_cacheDeletedEntities.clear();
        m_refComponents.clear();
        for(uint32_t i = 0; i < m_refDelComponents.size(); ++i)
        {
            m_refDelComponents[i].clear();
        }
    }
    virtual void instanciateComponentFromEntity(uint32_t numEntity, const std::array<uint32_t, N> &vect) = 0;
protected:
    template<uint32_t TN, Component_C CC>
    std::vector<CC> &getVectTuple()
    {
        return std::get<TN>(*m_tup);
    }
protected:
    std::unique_ptr<std::tuple<std::vector<C>...>> m_tup;
    std::vector<uint32_t> m_cacheDeletedEntities;
    //cache index of entities's component
    std::vector<std::array<VectUI_t, N>> m_refComponents;
    std::array<std::vector<uint32_t>, N> m_refDelComponents;
    EntitiesManager<N> m_entitiesManager;
};
}


//get tuple size
template <class T>
uint32_t getTupleElementsNumber()
{
    return std::tuple_size<T>{}; // or at run time
}
