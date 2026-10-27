#include "Interaction.hpp"
#include "Survival.hpp"
#include "SurvivalRules.hpp"
#include "DuskNotice.hpp"
#include "Action.hpp"
#include "State.hpp"
#include "WeatherEffects.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

namespace {bool reachable=true,strict=false;int failures=0;std::wstring notice;ZWEID clickedGround{-1,-1};}
namespace Expedition {void Notify(const wchar_t* text){notice=text;}}
namespace Renderer {ZWEID GetKachel(short,short){return clickedGround;}}
namespace Routing
{
    void ShortRoute(short,short){}
    void MarkRoute(bool){}
    bool FindTheWay(bool allowClosest){strict=!allowClosest;return reachable;}
}
namespace
{
    void check(bool value,const char* name){if(!value){++failures;std::cerr<<"FAIL: "<<name<<'\n';}}
    void setup()
    {
        Guy={};Guy.Pos={10,10};Guy.Resource[0]=Guy.Resource[1]=Guy.Resource[2]=100;
        for(auto& column:Scape)for(auto& t:column){t={};t.Objekt=-1;t.Entdeckt=true;t.Begehbar=true;}
        for(auto& b:Bmp){b.Anzahl=3;b.Surface=nullptr;b.Breite=20;b.Hoehe=30;}
        Spielzustand=State::GAME;Nacht=false;BootsFahrt=false;Tag=1;Stunden=0;Camera={};
        Interaction::Cancel();Survival::Reset();notice.clear();reachable=true;strict=false;clickedGround={-1,-1};
    }
}
int main()
{
    setup();Scape[11][10].Objekt=BUSCH;Scape[11][10].Phase=2;
    check(Interaction::Request(Interaction::Intent::FoodWater)&&Guy.Aktiv&&RouteZiel.x==11&&strict,"nearby berries start exact-target walking");
    Interaction::Update();check(Guy.Aktion==Action::NOTHING,"food is not consumed while walking");
    Guy.Pos={11,10};Guy.Aktiv=false;Interaction::Update();check(Guy.Aktion==Action::EAT,"arrival automatically eats berries");
    setup();Scape[11][10].Objekt=BUSCH;Scape[11][10].Phase=2;Interaction::Request(Interaction::Intent::FoodWater);
    Guy.Aktiv=false;Interaction::Update();check(Guy.Aktion==Action::NOTHING,"interrupted route cannot eat remotely");
    setup();Scape[11][10].Objekt=BUSCH;Scape[11][10].Phase=2;Interaction::Request(Interaction::Intent::FoodWater);
    Guy.Pos={11,10};Guy.Aktiv=false;Scape[11][10].Phase=0;Interaction::Update();check(Guy.Aktion==Action::NOTHING,"arrival rechecks fruit maturity");
    setup();Scape[11][10].Objekt=BAUM1;Interaction::Request(Interaction::Intent::Chop);
    check(!Guy.Aktiv&&Guy.Aktion==Action::NOTHING&&!notice.empty(),"missing axe explains requirement without starting");
    Guy.Inventar[ROHAXT]=1;Interaction::Request(Interaction::Intent::Chop);check(Guy.Aktiv&&RouteZiel.x==11,"nearby chop walks to tree");
    Interaction::Cancel();Guy.Aktiv=false;Guy.Pos={11,10};Interaction::Update();check(Guy.Aktion==Action::NOTHING,"stop cancels queued chop");
    Guy.Aktiv=true;Guy.Aktion=Action::LOG;const float health=Guy.Resource[GESUNDHEIT];Interaction::StopSafely();
    check(!Guy.Aktiv&&Guy.Aktion==Action::NOTHING&&Guy.Resource[GESUNDHEIT]==health,"emergency stop returns control without spending health or time");
    setup();Scape[11][10].Objekt=BUSCH;Scape[11][10].Phase=2;reachable=false;Interaction::Request(Interaction::Intent::FoodWater);
    check(!Guy.Aktiv&&Guy.Aktion==Action::NOTHING&&RouteZiel.x==-1,"unreachable target does not queue remote interaction");
    setup();Scape[11][10].Objekt=BUSCH;Scape[11][10].Phase=2;Scape[11][10].Entdeckt=false;
    Interaction::Request(Interaction::Intent::FoodWater);check(!Guy.Aktiv,"unrevealed fruit is not selected");
    setup();Scape[12][10].Objekt=BUSCH;Scape[12][10].Phase=2;Scape[12][10].xScreen=100;Scape[12][10].yScreen=100;
    check(Interaction::Click(110,110)&&RouteZiel.x==12,"sprite click locks correct object independent of ground tile");
    setup();NativeSurface sprite(20,30);DDBLTFX color{};color.dwFillColor=0xff00ff;sprite.Blt(nullptr,nullptr,nullptr,DDBLT_COLORFILL,&color);
    HDC dc=nullptr;sprite.GetDC(&dc);SetPixelV(dc,18,10,RGB(50,150,60));sprite.ReleaseDC(dc);
    Bmp[BUSCH].Surface=&sprite;Bmp[BUSCH].Anzahl=1;Scape[12][10].Objekt=BUSCH;Scape[12][10].xScreen=100;Scape[12][10].yScreen=100;
    check(Interaction::Click(121,110)&&RouteZiel.x==12,"near-edge click tolerates small misses around visible berries");
    Interaction::Cancel();Guy.Aktiv=false;check(!Interaction::Click(100,100),"empty transparent sprite corners do not capture ground clicks");Bmp[BUSCH].Surface=nullptr;
    setup();Scape[11][10].Objekt=BAUM1;
    check(!Interaction::Request(Interaction::Intent::Any)&&notice.empty()&&!Guy.Aktiv,"general interaction never treats a nearby tree as a chopping request");
    Scape[11][10].xScreen=100;Scape[11][10].yScreen=100;clickedGround={11,10};
    check(!Interaction::Click(110,110)&&notice.empty(),"clicking a tree without an axe remains a movement click without tool errors");
    Guy.Inventar[ROHAXT]=1;check(!Interaction::Click(110,110),"owning an axe does not turn ordinary movement clicks into chopping");
    setup();Scape[11][10].Objekt=BUSCH;Scape[11][10].Phase=2;Scape[11][10].xScreen=100;Scape[11][10].yScreen=100;
    Scape[12][10].Objekt=BAUM1;Scape[12][10].xScreen=100;Scape[12][10].yScreen=100;clickedGround={12,10};
    check(Interaction::Click(110,110)&&RouteZiel.x==11&&notice.find(L"石斧")==std::wstring::npos,"tree canopy overlapping berries cannot hijack eating");
    setup();Scape[11][10].Objekt=FLUSS1;Scape[11][10].xScreen=100;Scape[11][10].yScreen=100;
    Scape[12][10].Objekt=BAUM1;Scape[12][10].xScreen=100;Scape[12][10].yScreen=100;clickedGround={11,10};
    check(Interaction::Click(110,110)&&RouteZiel.x==11&&notice.find(L"水源")!=std::wstring::npos,"water ground under overlapping objects explicitly selects drinking");
    Guy.Pos={11,10};Guy.Aktiv=false;Interaction::Update();check(Guy.Aktion==Action::DRINK,"walking to clicked water automatically drinks upon arrival");
    setup();Scape[9][10].Objekt=FLUSS1;Scape[11][10].Objekt=BUSCH;Scape[11][10].Phase=2;Guy.Resource[WASSER]=1;
    Interaction::Request(Interaction::Intent::Eat);check(RouteZiel.x==11,"explicit eat selects fruit even when water is closer or more depleted");
    Interaction::Cancel();Guy.Aktiv=false;Guy.Resource[NAHRUNG]=1;Guy.Resource[WASSER]=100;
    Interaction::Request(Interaction::Intent::Drink);check(RouteZiel.x==9,"explicit drink never substitutes fruit");
    setup();Scape[11][10].Objekt=BAUM1;Scape[9][10].Objekt=FLUSS1;
    check(Interaction::Request(Interaction::Intent::Any)&&RouteZiel.x==9,"general interaction finds water instead of reporting missing tree tools");
    setup();Scape[11][10].Objekt=BUSCH;Scape[11][10].Phase=2;Scape[11][10].xScreen=100;Scape[11][10].yScreen=100;
    Scape[12][10].Objekt=BUSCH;Scape[12][10].Phase=2;Scape[12][10].xScreen=125;Scape[12][10].yScreen=100;
    check(Interaction::Click(119,110)&&RouteZiel.x==11,"closer visible target wins over later map iteration order");
    setup();Scape[11][10].Objekt=BAUM1;Interaction::Request(Interaction::Intent::Drink);
    check(!Guy.Aktiv&&notice.find(L"淡水")!=std::wstring::npos&&notice.find(L"石斧")==std::wstring::npos,"missing water explains water requirement without unrelated tool errors");
    setup();Scape[12][10].Objekt=FELD;Scape[12][10].Phase=5;Scape[12][10].AkNummer=8;clickedGround={12,10};
    check(Interaction::Click(110,110)&&RouteZiel.x==12&&notice.find(L"未完成")!=std::wstring::npos,"unfinished farmland is construction instead of unripe food");
    Guy.Pos={12,10};Guy.Aktiv=false;Interaction::Update();check(Guy.Aktion==Action::FIELD&&Scape[12][10].AkNummer==8,"arrival resumes existing field progress without resetting it");
    setup();Scape[12][10].Objekt=FELD;Scape[12][10].Phase=0;clickedGround={12,10};
    check(!Interaction::Click(110,110),"unripe completed crops still permit ordinary movement");
    setup();Guy.Resource[GESUNDHEIT]=24;Survival::Update();check(Survival::TakeAlert()==1,"critical health triggers pause request");
    Survival::Update();check(Survival::TakeAlert()==0,"critical warning does not repeat every frame");
    Guy.Resource[GESUNDHEIT]=60;Survival::Update();Guy.Resource[GESUNDHEIT]=20;Survival::Update();check(Survival::TakeAlert()==1,"recovered health rearms a later crisis");
    setup();Stunden=10;Survival::Update();check(Survival::TakeAlert()==2,"dusk warns before unsheltered night");
    Survival::Update();check(Survival::TakeAlert()==0,"dusk warning occurs once per day");
    ++Tag;Survival::Update();check(Survival::TakeAlert()==2,"dusk notice can appear on a later day");
    DuskNotice::State dusk;dusk.show(1000,false);
    check(dusk.visible(1000)&&dusk.visible(5999)&&!dusk.visible(6000),"dusk notice lasts five seconds without needing dismissal");
    dusk.show(7000,true);check(dusk.atSea&&dusk.visible(7000),"sea warning offers a relevant return-to-shore reminder");
    dusk.reset();check(!dusk.visible(7000),"new game clears the old notice");
    setup();Stunden=10;Scape[10][10].Objekt=ZELT;Scape[10][10].Phase=0;Survival::Update();check(Survival::TakeAlert()==0,"safe shelter avoids unnecessary dusk interruption");
    setup();check(Survival::ProtectHealth(10,-2)==1&&Survival::TakeAlert()==1,"first lethal daytime damage preserves last chance and warns");
    check(Survival::ProtectHealth(1,-2)==1,"chained damage is protected until warning acknowledged");
    Survival::Acknowledge();check(Survival::ProtectHealth(1,-2)==-2,"last chance cannot make player invulnerable");
    Guy.Resource[WASSER]=10;Guy.Resource[NAHRUNG]=10;check(!Survival::Advice().empty()&&!Survival::DeathReason().empty(),"crisis and death explain cause and remedies");
    NativeSurface scene(1280,720);rcSpielflaeche={0,64,960,656};color.dwFillColor=0x446655;scene.Blt(nullptr,nullptr,nullptr,DDBLT_COLORFILL,&color);
    WeatherEffects::DrawRain(scene,ExpeditionRules::Weather::Rain,3.5);
    const auto* painted=reinterpret_cast<const unsigned*>(scene.pixels());int changed=0;
    for(int y=0;y<720;++y)for(int x=0;x<1280;++x)if((painted[y*1280+x]&0xffffff)!=0x446655)++changed;
    check(changed>0&&changed<15000,"rain is a sparse translucent effect rather than an opaque sheet");
    check((painted[0]&0xffffff)==0x446655&&(painted[200*1280+1000]&0xffffff)==0x446655,"rain excludes top controls and sidebar");
    color.dwFillColor=0x10252a;scene.Blt(nullptr,nullptr,nullptr,DDBLT_COLORFILL,&color);WeatherEffects::DrawRain(scene,ExpeditionRules::Weather::Storm,3.5);
    painted=reinterpret_cast<const unsigned*>(scene.pixels());check((painted[200*1280+200]&0xffffff)==0x10252a,"rain does not brighten the unrevealed dark map");
    if(failures)return EXIT_FAILURE;
    std::cout<<"Interaction and survival behavior: all checks passed.\n";
}
