#include "ModernUI.hpp"
#include "NativeDisplay.hpp"
#include "Localization.hpp"
#include "Expedition.hpp"
#include "ExpeditionRules.hpp"
#include "headers.hpp"
#include "extern.hpp"
#include "Action.hpp"
#include "Renderer.hpp"
#include "Game.hpp"
#include "World.hpp"
#include "Math.hpp"
#include "Direct.hpp"
#include "Sound.hpp"
#include "Menu.hpp"
#include "Interaction.hpp"
#include "Survival.hpp"
#include "Crafting.hpp"
#include "SceneView.hpp"
#include "Nightfall.hpp"
#include "Construction.hpp"
#include "DuskNotice.hpp"
#include "DayJournal.hpp"
#include "Campfires.hpp"
#include "State.hpp"
#include <SFML/Window.hpp>
#include <array>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <vector>

namespace ModernUI
{
    namespace
    {
        constexpr COLORREF BG=RGB(17,30,33),CARD=RGB(26,44,47),LINE=RGB(46,66,66),FG=RGB(226,232,219),MUTED=RGB(143,164,155),GOLD=RGB(215,189,137),GREEN=RGB(135,185,150);
        struct Button{RECT rect;std::wstring label;std::function<void()> click;bool enabled=true;std::wstring hint;};
        std::vector<Button> buttons;
        std::size_t overlayStart=0;
        bool paused=false,settings=false,craft=false,journal=false,quit=false,confirmNew=false;
        int safety=0;
        DuskNotice::State duskNotice;
        int selection=-1;
        std::vector<Campfires::Info> fireList;
        bool fireDetails=false;int fireOffset=0;RECT fireArea{632,70,948,112};
        int craftCategory=0,craftOffset=0;
        HDC active=nullptr;
        std::array<HFONT,4> fonts{};
        NativeSurface* target=nullptr;
        bool inside(int x,int y,const RECT& r){return x>=r.left&&y>=r.top&&x<r.right&&y<r.bottom;}
        HFONT font(int size)
        {
            const int index=size>=32?3:size>=22?2:size<=13?0:1;
            if(!fonts[index])fonts[index]=CreateFontW(-(index==3?46:index==2?25:index==0?13:16),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
            return fonts[index];
        }
        void fill(int x,int y,int w,int h,COLORREF color){RECT r{x,y,x+w,y+h};HBRUSH b=CreateSolidBrush(color);FillRect(active,&r,b);DeleteObject(b);}
        void line(int x,int y,int w,COLORREF color=LINE){fill(x,y,w,1,color);}
        void txt(int x,int y,const std::wstring& value,int size=16,COLORREF color=FG)
        {
            auto old=SelectObject(active,font(size));SetBkMode(active,TRANSPARENT);SetTextColor(active,color);TextOutW(active,x,y,value.data(),int(value.size()));SelectObject(active,old);
        }
        void begin(NativeSurface* s){target=s;s->GetDC(&active);}
        void end(){target->ReleaseDC(active);active=nullptr;}
        void button(int x,int y,int w,int h,std::wstring label,std::function<void()> fn,bool enabled=true,std::wstring hint={})
        {
            const auto mouse=NativeDisplay::MousePoint();RECT rect{x,y,x+w,y+h};const bool hover=inside(mouse.x,mouse.y,rect);
            fill(x,y,w,h,hover?RGB(48,69,66):CARD);if(hover)fill(x,y,2,h,GOLD);
            auto old=SelectObject(active,font(16));SetBkMode(active,TRANSPARENT);SetTextColor(active,enabled?(hover?GOLD:FG):RGB(98,118,111));RECT textRect{x+12,y+(h-20)/2,x+w-8,y+h};DrawTextW(active,label.c_str(),-1,&textRect,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);SelectObject(active,old);
            buttons.push_back({rect,std::move(label),std::move(fn),enabled,std::move(hint)});
        }
        std::vector<const Crafting::Recipe*> craftRows()
        {std::vector<const Crafting::Recipe*> rows;for(const auto& r:Crafting::Recipes())if(r.category==craftCategory)rows.push_back(&r);return rows;}
        void showRecipe(int item)
        {if(const auto* r=Crafting::Find(item)){craft=true;HauptMenue=Menu::NONE;craftCategory=r->category;const auto rows=craftRows();for(int i=0;i<int(rows.size());++i)if(rows[i]->product==item)craftOffset=(i/4)*4;}}
        void legacy(int id)
        {
            if(id==BUTTSTOP&&Crafting::Busy()){Crafting::Cancel();return;}
            if(Nightfall::Pending()&&id!=BUTTSTOP){Expedition::Notify(L"天黑了，先收工休息，明早再开始新的行动。");return;}
            if(id==BUTTSTOP)Interaction::Cancel();
            if(const int missing=Crafting::MissingForAction(id);missing>=0){showRecipe(missing);const auto why=Crafting::Requirement(id);Expedition::Notify(why.c_str());return;}
            if(id==BUTTESSEN){Interaction::Request(Interaction::Intent::FoodWater);return;}
            if(id==BUTTFAELLEN){Interaction::Request(Interaction::Intent::Chop);return;}
            if(Guy.Aktiv&&id!=BUTTSTOP){Expedition::Notify(L"正在行动中。可以先点击“停止”，再选择新的行动。");return;}
            if(Bmp[id].Phase==-1){Expedition::Notify(L"尚未解锁。打开“制作”准备所需工具，或探索沉船获得新物品。");return;}
            const auto old=MousePosition;
            MousePosition={short((Bmp[id].rcDes.left+Bmp[id].rcDes.right)/2),short((Bmp[id].rcDes.top+Bmp[id].rcDes.bottom)/2)};
            Math::MouseInPanel(0,1);MousePosition=old;
        }
        void place(int id,int x,int y,int w=136,int h=42){Bmp[id].rcDes={x,y,x+w,y+h};}
        void actionButton(int id,const wchar_t* title,int x,int y,int w=136,int h=42)
        {
            place(id,x,y,w,h);button(x,y,w,h,title,[id]{legacy(id);},Bmp[id].Phase!=-1);
        }
        void closePaper(bool yes)
        {
            if(Frage==0)Frage=yes?1:2;
            Renderer::Textloeschen(TXTPAPIER);PapierText=-1;Guy.Aktiv=false;
        }
        std::wstring materials(int object)
        {
            const auto& tile=Scape[Guy.Pos.x][Guy.Pos.y];if(tile.Objekt==object&&Construction::Unfinished(tile))return Construction::Rows(tile);
            const std::pair<int,const wchar_t*> names[]={{ROHAST,L"树枝"},{ROHSTEIN,L"石头"},{ROHBLATT,L"棕叶"},{ROHLIANE,L"藤蔓"},{ROHSTAMM,L"原木"}};
            std::wstring s;
            for(auto [id,name]:names)if(Bmp[object].Rohstoff[id])s+=std::wstring(name)+L" "+std::to_wstring(Bmp[object].Rohstoff[id])+L"  ";
            return s;
        }
        void recipe(const wchar_t* name,const wchar_t* cost,int a,int b,int x,int y,int tool)
        {
            const bool owned=Guy.Inventar[tool]>0;
            const bool enough=Guy.Inventar[a]>0&&Guy.Inventar[b]>0&&(tool!=ROHEGGE||Guy.Inventar[ROHAXT]>0);
            fill(x,y,284,70,CARD);txt(x+12,y+9,name);txt(x+12,y+37,cost,13,MUTED);
            button(x+194,y+15,78,38,owned?L"已制作":L"制作",[=]{
                if(owned){Expedition::Notify(L"已经拥有这个工具。");return;}
                if(!enough){Expedition::Notify(L"材料不足，先搜索树林或海滩收集所需材料。");return;}
                if(Guy.Aktiv||Guy.Aktion!=Action::NOTHING){Expedition::Notify(L"请先完成当前行动。");return;}
                TwoClicks=a;World::CheckBenutze(b);
            },!owned&&enough);
        }
        void waterControls(int y)
        {
            const auto info=Expedition::GetInfo();
            txt(992,y,info.waterskin?L"水壶 · 剩余 "+std::to_wstring(info.water)+L" / 3 份":L"制作水壶后，可以随身带水。",16,GOLD);
            button(980,y+30,136,36,L"装满水壶",[]{Expedition::FillWaterskin();},info.waterskin&&info.water<3&&Expedition::NearFreshwater()&&!Crafting::Busy());
            button(1128,y+30,136,36,L"喝一口 +25",[]{Expedition::DrinkWaterskin();},info.waterskin&&info.water>0&&Guy.Resource[WASSER]<=85&&!Crafting::Busy());
            txt(992,y+75,info.water==0?L"走到淡水旁装水；海水不能饮用。":L"装水与喝水是两个独立按钮，K 为快捷操作。",13,MUTED);
        }
        void top()
        {
            fill(0,0,MAXX,64,BG);line(0,63,MAXX);
            txt(24,17,L"漂流之境",25,GOLD);txt(164,25,L"荒岛生存",13,MUTED);
            const wchar_t* names[]={L"水分",L"饱食",L"生命"};
            const COLORREF colors[]={Guy.Resource[0]<50?RGB(224,126,98):RGB(115,172,185),Guy.Resource[1]<50?RGB(224,126,98):RGB(194,177,116),Guy.Resource[2]<=35?RGB(235,103,91):GREEN};
            for(int i=0;i<3;++i){const int x=300+i*112;txt(x,13,names[i],13,MUTED);txt(x+64,13,std::to_wstring(int(Guy.Resource[i])),13,FG);fill(x,39,88,4,LINE);fill(x,39,int(88*std::clamp(Guy.Resource[i],0.f,100.f)/100),4,colors[i]);}
            if(!fireList.empty())
            {
                button(646,10,124,42,L"篝火 · "+std::to_wstring(fireList.size()),[]{});
            }
            const auto w=ExpeditionRules::weather(Tag);const wchar_t* weather=w==ExpeditionRules::Weather::Clear?L"晴朗":w==ExpeditionRules::Weather::Rain?L"阵雨":L"风暴";
            wchar_t clock[64]{};std::swprintf(clock,64,L"第 %d 天  %02d:%02d  ·  %ls",Tag,Stunden+6,Minuten,weather);txt(790,24,clock,16,GOLD);
            txt(790,47,Nightfall::Pending()?L"收工后自动返营休息":L"持续走时 · 一天约4分48秒",13,MUTED);
            button(1144,13,112,38,L"菜单  Esc",[]{paused=true;});
        }
        void firePanel()
        {
            const bool wasOpen=fireDetails;fireDetails=false;if(fireList.empty()||PapierText!=-1||Nacht)return;
            const auto mouse=NativeDisplay::MousePoint();
            const int rows=std::min(8,int(fireList.size()));fireArea={632,70,948,112+rows*50+22};
            bool show=Campfires::Affected()||inside(mouse.x,mouse.y,RECT{646,10,770,64})||(wasOpen&&inside(mouse.x,mouse.y,fireArea));
            int hover=-1;
            if(mouse.x>=0&&mouse.x<960&&mouse.y>=64&&mouse.y<656&&!(wasOpen&&inside(mouse.x,mouse.y,fireArea)))
            {
                const auto scene=SceneView::ToScene(mouse.x,mouse.y);
                for(int i=0;i<int(fireList.size());++i){const auto& f=fireList[i];const auto b=Campfires::Sprite(Scape[f.x][f.y]);if(scene.x+Camera.x>=b.x&&scene.x+Camera.x<b.x+b.w&&scene.y+Camera.y>=b.y&&scene.y+Camera.y<b.y+b.h){show=true;hover=i;break;}}
            }
            if(!show)return;
            if(hover>=0)fireOffset=(hover/8)*8;
            fireOffset=std::clamp(fireOffset,0,std::max(0,int(fireList.size())-rows));fireDetails=true;
            fill(fireArea.left,fireArea.top,316,fireArea.bottom-fireArea.top,BG);fill(632,70,316,2,GOLD);
            txt(645,81,Campfires::Affected()?L"风雨正在影响篝火":L"篝火 · 剩余现实时间",16,GOLD);
            for(int row=0;row<rows&&fireOffset+row<int(fireList.size());++row)
            {
                const auto f=fireList[fireOffset+row];const int y=112+row*50;fill(640,y,300,46,CARD);
                txt(650,y+4,f.name,13,FG);txt(797,y+4,Campfires::Countdown(f.seconds),13,GOLD);
                txt(650,y+25,Campfires::Effect(),13,MUTED);
                button(868,y+8,64,29,L"添柴",[f]{Interaction::Target(f.x,f.y,Interaction::Intent::FeedFire);});
            }
            txt(645,fireArea.bottom-18,fireList.size()>8?L"滚轮翻看 · "+std::to_wstring(fireOffset+1)+L"–"+std::to_wstring(std::min(fireOffset+rows,int(fireList.size())))+L" / "+std::to_wstring(fireList.size()):L"现实分:秒 · 暂停与失焦时停计",13,MUTED);
        }
        void sidebar()
        {
            fill(960,64,320,656,BG);fill(960,64,1,656,LINE);
            txt(980,83,L"营地与探索",25);txt(980,116,L"选择行动，材料与条件会在下方显示。",13,MUTED);
            const wchar_t* tabs[]={L"行动",L"建造",L"背包",L"制作"};
            const int menus[]={Menu::ACTION,Menu::BUILD,Menu::INVENTORY,Menu::NONE};
            for(int i=0;i<4;++i){const bool chosen=(i==3?craft:!craft&&HauptMenue==menus[i]);button(980+i*72,149,66,34,tabs[i],[i]{craft=i==3;HauptMenue=i==0?Menu::ACTION:i==1?Menu::BUILD:i==2?Menu::INVENTORY:Menu::NONE;});if(chosen)fill(980+i*72,182,66,2,GOLD);}
            if(craft)
            {
                const wchar_t* categories[]={L"工具",L"补给",L"探索物品"};
                for(int category=0;category<3;++category){button(980+category*98,201,88,32,categories[category],[category]{craftCategory=category;craftOffset=0;});if(category==craftCategory)fill(980+category*98,232,88,2,GOLD);}
                const auto rows=craftRows();craftOffset=std::clamp(craftOffset,0,std::max(0,int(rows.size())-1));
                for(int slot=0;slot<4&&craftOffset+slot<int(rows.size());++slot)
                {
                    const auto* r=rows[craftOffset+slot];const int y=240+slot*96;const bool owned=Crafting::Owned(r->product),can=Crafting::CanMake(r->product);
                    fill(980,y,284,90,CARD);txt(992,y+8,r->name,16,owned?GREEN:FG);
                    if(r->prerequisite>=0&&!Crafting::Owned(r->prerequisite))txt(1074,y+12,L"先有石斧",13,GOLD);
                    else if(r->category!=2)txt(1074,y+12,std::to_wstring(Crafting::DurationSeconds(r->product))+L"秒",13,MUTED);
                    txt(992,y+34,r->purpose,13,MUTED);
                    auto old=SelectObject(active,font(13));SetTextColor(active,can?FG:MUTED);RECT costs{992,y+54,1252,y+88};const auto text=Crafting::Materials(*r);DrawTextW(active,text.c_str(),-1,&costs,DT_WORDBREAK|DT_NOPREFIX);SelectObject(active,old);
                    const int product=r->product;
                    if(r->category==2)button(1162,y+7,90,28,owned?L"已获得":L"获取方式",[product]{const auto message=Crafting::Problem(product);Expedition::Notify(message.c_str());});
                    else button(1162,y+7,90,28,Crafting::Job().product==product?L"制作中":(owned&&product!=Crafting::Bandage)?L"已拥有":L"制作",[product]{Crafting::Make(product);},can&&!Crafting::Busy());
                }
                if(craftCategory==1)waterControls(448);
                button(980,628,88,28,L"上一组",[]{craftOffset=std::max(0,craftOffset-4);},craftOffset>0);
                button(1176,628,88,28,L"下一组",[count=int(rows.size())]{if(craftOffset+4<count)craftOffset+=4;},craftOffset+4<int(rows.size()));
                txt(1081,635,std::to_wstring(craftOffset+1)+L"–"+std::to_wstring(std::min(craftOffset+4,int(rows.size())))+L" / "+std::to_wstring(rows.size()),13,MUTED);
            }
            else if(HauptMenue==Menu::INVENTORY)
            {
                const wchar_t* items[]={L"树枝",L"原木",L"石头",L"石斧",L"棕叶",L"石锄",L"藤蔓",L"钓竿",L"木石锤",L"望远镜",L"生火工具",L"木铲",L"藏宝图",L"投石索"};
                for(int i=0;i<14;++i){int id=ROHAST+i,x=980+(i%3)*98,y=201+(i/3)*64;button(x,y,90,55,std::wstring(items[i])+L" "+std::to_wstring(Guy.Inventar[id]),[id]{selection=id;});}
                txt(980,544,selection>=ROHAST&&selection<=ROHSCHLEUDER?std::wstring(items[selection-ROHAST])+L" · 数量 "+std::to_wstring(Guy.Inventar[selection]):L"物品会在需要时自动使用。",13,MUTED);waterControls(574);
            }
            else
            {
                static const int actions[]={BUTTSUCHEN,BUTTESSEN,BUTTSCHLAFEN,BUTTFAELLEN,BUTTANGELN,BUTTANZUENDEN,BUTTAUSSCHAU,BUTTSCHATZKARTE,BUTTSCHATZ,BUTTSCHLEUDER};
                static const wchar_t* actionNames[]={L"搜索周围",L"进食 / 饮水",L"休息片刻",L"砍伐树木",L"钓鱼",L"点燃篝火",L"眺望船只",L"查看藏宝图",L"挖掘宝藏",L"猎鸟"};
                static const int builds[]={BUTTZELT,BUTTFELD,BUTTBOOT,BUTTROHR,BUTTSOS,BUTTHAUS1,BUTTHAUS2,BUTTHAUS3,BUTTFEUERST,BUTTDESTROY};
                static const wchar_t* buildNames[]={L"帐篷",L"农田",L"独木舟",L"灌溉水管",L"求救标记",L"树屋梯子",L"树屋平台",L"树屋",L"篝火堆",L"拆除建筑"};
                for(int i=0;i<10;++i){const bool build=HauptMenue==Menu::BUILD;const int id=build?builds[i]:actions[i];actionButton(id,build?buildNames[i]:actionNames[i],980+(i%2)*148,202+(i/2)*55);}
                txt(980,502,HauptMenue==Menu::BUILD?L"在人物当前所在的地块上建造。":L"移动到目标附近，再选择对应行动。",13,MUTED);
                txt(980,528,L"灰色动作也可点击，查看工具与获取方式。",13,MUTED);
                if(HauptMenue==Menu::ACTION&&!Construction::Unfinished(Scape[Guy.Pos.x][Guy.Pos.y]))
                {
                    button(980,568,136,38,L"吃东西",[]{Interaction::Request(Interaction::Intent::Eat);});
                    button(1128,568,136,38,L"喝淡水",[]{Interaction::Request(Interaction::Intent::Drink);});
                    txt(980,622,L"砍树请用“砍伐树木”，不会自动误触。",13,MUTED);
                }
                const auto mouse=NativeDisplay::MousePoint();
                for(int i=0;i<10;++i){const int id=HauptMenue==Menu::BUILD?builds[i]:actions[i];if(inside(mouse.x,mouse.y,Bmp[id].rcDes)){fill(980,498,284,60,BG);auto old=SelectObject(active,font(13));SetTextColor(active,GOLD);RECT hint{980,502,1264,556};const auto text=Crafting::Requirement(id);DrawTextW(active,text.c_str(),-1,&hint,DT_WORDBREAK|DT_NOPREFIX);SelectObject(active,old);}}
                if(HauptMenue==Menu::BUILD){const int objects[]={ZELT,FELD,BOOT,ROHR,SOS,HAUS1,HAUS2,HAUS3,FEUERSTELLE};for(int i=0;i<9;++i)if(inside(mouse.x,mouse.y,Bmp[builds[i]].rcDes)){txt(980,565,L"所需材料",13,GOLD);const auto s=materials(objects[i]);RECT r{980,592,1262,650};auto old=SelectObject(active,font(13));SetTextColor(active,MUTED);DrawTextW(active,s.c_str(),-1,&r,DT_WORDBREAK);SelectObject(active,old);}}
                const auto& project=Scape[Guy.Pos.x][Guy.Pos.y];if(Construction::Unfinished(project))
                {
                    fill(980,492,284,164,BG);txt(980,494,L"继续施工 · "+std::wstring(Construction::Name(project.Objekt)),16,GOLD);
                    txt(980,518,L"进度 "+std::to_wstring(project.AkNummer)+L"/"+std::to_wstring(Bmp[project.Objekt].AkAnzahl)+L" · 只计算剩余用料",13,MUTED);
                    auto old=SelectObject(active,font(13));SetTextColor(active,FG);RECT needs{980,540,1264,624};const auto remaining=Construction::Rows(project);DrawTextW(active,remaining.c_str(),-1,&needs,DT_WORDBREAK|DT_NOPREFIX);SelectObject(active,old);
                    button(980,628,136,28,L"继续建造",[]{legacy(BUTTWEITER);},Bmp[BUTTWEITER].Phase!=-1);
                }
            }
            line(980,666,284);button(980,681,136,28,L"生存手记",[]{journal=true;});button(1128,681,136,28,L"声音设置",[]{paused=true;settings=true;});
        }
        void bottom()
        {
            const auto supplies=Expedition::GetInfo();
            fill(0,656,960,64,BG);line(0,656,960);
            button(18,668,136,38,L"互动  E",[]{Interact();});
            actionButton(BUTTSTOP,Crafting::UsingSupply()?L"停止动作":Crafting::Busy()?L"停止制作":L"停止行动",168,668,128,38);
            actionButton(BUTTWEITER,L"继续建造",310,668,128,38);
            const auto waterLabel=!supplies.waterskin?L"制作水壶":std::wstring(Expedition::NearFreshwater()&&supplies.water<3?L"装水 K · ":L"喝水 K · ")+std::to_wstring(supplies.water)+L"/3";
            button(452,668,128,38,waterLabel,[owned=supplies.waterskin]{if(owned)Expedition::UseWaterskin();else showRecipe(Crafting::Waterskin);});
            button(594,668,128,38,L"绷带 J · "+std::to_wstring(supplies.bandages),[]{Expedition::UseBandage();});
            actionButton(BUTTABLEGEN,L"上船 / 靠岸",736,668,150,38);
        }
        void modalPanel(const wchar_t* title)
        {
            static NativeSurface dim(1,1);HDC source=nullptr;dim.GetDC(&source);
            BLENDFUNCTION blend{AC_SRC_OVER,0,150,0};AlphaBlend(active,0,64,1280,656,source,0,0,1,1,blend);dim.ReleaseDC(source);
            fill(290,130,700,460,BG);fill(290,130,700,2,GOLD);txt(326,157,title,25);line(326,203,628);
        }
    }
    void Text(int x,int y,const std::wstring& s,int size,unsigned color)
    {
        begin(lpDDSSchrift);auto previous=SelectObject(active,font(size));SIZE extent{};
        GetTextExtentPoint32W(active,s.data(),int(s.size()),&extent);SelectObject(active,previous);
        fill(x,y,extent.cx+2,extent.cy+2,BG);txt(x,y,s,size,color?COLORREF(color):FG);end();
    }
    void Layout()
    {
        rcSpielflaeche={0,64,960,656};rcPanel={960,64,1280,720};rcKarte={812,84,932,204};
        TextBereich[TXTTEXTFELD].rcText={18,623,942,650};
        TextBereich[TXTPAPIER].rcText={326,230,954,520};
        TextBereich[TXTCHANCE].Aktiv=false;TextBereich[TXTTAGESZEIT].Aktiv=false;
        // Keep hidden legacy control rectangles away from modern actions.
        for(int id:{BUTTGITTER,BUTTANIMATION,BUTTBEENDEN,BUTTNEU,BUTTTAGNEU,BUTTSOUND,BUTTAKTION,BUTTBAUEN,BUTTINVENTAR})place(id,-2000,-2000);
    }
    void DrawPanel()
    {
        Layout();buttons.clear();overlayStart=0;fireList=Campfires::All();
        RECT source{0,0,120,120};lpDDSBack->Blt(&rcKarte,lpDDSKarte,&source,0,nullptr);
        begin(lpDDSBack);top();sidebar();bottom();
        fill(812+Guy.Pos.x*2,84+Guy.Pos.y*2,3,3,GOLD);
        const auto info=Expedition::GetInfo();
        txt(24,82,L"已探索 "+std::to_wstring(info.explored)+L"% · 救援进度 "+std::to_wstring(info.signal)+L"%",13,GOLD);
        txt(24,106,L"单击地面移动 · 右键拖动视角 · E 就近互动",13,MUTED);
        txt(812,214,L"缩放 "+std::to_wstring(int(SceneView::Scale()*100+.5f))+L"% · 滚轮调整",13,MUTED);
        if(duskNotice.visible(GetTickCount64()))
        {
            fill(440,83,330,38,BG);fill(440,83,3,38,GOLD);
            txt(454,93,duskNotice.atSea?L"天快黑了，请尽快靠岸。":L"天快黑了，记得准备过夜。",15,GOLD);
        }
        const auto spoken=Nightfall::Line();if(!spoken.empty())
        {
            const auto p=SceneView::ToScreen(Guy.PosScreen.x-Camera.x,Guy.PosScreen.y-Camera.y);
            const int x=std::clamp(p.x-120,24,696),y=std::clamp(p.y-90,166,480);
            fill(x,y,240,40,BG);fill(x,y+39,240,1,GOLD);txt(x+12,y+11,spoken,13,GOLD);
        }
        if(Guy.Resource[WASSER]<50||Guy.Resource[NAHRUNG]<50||Guy.Resource[GESUNDHEIT]<=35)
        {
            auto old=SelectObject(active,font(16));
            std::wstring message=L"生存警告：";
            if(Guy.Resource[WASSER]<50)message+=L"缺水 · ";
            if(Guy.Resource[NAHRUNG]<50)message+=L"饥饿 · ";
            if(Guy.Resource[GESUNDHEIT]<=35)message+=L"生命危急 · ";
            message+=L"先补水进食，停止高消耗劳动。K 水壶 / J 绷带 / 点击成熟浆果。";
            RECT r{30,143,766,4096};DrawTextW(active,message.c_str(),-1,&r,DT_WORDBREAK|DT_NOPREFIX|DT_CALCRECT);
            const int height=r.bottom-r.top+16;
            fill(18,136,760,height,RGB(63,37,32));fill(18,136,3,height,RGB(224,126,98));
            SetTextColor(active,RGB(245,200,173));DrawTextW(active,message.c_str(),-1,&r,DT_WORDBREAK|DT_NOPREFIX);SelectObject(active,old);
        }
        firePanel();end();
    }
    bool FirePanelHit(int x,int y){return fireDetails&&inside(x,y,fireArea);}
    bool OverlayActive(){return safety||paused||journal||confirmNew||Spielzustand==State::LOGO||Spielzustand==State::OUTRO;}
    bool DuskNoticeVisible(){return duskNotice.visible(GetTickCount64());}
    void CheckSafety()
    {
        if(PapierText!=-1)return;
        Survival::Update();const int alert=Survival::TakeAlert();if(!alert)return;
        if(alert==2){duskNotice.show(GetTickCount64(),BootsFahrt);return;}
        safety=alert;
        if(alert==1){Crafting::Cancel(false);Interaction::Cancel();
        if(Guy.Aktion!=Action::DEATH&&Guy.Aktion!=Action::DAY_END&&(Guy.Aktiv||Guy.Aktion!=Action::NOTHING))
        {Interaction::StopSafely();}}
    }
    bool ShouldQuit(){return quit;}
    void StartGame(bool fresh)
    {
        paused=false;settings=false;journal=false;craft=false;safety=0;fireDetails=false;fireOffset=0;duskNotice.reset();Survival::Reset();Interaction::Cancel();NativeDisplay::SetFade(1,1,1);
        Game::NeuesSpiel(fresh);
        LAnimation=true;for(int i=MEERWELLEN;i<=SCHLEUSE6;++i)Bmp[i].Animation=true;World::Generate();
        Crafting::SyncUnlocks();
        HauptMenue=Menu::ACTION;Layout();
        Camera.x=Guy.PosScreen.x-480;Camera.y=Guy.PosScreen.y-360;
        Renderer::LimitScroll();
        Expedition::Notify(L"单击地面即可移动。按 E 互动；打开“制作”准备工具。天黑前寻找庇护所。");
    }
    void DrawStart()
    {
        buttons.clear();overlayStart=0;NativeDisplay::SetFade(1,1,1);begin(lpDDSBack);
        for(int y=0;y<720;++y)fill(0,y,1280,1,RGB(15+y/75,35+y/42,42+y/40));
        for(int i=0;i<18;++i)fill(550+i*18,470-i*7,590-i*22,2,RGB(54,87,79));
        txt(98,118,L"漂流之境",46,GOLD);txt(100,193,L"一座岛。一段归途。",25);txt(100,242,L"探索荒岛，建立营地，等待归来的船。",16,MUTED);
        const bool saved=std::filesystem::exists("save.dat");
        button(100,330,284,50,L"继续旅程",[]{StartGame(false);},saved);
        button(100,394,284,50,L"新的旅程",[]{if(std::filesystem::exists("save.dat"))confirmNew=true;else StartGame(true);});
        button(100,458,136,42,L"设置",[]{paused=true;settings=true;});button(248,458,136,42,L"退出",[]{quit=true;});
        txt(100,628,L"全中文界面 · 单击移动 · 一键制作",13,MUTED);txt(100,659,L"按回车继续，按 Esc 打开菜单。",13,MUTED);end();
        DrawOverlay();NativeDisplay::Present();
    }
    void DrawCredits()
    {
        buttons.clear();overlayStart=0;NativeDisplay::SetFade(1,1,1);begin(lpDDSBack);fill(0,0,1280,720,BG);
        txt(410,180,L"每一次漂流，都是新的开始。",25,GOLD);txt(470,262,L"原作：德克·普拉特",16);txt(470,299,L"移植：卢卡斯·迪伦贝格",16);
        button(466,390,340,50,L"返回主菜单",[]{Spielzustand=State::LOGO;});button(466,456,340,42,L"退出游戏",[]{quit=true;});end();NativeDisplay::Present();
    }
    void DrawDialog()
    {
        begin(lpDDSBack);modalPanel(Nacht?L"夜晚的手记":L"岛屿见闻");
        const bool question=Frage==0;
        if(question){fill(326,535,284,38,CARD);fill(638,535,284,38,CARD);txt(350,543,L"确认",16,GOLD);txt(662,543,L"取消");Bmp[JA].rcDes={326,535,610,573};Bmp[NEIN].rcDes={638,535,922,573};}
        else{fill(326,535,596,38,CARD);txt(534,543,L"点击继续  /  回车",16,GOLD);}
        end();
    }
    void DrawNightJournal()
    {
        if(!DayJournal::Active())return;
        begin(lpDDSBack);
        for(int y=0;y<720;++y)fill(0,y,1280,1,RGB(10+y/145,22+y/100,35+y/80));
        for(int i=0;i<45;++i){const int x=40+(i*197)%1200,y=25+(i*79)%165;fill(x,y,2,2,RGB(84+i%4*18,104+i%4*16,115+i%4*14));}
        fill(104,67,1072,588,RGB(19,34,42));fill(104,67,1072,2,GOLD);
        txt(145,91,L"第 "+std::to_wstring(Tag)+L" 天",13,GOLD);txt(145,117,L"夜晚的手记",25,FG);
        txt(145,154,L"让今天慢慢落下，也为明天留一点光。",13,MUTED);
        line(145,185,990,LINE);
        txt(145,207,L"今天的足迹",16,GOLD);
        const auto summary=DayJournal::Summary();int y=246;
        for(const auto& item:summary){txt(145,y,L"·  "+item,16,FG);y+=28;}
        fill(755,203,380,230,RGB(25,45,51));fill(755,203,2,230,RGB(135,126,91));
        txt(781,222,L"今日收获",16,GOLD);txt(781,250,L"实际入包的数量，含后来用掉的物资",13,MUTED);
        const auto loot=DayJournal::LootSummary();
        if(loot.empty())txt(781,292,L"今天没有记录到物资收获。",13,MUTED);
        else for(unsigned i=0;i<loot.size();++i)
        {
            const int x=781+int(i%2)*174, row=290+int(i/2)*26;
            txt(x,row,loot[i].name,13,FG);txt(x+107,row,L"+"+std::to_wstring(loot[i].count),13,GOLD);
        }
        fill(145,467,990,96,RGB(31,48,51));fill(145,467,3,96,GOLD);
        txt(170,483,L"给今天的你",13,GOLD);
        auto old=SelectObject(active,font(16));SetBkMode(active,TRANSPARENT);SetTextColor(active,RGB(237,225,196));
        const auto encouragement=DayJournal::Encouragement(Tag);RECT r{170,511,1105,555};DrawTextW(active,encouragement.c_str(),-1,&r,DT_WORDBREAK|DT_NOPREFIX);SelectObject(active,old);
        const auto& tile=Scape[Guy.Pos.x][Guy.Pos.y];
        const bool tent=tile.Objekt==ZELT&&tile.Phase<Bmp[ZELT].Anzahl;
        const bool house=tile.Objekt==HAUS3&&tile.Phase<Bmp[HAUS3].Anzahl;
        txt(145,588,house?L"树屋休息 · 恢复生命":tent?L"帐篷休息 · 生命 −5":L"野外休息 · 生命 −20",13,MUTED);
        txt(793,588,DayJournal::CanSkip()?L"回车或点击此处，迎接清晨 →":L"让海浪陪你休息一会儿……",13,GOLD);
        end();
    }
    void DrawOverlay()
    {
        if(!paused&&!journal&&!confirmNew&&!safety)return;
        overlayStart=buttons.size();
        begin(lpDDSBack);modalPanel(safety?L"生命危急 · 已自动暂停":confirmNew?L"开始新的旅程":journal?L"生存手记":settings?L"游戏设置":L"暂停旅程");
        if(safety)
        {
            txt(326,231,L"当前行动已停止，请先处理生存需要。",16,GOLD);
            const auto message=Survival::Advice();auto old=SelectObject(active,font(16));SetTextColor(active,FG);
            RECT r{326,280,940,423};DrawTextW(active,message.c_str(),-1,&r,DT_WORDBREAK|DT_NOPREFIX);SelectObject(active,old);
            txt(326,443,L"首次致命的日间消耗会保留 1 点生命，给你一次处理机会。",13,MUTED);
            button(326,485,596,40,L"停止行动，立即处理补给",[]{safety=0;Survival::Acknowledge();});
            button(326,537,596,36,L"查看生存手记",[]{safety=0;Survival::Acknowledge();journal=true;});
        }
        else if(confirmNew)
        {
            txt(326,248,L"开始新旅程会替换当前岛屿的存档。",16);
            txt(326,289,L"如果想保留进度，请返回主菜单并选择“继续旅程”。",16,MUTED);
            button(326,402,284,46,L"开始新旅程",[]{confirmNew=false;StartGame(true);});
            button(638,402,284,46,L"返回主菜单",[]{confirmNew=false;});
        }
        else if(journal)
        {
            const wchar_t* goals[]={L"一、寻找淡水与食物，制作水壶。",L"二、建造帐篷或树屋，准备过夜。",L"三、制作独木舟，探索海上的沉船。",L"四、取得火柴，建成求救标记与篝火。",L"五、维持火源与信号，等待救援。"};
            const auto info=Expedition::GetInfo();
            for(int i=0;i<5;++i)txt(326,229+i*39,(info.stages[i]?std::wstring(L"✓ "):std::wstring(L"○ "))+goals[i],16,info.stages[i]?GREEN:FG);
            txt(326,442,L"水分和饱食低于 50 时，生命会逐渐下降。",13,MUTED);txt(326,468,L"每天 18:00 入夜，请提前返回附近的庇护所。",13,MUTED);
            button(326,530,596,38,L"返回岛屿",[]{journal=false;});
        }
        else if(settings)
        {
            button(326,231,284,40,Soundzustand>0?L"声音：开启":L"声音：关闭",[]{Sound::SetEnabled(Soundzustand<=0);});
            button(638,231,284,40,L"切换天气氛围",[]{Expedition::ToggleEffects();});
            button(638,283,284,40,Gitter?L"地块网格：开启":L"地块网格：关闭",[]{Gitter=!Gitter;if(Spielzustand==State::GAME)World::Generate();});
            txt(342,294,L"水面动画：持续播放",14,GREEN);
            const wchar_t* labels[]={L"音乐",L"环境",L"动作音效"};
            for(int i=0;i<3;++i)
            {
                const auto channel=static_cast<Sound::Bus>(i);const int y=341+i*47;
                txt(342,y+10,labels[i],16,FG);
                txt(625,y+10,std::to_wstring(Sound::GetMix(channel))+L"%",15,GOLD);
                button(734,y,72,35,L"−",[channel]{Sound::SetMix(channel,Sound::GetMix(channel)-10);},Sound::GetMix(channel)>0);
                button(830,y,72,35,L"+",[channel]{Sound::SetMix(channel,Sound::GetMix(channel)+10);},Sound::GetMix(channel)<100);
            }
            txt(342,495,L"声音设置自动保留；切出窗口时会渐静。",13,MUTED);
            button(326,530,596,38,L"返回",[]{settings=false;paused=Spielzustand!=State::LOGO;});
        }
        else
        {
            button(326,231,596,48,L"继续旅程",[]{paused=false;});
            button(326,294,284,42,L"设置",[]{settings=true;});button(638,294,284,42,L"生存手记",[]{paused=false;journal=true;});
            button(326,354,596,42,L"保存进度",[]{Game::SaveGame();Expedition::Notify(L"旅程已保存。");paused=false;});
            button(326,414,284,42,L"返回主菜单",[]{Game::SaveGame();paused=false;Spielzustand=State::LOGO;});
            button(638,414,284,42,L"保存并退出",[]{Game::SaveGame();quit=true;});
            button(326,530,596,38,L"重新开始这一天",[]{paused=false;Guy.AkNummer=0;Guy.Aktiv=false;Guy.Aktion=Action::DAY_RESTART;});
        }
        end();
    }
    bool HandleClick(int x,int y)
    {
        if(DayJournal::Active()&&!OverlayActive()){if(inside(x,y,RECT{775,575,1140,620}))DayJournal::Skip();return true;}
        if(Spielzustand!=State::LOGO&&PapierText!=-1&&!paused&&!journal&&!safety)
        {
            if(Frage==0){if(inside(x,y,Bmp[JA].rcDes))closePaper(true);else if(inside(x,y,Bmp[NEIN].rcDes))closePaper(false);}
            else closePaper(true);return true;
        }
        if(Spielzustand==State::GAME&&!OverlayActive()&&!FirePanelHit(x,y)&&inside(x,y,rcKarte))
        {
            const auto old=MousePosition;MousePosition={short(x),short(y)};Math::MouseInPanel(0,0);MousePosition=old;return true;
        }
        const std::size_t first=OverlayActive()?overlayStart:0;
        for(std::size_t n=buttons.size();n>first;--n)
        {
            const auto& b=buttons[n-1];if(!inside(x,y,b.rect))continue;
            auto fn=b.click;if(!b.enabled&&Spielzustand==State::LOGO)return true;
            fn();return true;
        }
        if(OverlayActive()||FirePanelHit(x,y)||x>=960||y<64||y>=656)return true;
        if(Crafting::Busy()||Nightfall::Pending())
        {const auto message=Crafting::Busy()?L"正在"+std::wstring(Crafting::UsingSupply()?L"":L"制作")+Crafting::Name(Crafting::Job().product)+L"，可等待完成或点击下方停止按钮。":L"正在收工返营，请稍候。";Expedition::Notify(message.c_str());return true;}
        if(Spielzustand==State::GAME){const auto p=SceneView::ToScene(x,y);return Interaction::Click(p.x,p.y);}return false;
    }
    bool HandleWheel(float delta,int x,int y)
    {
        if(OverlayActive()||PapierText!=-1)return true;
        if(FirePanelHit(x,y)){fireOffset=std::clamp(fireOffset+(delta<0?1:delta>0?-1:0),0,std::max(0,int(fireList.size())-8));return true;}
        if(craft&&x>=960&&y>=196&&y<666){const int count=int(craftRows().size());craftOffset=std::clamp(craftOffset+(delta<0?1:delta>0?-1:0),0,std::max(0,count-4));return true;}
        if(Spielzustand==State::GAME&&x>=0&&x<960&&y>=64&&y<656){SceneView::Zoom(delta,x,y);Direct::Reset();return true;}return false;
    }
    bool HandleKey(int key)
    {
        if(safety)return true;
        if(DayJournal::Active()&&key==sf::Keyboard::Return&&!OverlayActive()){DayJournal::Skip();return true;}
        if(key==sf::Keyboard::Escape){if(confirmNew){confirmNew=false;return true;}if(PapierText!=-1){closePaper(false);return true;}if(journal)journal=false;else{paused=!paused;settings=false;}return true;}
        if(Spielzustand!=State::LOGO&&key==sf::Keyboard::Return&&PapierText!=-1){closePaper(true);return true;}
        if(Spielzustand==State::LOGO&&key==sf::Keyboard::Return){StartGame(!std::filesystem::exists("save.dat"));return true;}
        if(Spielzustand!=State::GAME)return false;
        if(key==sf::Keyboard::F1){journal=!journal;return true;}
        if(OverlayActive())return true;
        if(key==sf::Keyboard::E){Interact();return true;}
        if(key==sf::Keyboard::B){craft=false;HauptMenue=Menu::BUILD;return true;}
        if(key==sf::Keyboard::I){craft=false;HauptMenue=Menu::INVENTORY;return true;}
        if(key==sf::Keyboard::C){craft=true;HauptMenue=Menu::NONE;return true;}
        return false;
    }
    void Interact()
    {
        if(Nightfall::Pending()){Expedition::Notify(L"天黑了，先收工休息。");return;}
        if(Guy.Aktiv){Expedition::Notify(L"正在行动。需要改变计划时，先点击“停止行动”。");return;}
        const auto& t=Scape[Guy.Pos.x][Guy.Pos.y];const int old=HauptMenue;HauptMenue=Menu::ACTION;
        if(Interaction::Request(Interaction::Intent::Any)){HauptMenue=old;return;}
        if((t.Objekt>=FLUSS1&&t.Objekt<=SCHLEUSE6)||(t.Objekt==ROHR&&t.Phase==1)||((t.Objekt==BUSCH||t.Objekt==FELD)&&t.Phase==Bmp[t.Objekt].Anzahl-1))legacy(BUTTESSEN);
        else if(t.Objekt==FEUER)Interaction::Target(Guy.Pos.x,Guy.Pos.y,Interaction::Intent::FeedFire);
        else if(t.Objekt==FEUERSTELLE)legacy(BUTTANZUENDEN);
        else if(t.Objekt==ZELT||t.Objekt==HAUS3)legacy(BUTTSCHLAFEN);
        else legacy(BUTTSUCHEN);
        HauptMenue=old;
    }
}
