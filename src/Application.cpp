#include "Application.hpp"

#include "Action.hpp"
#include "Direct.hpp"
#include "Game.hpp"
#include "Math.hpp"
#include "Sound.hpp"
#include "Renderer.hpp"
#include "Routing.hpp"
#include "State.hpp"
#include "World.hpp"

#include "types.hpp"
#include "NativeDisplay.hpp"
#include "ModernUI.hpp"
#include "Menu.hpp"
#include "Interaction.hpp"
#include "SceneView.hpp"
#include "Crafting.hpp"
#include "PaceRules.hpp"
#include "Expedition.hpp"
#include "GameClock.hpp"
#include "ClockRules.hpp"
#include "Nightfall.hpp"
#include "Construction.hpp"
#include "AudioRules.hpp"
#include "DayJournal.hpp"

#include <ctime>
#include <algorithm>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <vector>
#include <iomanip>
#include <imm.h>
#include <filesystem>
#include <stdexcept>

bool Application::checkRevision()
{
    m_window.setVisible(false);m_window.setFramerateLimit(0);
    std::ofstream report("revision-check.json");bool legacyLoaded=false;
    const auto require=[](bool okay,const char* message){if(!okay)throw std::runtime_error(message);};
    const auto finish=[&](){for(int i=0;i<100&&Crafting::Busy();++i)Crafting::Update(100000,true);require(!Crafting::Busy(),"craft did not finish");};
    try
    {
        ModernUI::StartGame(true);PapierText=-1;Frage=-1;Guy.Aktiv=false;Guy.Aktion=Action::NOTHING;
        if(std::filesystem::exists("legacy-save.dat"))
        {
            std::filesystem::copy_file("legacy-save.dat","save.dat",std::filesystem::copy_options::overwrite_existing);
            require(Game::LoadGame(),"legacy save did not load");legacyLoaded=true;
            const float water=Guy.Resource[WASSER];const int day=Tag;const auto pos=Guy.Pos;Game::SaveGame();require(Game::LoadGame()&&Guy.Resource[WASSER]==water&&Tag==day&&Guy.Pos.x==pos.x&&Guy.Pos.y==pos.y,"legacy progress changed during roundtrip");
        }
        {
            Spielzustand=State::GAME;BootsFahrt=false;Guy.Aktiv=true;Guy.Zustand=GUYLINKS;
            Scape[Guy.Pos.x][Guy.Pos.y].LaufZeit=1;Steps=1000;Step=0;
            for(int i=0;i<81;++i){++Bild;Math::CalcGuyKoor();}
            require(Step==54,"land walking cadence did not slow by one third");
            BootsFahrt=true;Step=0;Guy.Zustand=GUYBOOTLINKS;
            for(int i=0;i<81;++i){++Bild;Math::CalcGuyKoor();}
            require(Step==81,"land walking change affected boat movement");
        }
        report<<"{\n  \"legacy_loaded\":"<<(legacyLoaded?"true":"false")<<",\n  \"maps\": [\n";
        const int seeds[]={1,7,42,2026};
        for(int n=0;n<4;++n)
        {
            std::srand(seeds[n]);ModernUI::StartGame(true);PapierText=-1;Frage=-1;Interaction::StopSafely();PaceRules::reset();Stunden=Minuten=0;
            for(int y=0;y<MAXYKACH;++y)for(int x=0;x<MAXXKACH;++x)
                require(Scape[x][y].Entdeckt==(std::abs(x-Guy.Pos.x)<=1&&std::abs(y-Guy.Pos.y)<=1),"new game visibility is not centered 3x3");
            Guy.Resource[WASSER]=Guy.Resource[NAHRUNG]=Guy.Resource[GESUNDHEIT]=100;
            World::AddTime(1,0);World::AddTime(0,1);require(Stunden==0&&Minuten==0,"legacy actions still jump the clock");
            for(int i=0;i<240;++i)GameClock::Advance(100000,true);
            require(Stunden==1&&Minuten==0,"idle clock does not advance one hour in 24 seconds");
            const float waterBefore=Guy.Resource[WASSER],foodBefore=Guy.Resource[NAHRUNG];
            World::AddResource(WASSER,-10);World::AddResource(NAHRUNG,-10);require(Guy.Resource[WASSER]==waterBefore-5&&Guy.Resource[NAHRUNG]==foodBefore-5,"survival loss was not halved");
            for(int item:{ROHAST,ROHSTEIN,ROHLIANE,ROHBLATT,ROHSTAMM})Guy.Inventar[item]=10;
            for(int item:{ROHAXT,ROHEGGE,ROHANGEL,ROHSCHLEUDER,ROHHAMMER,ROHSCHAUFEL,ROHSTREICHHOLZ}){require(Crafting::Make(item),"actual native crafting failed");finish();}
            require(Bmp[BUTTANZUENDEN].Phase==0&&Bmp[BUTTHAUS3].Phase==0,"native actions did not unlock");
            int clicks=0;
            for(bool water:{false,true})
            {
                ZWEID target{-1,-1};
                for(int y=2;y<MAXYKACH-2&&target.x<0;++y)for(int x=2;x<MAXXKACH-2;++x)
                {const auto& t=Scape[x][y];if(t.Begehbar&&(water?(t.Objekt>=FLUSS1&&t.Objekt<=SCHLEUSE6):t.Objekt==BUSCH)){target={short(x),short(y)};break;}}
                require(target.x>=0,"native island has no interaction test target");
                auto& tile=Scape[target.x][target.y];if(!water)tile.Phase=float(Bmp[BUSCH].Anzahl-1);tile.Entdeckt=true;
                const int worldX=tile.xScreen+tile.ObPos.x+Bmp[tile.Objekt].Breite/2,worldY=tile.yScreen+tile.ObPos.y+Bmp[tile.Objekt].Hoehe/2;
                for(unsigned zoom:{0u,4u,8u})
                {
                    Interaction::StopSafely();PapierText=-1;Frage=-1;PaceRules::zoomStep=zoom;Camera={short(worldX-480),short(worldY-360)};Renderer::LimitScroll();World::Generate();Renderer::Zeige();
                    const auto& bitmap=Bmp[tile.Objekt];const auto* pixels=bitmap.Surface->pixels();
                    SceneView::Point screen{-1,-1};
                    const int frame=std::max(0,int(tile.Phase));
                    for(int dy=1;dy<bitmap.Hoehe-1&&screen.x<0;++dy)for(int dx=1;dx<bitmap.Breite-1;++dx)
                    {
                        const auto candidate=SceneView::ToScreen(tile.xScreen+tile.ObPos.x+dx-Camera.x,tile.yScreen+tile.ObPos.y+dy-Camera.y);
                        if(candidate.x<=0||candidate.x>=960||candidate.y<=64||candidate.y>=656)continue;
                        const auto scene=SceneView::ToScene(candidate.x,candidate.y);
                        const int sx=bitmap.rcSrc.left+scene.x+Camera.x-tile.xScreen-tile.ObPos.x;
                        const int sy=bitmap.rcSrc.top+(tile.Reverse?std::max(0,bitmap.Anzahl-1-frame):frame)*bitmap.Hoehe+scene.y+Camera.y-tile.yScreen-tile.ObPos.y;
                        if(sx<0||sy<0||sx>=bitmap.Surface->width()||sy>=bitmap.Surface->height())continue;
                        const auto* p=pixels+4*(sy*bitmap.Surface->width()+sx);
                        if(p[0]!=255||p[1]!=0||p[2]!=255){screen=candidate;break;}
                    }
                    require(screen.x>=0,"zoom target has no visible clickable pixel");
                    ModernUI::HandleClick(screen.x,screen.y);
                    require((RouteZiel.x==target.x&&RouteZiel.y==target.y)||Guy.Aktion==(water?Action::DRINK:Action::EAT),"zoomed click selected the wrong food or water target");++clicks;
                }
            }
            Interaction::StopSafely();Game::SaveGame();require(Game::LoadGame()&&Guy.Inventar[ROHHAMMER]==1&&Guy.Inventar[ROHSTREICHHOLZ]==1&&PaceRules::zoomStep==8,"new save did not preserve tools or zoom");
            report<<"    {\"seed\":"<<seeds[n]<<",\"action_time_jump\":0,\"idle_minutes_per_24_seconds\":60,\"loss\":5,\"tools_crafted\":7,\"zoomed_clicks\":"<<clicks<<",\"save_roundtrip\":true}"<<(n==3?"":",")<<"\n";
        }
        ModernUI::StartGame(true);PapierText=-1;Frage=-1;Interaction::StopSafely();
        for(int item:{ROHAST,ROHSTEIN,ROHLIANE,ROHBLATT,ROHSTAMM})Guy.Inventar[item]=10;
        Guy.Resource[WASSER]=Guy.Resource[NAHRUNG]=Guy.Resource[GESUNDHEIT]=100;
        const auto click=[&](int x,int y){ModernUI::DrawPanel();require(ModernUI::HandleClick(x,y),"panel click was not handled");finish();};
        ModernUI::HandleKey(sf::Keyboard::C);click(1010,217);click(1200,260);
        require(Guy.Inventar[ROHAXT]==1,"first tool card did not craft stone axe");
        click(1200,640);click(1200,640);click(1200,260);
        require(Guy.Inventar[ROHHAMMER]==1&&Guy.Inventar[ROHSCHLEUDER]==0,"next group or disabled last-page button selected wrong recipe");
        click(1010,640);click(1200,452);require(Guy.Inventar[ROHANGEL]==1,"previous group did not return to first recipes");
        const unsigned startZoom=PaceRules::zoomStep;
        ModernUI::HandleWheel(-1,1100,400);ModernUI::HandleWheel(-1,1100,400);ModernUI::HandleWheel(-1,1100,400);click(1200,260);
        require(Guy.Inventar[ROHSCHLEUDER]==1&&PaceRules::zoomStep==startZoom,"recipe wheel scrolled wrong row or zoomed map");
        Guy.Inventar[ROHSTREICHHOLZ]=0;Crafting::SyncUnlocks();ModernUI::HandleKey(sf::Keyboard::B);click(1010,166);click(1190,330);click(1200,452);
        require(Guy.Inventar[ROHSTREICHHOLZ]==1,"locked fire action did not open matching tool group");
        ModernUI::HandleKey(sf::Keyboard::C);click(1100,217);click(1200,260);
        require(Expedition::GetInfo().waterskin&&Expedition::GetInfo().water==0,"waterskin craft also filled or used water");
        Scape[Guy.Pos.x][Guy.Pos.y].Objekt=FLUSS1;Scape[Guy.Pos.x][Guy.Pos.y].Entdeckt=true;
        ModernUI::DrawPanel();ModernUI::HandleClick(1010,490);
        require(Crafting::UsingSupply()&&Expedition::GetInfo().water==0&&Guy.Zustand==GUYBINDENUNTEN,"fill button applied water before its animation");
        finish();require(Expedition::GetInfo().water==3,"visible fill button did not fill waterskin");
        Guy.Resource[WASSER]=50;ModernUI::DrawPanel();ModernUI::HandleClick(1200,490);
        require(Crafting::UsingSupply()&&Guy.Zustand==GUYTRINKEN&&Guy.Resource[WASSER]==50&&Expedition::GetInfo().water==3,"drink button applied an instant effect");
        Crafting::Update(250000,false);require(Crafting::Job().fraction==0,"paused drink still advances");
        Crafting::Update(250000,true);Renderer::Zeige();require(Bmp[GUYTRINKEN].Phase==1,"drinking sprite does not advance");
        Game::SaveGame();require(Game::LoadGame()&&Crafting::UsingSupply()&&Guy.Zustand==GUYTRINKEN,"native drinking save did not resume");
        finish();require(Expedition::GetInfo().water==2&&Guy.Resource[WASSER]==75,"visible drink button did not drink independently of filling");
        Guy.Resource[WASSER]=Guy.Resource[NAHRUNG]=Guy.Resource[GESUNDHEIT]=50;click(1200,356);click(1200,356);
        require(Expedition::GetInfo().bandages==2&&Guy.Resource[GESUNDHEIT]==50,"bandage card used supplies instead of crafting");
        Direct::QueueKeyPress(sf::Keyboard::J);Direct::CheckKey();
        require(Crafting::UsingSupply()&&Guy.Zustand==GUYBINDENOBEN&&Guy.Resource[GESUNDHEIT]==50,"bandage shortcut skipped its animation");finish();
        require(Expedition::GetInfo().bandages==1&&Guy.Resource[GESUNDHEIT]==70,"bandage animation failed to apply one heal");
        ModernUI::HandleKey(sf::Keyboard::B);click(1010,166);click(1010,385);click(1200,260);
        require(Guy.Inventar[ROHFERNROHR]==0&&Expedition::CurrentNotice().find(L"沉船")!=std::wstring::npos,"locked lookout did not show salvage instructions");
        ModernUI::HandleWheel(1,1100,400);require(PaceRules::zoomStep==startZoom,"acquisition list changed scene zoom");
        ModernUI::HandleWheel(1,480,400);require(PaceRules::zoomStep==startZoom+1,"world wheel did not zoom");
        ModernUI::HandleKey(sf::Keyboard::Escape);ModernUI::HandleWheel(1,480,400);
        require(PaceRules::zoomStep==startZoom+1,"pause menu allowed map zoom");ModernUI::HandleKey(sf::Keyboard::Escape);
        ModernUI::StartGame(true);PapierText=-1;Frage=-1;Interaction::StopSafely();
        Guy.Inventar[ROHAST]=Guy.Inventar[ROHSTEIN]=10;
        require(Crafting::Make(ROHAXT),"saved job did not start");for(int i=0;i<4;++i)Crafting::Update(250000,true);
        const float fraction=Crafting::Job().fraction;Game::SaveGame();require(Game::LoadGame()&&Crafting::Busy()&&Crafting::Job().fraction==fraction,"active crafting job did not survive native save/load");finish();
        require(Guy.Inventar[ROHAXT]==1&&Guy.Inventar[ROHAST]==9,"resumed job did not consume exactly one recipe");

        // Exercise real route following and tent-entry animations at night.
        const auto nightSetup=[&](){
            ModernUI::StartGame(true);PapierText=-1;Frage=-1;Interaction::StopSafely();
            for(int y=24;y<=34;++y)for(int x=24;x<=34;++x){auto& t=Scape[x][y];t.Objekt=-1;t.Typ=0;t.Art=0;t.Begehbar=t.Entdeckt=true;t.Phase=-1;}
            Guy.Pos={27,27};auto& t=Scape[27][27];Guy.PosScreen={short(t.xScreen+(EckKoor[0][0].x+EckKoor[0][2].x)/2),short(t.yScreen+(EckKoor[0][1].y+EckKoor[0][3].y)/2)};Guy.PosAlt=Guy.PosScreen;
            for(int item:{ROHAST,ROHSTEIN,ROHLIANE,ROHBLATT,ROHSTAMM})Guy.Inventar[item]=10;
            Guy.Inventar[ROHAXT]=1;Stunden=11;Minuten=59;LastBild=60;Bild=0;World::Generate();
        };
        nightSetup();Scape[30][27].Objekt=ZELT;Scape[30][27].Phase=0;
        require(Crafting::Make(ROHHAMMER),"dusk craft did not start");Stunden=12;Minuten=0;Nightfall::Update(100000,true);
        require(Guy.Aktion==Action::CRAFT,"nightfall interrupted crafting");finish();Nightfall::Update(100000,true);
        require(!Nightfall::Line().empty(),"finished dusk craft did not speak");for(int i=0;i<8;++i)Nightfall::Update(250000,true);
        require(Nightfall::Walking()&&Guy.Pos.x==27&&RouteZiel.x==30,"nightfall did not route to nearby tent");
        for(int i=0;i<1200&&Nightfall::Walking();++i){++Bild;Math::Animationen();Nightfall::Update(16667,true);}
        require(Guy.Pos.x==30&&Guy.Pos.y==27&&Guy.Aktion==Action::DAY_END,"real night walk failed to reach tent");
        for(int i=0;i<1200&&Guy.Zustand!=GUYGEHINZELT&&Guy.Zustand!=GUYSCHLAFZELT;++i){++Bild;Math::Animationen();if(!Guy.Aktiv)Action::handler(Guy.Aktion);}
        require(Guy.Zustand==GUYGEHINZELT||Guy.Zustand==GUYSCHLAFZELT,"arrival did not play entering-tent animation");
        nightSetup();Guy.Aktion=Action::TENT;Guy.AkNummer=0;Bmp[ZELT].First=false;Stunden=12;Minuten=0;
        Nightfall::Update(100000,true);
        for(int i=0;i<6000&&Guy.Aktion==Action::TENT;++i){++Bild;Math::Animationen();if(!Guy.Aktiv)Action::handler(Guy.Aktion);Nightfall::Update(16667,true);}
        require(Scape[27][27].Objekt==ZELT&&Scape[27][27].Phase==0&&!Nightfall::Line().empty(),"unfinished tent did not finish before resting");
        nightSetup();Stunden=2;Minuten=0;Bmp[FELD].Rohstoff[ROHAST]=4;Bmp[FELD].First=false;Guy.Inventar[ROHEGGE]=1;Guy.Inventar[ROHAST]=1;Guy.Aktion=Action::FIELD;
        for(int i=0;i<6000&&Guy.Aktion!=Action::CANCEL;++i){++Bild;Math::Animationen();if(!Guy.Aktiv)Action::handler(Guy.Aktion);}
        require(Guy.Aktion==Action::CANCEL&&Scape[27][27].AkNummer>0,"field did not pause for actual missing materials");
        const int savedStage=Scape[27][27].AkNummer;const auto missing=Construction::Missing(Scape[27][27]);
        require(missing==L"树枝3","paused field shortage did not reflect its remaining materials");
        for(int i=0;i<1200&&Guy.Aktion!=Action::NOTHING;++i){++Bild;Math::Animationen();if(!Guy.Aktiv)Action::handler(Guy.Aktion);}
        require(Guy.Aktion==Action::NOTHING,"field shortage cancel did not release player");
        Guy.Pos={29,27};auto& away=Scape[29][27];Guy.PosScreen={short(away.xScreen+27),short(away.yScreen+22)};Guy.PosAlt=Guy.PosScreen;Guy.Inventar[ROHAST]=10;
        const auto& field=Scape[27][27];const int fieldX=field.xScreen+27,fieldY=field.yScreen+22;Camera={short(fieldX-480),short(fieldY-360)};World::Generate();Renderer::Zeige();
        const auto fieldScreen=SceneView::ToScreen(fieldX-Camera.x,fieldY-Camera.y);ModernUI::HandleClick(fieldScreen.x,fieldScreen.y);
        require(Guy.Aktiv&&RouteZiel.x==27&&RouteZiel.y==27,"click on unfinished field did not start a return route");
        for(int i=0;i<1200&&Guy.Aktion!=Action::FIELD;++i){++Bild;Math::Animationen();Interaction::Update();}
        require(Guy.Aktion==Action::FIELD&&Scape[27][27].AkNummer==savedStage,"returning to field reset its existing progress");
        for(int i=0;i<6000&&Guy.Aktion==Action::FIELD;++i){++Bild;Math::Animationen();if(!Guy.Aktiv)Action::handler(Guy.Aktion);}
        require(Scape[27][27].Phase==0&&Guy.Aktion==Action::NOTHING,"resumed field did not finish cultivating");
        Gitter=false;Direct::QueueKeyPress(sf::Keyboard::G);Direct::ClearInput();Direct::CheckKey();
        require(!Gitter,"focus reset replayed an old grid key");
        NativeDisplay::Resize(640,360);NativeDisplay::Resize(1280,720);require(!Gitter,"window resize changed grid preference");
        for(bool sea:{true,false})
        {
            ZWEID water{-1,-1};for(int y=2;y<MAXYKACH-2&&water.x<0;++y)for(int x=2;x<MAXXKACH-2;++x){const int object=Scape[x][y].Objekt;if(sea?object==MEERWELLEN:object>=FLUSS1&&object<=SCHLEUSE6){water={short(x),short(y)};break;}}
            require(water.x>=0,"water animation test has no target");auto& tile=Scape[water.x][water.y];tile.Entdeckt=true;tile.Phase=0;Camera={short(tile.xScreen-450),short(tile.yScreen-330)};World::Generate();Renderer::Zeige();
            const auto hash=[](){std::uint64_t value=1469598103934665603ull;const auto* bytes=lpDDSBack->pixels();for(int y=250;y<430;++y)for(int x=350;x<650;++x){const auto* p=bytes+4*(y*1280+x);value=(value^p[0]^unsigned(p[1]<<8)^unsigned(p[2]<<16))*1099511628211ull;}return value;};
            const auto first=hash();tile.Phase=1;Renderer::Zeige();require(hash()!=first,"water animation frames do not change rendered pixels");
            tile.Phase=0;bool advanced=false;for(int i=0;i<120&&!advanced;++i){++Bild;Math::Animationen();advanced=tile.Phase!=0;}require(advanced,"water animation scheduler is stopped");
        }
        // Dusk must not pause the clock or interrupt work; life-threatening alerts still do.
        ModernUI::StartGame(true);PapierText=-1;Frage=-1;Interaction::StopSafely();
        Guy.Resource[WASSER]=Guy.Resource[NAHRUNG]=Guy.Resource[GESUNDHEIT]=100;
        Guy.Inventar[ROHAXT]=0;Guy.Inventar[ROHAST]=Guy.Inventar[ROHSTEIN]=10;
        Scape[Guy.Pos.x][Guy.Pos.y].Objekt=-1;BootsFahrt=false;Stunden=10;Minuten=0;
        require(Crafting::Make(ROHAXT),"dusk reminder test could not start work");
        ModernUI::CheckSafety();
        require(ModernUI::DuskNoticeVisible()&&!ModernUI::OverlayActive()&&Crafting::Busy()&&Guy.Aktion==Action::CRAFT,"dusk reminder blocked or cancelled work");
        Crafting::Update(250000,true);GameClock::Advance(200000,true);GameClock::Advance(200000,true);
        require(Crafting::Job().fraction>0&&Minuten==1,"work or clock stopped while dusk reminder was visible");
        Guy.Resource[GESUNDHEIT]=20;ModernUI::CheckSafety();
        require(ModernUI::OverlayActive()&&!Crafting::Busy(),"critical health no longer pauses and stops work");

        // Exercise the production mixer and decoded assets in the isolated runtime.
        ModernUI::StartGame(true);PapierText=-1;Interaction::StopSafely();
        for(auto& column:Scape)for(auto& tile:column){tile.Entdeckt=true;tile.Objekt=-1;}
        Guy.Pos={20,20};Scape[21][20].Objekt=MEERWELLEN;Scape[20][21].Objekt=FLUSS1;Scape[19][20].Objekt=BAUM1;
        Sound::SetEnabled(true);Sound::SetMix(Sound::Music,55);Sound::SetMix(Sound::Ambience,68);Sound::SetMix(Sound::Effects,80);
        require(Sound::Inspect().loaded==Sound::COUNT-1,"some redesigned audio assets did not decode");
        Sound::PlaySound(Sound::CLICK,100);Sound::PlaySound(Sound::CLICK2,100);
        require(!Sound::IsPlaying(Sound::CLICK)&&!Sound::IsPlaying(Sound::CLICK2),"mouse click audio was still played");
        for(int i=0;i<60;++i)Sound::Update(16667,true,false);
        require(Sound::Inspect().ambience>0,"nearby water and trees did not enter the ambient mix");
        Sound::PlaySound(Sound::DRINK,50);
        require(std::abs(Sound::CurrentVolume(Sound::DRINK)-AudioRules::gain(Wav[Sound::DRINK].Volume,50,80))<.1f,"requested sound volume is ignored");
        Sound::Update(16667,false,false);require(!Sound::IsPlaying(Sound::DRINK),"focus loss did not pause an action sound");
        Guy.Aktiv=true;Guy.Zustand=GUYFAELLEN;Sound::Update(16667,true,false);
        require(Wav[Sound::LOG].Loop&&Sound::IsPlaying(Sound::LOG)&&Sound::CurrentVolume(Sound::LOG)>50,"original accented axe loop did not start");
        Sound::Update(16667,true,true);require(!Sound::IsPlaying(Sound::LOG),"pause did not pause the original axe rhythm");
        Sound::Update(16667,true,false);require(Sound::IsPlaying(Sound::LOG),"axe rhythm did not resume after pause");
        Guy.Aktiv=false;Guy.Zustand=GUYLINKS;Sound::Update(16667,true,false);require(!Sound::IsPlaying(Sound::LOG),"stopped chopping left the axe loop playing");
        Sound::SetEnabled(false);require(Sound::Inspect().playing==0,"mute left an audio player running");
        Sound::SetEnabled(true);Sound::Update(16667,true,false);
        Guy.Resource[WASSER]=50;Guy.Inventar[ROHBLATT]=Guy.Inventar[ROHLIANE]=10;Expedition::CraftWaterskin();Expedition::FillWaterskin();
        Sound::Update(16667,true,false);require(Sound::IsPlaying(Sound::FILL),"filling did not start its own sound");
        Crafting::Cancel(false);Sound::Update(16667,true,false);require(!Sound::IsPlaying(Sound::FILL),"cancelled filling left its sound playing");
        Expedition::FillWaterskin();finish();Expedition::DrinkWaterskin();Sound::Update(16667,true,false);
        require(Sound::IsPlaying(Sound::DRINK),"waterskin animation did not start drinking audio");
        Sound::Update(16667,true,true);require(!Sound::IsPlaying(Sound::DRINK),"pause did not pause the drinking sound");
        Crafting::Cancel(false);Sound::Update(16667,true,false);
        Spielzustand=State::LOGO;for(int i=0;i<160;++i)Sound::Update(16667,true,false);
        require(Sound::Inspect().music>0&&Sound::Inspect().ambience<.2f,"leaving the island did not fade ambience into menu music");
        ModernUI::DrawStart();ModernUI::HandleClick(160,476);ModernUI::DrawStart();
        const int oldMusic=Sound::GetMix(Sound::Music),oldEnvironment=Sound::GetMix(Sound::Ambience);
        ModernUI::HandleClick(862,355);ModernUI::DrawStart();ModernUI::HandleClick(760,402);
        require(Sound::GetMix(Sound::Music)==std::min(100,oldMusic+10)&&Sound::GetMix(Sound::Ambience)==std::max(0,oldEnvironment-10),"visible sound mix buttons changed the wrong channel");
        ModernUI::DrawStart();ModernUI::HandleClick(430,248);require(Soundzustand==0&&Sound::Inspect().playing==0,"visible mute button left audio running");
        ModernUI::DrawStart();ModernUI::HandleClick(430,248);require(Soundzustand==1,"visible sound toggle did not reenable audio");
        ModernUI::DrawStart();ModernUI::HandleClick(600,548);ModernUI::DrawStart();ModernUI::HandleClick(150,350);
        require(Spielzustand==State::GAME&&!ModernUI::OverlayActive(),"sound settings return button did not restore the main menu controls");
        // Exercise the actual overnight transition, save, resume, music and skip.
        ModernUI::StartGame(true);PapierText=-1;Interaction::StopSafely();
        BootsFahrt=false;Guy.Resource[GESUNDHEIT]=100;auto& nightTile=Scape[Guy.Pos.x][Guy.Pos.y];
        Tag=5;Stunden=11;nightTile.Art=2;nightTile.Objekt=-1;Guy.Inventar[ROHSTEIN]=9;Guy.Aktion=Action::SEARCH;Guy.AkNummer=10;Action::handler(Guy.Aktion);
        require(Guy.Inventar[ROHSTEIN]==10&&DayJournal::loot[DayJournal::Stone]==1,"search journal did not record the actual quantity received at the inventory limit");
        PapierText=-1;Guy.Aktiv=false;nightTile.Objekt=ZELT;nightTile.Phase=0;Stunden=12;
        DayJournal::Mark(Tag,DayJournal::Food);DayJournal::Mark(Tag,DayJournal::Craft);DayJournal::RecordLoot(Tag,DayJournal::Branch,3);
        Guy.Aktion=Action::DAY_END;Guy.AkNummer=6;Guy.Aktiv=false;Action::handler(Guy.Aktion);
        require(Nacht&&DayJournal::Active()&&PapierText==-1&&Guy.Resource[GESUNDHEIT]==95.f,"overnight journal did not preserve the shelter outcome");
        Renderer::Zeige();const auto* nightPixels=lpDDSBack->pixels();require(nightPixels[4*(120*1280+250)]!=0,"night journal was not rendered");
        {std::ofstream preview("night-preview.ppm",std::ios::binary);preview<<"P6\n1280 720\n255\n";for(int i=0;i<1280*720;++i){const char rgb[]={char(nightPixels[4*i+2]),char(nightPixels[4*i+1]),char(nightPixels[4*i])};preview.write(rgb,3);}}
        for(int i=0;i<180;++i)Sound::Update(16667,true,false);
        require(Sound::CurrentVolume(Sound::LOGO)>0&&Sound::IsPlaying(Sound::LOGO),"night music did not fade in");
        Game::SaveGame();Nacht=false;DayJournal::Reset();require(Game::LoadGame()&&Nacht&&DayJournal::Active()&&(DayJournal::events&DayJournal::Food)&&(DayJournal::events&DayJournal::Craft)&&DayJournal::loot[DayJournal::Branch]==3,"night journal or daily facts did not survive loading");
        require(Guy.Resource[GESUNDHEIT]==95.f,"loading a night applied the sleep penalty twice");
        for(int i=0;i<16;++i)require(!DayJournal::Advance(250000,true),"night ended before the skip became available");
        ModernUI::HandleKey(sf::Keyboard::Return);require(DayJournal::Advance(1,true),"night skip did not advance after reading time");
        Guy.Aktiv=false;Action::handler(Guy.Aktion);
        require(!Nacht&&Tag==6&&DayJournal::events==0&&Guy.Resource[GESUNDHEIT]==95.f,"morning did not clear the journal or repeated sleep effects");
        Sound::SetEnabled(false);
        report<<"  ],\n  \"panel_interactions\":true,\n  \"timed_supply_actions\":true,\n  \"active_drink_save\":true,\n  \"active_craft_save\":true,\n  \"night_craft_walk_tent_entry\":true,\n  \"night_construction_finishes\":true,\n  \"paused_field_return_and_finish\":true,\n  \"remaining_material_shortage\":true,\n  \"focus_and_grid\":true,\n  \"river_and_sea_animation\":true,\n  \"audio_assets_and_mixer\":true,\n  \"silent_ui_clicks\":true,\n  \"nonblocking_dusk_and_critical_pause\":true,\n  \"passed\":true\n}\n";return true;
    }
    catch(const std::exception& error){report<<"\n  \"error\":\""<<error.what()<<"\",\"passed\":false\n}\n";return false;}
}

