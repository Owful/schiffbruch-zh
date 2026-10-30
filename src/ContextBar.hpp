#pragma once
#include <array>
#include <algorithm>
#include <cmath>

// Stable identities keep callbacks attached to actions while the row reflows.
namespace ContextBar
{
    enum Action { Interact, Stop, Continue, Water, Bandage, Boat, Feed, Count };
    struct Slot { float x=18,alpha=0,settled=0; bool wanted=false; };
    struct State
    {
        std::array<Slot,Count> slots{};
        static constexpr int Width=122,Gap=10;
        void reset(){slots={};}
        void update(const std::array<bool,Count>& wanted,float seconds)
        {
            const float dt=std::clamp(seconds,0.f,.05f);
            bool outgoing=false;
            for(int i=0;i<Count;++i)
            {
                auto& s=slots[i];if(s.wanted!=wanted[i])s.settled=0;s.wanted=wanted[i];
                if(!s.wanted)s.alpha=std::max(0.f,s.alpha-dt/.18f);
                outgoing|=!s.wanted&&s.alpha>0;
            }
            float x=18;bool moving=false;
            for(auto& s:slots)
            {
                if(s.alpha<=0&&(!s.wanted||outgoing)){s.settled=0;continue;}
                if(s.alpha<=0)s.x=x;
                const float distance=x-s.x;
                s.x+=distance*std::min(1.f,dt*22.f);
                if(std::abs(x-s.x)<.5f){s.x=x;s.settled+=dt;}else{s.settled=0;moving=true;}
                x+=Width+Gap; // Outgoing buttons retain space until fully faded.
            }
            // Make space before showing an incoming button so labels never cross.
            if(!outgoing&&!moving)for(auto& s:slots)if(s.wanted)s.alpha=std::min(1.f,s.alpha+dt/.18f);
        }
        bool clickable(int i)const
        {const auto& s=slots[i];return s.wanted&&s.alpha>=1.f&&s.settled>=.10f;}
    };
}
