#pragma once

#include <BaseECS/component.hpp>
#include <constants.hpp>
#include <PictureData.hpp>
#include <functional>

using VectSpriteDataRef_t = std::vector<std::reference_wrapper<SpriteData>>;
using VectVectSpriteDataRef_t = std::vector<VectSpriteDataRef_t>;
using PairDoubleStr_t = std::pair<double, std::string>;

struct WriteComponent : public ecs::Component
{
    WriteComponent()
    {
        muiTypeComponent = Components_e::WRITE_COMPONENT;
    }
    void addTextLine(const PairDoubleStr_t &data)
    {
        m_fontSpriteData.emplace_back(VectSpriteDataRef_t{});
        m_vectMessage.emplace_back(data);
    }
    void clear()
    {
        m_fontSpriteData.clear();
        m_vectMessage.clear();
    }
    VectVectSpriteDataRef_t m_fontSpriteData;
    //first GL Left position, second message
    std::vector<PairDoubleStr_t> m_vectMessage;
    float m_fontSize;
    PairFloat_t m_upLeftPositionGL;
    uint32_t m_numTexture;
    virtual ~WriteComponent() = default;
};