Application::Application(const std::string& name, HINSTANCE instance_handle)
    : m_window({MAXX, MAXY}, name, sf::Style::Default)
      , m_name(name)
      , m_time(std::time(nullptr))
{
    // Set globals
    m_window.setTitle(sf::String(L"漂流之境 · 交互修正版"));
    // Gameplay has no text fields; keep Chinese IME composition from consuming shortcuts.
    ImmAssociateContext(m_window.getSystemHandle(),nullptr);
    // Start with readable text on high-resolution displays; the window remains resizable.
    ShowWindow(m_window.getSystemHandle(), SW_MAXIMIZE);
    m_window.setKeyRepeatEnabled(false);
    m_window.setFramerateLimit(60);
    NativeDisplay::Bind(m_window);
    g_hInst = instance_handle;

    Soundzustand = 1;
    Sound::Init();

    Direct::InitDDraw(m_window.getSystemHandle());

    PapierText=-1;Frage=-1;
    Spielzustand = State::LOGO;
    Game::InitWaves(); // Nur zum Wavinitialisieren

    srand(static_cast<unsigned>(std::time(nullptr))); // Random initialisieren
}

void Application::process_events()
{
    sf::Event event;
    while (m_window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
        {
            m_window.close();
        }
        else if(event.type==sf::Event::LostFocus){Direct::ClearInput();}
        else if(event.type==sf::Event::GainedFocus)
        {
            Direct::ClearInput();NativeDisplay::Resize(m_window.getSize().x,m_window.getSize().y);
            if(Spielzustand==State::GAME)World::Generate();
        }
        else if (event.type == sf::Event::KeyPressed)
        {
#ifdef SCHIFFBRUCH_QA_PREVIEW
            if(event.key.code==sf::Keyboard::F2){Guy.Resource[WASSER]=10;Guy.Resource[NAHRUNG]=15;Guy.Resource[GESUNDHEIT]=22;}
#endif
            if(!ModernUI::HandleKey(event.key.code))Direct::QueueKeyPress(event.key.code);
        }
        else if(event.type==sf::Event::Resized){NativeDisplay::Resize(event.size.width,event.size.height);}
        else if(event.type==sf::Event::MouseButtonPressed&&event.mouseButton.button==sf::Mouse::Left)
        {
            const auto p=m_window.mapPixelToCoords({event.mouseButton.x,event.mouseButton.y});
            if(!ModernUI::HandleClick(int(p.x),int(p.y))&&Spielzustand==State::GAME){const auto scene=SceneView::ToScene(int(p.x),int(p.y));Interaction::Cancel();MousePosition={short(scene.x),short(scene.y)};Math::MouseInSpielflaeche(0,1,0,0);}
        }
        else if(event.type==sf::Event::MouseWheelScrolled)
        {const auto p=m_window.mapPixelToCoords({event.mouseWheelScroll.x,event.mouseWheelScroll.y});ModernUI::HandleWheel(event.mouseWheelScroll.delta,int(p.x),int(p.y));}
        else if (event.type == sf::Event::KeyReleased)
        {
            if (event.key.code == sf::Keyboard::F4)
            {
                m_window.close();
            }
        }
    }
}

