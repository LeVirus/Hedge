TEMPLATE = app
CONFIG += console c++20
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++23
QMAKE_CXXFLAGS_DEBUG += -Wall -Wextra -Wpedantic -Og
INCLUDEPATH += includesLib
LIBS += -I"includesLib/" -L"$$PWD/lib/" -lECS -lglad -ldl -lglfw -lX11 -lXxf86vm -lXrandr \ #-L"$$PWD/lib/libglfw.so.3"
-pthread -lXi -lopenal -lsndfile

SOURCES += main.cpp \
    AudioEngine.cpp \
    ECS/Systems/IASystem.cpp \
    ECS/Systems/SoundSystem.cpp \
    ECS/Systems/VisionSystem.cpp \
    FontData.cpp \
    Game.cpp \
    MainEngine.cpp \
    LevelManager.cpp \
    GraphicEngine.cpp \
    Level.cpp \
    PictureData.cpp \
    OpenGLUtils/Shader.cpp \
    OpenGLUtils/Texture.cpp \
    ECS/Systems/ColorDisplaySystem.cpp \
    OpenGLUtils/VerticesData.cpp \
    ECS/Systems/MapDisplaySystem.cpp \
    PhysicalEngine.cpp \
    ECS/Systems/InputSystem.cpp \
    ECS/Systems/CollisionSystem.cpp \
    CollisionUtils.cpp \
    ECS/Systems/StaticDisplaySystem.cpp \
    ZoneLevelColl.cpp

HEADERS += \
    AudioEngine.hpp \
    ComponentManagerExtend.hpp \
    ComponentsGroup.hpp \
    ECS/Components/AudioComponent.hpp \
    ECS/Components/CheckpointComponent.hpp \
    ECS/Components/EnemyConfComponent.hpp \
    ECS/Components/LogComponent.hpp \
    ECS/Components/MemSpriteDataComponent.hpp \
    ECS/Components/SegmentCollisionComponent.hpp \
    ECS/Components/ShotConfComponent.hpp \
    ECS/Components/TimerComponent.hpp \
    ECS/Components/WeaponComponent.hpp \
    ECS/Components/WriteComponent.hpp \
    ECS/Systems/IASystem.hpp \
    ECS/Systems/SoundSystem.hpp \
    ECS/Systems/VisionSystem.hpp \
    FontData.hpp \
    Game.hpp \
    MainEngine.hpp \
    LevelManager.hpp \
    ECS/includesLib/component.hpp \
    ECS/includesLib/componentmanager.hpp \
    ECS/includesLib/ECSconstantes.hpp \
    ECS/includesLib/engine.hpp \
    ECS/includesLib/entity.hpp \
    ECS/includesLib/system.hpp \
    ECS/includesLib/systemmanager.hpp \
    ZoneLevelColl.hpp \
    alias.hpp \
    includesLib/BaseECS/component.hpp \
    includesLib/BaseECS/componentmanager.hpp \
    includesLib/BaseECS/engine.hpp \
    includesLib/BaseECS/entity.hpp \
    includesLib/BaseECS/system.hpp \
    includesLib/BaseECS/systemmanager.hpp \
    Level.hpp \
    PictureData.hpp \
    constants.hpp \
    OpenGLUtils/Shader.hpp \
    includesLib/ECS_Headers/Component.hpp \
    includesLib/ECS_Headers/ComponentsManager.hpp \
    includesLib/ECS_Headers/ECSManager.hpp \
    includesLib/ECS_Headers/EntitiesManager.hpp \
    includesLib/ECS_Headers/System.hpp \
    includesLib/ECS_Headers/SystemManager.hpp \
    includesLib/glad.h \
    OpenGLUtils/Texture.hpp \
    includesLib/glfw3.h \
    ECS/Components/PositionVertexComponent.hpp \
    ECS/Components/SpriteTextureComponent.hpp \
    ECS/Components/ColorVertexComponent.hpp \
    GraphicEngine.hpp \
    ECS/Systems/ColorDisplaySystem.hpp \
    OpenGLUtils/glheaders.hpp \
    OpenGLUtils/VerticesData.hpp \
    ECS/Systems/MapDisplaySystem.hpp \
    ECS/Components/MoveableComponent.hpp \
    ECS/Components/MapCoordComponent.hpp \
    PhysicalEngine.hpp \
    ECS/Systems/InputSystem.hpp \
    ECS/Components/InputComponent.hpp \
    ECS/Systems/CollisionSystem.hpp \
    ECS/Components/CircleCollisionComponent.hpp \
    ECS/Components/RectangleCollisionComponent.hpp \
    CollisionUtils.hpp \
    ECS/Components/GeneralCollisionComponent.hpp \
    ECS/Components/PlayerConfComponent.hpp \
    ECS/Systems/StaticDisplaySystem.hpp \
    ECS/Components/MemPositionsVertexComponents.hpp \
    ECS/Components/ObjectConfComponent.hpp \
    includesLib/iniwriter.h

