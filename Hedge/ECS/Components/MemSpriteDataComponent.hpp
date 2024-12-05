#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>
#include <PictureData.hpp>
#include <vector>

/**
 * @brief The MemSpriteDataComponent struct stores all used sprite
 * in the case of animated entities.
 */
struct MemSpriteDataComponent : public ECS::Component
{
    MemSpriteDataComponent() = default;
    std::vector<SpriteData const *> m_vectSpriteData;
    uint32_t m_current = 0;
    virtual ~MemSpriteDataComponent() = default;
};
