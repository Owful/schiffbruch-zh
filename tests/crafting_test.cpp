#include "Crafting.hpp"
#include "Expedition.hpp"
#include "PaceRules.hpp"
#include "headers.hpp"
#include "extern.hpp"
#include "Action.hpp"
#include "State.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>
namespace World
{
    void AddTime(short h,short m){const int elapsed=PaceRules::workMinutes(h*60+m);Minuten+=elapsed;while(Minuten>=60){++Stunden;Minuten-=60;}}
    void AddResource(short kind,float amount){Guy.Resource[kind]=std::clamp(Guy.Resource[kind]+PaceRules::resourceChange(amount),0.f,100.f);}
}
int main()
{
    int failures=0;const auto check=[&](bool ok,const char* name){if(!ok){++failures;std::cerr<<"FAIL: "<<name<<'\n';}};
    const auto make=[](int item){if(!Crafting::Make(item))return false;while(Crafting::Busy())Crafting::Update(100000,true);return true;};
    Guy={};Guy.Pos={20,20};Guy.Resource[0]=Guy.Resource[1]=80;Guy.Resource[2]=40;Spielzustand=State::GAME;BootsFahrt=Nacht=false;PapierText=-1;Guy.Aktion=Action::NOTHING;Tag=1;Stunden=Minuten=0;PaceRules::reset();
    for(auto& column:Scape)for(auto& t:column){t={};t.Objekt=-1;t.Art=2;t.Begehbar=t.Entdeckt=true;}
    for(auto& bitmap:Bmp){bitmap={};bitmap.Phase=-1;}Expedition::Reset();Crafting::SyncUnlocks();
    std::set<int> products;for(const auto& r:Crafting::Recipes())check(products.insert(r.product).second,"every recipe has a unique explicit output");
    check(Crafting::MissingForAction(BUTTFAELLEN)==ROHAXT&&Crafting::MissingForAction(BUTTANZUENDEN)==ROHSTREICHHOLZ,"locked actions identify their actual required tools");
    check(!Crafting::Make(ROHAXT)&&Guy.Inventar[ROHAXT]==0,"missing materials cannot create tools or become negative");
    for(int item:{ROHAST,ROHSTEIN,ROHLIANE,ROHBLATT,ROHSTAMM})Guy.Inventar[item]=10;
    check(!Crafting::Make(ROHEGGE)&&Guy.Inventar[ROHSTEIN]==10,"prerequisites are checked before materials are consumed");
    check(Crafting::Make(ROHAXT)&&Crafting::Busy()&&Guy.Inventar[ROHAXT]==0&&Guy.Inventar[ROHAST]==10,"crafting starts animation and reserves materials without deducting them");
    Crafting::Update(100000,false);check(Crafting::Job().fraction==0,"paused crafting does not advance");
    Crafting::Update(100000,true);Crafting::Cancel();check(!Crafting::Busy()&&Guy.Inventar[ROHAST]==10&&Guy.Aktion==Action::NOTHING,"cancellation restores control without material loss");
    check(make(ROHAXT)&&Bmp[BUTTBOOT].Phase==0&&Bmp[BUTTROHR].Phase==0,"stone axe unlocks felling boat and water pipe");
    check(make(ROHEGGE)&&Guy.Inventar[ROHEGGE]==1&&Bmp[BUTTFELD].Phase==0,"stone hoe recipe creates a hoe rather than another axe");
    const int branches=Guy.Inventar[ROHAST];check(!Crafting::Make(ROHAXT)&&Guy.Inventar[ROHAST]==branches,"owned tools cannot consume materials twice");
    check(Crafting::Make(ROHANGEL),"fishing rod starts");Crafting::Update(100000,true);
    std::stringstream job;Crafting::Save(job);Crafting::Reset();check(Crafting::Load(job)&&Crafting::Busy()&&Crafting::Job().fraction>0,"in-progress crafting resumes from its saved progress");
    while(Crafting::Busy())Crafting::Update(100000,true);check(Bmp[BUTTANGELN].Phase==0,"fishing rod unlocks fishing");
    check(make(ROHSCHLEUDER)&&Bmp[BUTTSCHLEUDER].Phase==0,"sling unlocks hunting");
    check(make(ROHHAMMER)&&Bmp[BUTTHAUS1].Phase==0&&Bmp[BUTTHAUS3].Phase==0,"hammer unlocks every treehouse stage");
    check(make(ROHSCHAUFEL)&&Crafting::MissingForAction(BUTTSCHATZ)==ROHKARTE,"digging still needs an actual map after crafting a shovel");
    check(make(ROHSTREICHHOLZ)&&Bmp[BUTTANZUENDEN].Phase==0,"fire-making kit really unlocks lighting");
    check(!Crafting::Make(ROHFERNROHR)&&!Crafting::Make(ROHKARTE),"exploration finds cannot be fabricated from raw materials");
    Guy.Inventar[ROHKARTE]=Guy.Inventar[ROHFERNROHR]=1;Crafting::SyncUnlocks();check(Bmp[BUTTAUSSCHAU].Phase==0&&Bmp[BUTTSCHATZ].Phase==0,"salvaged items and crafted gear combine to unlock their actions");
    check(make(Crafting::Waterskin)&&Expedition::GetInfo().waterskin&&Expedition::GetInfo().water==0,"crafting a waterskin never invents filled drinking water");
    Guy.Inventar[ROHBLATT]=10;Guy.Inventar[ROHLIANE]=10;
    check(make(Crafting::Bandage)&&make(Crafting::Bandage)&&Expedition::GetInfo().bandages==2&&Guy.Resource[2]==40,"making another bandage cannot silently consume a carried bandage or heal");
    check(make(Crafting::Bandage)&&!Crafting::Make(Crafting::Bandage)&&Expedition::GetInfo().bandages==3,"bandage capacity is enforced without further material loss");
    Expedition::UseBandage();check(Expedition::GetInfo().bandages==2&&Guy.Resource[2]==60,"explicit bandage use remains a separate command");
    Guy.Aktiv=true;Guy.Inventar[ROHAXT]=0;const int stone=Guy.Inventar[ROHSTEIN];check(!Crafting::Make(ROHAXT)&&Guy.Inventar[ROHSTEIN]==stone,"crafting while walking or working cannot mutate inventory");
    if(failures)return 1;std::cout<<"Crafting outputs, prerequisites, action unlocks and explicit consumable crafting passed.\n";
}
