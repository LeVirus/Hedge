#include "VerticesData.hpp"
#include <OpenGLUtils/glheaders.hpp>
#include <OpenGLUtils/Shader.hpp>
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/ColorVertexComponent.hpp>
#include <ECS/Components/SpriteTextureComponent.hpp>
#include <ECS/Components/WriteComponent.hpp>
#include <PictureData.hpp>
#include <CollisionUtils.hpp>
#include <numeric>
#include <cassert>

//===================================================================
VerticesData::VerticesData(Shader_e shaderNum) : m_shaderNum(shaderNum)
{
    init();
}

//===================================================================
void VerticesData::confVertexBuffer()
{
    bindGLBuffers();
    attribGLVertexPointer();
}

//===================================================================
void VerticesData::init()
{
    genGLBuffers();
    setVectGLPointer();
}

//===================================================================
void VerticesData::setVectGLPointer()
{
    switch (m_shaderNum)
    {
    case Shader_e::COLOR_S:
        //first point, second color or texture
        m_shaderInterpretData = {2, 4};
        break;
    case Shader_e::TEXTURE_S:
        m_shaderInterpretData = {2, 2};
        break;
    case Shader_e::COLORED_TEXTURE_S:
        //2 first ==> pos, 4 ==> color, 2 text
        m_shaderInterpretData = {2, 4, 2};
        break;
    case Shader_e::TOTAL_SHADER_S:
        assert("Incoherant shader enum.");
    }
    m_sizeOfVertex = std::accumulate(m_shaderInterpretData.begin(),
                                         m_shaderInterpretData.end(), 0);
}

//===================================================================
bool VerticesData::loadVertexColorComponent(const PositionVertexComponent &posComp,
                                            const ColorVertexComponent &colorComp)
{
    if(m_shaderNum != Shader_e::COLOR_S)
    {
        return false;
    }
    assert(posComp.m_vertex.size() == colorComp.m_vertex.size() && "Color and Pos Component does not match.");
    size_t sizeVertex = posComp.m_vertex.size();
    for(uint32_t j = 0; j < sizeVertex; ++j)
    {
        m_vertexBuffer.emplace_back(posComp.m_vertex[j].first);
        m_vertexBuffer.emplace_back(posComp.m_vertex[j].second);
        m_vertexBuffer.emplace_back(std::get<0>(colorComp.m_vertex[j]));
        m_vertexBuffer.emplace_back(std::get<1>(colorComp.m_vertex[j]));
        m_vertexBuffer.emplace_back(std::get<2>(colorComp.m_vertex[j]));
        m_vertexBuffer.emplace_back(std::get<3>(colorComp.m_vertex[j]));
    }
    if(sizeVertex > 4)
    {
        assert(sizeVertex % 4 == 0);
        uint32_t rectNumber = sizeVertex / 4;
        for(uint32_t j = 0; j < rectNumber; ++j)
        {
            addIndices(BaseShapeTypeGL_e::RECTANGLE);
        }
        return true;
    }
    BaseShapeTypeGL_e shapeType = (sizeVertex == 3 ? BaseShapeTypeGL_e::TRIANGLE : BaseShapeTypeGL_e::RECTANGLE);
    addIndices(shapeType);
    return true;
}

//===================================================================
void VerticesData::loadVertexStandartTextureComponent(const PositionVertexComponent &posComp, SpriteTextureComponent &spriteComp)
{
    size_t sizeVertex = posComp.m_vertex.size();
    //first rect 0  1   2   3
    for(uint32_t j = 0; j < 4; ++j)
    {
        addTexturePoint(posComp.m_vertex[j], spriteComp.m_spriteData->m_texturePosVertex[j]);
    }
    //treat second rect >> 1    4   5   2
    if(sizeVertex > 4)
    {
        uint32_t k;
        for(uint32_t j = 0; j < 4; ++j)
        {
            if(j == 0)
            {
                k = 1;
            }
            else if(j == 1)
            {
                k = 4;
            }
            else if(j == 2)
            {
                k = 5;
            }
            else
            {
                k = 2;
            }
            addTexturePoint(posComp.m_vertex[k], spriteComp.m_spriteData->m_texturePosVertex[j]);
        }
    }
    BaseShapeTypeGL_e shape = BaseShapeTypeGL_e::RECTANGLE;
    if(sizeVertex == 3)
    {
        shape = BaseShapeTypeGL_e::TRIANGLE;
    }
    else if(sizeVertex == 4)
    {
        shape = BaseShapeTypeGL_e::RECTANGLE;
    }
    else
    {
        shape = BaseShapeTypeGL_e::DOUBLE_RECT;
    }
    addIndices(shape);
}

