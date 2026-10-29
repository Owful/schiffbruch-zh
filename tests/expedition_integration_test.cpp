#include "Expedition.hpp"
#include "ExpeditionRules.hpp"
#include "Crafting.hpp"
#include "extern.hpp"
#include "Action.hpp"
#include "State.hpp"
#include "SceneView.hpp"
#include "PaceRules.hpp"
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
namespace Renderer { void LimitScroll() {} }
namespace NativeDisplay {sf::Vector2i MousePoint(){return {-1,-1};}}
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
        Crafting::Reset();
        Guy={};Guy.Pos={10,30};Guy.Aktion=Action::NOTHING;
        for(auto& v:Guy.Resource)v=70;
        for(auto& column:Scape)for(auto& t:column){t={};t.Art=1;t.Objekt=-1;}
        for(int x=10;x<50;++x)for(int y=10;y<50;++y){Scape[x][y].Art=2;Scape[x][y].Entdeckt=true;}
        Spielzustand=State::GAME;Nacht=false;BootsFahrt=false;PapierText=-1;Tag=1;Stunden=0;Minuten=0;
        for(auto& b:Bmp)b.Anzahl=3;
        Expedition::Reset();
    }
    void finish(){for(int i=0;i<60&&Crafting::Busy();++i)Crafting::Update(100000,true);check(!Crafting::Busy(),"supply action finishes promptly");}
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
    check(Crafting::UsingSupply()&&Guy.Zustand==GUYBINDENUNTEN&&current().water==0,"refill animates before filling");finish();
    check(current().water==3&&current().waterVisited,"river refills waterskin and records water milestone");
    Scape[10][30].Objekt=-1;const float before=Guy.Resource[WASSER];Expedition::UseWaterskin();
    check(Guy.Zustand==GUYTRINKEN&&current().water==3&&Guy.Resource[WASSER]==before,"drinking does not apply an instant effect");finish();
    check(current().water==2&&Guy.Resource[WASSER]==before+25,"portable water improves thirst and consumes one charge");
    const int timeBefore=Minuten;Expedition::UseWaterskin();
    check(current().water==2&&Minuten==timeBefore,"high hydration preserves water and time");
    Guy.Inventar[ROHBLATT]=3;Guy.Inventar[ROHLIANE]=1;Expedition::CraftBandage();
    check(current().bandages==1&&Guy.Inventar[ROHBLATT]==0,"bandage crafting consumes materials");
    Expedition::UseBandage();check(current().bandages==1&&Guy.Zustand==GUYBINDENOBEN,"bandage waits for its binding animation");finish();
    check(current().bandages==0&&Guy.Resource[GESUNDHEIT]==90,"bandage heals 20 and is consumed");
    Guy.Aktiv=true;Guy.Inventar[ROHBLATT]=3;Guy.Inventar[ROHLIANE]=1;Expedition::UseBandage();
    check(current().bandages==0&&Guy.Inventar[ROHBLATT]==3,"commands cannot interrupt a running action");Guy.Aktiv=false;
    const auto saved=current();std::stringstream data(std::ios::in|std::ios::out|std::ios::binary);Expedition::Save(data);
    Expedition::Reset();data.seekg(0);Expedition::Load(data);check(current().water==saved.water&&current().waterskin,"live save and load preserve crafted gear");
    setup();const auto caches=current().caches;
    int count=0;for(const auto& c:caches)if(c.x!=255){++count;check(std::abs(int(c.x)-10)+std::abs(int(c.y)-30)>=7,"cache is away from landing");}
    check(count==3&&caches[3].x==255,"new island has at most three spaced caches");
    {
        NativeSurface frame(1280,720);lpDDSBack=&frame;rcSpielflaeche={0,64,960,656};
        auto& tile=Scape[caches[0].x][caches[0].y];tile.xScreen=500;tile.yScreen=330;
        MousePosition={0,0};
        for(unsigned zoom:{0u,4u,8u}) for(int offset:{0,40})
        {
            PaceRules::zoomStep=zoom;Camera={short(offset),short(offset)};
            DDBLTFX clear{};frame.Blt(nullptr,nullptr,nullptr,DDBLT_COLORFILL,&clear);
            Expedition::DrawCaches(false);Expedition::DrawCaches(true);SceneView::Apply();
            const auto pixel=SceneView::ToScreen(tile.xScreen+31-Camera.x,tile.yScreen+24-Camera.y);
            DDSURFACEDESC2 locked{};frame.Lock(nullptr,&locked,0,nullptr);
            const auto* pixels=static_cast<const unsigned*>(locked.lpSurface);
            check((pixels[pixel.y*1280+pixel.x]&0xffffff)!=0,"cache remains on its world tile during zoom and camera movement");
            frame.Unlock(nullptr);
        }
        Camera={0,0};PaceRules::zoomStep=0;
        for(bool front:{false,true})
        {
            Guy.PosScreen.y=short(tile.yScreen+(front?0:60));DDBLTFX clear{};frame.Blt(nullptr,nullptr,nullptr,DDBLT_COLORFILL,&clear);
            Expedition::DrawCaches(!front);
            DDSURFACEDESC2 locked{};frame.Lock(nullptr,&locked,0,nullptr);const auto* pixels=static_cast<const unsigned*>(locked.lpSurface);
            check((pixels[(tile.yScreen+24)*1280+tile.xScreen+31]&0xffffff)==0,"cache is omitted from the wrong depth layer");frame.Unlock(nullptr);
            Expedition::DrawCaches(front);frame.Lock(nullptr,&locked,0,nullptr);pixels=static_cast<const unsigned*>(locked.lpSurface);
            check((pixels[(tile.yScreen+24)*1280+tile.xScreen+31]&0xffffff)!=0,"cache is drawn on the correct side of the player");frame.Unlock(nullptr);
        }
        lpDDSBack=nullptr;PaceRules::reset();
    }
    setup();Guy.Pos={caches[0].x,caches[0].y};Expedition::OnSearch();
    check(current().claimed==0,"search never opens a sealed cache");
    check(!Crafting::StartSupplyAction(Crafting::OpenCache),"cache requires a hammer");
    Guy.Inventar[ROHHAMMER]=1;
    check(Crafting::StartSupplyAction(Crafting::OpenCache)&&current().claimed==0&&Guy.Zustand==GUYSCHLAGEN,"hammer action starts before rewards");
    Crafting::Update(250000,false);check(Crafting::Job().fraction==0,"paused hammer action waits");
    Crafting::Cancel(false);check(current().claimed==0,"cancelled hammer keeps cache sealed");
    for(unsigned i=0;i<3;++i){setup();Guy.Inventar[ROHHAMMER]=1;Guy.Pos={caches[i].x,caches[i].y};Crafting::StartSupplyAction(Crafting::OpenCache);finish();check((current().claimed&(1u<<i))!=0,"hammer opens each cache");check(!Crafting::StartSupplyAction(Crafting::OpenCache),"cache cannot be farmed repeatedly");}
    setup();Guy.Pos={caches[0].x,caches[0].y};Guy.Inventar[ROHHAMMER]=1;
    for(int item:{ROHAST,ROHSTAMM,ROHSTEIN,ROHBLATT,ROHLIANE})Guy.Inventar[item]=10;
    check(!Crafting::StartSupplyAction(Crafting::OpenCache)&&current().claimed==0,"full inventory leaves the entire cache sealed");
    setup();auto& fire=Scape[Guy.Pos.x][Guy.Pos.y];fire.Objekt=FEUER;fire.Timer=.6f;Guy.Inventar[ROHSTAMM]=2;
    check(Crafting::StartSupplyAction(Crafting::FeedFire)&&fire.Timer==.6f&&Guy.Inventar[ROHSTAMM]==2,"feed action does not consume or refill instantly");
    Crafting::Cancel(false);check(Guy.Inventar[ROHSTAMM]==2,"cancelled feeding keeps wood");
    Crafting::StartSupplyAction(Crafting::FeedFire);finish();check(Guy.Inventar[ROHSTAMM]==1&&std::abs(fire.Timer-.3f)<.001f,"feeding consumes one log and restores fuel");
    Guy.Inventar[ROHSTAMM]=0;Guy.Inventar[ROHAST]=2;Crafting::StartSupplyAction(Crafting::FeedFire);finish();check(Guy.Inventar[ROHAST]==0&&std::abs(fire.Timer-.18f)<.001f,"branches are an alternative fuel");
    Guy.Inventar[ROHSTAMM]=1;Crafting::StartSupplyAction(Crafting::FeedFire);fire.Objekt=-1;finish();check(Guy.Inventar[ROHSTAMM]==1,"extinguished fire cannot consume wood at completion");
    setup();Scape[12][12].Objekt=SOS;Scape[13][12].Objekt=FEUER;Guy.Inventar[ROHFERNROHR]=1;
    Expedition::OnTime(120,0);check(current().signal==120000,"complete signal setup accumulates time");
    Scape[13][12].Objekt=-1;Expedition::OnTime(60,0);check(current().signal==120000,"extinguished fire pauses rescue progress");
    Scape[13][12].Objekt=FEUER;BootsFahrt=true;Expedition::OnTime(60,0);check(current().signal==120000,"being at sea pauses rescue progress");
    BootsFahrt=false;Tag=3;Expedition::OnTime(60,0);check(current().signal==159000,"rain modifies actual live signal progress");
    Tag=1;Expedition::OnTime(201,0);check(current().signal==ExpeditionRules::SignalTarget&&Guy.Aktion==Action::RESCUED,"threshold starts original rescue action");
    if(failures)return EXIT_FAILURE;
    std::cout<<"Live expedition commands: all checks passed.\n";
}
