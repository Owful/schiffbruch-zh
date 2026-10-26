#include "ClockRules.hpp"
#include "GameClock.hpp"
#include "Nightfall.hpp"
#include "Action.hpp"
#include "State.hpp"
#include "Construction.hpp"
#include <cmath>
#include <iostream>
#include <sstream>

namespace {bool overlay=false,reachable=true;std::wstring notice;int attempts=0;}
namespace ModernUI {bool OverlayActive(){return overlay;}}
namespace Expedition {void Notify(const wchar_t* text){notice=text;}}
namespace Crafting {void Cancel(bool){}}
namespace Interaction
{
    void Cancel(){}
    void StopSafely(){Guy.Aktiv=false;Guy.Aktion=Action::NOTHING;Guy.AkNummer=0;}
}
namespace Routing {void MarkRoute(bool){}bool FindTheWay(bool){++attempts;return reachable;}}
namespace World
{
    void AddResource(short kind,float amount){Guy.Resource[kind]+=amount<0?amount*.5f:amount;}
    void AddFixedTime(short h,short m){Minuten+=h*60+m;while(Minuten>=60){++Stunden;Minuten-=60;}}
}
void setup()
{
    Guy={};Guy.Pos={20,20};for(auto& value:Guy.Resource)value=100;
    Spielzustand=State::GAME;Stunden=Minuten=0;Nacht=BootsFahrt=overlay=false;PapierText=-1;
    ClockRules::reset();Nightfall::Reset();reachable=true;attempts=0;notice.clear();
    for(auto& column:Scape)for(auto& tile:column){tile={};tile.Objekt=-1;tile.Entdeckt=tile.Begehbar=true;}
    for(auto& bitmap:Bmp)bitmap.Anzahl=3;
}
int main()
{
    int failures=0;const auto check=[&](bool okay,const char* name){if(!okay){++failures;std::cerr<<"FAIL: "<<name<<'\n';}};
    setup();for(int i=0;i<100;++i)GameClock::Advance(100000,true);
    check(Minuten==25&&std::abs(Guy.Resource[WASSER]-99)<.001,"standing still advances the clock and slow metabolism");
    Guy.Aktiv=true;Guy.Aktion=Action::LOG;for(int i=0;i<100;++i)GameClock::Advance(100000,true);
    check(Minuten==50,"working uses the same rate as standing");
    overlay=true;for(int i=0;i<100;++i)GameClock::Advance(100000,true);overlay=false;
    GameClock::Advance(10000000,false);PapierText=1;GameClock::Advance(100000,true);PapierText=-1;
    check(Minuten==50,"menus reading and focus loss pause time without accumulating debt");
    ClockRules::reset();int minutes=0;for(int i=0;i<2880;++i)minutes+=ClockRules::advance(100000,true);
    check(minutes==720,"daylight lasts exactly four minutes forty-eight seconds");
    ClockRules::advance(200000,true);std::stringstream saved;ClockRules::save(saved);ClockRules::reset();
    check(ClockRules::load(saved)&&ClockRules::remainder==200000,"clock fraction survives saving");
    std::stringstream corrupt(saved.str().substr(0,5));check(!ClockRules::load(corrupt)&&ClockRules::remainder==200000,"truncated clock footer cannot overwrite valid time");
    setup();Stunden=12;Guy.Aktion=Action::CRAFT;Guy.Aktiv=true;
    Nightfall::Update(100000,true);check(Guy.Aktion==Action::CRAFT&&Nightfall::Line().empty(),"nightfall waits for crafting to finish");
    Guy.Aktion=Action::NOTHING;Guy.Aktiv=false;Nightfall::Update(100000,true);
    check(!Nightfall::Line().empty(),"finished work produces a short character line");
    Scape[23][20].Objekt=ZELT;Scape[23][20].Phase=0;
    for(int i=0;i<8;++i)Nightfall::Update(250000,true);
    check(Nightfall::Walking()&&attempts==1&&RouteZiel.x==23&&Guy.Pos.x==20,"nearby tent uses an actual route without teleporting");
    Guy.Pos={23,20};Guy.Aktiv=false;Nightfall::Update(100000,true);
    check(Guy.Aktion==Action::DAY_END,"sleep starts only after arriving at shelter");
    setup();Stunden=12;Guy.Aktion=Action::TENT;Guy.Aktiv=true;Nightfall::Update(100000,true);
    check(Guy.Aktion==Action::TENT,"nightfall does not interrupt construction");
    setup();Stunden=12;reachable=false;Scape[21][20].Objekt=ZELT;Scape[21][20].Phase=0;
    Nightfall::Update(100000,true);for(int i=0;i<8;++i)Nightfall::Update(250000,true);
    check(Guy.Pos.x==20&&Guy.Aktion==Action::DAY_END&&!Nightfall::Walking(),"blocked shelter falls back without teleporting or hanging");
    setup();Stunden=12;Scape[27][20].Objekt=ZELT;Nightfall::Update(100000,true);for(int i=0;i<8;++i)Nightfall::Update(250000,true);
    check(attempts==0&&Guy.Aktion==Action::DAY_END,"distant shelter is not silently used");
    setup();auto& project=Scape[20][20];project.Objekt=FELD;project.Phase=5;project.Rohstoff[ROHAST]=2;Bmp[FELD].Rohstoff[ROHAST]=9;Guy.Inventar[ROHAST]=1;
    check(Construction::Missing(project)==L"树枝1"&&Construction::Rows(project).find(L"还需2")!=std::wstring::npos,"construction shortage uses remaining work instead of full recipe");
    Guy.Inventar[ROHAST]=2;check(Construction::Missing(project).empty(),"gathered material clears the exact remaining shortage");
    if(failures)return 1;std::cout<<"Fixed clock, pause, metabolism and nightfall handoff checks passed.\n";
}
