#include "Expedition.hpp"
#include "ExpeditionRules.hpp"
#include "extern.hpp"
#include "Action.hpp"
#include "State.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>

// Isolate player commands from the legacy renderer and animation scheduler.
namespace World
{
    void AddResource(short kind,float amount) { Guy.Resource[kind]=std::clamp(Guy.Resource[kind]+amount,0.f,100.f); }
    void AddTime(short hours,short minutes)
    {
        const int start=Stunden;Minuten+=hours*60+minutes;
        while(Minuten>=60){Minuten-=60;++Stunden;}
        Expedition::OnTime(hours*60+minutes,start);
    }
}
namespace
{
    int failures=0;
    void check(bool condition,const char* name){if(!condition){std::cerr<<"FAIL: "<<name<<'\n';++failures;}}
    ExpeditionRules::Progress current()
    {
        std::stringstream data(std::ios::in|std::ios::out|std::ios::binary);
        Expedition::Save(data);data.seekg(0);ExpeditionRules::Progress p;
        check(ExpeditionRules::read(data,p),"live extension is valid");return p;
    }
    void setup()
    {
        Guy={};Guy.Pos={10,30};Guy.Aktion=Action::NOTHING;
        for(auto& v:Guy.Resource)v=70;
        for(auto& column:Scape)for(auto& t:column){t={};t.Art=1;t.Objekt=-1;}
        for(int x=10;x<50;++x)for(int y=10;y<50;++y){Scape[x][y].Art=2;Scape[x][y].Entdeckt=true;}
        Spielzustand=State::GAME;Nacht=false;BootsFahrt=false;PapierText=-1;Tag=1;Stunden=0;Minuten=0;
        for(auto& b:Bmp)b.Anzahl=3;
        Expedition::Reset();
    }
}
int main()
{
    setup();Guy.Inventar[ROHAST]=8;Guy.Inventar[ROHBLATT]=10;Guy.Inventar[ROHLIANE]=9;
    Expedition::CollectTimber();
    check(Guy.Inventar[ROHSTAMM]==1&&Guy.Inventar[ROHAST]==10&&Guy.Inventar[ROHBLATT]==10&&Guy.Inventar[ROHLIANE]==10,"timber rewards respect per-item inventory capacity");
    check(Expedition::CurrentNotice().find(L"+2")!=std::wstring::npos&&Expedition::CurrentNotice().find(L"+0")!=std::wstring::npos,"pickup receipt reports actual gains including full items");
    setup();
    Expedition::UseWaterskin();check(!current().waterskin&&Minuten==0,"missing recipe ingredients do not consume time");
    Guy.Inventar[ROHBLATT]=2;Guy.Inventar[ROHLIANE]=2;
    Expedition::UseWaterskin();check(!current().waterskin&&Guy.Inventar[ROHBLATT]==2,"water shortcut never silently starts crafting");
    Expedition::CraftWaterskin();check(current().waterskin&&current().water==0&&Guy.Inventar[ROHBLATT]==0,"completed waterskin consumes exact materials");
    Scape[10][30].Objekt=FLUSS1;Expedition::UseWaterskin();
    check(current().water==3&&current().waterVisited,"river refills waterskin and records water milestone");
    Scape[10][30].Objekt=-1;const float before=Guy.Resource[WASSER];Expedition::UseWaterskin();
    check(current().water==2&&Guy.Resource[WASSER]==before+25,"portable water improves thirst and consumes one charge");
    const int timeBefore=Minuten;Expedition::UseWaterskin();
    check(current().water==2&&Minuten==timeBefore,"high hydration preserves water and time");
    Guy.Inventar[ROHBLATT]=3;Guy.Inventar[ROHLIANE]=1;Expedition::CraftBandage();
    check(current().bandages==1&&Guy.Inventar[ROHBLATT]==0,"bandage crafting consumes materials");
    Expedition::UseBandage();check(current().bandages==0&&Guy.Resource[GESUNDHEIT]==90,"bandage heals 20 and is consumed");
    Guy.Aktiv=true;Guy.Inventar[ROHBLATT]=3;Guy.Inventar[ROHLIANE]=1;Expedition::UseBandage();
    check(current().bandages==0&&Guy.Inventar[ROHBLATT]==3,"commands cannot interrupt a running action");Guy.Aktiv=false;
    const auto saved=current();std::stringstream data(std::ios::in|std::ios::out|std::ios::binary);Expedition::Save(data);
    Expedition::Reset();data.seekg(0);Expedition::Load(data);check(current().water==saved.water&&current().waterskin,"live save and load preserve crafted gear");
    setup();const auto caches=current().caches;
    check(caches[0].x!=255&&std::abs(int(caches[0].x)-10)+std::abs(int(caches[0].y)-30)<=2,"first cache is near the landing");
    Guy.Pos={caches[0].x,caches[0].y};Expedition::OnSearch();
    check(current().claimed==1&&Guy.Inventar[ROHAST]==3&&Guy.Inventar[ROHBLATT]==2,"search claims cache rewards");
    Expedition::OnSearch();check(Guy.Inventar[ROHAST]==3,"claimed cache cannot be farmed repeatedly");
    setup();Guy.Pos={caches[0].x,caches[0].y};Guy.Inventar[ROHAST]=10;Guy.Inventar[ROHBLATT]=10;Expedition::OnSearch();
    check(current().claimed==0,"completely full inventory leaves cache unclaimed");
    setup();Scape[12][12].Objekt=SOS;Scape[13][12].Objekt=FEUER;Guy.Inventar[ROHFERNROHR]=1;
    Expedition::OnTime(120,0);check(current().signal==120000,"complete signal setup accumulates time");
    Scape[13][12].Objekt=-1;Expedition::OnTime(60,0);check(current().signal==120000,"extinguished fire pauses rescue progress");
    Scape[13][12].Objekt=FEUER;BootsFahrt=true;Expedition::OnTime(60,0);check(current().signal==120000,"being at sea pauses rescue progress");
    BootsFahrt=false;Tag=3;Expedition::OnTime(60,0);check(current().signal==159000,"rain modifies actual live signal progress");
    Tag=1;Expedition::OnTime(201,0);check(current().signal==ExpeditionRules::SignalTarget&&Guy.Aktion==Action::RESCUED,"threshold starts original rescue action");
    if(failures)return EXIT_FAILURE;
    std::cout<<"Live expedition commands: all checks passed.\n";
}
