#pragma once

#include <ECS_Headers/ComponentsManager.hpp>

template <typename S>
concept Component_C = std::derived_from<S, ECS::Component>;

template <uint32_t N, Component_C... C>
class ComponentManagerExtend : public ECS::ComponentsManager<N, C...>
{
public:
    ComponentManagerExtend() = default;
    virtual ~ComponentManagerExtend() = default;
    virtual void instanciateComponentFromEntity(uint32_t numEntity, const std::array<uint32_t, N> &vect)override
    {

    }

};
