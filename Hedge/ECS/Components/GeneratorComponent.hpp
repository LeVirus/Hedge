#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct GeneratorComponent : public ECS::Component
{
    GeneratorComponent() = default;
    // false shots, true enemies
    bool m_genEnemies = false;
    //iteration between generation
    uint32_t m_cycles = 5, m_damage, m_memEnemyLife;
    Direction_e m_dir;
    std::vector<uint32_t> m_vectElementGen;
    virtual ~GeneratorComponent() = default;
};
