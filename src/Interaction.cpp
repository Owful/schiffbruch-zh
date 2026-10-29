#include "Interaction.hpp"
#include "Action.hpp"
#include "Routing.hpp"
#include "Expedition.hpp"
#include "State.hpp"
#include "Renderer.hpp"
#include "Construction.hpp"
#include "Crafting.hpp"
#include "Campfires.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Interaction
{
    namespace
    {
        ZWEID target{-1,-1};
        short pending=Action::NOTHING;
        bool valid(int x,int y){return x>0&&y>0&&x<MAXXKACH-1&&y<MAXYKACH-1;}
        short actionAt(int x,int y,Intent intent)
        {
            if(!valid(x,y)||!Scape[x][y].Entdeckt||!Scape[x][y].Begehbar)return Action::NOTHING;
            const auto& t=Scape[x][y];const int o=t.Objekt;
            if(intent==Intent::OpenCache)return Expedition::CacheAt(x,y)>=0?Action::OPEN_CACHE:Action::NOTHING;
            if(intent==Intent::FeedFire)return o==FEUER?Action::FEED_FIRE:Action::NOTHING;
            if(intent==Intent::ResumeBuild)
            {
                if(o<FELD||o>FEUERSTELLE||t.Phase<Bmp[o].Anzahl)return Action::NOTHING;
                switch(o){case FELD:return Action::FIELD;case ZELT:return Action::TENT;case BOOT:return Action::BOAT;case ROHR:return Action::PIPE;case SOS:return Action::SOS_SIGN;case HAUS1:return Action::HOUSE1;case HAUS2:return Action::HOUSE2;case HAUS3:return Action::HOUSE3;case FEUERSTELLE:return Action::FIREPLACE;default:return Action::NOTHING;}
            }
            if(intent!=Intent::Chop)
            {
                if(intent!=Intent::Drink&&(o==BUSCH||o==FELD)&&t.Phase==Bmp[o].Anzahl-1)return Action::EAT;
                if(intent!=Intent::Eat&&((o>=FLUSS1&&o<=SCHLEUSE6)||(o==ROHR&&t.Phase==1)))return Action::DRINK;
            }
            // Felling is deliberate: neither a movement click nor general interaction implies it.
            if(intent==Intent::Chop&&o>=BAUM1&&o<=BAUM4)return Action::LOG;
            return Action::NOTHING;
        }
        bool begin(int x,int y,short action)
        {
            if(Guy.Aktiv||Guy.Aktion!=Action::NOTHING){Expedition::Notify(L"正在行动，请先点击“停止行动”，再选择目标。");return true;}
            if(BootsFahrt){Expedition::Notify(L"请先靠岸，再收集岛上的食物与木材。");return true;}
            if(action==Action::OPEN_CACHE&&Guy.Inventar[ROHHAMMER]<1){Expedition::CacheHint(x,y);return true;}
            if(action==Action::FEED_FIRE&&Guy.Inventar[ROHSTAMM]<1&&Guy.Inventar[ROHAST]<2){Expedition::Notify(L"添柴需要原木 1，或树枝 2。请先收集木材。");return true;}
            if(action==Action::LOG&&Guy.Inventar[ROHAXT]==0){Expedition::Notify(L"砍树需要石斧。打开“制作”，用树枝 1 + 石头 1 制作。");return true;}
            if(action==Action::LOG&&Guy.Inventar[ROHSTAMM]>=10){Expedition::Notify(L"原木已满（10 / 10），先用来建造，再砍伐。");return true;}
            Cancel();
            if(Guy.Pos.x==x&&Guy.Pos.y==y)
            {
                Guy.AkNummer=0;
                if(action==Action::OPEN_CACHE||action==Action::FEED_FIRE)return Crafting::StartSupplyAction(action==Action::OPEN_CACHE?Crafting::OpenCache:Crafting::FeedFire),true;
                if(actionAt(x,y,Intent::ResumeBuild)==action)
                {
                    Guy.PosAlt=Guy.PosScreen;const auto saved=Scape[x][y].GPosAlt;
                    if(saved.x>0&&saved.y>0)Routing::ShortRoute(saved.x,saved.y);
                    Bmp[BUTTSTOP].Phase=0;const auto message=Construction::Message(Scape[x][y]);Expedition::Notify(message.c_str());
                }
                Guy.Aktion=action;return true;
            }
            Routing::MarkRoute(false);RouteStart=Guy.Pos;RouteZiel={short(x),short(y)};
            if(!Routing::FindTheWay(false))
            {
                RouteStart={-1,-1};RouteZiel={-1,-1};
                Expedition::Notify(L"目标暂时无法到达。请先找到通路；不会隔着障碍拾取。");return true;
            }
            target={short(x),short(y)};pending=action;Guy.Aktiv=true;Guy.Aktion=Action::NOTHING;
            RoutePunkt=-1;Steps=0;Step=0;Bmp[BUTTSTOP].Phase=0;
            Expedition::Notify(action==Action::OPEN_CACHE?L"正在走向漂流箱，抵达后用木石锤砸开。":action==Action::FEED_FIRE?L"正在走向篝火，抵达后添加木材。":action==Action::EAT?L"已锁定成熟果实，走近后自动进食。":action==Action::DRINK?L"已锁定水源，走近后自动饮水。":action==Action::LOG?L"已锁定树木，走近后自动砍伐。":L"已锁定未完成的建筑，走近后继续施工。");return true;
        }
    }
    void Cancel(){target={-1,-1};pending=Action::NOTHING;}
    bool Target(int x,int y,Intent intent){const auto action=actionAt(x,y,intent);if(action==Action::NOTHING){Expedition::Notify(L"目标已改变，请重新选择。");return true;}return begin(x,y,action);}
    void StopSafely()
    {
        Cancel();Routing::MarkRoute(false);
        auto& tile=Scape[Guy.Pos.x][Guy.Pos.y];tile.GPosAlt=Guy.PosScreen;
        Guy.PosScreen={short(tile.xScreen+(EckKoor[tile.Typ][0].x+EckKoor[tile.Typ][2].x)/2),short(tile.yScreen+(EckKoor[tile.Typ][1].y+EckKoor[tile.Typ][3].y)/2)};
        Guy.PosAlt=Guy.PosScreen;Guy.Aktiv=false;Guy.Aktion=Action::NOTHING;Guy.AkNummer=0;
        Guy.Zustand=BootsFahrt?GUYBOOTLINKS:GUYLINKS;
        RouteZiel={-1,-1};RouteStart={-1,-1};RouteLaenge=0;RoutePunkt=-1;Step=0;Steps=0;Bmp[BUTTSTOP].Phase=-1;
    }
    bool Request(Intent intent)
    {
        if(!valid(Guy.Pos.x,Guy.Pos.y))return false;
        int best=std::numeric_limits<int>::max(),bx=-1,by=-1;short selected=Action::NOTHING;
        for(int y=Guy.Pos.y-1;y<=Guy.Pos.y+1;++y)for(int x=Guy.Pos.x-1;x<=Guy.Pos.x+1;++x)
        {
            const short a=actionAt(x,y,intent);if(a==Action::NOTHING)continue;
            int score=(std::abs(x-Guy.Pos.x)+std::abs(y-Guy.Pos.y))*10;
            if(intent==Intent::Any||intent==Intent::FoodWater)
                score+=int(Guy.Resource[a==Action::DRINK?WASSER:NAHRUNG]*.5f);
            if(score<best){best=score;bx=x;by=y;selected=a;}
        }
        if(bx>=0)return begin(bx,by,selected);
        if(intent==Intent::Any)return false;
        Expedition::Notify(intent==Intent::Chop?L"附近没有可砍伐的小树。走近小树后，点击“砍伐树木”。":intent==Intent::Eat?L"附近没有成熟的浆果或作物。可直接点击远处的成熟果丛，走近后自动进食。":intent==Intent::Drink?L"附近没有可饮用的淡水。点击河流可走近饮水；海水不能喝。":L"附近没有成熟果实或淡水。也可以明确选择“吃东西”或“喝淡水”，不会触发其他行动。");return true;
    }
    bool Click(int screenX,int screenY)
    {
        for(int y=1;y<MAXYKACH-1;++y)for(int x=1;x<MAXXKACH-1;++x)
        {
            const auto& t=Scape[x][y];if(!t.Entdeckt||Expedition::CacheAt(x,y)<0)continue;
            const int px=screenX+Camera.x-t.xScreen-27,py=screenY+Camera.y-t.yScreen-29;
            if(px>=-8&&px<=8&&py>=-11&&py<=2)return Target(x,y,Intent::OpenCache);
        }
        // Ground food/water remains selectable even when a tree crown overlaps it.
        const ZWEID ground=Renderer::GetKachel(short(screenX+Camera.x),short(screenY+Camera.y));
        if(valid(ground.x,ground.y))
        {
            const short construction=actionAt(ground.x,ground.y,Intent::ResumeBuild);
            if(construction!=Action::NOTHING)return begin(ground.x,ground.y,construction);
        }
        int bx=-1,by=-1,best=std::numeric_limits<int>::max();short selected=Action::NOTHING;
        bool immature=false;
        for(int y=1;y<MAXYKACH-1;++y)for(int x=1;x<MAXXKACH-1;++x)
        {
            const auto& t=Scape[x][y];if(!t.Entdeckt)continue;
            const int o=t.Objekt;
            const short construction=actionAt(x,y,Intent::ResumeBuild);
            const bool fruit=o==BUSCH||o==FELD;
            const bool water=(o>=FLUSS1&&o<=SCHLEUSE6)||(o==ROHR&&t.Phase==1);
            const bool fire=o==FEUER;
            if(!fruit&&!water&&!fire&&construction==Action::NOTHING)continue;
            const auto& b=Bmp[o];int px=screenX+Camera.x-t.xScreen-t.ObPos.x,py=screenY+Camera.y-t.yScreen-t.ObPos.y;
            if(fire)
            {
                const auto bounds=Campfires::Sprite(t);const int lx=screenX+Camera.x-bounds.x,ly=screenY+Camera.y-bounds.y;
                if(lx<0||ly<0||lx>=bounds.w||ly>=bounds.h)continue;
                px=lx*b.Breite/bounds.w;py=ly*b.Hoehe/bounds.h;
            }
            // Food and water require a hit on the visible object itself. A click on
            // nearby ground must remain a movement command, even on the same tile.
            const int tolerance=construction!=Action::NOTHING?7:0;
            if(px<-tolerance||py<-tolerance||px>=b.Breite+tolerance||py>=b.Hoehe+tolerance)continue;
            int distance=std::max({0,-px,px-b.Breite+1})*std::max({0,-px,px-b.Breite+1})
                +std::max({0,-py,py-b.Hoehe+1})*std::max({0,-py,py-b.Hoehe+1});
            if(b.Surface)
            {
                distance=std::numeric_limits<int>::max();const auto* pixels=b.Surface->pixels();
                for(int dy=-tolerance;dy<=tolerance;++dy)for(int dx=-tolerance;dx<=tolerance;++dx)
                {
                    if(dx*dx+dy*dy>tolerance*tolerance)continue;
                    const int localX=px+dx,localY=py+dy;
                    if(localX<0||localY<0||localX>=b.Breite||localY>=b.Hoehe)continue;
                    const int frame=std::max(0,int(t.Phase));
                    const int sx=b.rcSrc.left+localX,sy=b.rcSrc.top+(t.Reverse?std::max(0,b.Anzahl-1-frame):frame)*b.Hoehe+localY;
                    if(sx<0||sy<0||sx>=b.Surface->width()||sy>=b.Surface->height())continue;
                    const auto* pixel=pixels+4*(sy*b.Surface->width()+sx);
                    if(pixel[0]!=255||pixel[1]!=0||pixel[2]!=255)distance=std::min(distance,dx*dx+dy*dy);
                }
                if(distance==std::numeric_limits<int>::max())continue;
            }
            const short action=fire?Action::FEED_FIRE:construction!=Action::NOTHING?construction:actionAt(x,y,Intent::FoodWater);
            if(action==Action::NOTHING){immature|=fruit&&t.Begehbar;continue;}
            const int score=distance*100+int(Guy.Resource[action==Action::DRINK?WASSER:NAHRUNG]);
            if(score<best){best=score;bx=x;by=y;selected=action;}
        }
        if(bx<0)
        {
            if(immature){Expedition::Notify(L"这些果实还没有成熟，可以先走近，稍后再来采收。");return false;}
            return false;
        }
        return begin(bx,by,selected);
    }
    void Update()
    {
        if(pending==Action::NOTHING)return;
        if(Spielzustand!=State::GAME||BootsFahrt||Guy.Aktion!=Action::NOTHING){Cancel();return;}
        if(Guy.Aktiv)return;
        const auto action=pending;const auto pos=target;Cancel();
        const auto intent=action==Action::OPEN_CACHE?Intent::OpenCache:action==Action::FEED_FIRE?Intent::FeedFire:action==Action::LOG?Intent::Chop:(action==Action::EAT||action==Action::DRINK)?Intent::FoodWater:Intent::ResumeBuild;
        if(Guy.Pos.x!=pos.x||Guy.Pos.y!=pos.y||actionAt(pos.x,pos.y,intent)!=action)
        {Expedition::Notify(L"行动已取消：目标改变或路线中断。请重新选择目标。");return;}
        begin(pos.x,pos.y,action);
    }
}
