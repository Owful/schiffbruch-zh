#include "Expedition.hpp"
#include "ExpeditionRules.hpp"
#include "headers.hpp"
#include "extern.hpp"
#include "Action.hpp"
#include "World.hpp"
#include "State.hpp"
#include "WeatherEffects.hpp"
#include "Crafting.hpp"
#include "SceneView.hpp"
#include "DayJournal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cwchar>
#include <string>
#include <vector>

namespace Expedition
{
    namespace
    {
        ExpeditionRules::Progress progress;
        bool journal = false, effects = true, hud = true;
        std::wstring notice;
        ULONGLONG noticeUntil = 0;
        constexpr COLORREF Gold = RGB(224,197,139), Ink = RGB(19,39,40), Muted = RGB(155,177,163);

        bool positionValid(int x, int y) { return x >= 0 && y >= 0 && x < int(MAXXKACH) && y < int(MAXYKACH); }
        bool built(const SCAPE& tile, int object)
        {
            return tile.Objekt == object && tile.Phase >= 0 && tile.Phase < Bmp[object].Anzahl;
        }
        bool freshwater(const SCAPE& tile)
        {
            return (tile.Objekt >= FLUSS1 && tile.Objekt <= SCHLEUSE6) || (tile.Objekt == ROHR && tile.Phase == 1);
        }
        bool ready()
        {
            return Spielzustand == State::GAME && !Guy.Aktiv && Guy.Aktion == Action::NOTHING && PapierText == -1 && !Nacht && positionValid(Guy.Pos.x,Guy.Pos.y);
        }
        struct Snapshot
        {
            bool shelter = false, fire = false, sos = false;
            int land = 0, explored = 0;
        };
        Snapshot inspect()
        {
            Snapshot out;
            for (const auto& column : Scape) for (const auto& tile : column)
            {
                if (tile.Art != 1) { ++out.land; if (tile.Entdeckt) ++out.explored; }
                out.shelter |= built(tile,ZELT) || built(tile,HAUS3);
                out.fire |= built(tile,FEUER);
                out.sos |= built(tile,SOS);
            }
            return out;
        }
        void populateCaches()
        {
            struct Candidate { int x,y; unsigned score; };
            std::vector<Candidate> candidates;
            int landingX = 1;
            while (landingX < int(MAXXKACH)-1 && Scape[landingX][MAXYKACH/2].Art == 1) ++landingX;
            for (int y=1;y<int(MAXYKACH)-1;++y) for (int x=1;x<int(MAXXKACH)-1;++x)
            {
                const auto& t=Scape[x][y];
                if (t.Art != 2 || t.Typ != 0 || t.Objekt != -1) continue;
                const unsigned score = (unsigned(x)*73856093u) ^ (unsigned(y)*19349663u);
                candidates.push_back({x,y,score});
            }
            std::sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.score<b.score;});
            unsigned n=0;
            for (const auto& c:candidates)
            {
                if(n==3) break;
                if(std::abs(c.x-landingX)+std::abs(c.y-int(MAXYKACH/2))<7)continue;
                bool spaced=true;
                for(unsigned i=0;i<n;++i) if(std::abs(c.x-progress.caches[i].x)+std::abs(c.y-progress.caches[i].y)<10)spaced=false;
                if(spaced){progress.caches[n]={static_cast<std::uint8_t>(c.x),static_cast<std::uint8_t>(c.y),static_cast<std::uint8_t>(n%3)};++n;}
            }
        }
        int give(int item,int count)
        {
            const int amount=std::max(0,std::min(count,10-int(Guy.Inventar[item])));
            Guy.Inventar[item]+=static_cast<short>(amount);return amount;
        }
        void fill(HDC dc,int x,int y,int w,int h,COLORREF color)
        {
            RECT r{x,y,x+w,y+h};HBRUSH brush=CreateSolidBrush(color);FillRect(dc,&r,brush);DeleteObject(brush);
        }
        struct Fonts
        {
            HFONT body=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
            HFONT compact=CreateFontW(-13,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
            HFONT title=CreateFontW(-23,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
            ~Fonts(){DeleteObject(body);DeleteObject(compact);DeleteObject(title);}
        };
        void text(HDC dc,int x,int y,const wchar_t* str,COLORREF color=Muted,int size=0)
        {
            static Fonts fonts;
            auto old=SelectObject(dc,size==1?fonts.compact:size==2?fonts.title:fonts.body);
            SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);TextOutW(dc,x,y,str,static_cast<int>(std::wcslen(str)));SelectObject(dc,old);
        }
        void stroke(HDC dc,int x1,int y1,int x2,int y2,COLORREF color,int width=1)
        {
            HPEN pen=CreatePen(PS_SOLID,width,color);auto old=SelectObject(dc,pen);MoveToEx(dc,x1,y1,nullptr);LineTo(dc,x2,y2);SelectObject(dc,old);DeleteObject(pen);
        }
        void panel(HDC dc,int x,int y,int w,int h)
        {
            fill(dc,x,y,w,h,Ink);fill(dc,x,y,w,1,RGB(110,109,78));fill(dc,x,y,2,h,RGB(160,142,98));
        }
        const wchar_t* weatherName(ExpeditionRules::Weather w)
        {
            return w==ExpeditionRules::Weather::Clear?L"晴朗":w==ExpeditionRules::Weather::Rain?L"阵雨":L"风暴";
        }
    }

    void Notify(const wchar_t* message) { notice=message;noticeUntil=GetTickCount64()+12000; }
    std::wstring CurrentNotice(){return notice;}
    void CollectTimber()
    {
        const int trunk=give(ROHSTAMM,1),branches=give(ROHAST,5),leaves=give(ROHBLATT,5),vines=give(ROHLIANE,2);
        std::wstring message=L"砍伐收获：原木 +"+std::to_wstring(trunk)+L"，树枝 +"+std::to_wstring(branches)+L"，棕叶 +"+std::to_wstring(leaves)+L"，藤蔓 +"+std::to_wstring(vines)+L"。";
        if(trunk<1||branches<5||leaves<5||vines<2)message+=L"部分物资已达携带上限（10），仅计入实际装入背包的数量。";
        Notify(message.c_str());
    }
    void Reset()
    {
        progress={};populateCaches();journal=false;
        Notify(L"岛上偶尔能发现漂流补给。走到木箱所在位置并搜索，可以取得里面的物资。");
    }
    void Save(std::ostream& out) { ExpeditionRules::write(out,progress); }
    void Load(std::istream& in)
    {
        progress={};
        if(!ExpeditionRules::read(in,progress))populateCaches();
        journal=false;
    }
    void OnTime(int minutes,int startingHour)
    {
        if(Spielzustand!=State::GAME || Nacht || Guy.Aktion==Action::DAY_END || minutes<=0 || !positionValid(Guy.Pos.x,Guy.Pos.y))return;
        const auto& tile=Scape[Guy.Pos.x][Guy.Pos.y];
        const bool sheltered=built(tile,ZELT)||built(tile,HAUS3);
        const auto w=ExpeditionRules::weather(Tag);
        World::AddResource(WASSER,ExpeditionRules::waterChange(w,startingHour,minutes,sheltered||BootsFahrt));
        if(Guy.Zustand==GUYTRINKEN && freshwater(tile))progress.waterVisited=true;
        const auto snapshot=inspect();
        // A visible SOS, an active fire and the wreck's telescope make rescue predictable.
        if(!BootsFahrt && snapshot.sos && snapshot.fire && Guy.Inventar[ROHFERNROHR]>0 && Guy.Resource[GESUNDHEIT]>0 && Guy.Aktion!=Action::DEATH)
        {
            progress.signal=std::min(ExpeditionRules::SignalTarget,progress.signal+ExpeditionRules::signalGain(w,minutes));
            if(progress.signal>=ExpeditionRules::SignalTarget)
            {
                Guy.Aktiv=false;Guy.AkNummer=0;Guy.Aktion=Action::RESCUED;
                Notify(L"海平线出现了来船。你的求救信号被发现了！");
            }
        }
    }
    void OnSearch()
    {
        if(BootsFahrt)return;
        for(unsigned i=0;i<progress.caches.size();++i)
        {
            const auto& cache=progress.caches[i];
            if((progress.claimed&(1u<<i)) || cache.x!=Guy.Pos.x || cache.y!=Guy.Pos.y)continue;
            struct Reward {int item,count;const wchar_t* name;};
            const std::array<Reward,3> supplies = cache.kind==0 ? std::array<Reward,3>{{{ROHSTAMM,1,L"原木"},{ROHAST,2,L"树枝"},{ROHLIANE,2,L"藤蔓"}}} :
                cache.kind==1 ? std::array<Reward,3>{{{ROHSTEIN,3,L"石头"},{ROHAST,2,L"树枝"},{ROHBLATT,2,L"棕叶"}}} :
                std::array<Reward,3>{{{ROHLIANE,2,L"藤蔓"},{ROHBLATT,3,L"棕叶"},{ROHSTEIN,1,L"石头"}}};
            for(const auto& r:supplies)if(Guy.Inventar[r.item]+r.count>10)
            {std::wstring message=L"携带空间不足，补给箱仍留在这里。请为"+std::wstring(r.name)+L"腾出至少 "+std::to_wstring(r.count)+L" 格空间后再搜索。";Notify(message.c_str());return;}
            if(cache.kind==2&&progress.bandages>=3){Notify(L"绷带已满，补给箱仍留在这里。使用绷带后再搜索。");return;}
            std::wstring message=L"找到漂流补给：";
            for(const auto& r:supplies){give(r.item,r.count);message+=std::wstring(r.name)+L" +"+std::to_wstring(r.count)+L"，";}
            if(cache.kind==2){++progress.bandages;DayJournal::RecordLoot(Tag,DayJournal::Bandage,1);message+=L"绷带 +1，";}
            message.replace(message.size()-1,1,L"。");
            progress.claimed|=static_cast<std::uint8_t>(1u<<i);DayJournal::Mark(Tag,DayJournal::Cache);Notify(message.c_str());return;
        }
    }
    bool CraftWaterskin()
    {
        if(!ready()){Notify(L"请先完成当前动作并关闭提示，再制作水壶。");return false;}
        if(progress.waterskin){Notify(L"已经拥有水壶，无需重复制作。按 K 装水或饮用。");return false;}
        if(Guy.Inventar[ROHBLATT]<2||Guy.Inventar[ROHLIANE]<2){Notify(L"制作水壶需要棕叶2、藤蔓2。");return false;}
        Guy.Inventar[ROHBLATT]-=2;Guy.Inventar[ROHLIANE]-=2;progress.waterskin=true;
        World::AddTime(0,10);Notify(L"水壶制作完成。容量3份；到淡水旁按 K 装水，离开水源后按 K 饮用。");return true;
    }
    bool CraftBandage()
    {
        if(!ready()){Notify(L"请先完成当前动作并关闭提示，再制作绷带。");return false;}
        if(progress.bandages>=3){Notify(L"绷带已满（3/3），需要时按 J 使用。");return false;}
        if(Guy.Inventar[ROHBLATT]<3||Guy.Inventar[ROHLIANE]<1){Notify(L"制作绷带需要棕叶3、藤蔓1。");return false;}
        Guy.Inventar[ROHBLATT]-=3;--Guy.Inventar[ROHLIANE];++progress.bandages;World::AddTime(0,15);Notify(L"制作了一份绷带。按 J 使用可恢复20生命。");return true;
    }
    void UseWaterskin()
    {
        if(NearFreshwater()&&progress.water<3)FillWaterskin();else DrinkWaterskin();
    }
    bool NearFreshwater()
    {
        if(BootsFahrt)return false;
        for(int y=Guy.Pos.y-1;y<=Guy.Pos.y+1;++y)for(int x=Guy.Pos.x-1;x<=Guy.Pos.x+1;++x)
            if(positionValid(x,y)&&Scape[x][y].Entdeckt&&freshwater(Scape[x][y])&&std::abs(x-Guy.Pos.x)+std::abs(y-Guy.Pos.y)<=1)return true;
        return false;
    }
    void FillWaterskin()
    {
        Crafting::StartSupplyAction(Crafting::FillWater);
    }
    void DrinkWaterskin()
    {
        Crafting::StartSupplyAction(Crafting::DrinkWater);
    }
    void UseBandage()
    {
        Crafting::StartSupplyAction(Crafting::ApplyBandage);
    }
    std::wstring SupplyProblem(int action)
    {
        if(action==Crafting::ApplyBandage)
        {
            if(!progress.bandages)return L"还没有可用绷带，请打开“制作 → 补给”制作。";
            return Guy.Resource[GESUNDHEIT]>85?L"生命充足，先把绷带留给之后的伤口。":L"";
        }
        if(!progress.waterskin)return L"还没有水壶，请打开“制作 → 补给”制作。";
        if(action==Crafting::FillWater)
        {
            if(!NearFreshwater())return L"需要走到淡水河流或供水管旁，海水不能装进水壶。";
            return progress.water>=3?L"水壶已经装满：3 / 3。":L"";
        }
        if(action!=Crafting::DrinkWater)return L"没有对应的补给动作。";
        if(!progress.water)return L"水壶空了。到河流或装有水的水管旁按 K 补水。";
        return Guy.Resource[WASSER]>85?L"水分充足，先把这份水留给接下来的探索吧。":L"";
    }
    void CompleteSupplyAction(int action)
    {
        if(!ready())return;
        const auto problem=SupplyProblem(action);if(!problem.empty()){Notify(problem.c_str());return;}
        if(action==Crafting::FillWater)
        {progress.water=3;progress.waterVisited=true;Notify(L"水壶已装满：3 / 3。");}
        else if(action==Crafting::DrinkWater)
        {--progress.water;World::AddResource(WASSER,25);DayJournal::Mark(Tag,DayJournal::Water);Notify(L"喝下一份储存的淡水，水分 +25。");}
        else if(action==Crafting::ApplyBandage)
        {--progress.bandages;World::AddResource(GESUNDHEIT,20);DayJournal::Mark(Tag,DayJournal::Care);Notify(L"包扎完成，生命 +20。");}
    }
    void ToggleJournal(){journal=!journal;}
    Info GetInfo()
    {
        const auto s=inspect();Info out;
        out.water=progress.water;out.bandages=progress.bandages;out.waterskin=progress.waterskin;
        out.signal=int(progress.signal*100/ExpeditionRules::SignalTarget);
        out.explored=s.land?100*s.explored/s.land:0;
        out.stages[0]=progress.waterVisited;out.stages[1]=s.shelter;
        out.stages[2]=Guy.Inventar[ROHFERNROHR]>0;out.stages[3]=s.sos&&s.fire;out.stages[4]=out.signal>=100;
        return out;
    }
    void ToggleEffects(){effects=!effects;Notify(effects?L"氛围效果已开启。":L"氛围效果已关闭；天气玩法仍然生效。");}
    void ToggleHud(){hud=!hud;}

    void DrawAtmosphere()
    {
        if(!effects||Nacht||PapierText!=-1||Guy.Aktion==Action::DAY_END||!lpDDSBack)return;
        HDC dc=nullptr;if(FAILED(lpDDSBack->GetDC(&dc)))return;
        const int saved=SaveDC(dc);IntersectClipRect(dc,rcSpielflaeche.left,rcSpielflaeche.top,rcSpielflaeche.right,rcSpielflaeche.bottom);
        const auto w=ExpeditionRules::weather(Tag);
        // A tiny GDI source gives the scene a tint without changing display-wide gamma.
        static NativeSurface tintPixel(1,1);HDC source=nullptr;tintPixel.GetDC(&source);
        const bool sunset=Stunden>=8;const COLORREF tint=w!=ExpeditionRules::Weather::Clear?RGB(32,53,68):sunset?RGB(179,107,54):RGB(96,126,129);
        ::SetPixel(source,0,0,tint);
        const BYTE opacity=w==ExpeditionRules::Weather::Storm?55:w==ExpeditionRules::Weather::Rain?28:sunset?static_cast<BYTE>(std::min(42,12+(Stunden-8)*7)):Stunden<2?14:0;
        if(opacity){BLENDFUNCTION blend{AC_SRC_OVER,0,opacity,0};AlphaBlend(dc,0,0,rcSpielflaeche.right,rcSpielflaeche.bottom,source,0,0,1,1,blend);}
        tintPixel.ReleaseDC(source);
        // Finish GDI writes before composing translucent rain into the same surface.
        RestoreDC(dc,saved);lpDDSBack->ReleaseDC(dc);
        WeatherEffects::DrawRain(*lpDDSBack,w,GetTickCount64()/1000.0);
        lpDDSBack->GetDC(&dc);const int fireSaved=SaveDC(dc);
        IntersectClipRect(dc,rcSpielflaeche.left,rcSpielflaeche.top,rcSpielflaeche.right,rcSpielflaeche.bottom);
        for(const auto& column:Scape)for(const auto& t:column)
        {
            if(!built(t,FEUER)||!t.Entdeckt)continue;
            const int x=t.xScreen+t.ObPos.x+Bmp[FEUER].Breite/2-Camera.x;
            const int y=t.yScreen+t.ObPos.y-Camera.y;
            if(x<0||y<0||x>rcSpielflaeche.right||y>rcSpielflaeche.bottom)continue;
            const int phase=int(GetTickCount64()/55);
            for(int i=0;i<4;++i){const int h=(phase+i*13)%48;const int dx=int(std::sin((phase+i)*.14)*5)+h/7;stroke(dc,x+dx,y-h,x+dx+3,y-h-3,RGB(170+h,137+h,85+h));}
        }
        RestoreDC(dc,fireSaved);lpDDSBack->ReleaseDC(dc);
    }
    void DrawCaches(bool inFrontOfPlayer)
    {
        if(Nacht||PapierText!=-1||!lpDDSBack)return;
        HDC dc=nullptr;if(FAILED(lpDDSBack->GetDC(&dc)))return;
        const int saved=SaveDC(dc);IntersectClipRect(dc,rcSpielflaeche.left,rcSpielflaeche.top,rcSpielflaeche.right,rcSpielflaeche.bottom);
        {
            // Draw in scene coordinates before zoom, so caches stay anchored to their tiles.
            for(unsigned i=0;i<progress.caches.size();++i)
            {
                const auto& cache=progress.caches[i];if((progress.claimed&(1u<<i))||!positionValid(cache.x,cache.y))continue;
                const auto& tile=Scape[cache.x][cache.y];if(!tile.Entdeckt)continue;
                const int x=tile.xScreen+27-Camera.x,y=tile.yScreen+29-Camera.y;
                if((tile.yScreen+30>Guy.PosScreen.y)!=inFrontOfPlayer)continue;
                if(x<8||y<20||x>rcSpielflaeche.right-10||y>rcSpielflaeche.bottom-10)continue;
                fill(dc,x-7,y-9,14,10,RGB(156,114,65));stroke(dc,x-7,y-10,x+7,y-10,Gold);stroke(dc,x-2,y-10,x-2,y+1,RGB(226,201,137),2);
            }
        }
        RestoreDC(dc,saved);lpDDSBack->ReleaseDC(dc);
    }
    void DrawHud()
    {
        if(Nacht||!lpDDSBack)return;
        HDC dc=nullptr;if(FAILED(lpDDSBack->GetDC(&dc)))return;
        const int saved=SaveDC(dc);IntersectClipRect(dc,0,0,rcSpielflaeche.right,rcSpielflaeche.bottom);
        if(PapierText==-1)for(unsigned i=0;i<progress.caches.size();++i)
        {
            const auto& cache=progress.caches[i];if((progress.claimed&(1u<<i))||!positionValid(cache.x,cache.y))continue;
            const auto& tile=Scape[cache.x][cache.y];if(!tile.Entdeckt)continue;
            const int x=tile.xScreen+27-Camera.x,y=tile.yScreen+29-Camera.y;
            if(std::abs(MousePosition.x-x)>=30||std::abs(MousePosition.y-y)>=30)continue;
            const auto screen=SceneView::ToScreen(x,y);
            if(screen.x<0||screen.x>=960||screen.y<64||screen.y>=656)continue;
            panel(dc,screen.x-76,screen.y-35,154,23);text(dc,screen.x-65,screen.y-33,L"漂流补给 · 搜索拾取",Gold,1);
        }
        if(GetTickCount64()<noticeUntil&&!notice.empty())
        {
            const int width=std::min<int>(rcSpielflaeche.right-36,920);
            static Fonts fonts;auto old=SelectObject(dc,fonts.body);
            RECT measured{0,0,width-26,4096};DrawTextW(dc,notice.c_str(),-1,&measured,DT_WORDBREAK|DT_NOPREFIX|DT_CALCRECT);
            const int height=std::max(42,int(measured.bottom)+16),y=rcSpielflaeche.bottom-48-height;
            panel(dc,18,y,width,height);
            RECT r{32,y+8,18+width-12,y+height-8};SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(222,213,182));DrawTextW(dc,notice.c_str(),-1,&r,DT_WORDBREAK|DT_NOPREFIX);SelectObject(dc,old);
        }
        RestoreDC(dc,saved);lpDDSBack->ReleaseDC(dc);
    }
}