//===================================================================
void VerticesData::loadVertexWriteTextureComponent(const PositionVertexComponent &posComp, const WriteComponent &writeComp)
{
    uint32_t size = writeComp.m_fontSpriteData.size();
    uint32_t l = 0;
    for(uint32_t i = 0; i < size; ++i)
    {
        for(uint32_t k = 0; k < writeComp.m_fontSpriteData[i].size(); ++k, ++l)
        {
            for(uint32_t j = 0; j < 4; ++j)
            {
                addTexturePoint(posComp.m_vertex[l * 4 + j],
                        writeComp.m_fontSpriteData[i][k].get().m_texturePosVertex[j]);
            }
            addIndices(BaseShapeTypeGL_e::RECTANGLE);
        }
    }
}

//===================================================================
void VerticesData::reserveVertex(uint32_t size)
{
    m_vertexBuffer.reserve(size);
}

//===================================================================
void VerticesData::reserveIndices(uint32_t size)
{
    m_indices.reserve(size);
}

//===================================================================
void VerticesData::addTexturePoint(const PairFloat_t &pos, const PairFloat_t &tex)
{
    m_vertexBuffer.insert(m_vertexBuffer.end(), {pos.first, pos.second, tex.first, tex.second});
}

//===================================================================
void VerticesData::addColoredTexturePoint(const PairFloat_t &pos, const PairFloat_t &tex, const std::array<float, 4> &color)
{
    m_vertexBuffer.insert(m_vertexBuffer.end(), {pos.first, pos.second, color[0], color[1], color[2], color[3], tex.first, tex.second});
}

//===================================================================
void VerticesData::addIndices(BaseShapeTypeGL_e shapeType)
{
    assert(shapeType != BaseShapeTypeGL_e::NONE);
    //first triangle
    if(shapeType == BaseShapeTypeGL_e::TRIANGLE)
    {
        m_indices.insert(m_indices.end(), {m_cursor, ++m_cursor, ++m_cursor});// 0 1 2
    }
    //if Triangle stop here
    else if(shapeType == BaseShapeTypeGL_e::RECTANGLE)
    {
        m_indices.insert(m_indices.end(), {m_cursor, ++m_cursor, ++m_cursor,
                                           m_cursor, ++m_cursor, m_cursor - 3});// 0 1 2 2 3 0
    }
    else if(shapeType == BaseShapeTypeGL_e::DOUBLE_RECT)
    {
        // 0 1 2 2 3 0  1 4 5 5 2 1
        m_indices.insert(m_indices.end(), {m_cursor, ++m_cursor, ++m_cursor,
                                           m_cursor, ++m_cursor, m_cursor - 3,
                                           ++m_cursor, ++m_cursor, ++m_cursor,
                                           m_cursor, ++m_cursor, m_cursor - 3});
    }
    ++m_cursor;
}

//===================================================================
void VerticesData::genGLBuffers()
{
    glGenBuffers(1, &m_ebo);
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
}

//===================================================================
void VerticesData::bindGLBuffers()
{
    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * m_vertexBuffer.size(),
                 &m_vertexBuffer[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * m_indices.size(),
                 &m_indices[0], GL_STATIC_DRAW);
}

//===================================================================
void VerticesData::attribGLVertexPointer()
{
    uint64_t offset = 0;
    for(uint32_t i = 0; i < m_shaderInterpretData.size(); ++i)
    {
        glVertexAttribPointer(i, m_shaderInterpretData[i], GL_FLOAT, GL_FALSE,
                              m_sizeOfVertex * sizeof(float), (void*)(offset));
        offset += m_shaderInterpretData[i] * sizeof(float);
        glEnableVertexAttribArray(i);
    }
}

//===================================================================
void VerticesData::drawElement()
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

//===================================================================
void VerticesData::clear()
{
    m_vertexBuffer.clear();
    m_indices.clear();
    m_cursor = 0;
}
