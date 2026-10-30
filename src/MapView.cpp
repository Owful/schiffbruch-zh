#include "MapView.hpp"
#include "extern.hpp"
#include "Renderer.hpp"
#include "SceneView.hpp"
#include "Campfires.hpp"
#include <array>

namespace MapView
{
    namespace
    {
        constexpr COLORREF Ink=RGB(16,37,42),Gold=RGB(224,197,139),Border=RGB(65,98,99);
        void outline(HDC dc,const MapProjection::Frame& frame)
        {
            std::array<POINT,5> points{};int i=0;
            for(const auto corner:std::array<MapProjection::Coordinate,5>{{{-.5,-.5},{59.5,-.5},{59.5,59.5},{-.5,59.5},{-.5,-.5}}})
            {const auto world=MapProjection::TileCenter(corner.x,corner.y);const auto p=frame.project(world.x,world.y);points[i++]={p.x,p.y};}
            auto pen=CreatePen(PS_SOLID,1,Border);auto previous=SelectObject(dc,pen);Polyline(dc,points.data(),int(points.size()));SelectObject(dc,previous);DeleteObject(pen);
        }
        void markers(HDC dc,const MapProjection::Frame& frame,bool large)
        {
            const auto crop=SceneView::Crop();
            const auto a=frame.project(Camera.x+crop.left,Camera.y+crop.top),b=frame.project(Camera.x+crop.right,Camera.y+crop.bottom);
            auto pen=CreatePen(PS_SOLID,1,RGB(101,150,153));auto previousPen=SelectObject(dc,pen);auto previousBrush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
            Rectangle(dc,a.x,a.y,b.x,b.y);SelectObject(dc,previousBrush);SelectObject(dc,previousPen);DeleteObject(pen);
            const auto player=frame.project(Guy.PosScreen.x,Guy.PosScreen.y);const int r=large?4:2;
            auto brush=CreateSolidBrush(Gold);pen=CreatePen(PS_SOLID,1,RGB(30,39,36));previousBrush=SelectObject(dc,brush);previousPen=SelectObject(dc,pen);
            Ellipse(dc,player.x-r,player.y-r,player.x+r+1,player.y+r+1);SelectObject(dc,previousBrush);SelectObject(dc,previousPen);DeleteObject(brush);DeleteObject(pen);
        }
    }
    MapProjection::Frame MiniFrame(){return MapProjection::Frame::Fit(rcKarte.left+4,rcKarte.top+4,rcKarte.right-rcKarte.left-8,rcKarte.bottom-rcKarte.top-8);}
    MapProjection::Frame OverviewFrame(){return MapProjection::Frame::Fit(134,172,1012,468);}
    void BuildMini()
    {
        if(!lpDDSKarte)return;
        HDC dc=nullptr;lpDDSKarte->GetDC(&dc);RECT area{0,0,120,120};auto bg=CreateSolidBrush(Ink);FillRect(dc,&area,bg);DeleteObject(bg);
        const auto frame=MapProjection::Frame::Fit(4,4,112,112);
        const auto previousPen=SelectObject(dc,GetStockObject(NULL_PEN));
        for(int y=0;y<MAXYKACH;++y)for(int x=0;x<MAXXKACH;++x)
        {
            const auto& t=Scape[x][y];if(!t.Entdeckt)continue;
            COLORREF color=t.Art==1?RGB(41,86,92):(t.Art==2||t.Art==3)?RGB(177,161,112):RGB(78+t.Hoehe*10,113+t.Hoehe*10,82+t.Hoehe*10);
            if(t.Objekt>=FLUSS1&&t.Objekt<=SCHLEUSE6)color=RGB(72,144,153);
            else if(t.Objekt>=BAUM1&&t.Objekt<=BAUM4)color=RGB(57,95,68);
            POINT corners[4];for(int i=0;i<4;++i){const auto p=frame.project(t.xScreen+EckKoor[t.Typ][i].x,t.yScreen+EckKoor[t.Typ][i].y);corners[i]={p.x,p.y};}
            auto brush=CreateSolidBrush(color);const auto old=SelectObject(dc,brush);Polygon(dc,corners,4);SelectObject(dc,old);DeleteObject(brush);
        }
        SelectObject(dc,previousPen);outline(dc,frame);lpDDSKarte->ReleaseDC(dc);
    }
    void DrawMini()
    {
        RECT source{0,0,120,120};lpDDSBack->Blt(&rcKarte,lpDDSKarte,&source,0,nullptr);
        HDC dc=nullptr;lpDDSBack->GetDC(&dc);const int saved=SaveDC(dc);IntersectClipRect(dc,rcKarte.left,rcKarte.top,rcKarte.right,rcKarte.bottom);
        markers(dc,MiniFrame(),false);RestoreDC(dc,saved);lpDDSBack->ReleaseDC(dc);
    }
    void DrawOverview()
    {
        const auto frame=OverviewFrame();RECT source{0,0,MAXSCAPEX,MAXSCAPEY};
        RECT dest{LONG(frame.left),LONG(frame.top),LONG(std::lround(frame.left+MAXSCAPEX*frame.scale)),LONG(std::lround(frame.top+MAXSCAPEY*frame.scale))};
        lpDDSBack->Blt(&dest,lpDDSScape,&source,0,nullptr);
        // Render the same sprites at their world foot positions; no rotation of characters or buildings.
        for(int y=0;y<MAXYKACH;++y)for(int x=0;x<MAXXKACH;++x)
        {
            const auto& t=Scape[x][y];const int object=t.Objekt;
            if(!t.Entdeckt||object<0||object>=BILDANZ)continue;const auto& b=Bmp[object];if(!b.Surface||b.Breite<=0||b.Hoehe<=0)continue;
            int phase=std::max(0,int(t.Phase));if(t.Reverse)phase=std::max(0,b.Anzahl-1-phase);
            RECT sprite{b.rcSrc.left,b.rcSrc.top+phase*b.Hoehe,b.rcSrc.right,b.rcSrc.top+(phase+1)*b.Hoehe};
            int px=t.xScreen+t.ObPos.x,py=t.yScreen+t.ObPos.y,w=b.Breite,h=b.Hoehe;
            if(object==FEUER){const auto bounds=Campfires::Sprite(t);px=bounds.x;py=bounds.y;w=bounds.w;h=bounds.h;}
            const auto a=frame.project(px,py),end=frame.project(px+w,py+h);RECT target{a.x,a.y,end.x,end.y};lpDDSBack->Blt(&target,b.Surface,&sprite,DDBLT_KEYSRC,nullptr);
        }
        HDC dc=nullptr;lpDDSBack->GetDC(&dc);const int saved=SaveDC(dc);IntersectClipRect(dc,134,172,1146,640);outline(dc,frame);markers(dc,frame,true);RestoreDC(dc,saved);lpDDSBack->ReleaseDC(dc);
    }
    bool Focus(int x,int y,const MapProjection::Frame& frame)
    {
        const auto world=frame.unproject(x,y);if(!MapProjection::Inside(world.x,world.y))return false;
        const auto tile=Renderer::GetKachel(short(std::lround(world.x)),short(std::lround(world.y)));
        if(tile.x<0||tile.y<0||!Scape[tile.x][tile.y].Entdeckt)return false;
        const auto& t=Scape[tile.x][tile.y];
        Camera={short(t.xScreen+27-480),short(t.yScreen+29-360)};Renderer::LimitScroll();return true;
    }
}
