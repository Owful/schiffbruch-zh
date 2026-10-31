#pragma once
#include "Action.hpp"
#include "Routing.hpp"
#include "Renderer.hpp"
#include "World.hpp"
#include "Math.hpp"
#include "Interaction.hpp"
#include "State.hpp"
#include <queue>
#include <array>
#include <cmath>
namespace Rescue
{
    enum Phase {Idle,Walk,Approach,Board,Depart};
    inline Phase phase=Idle;
    inline ZWEID shore{},sea{},edge{};
    inline float shipX=0,shipY=0,dockX=0,dockY=0,endX=0,endY=0;
    inline bool Aboard(){return phase==Depart;}
    inline bool Start()
    {
        phase=Idle;std::queue<ZWEID> open;bool seen[MAXXKACH][MAXYKACH]{};
        open.push(Guy.Pos);seen[Guy.Pos.x][Guy.Pos.y]=true;bool found=false;
        const std::array<ZWEID,4> directions{{{1,0},{0,1},{-1,0},{0,-1}}};
        while(!open.empty()&&!found)
        {
            const auto pos=open.front();open.pop();
            for(auto d:directions)
            {
                int x=pos.x+d.x,y=pos.y+d.y;
                if(x<1||y<1||x>=MAXXKACH-1||y>=MAXYKACH-1)continue;
                if(Scape[x][y].Art==1)
                {
                    const ZWEID water{short(x),short(y)};bool clear=true;
                    while(x>0&&y>0&&x<MAXXKACH-1&&y<MAXYKACH-1)
                    {if(Scape[x][y].Art!=1){clear=false;break;}x+=d.x;y+=d.y;}
                    if(clear){shore=pos;sea=water;edge={short(x),short(y)};found=true;break;}
                }
                else if(!seen[x][y]&&Scape[x][y].Begehbar){seen[x][y]=true;open.push({short(x),short(y)});}
            }
        }
        if(!found)return false;
        const auto& dock=Scape[sea.x][sea.y];dockX=dock.xScreen+27;dockY=dock.yScreen+29;
        const auto& horizon=Scape[edge.x][edge.y];endX=horizon.xScreen+27;endY=horizon.yScreen+29;shipX=endX;shipY=endY;
        if(Guy.Pos.x!=shore.x||Guy.Pos.y!=shore.y)
        {
            RouteStart=Guy.Pos;RouteZiel=shore;if(!Routing::FindTheWay(false))return false;
            RouteKoor[0]=Guy.PosScreen;RoutePunkt=-1;Steps=Step=0;Guy.Aktiv=true;Guy.Zustand=GUYLINKS;
        }
        else Guy.Aktiv=false;
        phase=Walk;Spielzustand=State::RESCUED;return true;
    }
    inline bool move(float x,float y,float dt)
    {
        const float dx=x-shipX,dy=y-shipY,length=std::hypot(dx,dy),distance=95.f*dt;
        if(length<=distance){shipX=x;shipY=y;return true;}
        shipX+=dx/length*distance;shipY+=dy/length*distance;return false;
    }
    inline void Advance(long long micros,bool running)
    {
        if(!running||phase==Idle)return;const float dt=float(std::min<long long>(micros,100000))/1000000;
        Math::Animationen();
        if(phase==Walk||phase==Board)
        {
            if(!Guy.Aktiv)
            {
                if(phase==Walk)phase=Approach;
                else{phase=Depart;Guy.Zustand=GUYWARTEN;}
            }
        }
        else if(phase==Approach&&move(dockX,dockY,dt))
        {phase=Board;Routing::ShortRoute(short(dockX),short(dockY-2));}
        else if(phase==Depart&&move(endX,endY,dt))
        {phase=Idle;Guy.Aktiv=false;Guy.Aktion=Action::NOTHING;Spielzustand=State::OUTRO;}
        Camera={short((phase==Depart?shipX:Guy.PosScreen.x)-640),short((phase==Depart?shipY:Guy.PosScreen.y)-360)};Renderer::LimitScroll();
    }
    inline void Draw()
    {
        if(phase==Idle||Spielzustand!=State::RESCUED)return;
        if(phase==Board)
        {
            HDC dc;lpDDSBack->GetDC(&dc);auto pen=CreatePen(PS_SOLID,4,RGB(142,113,69));auto old=SelectObject(dc,pen);
            const auto& t=Scape[shore.x][shore.y];MoveToEx(dc,t.xScreen+27-Camera.x,t.yScreen+29-Camera.y,nullptr);LineTo(dc,int(dockX)-Camera.x,int(dockY)-Camera.y);
            SelectObject(dc,old);DeleteObject(pen);lpDDSBack->ReleaseDC(dc);
        }
        const auto& b=Bmp[GUYSCHIFF];RECT source=b.rcSrc;
        RECT dest{LONG(shipX-30-Camera.x),LONG(shipY-28-Camera.y),LONG(shipX-30-Camera.x+b.Breite),LONG(shipY-28-Camera.y+b.Hoehe)};
        lpDDSBack->Blt(&dest,b.Surface,&source,DDBLT_KEYSRC,nullptr);
    }
}

