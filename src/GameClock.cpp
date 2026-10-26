#include "GameClock.hpp"
#include "ClockRules.hpp"
#include "ModernUI.hpp"
#include "World.hpp"
#include "Action.hpp"
#include "State.hpp"

namespace GameClock
{
    bool Running(bool focused)
    {
        return focused&&Spielzustand==State::GAME&&!Nacht&&PapierText==-1&&!ModernUI::OverlayActive()&&
            Guy.Aktion!=Action::DAY_END&&Guy.Aktion!=Action::DEATH&&Guy.Aktion!=Action::RESCUED&&Stunden<12;
    }
    void Advance(std::int64_t micros,bool focused)
    {
        const int minutes=ClockRules::advance(micros,Running(focused));
        for(int i=0;i<minutes;++i)
        {
            // AddResource retains the previous version's 50% reduction in losses.
            World::AddResource(WASSER,-.08f);World::AddResource(NAHRUNG,-.06f);
            World::AddFixedTime(0,1);
        }
    }
}
