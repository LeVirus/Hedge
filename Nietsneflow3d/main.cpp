#include <Game.hpp>
#include <MainEngine.hpp>

int main()
{
    Game game;
    game.initEngine();
    game.loadStandardData();
    bool gameLoaded = false, firstLaunch = true;
    LevelState levelState = {LevelState_e::NEW_GAME, {}};
    for(uint32_t i = 1; i < 3; ++i)
    {
        game.loadStandardEntities();
        if(firstLaunch)
        {
            //PLAYER DEPARTURE NOT SET
            game.loadPlayerEntity();
            if(!gameLoaded)
            {
                game.loadSavedSettingsData();
                gameLoaded = true;
            }
            levelState = game.displayTitleMenu();
            i = *levelState.m_levelToLoad;
            if(!game.loadLevelData(i))
            {
                break;
            }
            game.setPlayerDeparture();
            firstLaunch = false;
        }
        else
        {
            if(!game.loadLevelData(i))
            {
                break;
            }
            if(!gameLoaded)
            {
                game.loadSavedSettingsData();
                gameLoaded = true;
            }
        }
        levelState = game.launchGame(i, levelState.m_levelState);
        switch(levelState.m_levelState)
        {
        case LevelState_e::EXIT:
            break;
        case LevelState_e::NEW_GAME:
            i = 0;
            break;
        case LevelState_e::LEVEL_END:
            break;
        case LevelState_e::GAME_OVER:
            --i;
            break;
        case LevelState_e::RESTART_FROM_CHECKPOINT:
        case LevelState_e::RESTART_LEVEL:
        case LevelState_e::LOAD_GAME:
        {
            assert(levelState.m_levelToLoad);
            //reloop on the specific level
            i = *levelState.m_levelToLoad - 1;
            break;
        }
        }
        //quit
        if(levelState.m_levelState == LevelState_e::EXIT)
        {
            break;
        }
        game.clearLevel();
    }
    game.clearLevel();
    return 0;
}