void Application::run()
{
    LastBild = 60;
    Bild = 0;
    sf::Clock simulation;
    while (m_window.isOpen())
    {
        const auto elapsed=simulation.restart().asMicroseconds();
        sf::Clock frame;
        process_events();
        if(ModernUI::ShouldQuit())m_window.close();
        if (!m_window.isOpen()) break;

        ++Bild;
        const auto now = std::time(nullptr);
        if (now >= m_time + 5)
        {
            LastBild = static_cast<short>(std::max<std::time_t>(1, Bild / std::max<std::time_t>(1, now-m_time)));
            Bild = 0;
            m_time = now;
        }

        if (Spielzustand == State::LOGO)
        {
            Renderer::ZeigeLogo();
        }
        else if (Spielzustand == State::INTRO || Spielzustand == State::RESCUED)
        {
            if (Direct::CheckKey() == 0) { m_window.close(); break; }
            Math::Animationen();
            if (!Guy.Aktiv) Action::handler(Guy.Aktion);
            Renderer::ZeigeIntro();
        }
        else if (Spielzustand == State::GAME)
        {
            ModernUI::CheckSafety();
            const bool running=m_window.hasFocus()&&!ModernUI::OverlayActive()&&PapierText==-1;
            GameClock::Advance(elapsed,m_window.hasFocus());
            if(!ModernUI::OverlayActive()&&m_window.hasFocus())
            {
            World::CheckSpzButton();
            Direct::CheckMouse();
            if (Direct::CheckKey() == 0) { m_window.close(); break; }
            Renderer::LimitScroll();
            Math::Animationen();
            Crafting::Update(elapsed,running);
            if(DayJournal::Advance(elapsed,running))Guy.Aktiv=false;
            Interaction::Update();
            ModernUI::CheckSafety();
            if(!ModernUI::OverlayActive())
            if (!Guy.Aktiv&&!DayJournal::Active()) Action::handler(Guy.Aktion);
            Nightfall::Update(elapsed,m_window.hasFocus()&&!ModernUI::OverlayActive()&&PapierText==-1);
            }
            Renderer::Zeige();
        }
        else if (Spielzustand == State::OUTRO)
        {
            if (Direct::CheckKey() == 0) { m_window.close(); break; }
            Renderer::ZeigeAbspann();
        }
        Sound::Update(elapsed,m_window.hasFocus(),ModernUI::OverlayActive()||PapierText!=-1);
        const auto remaining = sf::milliseconds(16) - frame.getElapsedTime();
        if (remaining > sf::Time::Zero) sf::sleep(remaining);
    }
    if (Spielzustand == State::GAME && Guy.Resource[GESUNDHEIT] > 10 && Guy.Aktion != Action::DEATH)
        Game::SaveGame();
    Direct::finiObjects();
    SetCursor(LoadCursor(nullptr, IDC_ARROW));
}

