#include "Nightfall.hpp"
#include "Action.hpp"
#include "Crafting.hpp"
#include "Expedition.hpp"
#include "Interaction.hpp"
#include "Routing.hpp"
#include "State.hpp"
#include <algorithm>
#include <cstdlib>
#include <vector>

namespace Nightfall
{
    namespace
    {
        enum class Phase{Idle,Finishing,Speaking,Walking,Started};
        Phase phase=Phase::Idle;std::int64_t spoken=0;std::wstring line;ZWEID shelter{-1,-1};
        bool working()
        {
            switch(Guy.Aktion)
            {
            case Action::SEARCH:case Action::EAT:case Action::DRINK:case Action::LOG:case Action::FIELD:
            case Action::TENT:case Action::SLEEP:case Action::FISH:case Action::BOAT:case Action::PIPE:
            case Action::DESTROY:case Action::SOS_SIGN:case Action::HOUSE1:case Action::HOUSE2:case Action::HOUSE3:
            case Action::FIREPLACE:case Action::LIGHT:case Action::LOOKOUT:case Action::TREASURE:case Action::SLINGSHOT:case Action::CRAFT:return true;
            default:return false;
            }
        }
        bool habitable(int x,int y)
        {
            const auto& t=Scape[x][y];return t.Entdeckt&&t.Begehbar&&(t.Objekt==ZELT||t.Objekt==HAUS3)&&t.Phase>=0&&t.Phase<Bmp[t.Objekt].Anzahl;
        }
        void sleepHere()
        {
            Crafting::Cancel(false);Interaction::StopSafely();Guy.PosAlt=Guy.PosScreen;
            Guy.Aktion=Action::DAY_END;Guy.AkNummer=0;phase=Phase::Started;line.clear();
        }
        void goToShelter()
        {
            if(BootsFahrt){sleepHere();return;}
            struct Candidate{short x,y;int distance;};std::vector<Candidate> candidates;
            for(int y=std::max(1,int(Guy.Pos.y)-6);y<=std::min(int(MAXYKACH)-2,int(Guy.Pos.y)+6);++y)
                for(int x=std::max(1,int(Guy.Pos.x)-6);x<=std::min(int(MAXXKACH)-2,int(Guy.Pos.x)+6);++x)
                {
                    const int distance=std::abs(x-Guy.Pos.x)+std::abs(y-Guy.Pos.y);
                    if(distance<=6&&habitable(x,y))candidates.push_back({short(x),short(y),distance});
                }
            std::stable_sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.distance<b.distance;});
            for(const auto& c:candidates)
            {
                if(c.distance==0){sleepHere();return;}
                Interaction::Cancel();Routing::MarkRoute(false);RouteStart=Guy.Pos;RouteZiel={c.x,c.y};
                if(!Routing::FindTheWay(false))continue;
                shelter={c.x,c.y};phase=Phase::Walking;Guy.Aktiv=true;Guy.Aktion=Action::NOTHING;
                RoutePunkt=-1;Steps=Step=0;Bmp[BUTTSTOP].Phase=0;
                Expedition::Notify(L"天黑了，正在走回附近的庇护所。抵达后会进入帐篷或树屋睡觉。");return;
            }
            Expedition::Notify(candidates.empty()?L"附近六格内没有建好的庇护所，今晚只能露宿。":L"附近的庇护所无法到达，今晚只能在这里休息。");sleepHere();
        }
    }
    void Reset(){phase=Phase::Idle;spoken=0;line.clear();shelter={-1,-1};}
    bool Pending(){return Spielzustand==State::GAME&&!Nacht&&Stunden>=12;}
    bool Walking(){return phase==Phase::Walking;}
    std::wstring Line(){return phase==Phase::Speaking?line:L"";}
    void Update(std::int64_t micros,bool running)
    {
        if(Stunden<12){Reset();return;}
        if(!Pending()||!running||Guy.Aktion==Action::DEATH||Guy.Aktion==Action::RESCUED||Guy.Aktion==Action::DAY_END)return;
        if(phase==Phase::Idle)
        {
            phase=Phase::Finishing;
            if(working()){Expedition::Notify(L"夜幕到了，先做完手头的活，再回去休息。");return;}
            Interaction::StopSafely();
        }
        if(phase==Phase::Finishing)
        {
            if(working()||Guy.Aktiv||Guy.Aktion==Action::CANCEL)return;
            const wchar_t* lines[]={L"收工了，明天再接着干。",L"忙了一天，该睡一觉了。",L"今天就到这里吧。",L"听，海浪在催我休息。"};
            line=lines[std::rand()%4];Expedition::Notify(line.c_str());spoken=0;phase=Phase::Speaking;return;
        }
        if(phase==Phase::Speaking)
        {spoken+=std::clamp<std::int64_t>(micros,0,250000);if(spoken>=1800000)goToShelter();return;}
        if(phase==Phase::Walking&&!Guy.Aktiv)
        {
            if(Guy.Pos.x!=shelter.x||Guy.Pos.y!=shelter.y)Expedition::Notify(L"返营路线中断，先在当前位置休息。");
            sleepHere();
        }
    }
}
