#include <ECS_Headers/System.hpp>
#include <constants.hpp>

class MainEngine;

class GravitySystem : public ECS::System<Components_e::TOTAL_COMPONENTS>
{
public:
    GravitySystem();
    void execSystem()override;
    inline void linkMainEngine(MainEngine *mainEngine)
    {
        m_mainEngine = mainEngine;
    }
private:
    MainEngine *m_mainEngine;

};