void Application::update()
{
}

void Application::prepareExperiencePreview(bool danger)
{
    // Isolated developer preview; never used by the normal launcher.
    ModernUI::StartGame(true);Tag=3;Stunden=3;Minuten=0;
    bool placed=false;
    for(int y=2;y<MAXYKACH-2;++y)for(int x=2;x<MAXXKACH-2;++x)
    {
        auto& t=Scape[x][y];t.Entdeckt=true;
        if(t.Objekt==BUSCH)t.Phase=float(Bmp[BUSCH].Anzahl-1);
        if(!placed&&t.Objekt==BUSCH&&Scape[x-1][y].Begehbar&&Scape[x-1][y].Art!=1)
        {Guy.Pos={short(x-1),short(y)};placed=true;}
    }
    auto& t=Scape[Guy.Pos.x][Guy.Pos.y];
    Guy.PosScreen={short(t.xScreen+(EckKoor[t.Typ][0].x+EckKoor[t.Typ][2].x)/2),short(t.yScreen+(EckKoor[t.Typ][1].y+EckKoor[t.Typ][3].y)/2)};
    Guy.PosAlt=Guy.PosScreen;Guy.Aktiv=false;Guy.Aktion=Action::NOTHING;Guy.Inventar[ROHAXT]=1;
    for(int id:{ROHSTAMM,ROHAST,ROHBLATT,ROHLIANE})Guy.Inventar[id]=0;
    Guy.Resource[WASSER]=danger?10.f:75.f;Guy.Resource[NAHRUNG]=danger?15.f:60.f;Guy.Resource[GESUNDHEIT]=danger?22.f:90.f;
    World::Generate();Camera={short(Guy.PosScreen.x-480),short(Guy.PosScreen.y-360)};Renderer::LimitScroll();
    m_window.setVisible(true);ShowWindow(m_window.getSystemHandle(),SW_MAXIMIZE);
}

