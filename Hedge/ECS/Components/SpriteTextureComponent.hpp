#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>
#include <PictureData.hpp>
#include <optional>

struct SpriteTextureComponent : public ECS::Component
{
    SpriteTextureComponent() = default;
    SpriteData const *m_spriteData;
    std::optional<float> m_reverseVisibilityRate;
    std::optional<std::pair<float, float>> m_displaySize;
    virtual ~SpriteTextureComponent() = default;
};
