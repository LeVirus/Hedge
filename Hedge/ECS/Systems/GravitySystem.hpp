#include <ECS_Headers/System.hpp>
#include <constants.hpp>

class GravitySystem : public ECS::System<Components_e::TOTAL_COMPONENTS>
{
public:
    GravitySystem();
    void execSystem()override;
private:

};

