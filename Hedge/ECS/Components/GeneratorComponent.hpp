#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct GeneratorComponent : public ECS::Component
{
    GeneratorComponent() = default;
    // false shots, true enemies
    bool m_genEnemies = false;
    //iteration between generation
    uint32_t m_cycles = 5;
    virtual ~GeneratorComponent() = default;
};
