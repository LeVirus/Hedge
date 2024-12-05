#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>
#include <PictureData.hpp>
#include <vector>

/**
 * @brief The MemSpriteDataComponent struct stores all used sprite
 * as pointers in the case of animated entities.
 */
struct MemPositionsVertexComponents : public ECS::Component
{
    MemPositionsVertexComponents() = default;
    std::vector<std::array<PairFloat_t, 4>> m_vectSpriteData;
    virtual ~MemPositionsVertexComponents() = default;
};