void Application::prepareWorkshopPreview()
{
    prepareExperiencePreview(false);Tag=1;Stunden=3;Minuten=0;
    auto& tile=Scape[Guy.Pos.x][Guy.Pos.y];tile.Objekt=FELD;tile.Phase=5;tile.AkNummer=9;tile.Typ=0;tile.ObPos={5,15};
    for(auto& need:tile.Rohstoff)need=0;tile.Rohstoff[ROHAST]=3;tile.Rohstoff[ROHBLATT]=2;
    Guy.Inventar[ROHAST]=1;Guy.Inventar[ROHSTEIN]=10;Guy.Inventar[ROHLIANE]=10;Guy.Inventar[ROHAXT]=0;
    Guy.PosScreen={short(tile.xScreen+27),short(tile.yScreen+22)};Guy.PosAlt=Guy.PosScreen;tile.GPosAlt=Guy.PosScreen;
    for(auto& resource:Guy.Resource)resource=100;Crafting::SyncUnlocks();World::Generate();
    Camera={short(Guy.PosScreen.x-480),short(Guy.PosScreen.y-360)};Renderer::LimitScroll();
    Expedition::Notify(L"隔离验证场景：未完成农田只显示剩余用料；可打开制作检查动作和缺口。");
}

void Application::benchmark()
{
    m_window.setVisible(false);m_window.setFramerateLimit(0);m_window.setVerticalSyncEnabled(false);
    ModernUI::StartGame(true);
    for(auto& column:Scape)for(auto& tile:column)tile.Entdeckt=true;
    World::Generate();LastBild=60;
    std::ofstream out("render-benchmark.json");out << "{\n  \"scenes\": [\n";
    const auto measure=[&](const char* name,int count,bool comma)
    {
        std::vector<double> samples;
        for(int i=0;i<count+10;++i)
        {
            sf::Clock clock;Renderer::Zeige();
            if(i>=10)samples.push_back(clock.getElapsedTime().asMicroseconds()/1000.0);
        }
        std::sort(samples.begin(),samples.end());double sum=0;for(auto t:samples)sum+=t;
        out << "    {\"scene\":\"" << name << "\",\"mean_ms\":" << sum/samples.size() << ",\"p95_ms\":" << samples[samples.size()*95/100] << "}" << (comma?",":"") << "\n";
    };
    HauptMenue=Menu::ACTION;measure("actions",100,true);
    HauptMenue=Menu::BUILD;measure("build",100,true);
    HauptMenue=Menu::INVENTORY;measure("inventory",100,true);
    ModernUI::HandleKey(sf::Keyboard::C);measure("craft",100,true);
    Tag=3;measure("rain",100,true);Tag=5;measure("storm",100,true);Tag=1;
    ModernUI::HandleKey(sf::Keyboard::Escape);measure("pause",100,true);
    ModernUI::HandleKey(sf::Keyboard::Escape);
    PapierText=Renderer::DrawText(INTROTEXT,TXTPAPIER,1);measure("chinese_dialog",100,false);
    out << "  ]\n}\n";
}
