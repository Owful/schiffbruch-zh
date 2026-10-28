#include "Crafting.hpp"
#include "headers.hpp"
#include "extern.hpp"
#include "Expedition.hpp"
#include "World.hpp"
#include "Action.hpp"
#include "State.hpp"
#include "DayJournal.hpp"
#include <array>
#include <istream>
#include <ostream>
namespace Crafting
{
    namespace
    {
        int pendingProduct=-1;std::uint32_t elapsed=0;
        void animate()
        {
            Guy.Aktion=Action::CRAFT;Guy.Aktiv=true;Guy.AkNummer=0;
            Guy.Zustand=pendingProduct==DrinkWater?GUYTRINKEN:pendingProduct==FillWater?GUYBINDENUNTEN:
                (pendingProduct==ApplyBandage||pendingProduct==ROHANGEL||pendingProduct==Waterskin||pendingProduct==Bandage)?GUYBINDENOBEN:GUYSCHLAGEN;
            Bmp[BUTTSTOP].Phase=0;
        }
    }
    int DurationSeconds(int product){const auto* r=Find(product);return !r?0:r->category==1?3:r->minutes>0?5:4;}
    bool IsSupplyAction(int product){return product==FillWater||product==DrinkWater||product==ApplyBandage;}
    bool UsingSupply(){return Busy()&&IsSupplyAction(pendingProduct);}
    std::uint32_t DurationMicros(int product)
    {
        switch(product){case DrinkWater:return 1200000;case FillWater:return 1600000;case ApplyBandage:return 1900000;default:return DurationSeconds(product)*1000000u;}
    }
    bool Busy(){return pendingProduct>=0;}
    JobInfo Job(){return {pendingProduct,Busy()?float(elapsed)/DurationMicros(pendingProduct):0,Busy()?int((DurationMicros(pendingProduct)-elapsed+999999)/1000000):0};}
    void Cancel(bool notify)
    {
        const bool active=Busy(),supply=UsingSupply();pendingProduct=-1;elapsed=0;
        if(Guy.Aktion==Action::CRAFT){Guy.Aktion=Action::NOTHING;Guy.Aktiv=false;Guy.AkNummer=0;Guy.Zustand=BootsFahrt?GUYBOOTLINKS:GUYLINKS;Bmp[BUTTSTOP].Phase=-1;}
        if(active&&notify)Expedition::Notify(supply?L"已停止动作，水和绷带没有消耗。":L"已停止制作，材料没有扣除。");
    }
    void Reset(){Cancel(false);}
    void Save(std::ostream& out){const std::int32_t product=pendingProduct;out.write("CRF1",4);out.write(reinterpret_cast<const char*>(&product),4);out.write(reinterpret_cast<const char*>(&elapsed),4);}
    bool Load(std::istream& in)
    {
        Reset();std::array<char,4> magic{};std::int32_t product=-1;std::uint32_t time=0;
        if(!in.read(magic.data(),4)||magic!=std::array<char,4>{'C','R','F','1'}||!in.read(reinterpret_cast<char*>(&product),4)||!in.read(reinterpret_cast<char*>(&time),4))return false;
        if(product==-1)return time==0;
        const auto* r=Find(product);if((!IsSupplyAction(product)&&(!r||r->category==2))||time>=DurationMicros(product)||!Problem(product).empty())return false;
        pendingProduct=product;elapsed=time;animate();Bmp[Guy.Zustand].Phase=0;return true;
    }
    const std::vector<Recipe>& Recipes()
    {
        static const std::vector<Recipe> recipes={
            {ROHAXT,0,L"石斧",L"砍树；解锁独木舟和水管",{{ROHAST,1},{ROHSTEIN,1}},-1,0},
            {ROHEGGE,0,L"石锄",L"解锁农田，种植食物",{{ROHAST,1},{ROHSTEIN,1}},ROHAXT,0},
            {ROHANGEL,0,L"钓竿",L"在河边或船上钓鱼",{{ROHAST,1},{ROHLIANE,1}},-1,0},
            {ROHSCHLEUDER,0,L"投石索",L"猎鸟，获得食物",{{ROHSTEIN,1},{ROHLIANE,1}},-1,0},
            {ROHHAMMER,0,L"木石锤",L"解锁树屋梯子、平台与树屋",{{ROHAST,2},{ROHSTEIN,2},{ROHLIANE,1}},ROHAXT,20},
            {ROHSCHAUFEL,0,L"木铲",L"配合藏宝图挖掘宝藏",{{ROHSTAMM,1},{ROHAST,2},{ROHLIANE,1}},ROHAXT,20},
            {ROHSTREICHHOLZ,0,L"生火工具",L"点燃篝火，维持救援信号",{{ROHAST,2},{ROHSTEIN,2},{ROHBLATT,1},{ROHLIANE,1}},ROHAXT,20},
            {Waterskin,1,L"水壶",L"储水3份，离开水源也可饮用",{{ROHBLATT,2},{ROHLIANE,2}},-1,10},
            {Bandage,1,L"绷带",L"使用恢复20生命，最多携带3份",{{ROHBLATT,3},{ROHLIANE,1}},-1,15},
            {ROHFERNROHR,2,L"望远镜",L"扩大视野，解锁眺望船只",{},-1,0},
            {ROHKARTE,2,L"藏宝图",L"查看藏宝位置，配合木铲挖掘",{},-1,0}
        };return recipes;
    }
    const Recipe* Find(int product){for(const auto& r:Recipes())if(r.product==product)return &r;return nullptr;}
    std::wstring Name(int item)
    {
        switch(item){case FillWater:return L"装水";case DrinkWater:return L"喝水";case ApplyBandage:return L"包扎";default:break;}
        if(const auto* r=Find(item))return r->name;
        switch(item){case ROHAST:return L"树枝";case ROHSTEIN:return L"石头";case ROHLIANE:return L"藤蔓";case ROHBLATT:return L"棕叶";case ROHSTAMM:return L"原木";default:return L"工具";}
    }
    bool Owned(int product){if(product!=Waterskin&&product!=Bandage)return product>=0&&product<BILDANZ&&Guy.Inventar[product]>0;const auto info=Expedition::GetInfo();return product==Waterskin?info.waterskin:info.bandages>0;}
    std::wstring Materials(const Recipe& r)
    {
        if(r.category==2)return Owned(r.product)?L"已获得":L"乘独木舟，在海上沉船处搜索";
        std::wstring text;for(const auto& cost:r.costs){if(!text.empty())text+=L"  ";text+=Name(cost.item)+L" "+std::to_wstring(Guy.Inventar[cost.item])+L"/"+std::to_wstring(cost.count);const int missing=cost.count-Guy.Inventar[cost.item];if(missing>0)text+=L" 缺"+std::to_wstring(missing);}return text;
    }
    std::wstring Problem(int product)
    {
        if(IsSupplyAction(product))return Expedition::SupplyProblem(product);
        const auto* r=Find(product);if(!r)return L"没有对应配方。";
        if(r->category==2)return product==ROHFERNROHR?L"望远镜需要打捞：制作石斧，建造独木舟，上船后在普通沉船处选择“搜索周围”。":L"藏宝图需要探索：乘独木舟寻找另一处海盗沉船，在沉船处选择“搜索周围”。";
        if(product==Bandage&&Expedition::GetInfo().bandages>=3)return L"绷带已满（3/3），需要时按 J 使用。";
        if(product!=Bandage&&Owned(product))return L"已经拥有"+Name(product)+L"，无需重复制作。";
        if(r->prerequisite>=0&&!Owned(r->prerequisite))return L"先制作"+Name(r->prerequisite)+L"，再制作"+Name(product)+L"。";
        std::wstring missing;for(const auto& cost:r->costs)if(Guy.Inventar[cost.item]<cost.count){if(!missing.empty())missing+=L"、";missing+=Name(cost.item)+L"缺"+std::to_wstring(cost.count-Guy.Inventar[cost.item]);}
        return missing.empty()?L"":L"材料不足："+missing+L"。可在树林、海滩搜索，原木需要砍树。";
    }
    bool CanMake(int product){return Problem(product).empty();}
    void SyncUnlocks()
    {
        const auto set=[](int button,bool enabled){Bmp[button].Phase=enabled?0:-1;};
        set(BUTTFAELLEN,Owned(ROHAXT));set(BUTTBOOT,Owned(ROHAXT));set(BUTTROHR,Owned(ROHAXT));set(BUTTFELD,Owned(ROHEGGE));set(BUTTANGELN,Owned(ROHANGEL));set(BUTTSCHLEUDER,Owned(ROHSCHLEUDER));
        set(BUTTHAUS1,Owned(ROHHAMMER));set(BUTTHAUS2,Owned(ROHHAMMER));set(BUTTHAUS3,Owned(ROHHAMMER));set(BUTTANZUENDEN,Owned(ROHSTREICHHOLZ));set(BUTTAUSSCHAU,Owned(ROHFERNROHR));set(BUTTSCHATZKARTE,Owned(ROHKARTE));set(BUTTSCHATZ,Owned(ROHSCHAUFEL)&&Owned(ROHKARTE));
    }
    bool Make(int product)
    {
        const auto* r=Find(product);if(!r)return false;
        if(Spielzustand!=State::GAME||Nacht||BootsFahrt||Guy.Aktiv||Guy.Aktion!=Action::NOTHING||PapierText!=-1||Busy()){Expedition::Notify(L"请先完成当前行动并关闭对话，再在岸上制作。");return false;}
        if(Stunden>=12){Expedition::Notify(L"天黑了，先休息，明早再制作。");return false;}
        const auto problem=Problem(product);if(!problem.empty()){Expedition::Notify(problem.c_str());return false;}
        pendingProduct=product;elapsed=0;animate();Bmp[Guy.Zustand].Phase=0;
        const auto message=L"正在制作"+Name(product)+L"，完成后扣除材料。可点击“停止制作”取消。";Expedition::Notify(message.c_str());return true;
    }
    bool StartSupplyAction(int product)
    {
        if(!IsSupplyAction(product))return false;
        if(Busy())
        {
            if(pendingProduct!=product){const auto message=L"正在"+std::wstring(UsingSupply()?L"":L"制作")+Name(pendingProduct)+L"，请稍候或停止当前动作。";Expedition::Notify(message.c_str());}
            return false;
        }
        if(Guy.Aktiv||Guy.Aktion!=Action::NOTHING||Spielzustand!=State::GAME||Nacht||PapierText!=-1)
        {Expedition::Notify(L"请先完成当前动作并关闭提示，再使用补给。");return false;}
        if(Stunden>=12){Expedition::Notify(L"天黑了，先休息，明早再使用补给。");return false;}
        const auto problem=Problem(product);if(!problem.empty()){Expedition::Notify(problem.c_str());return false;}
        pendingProduct=product;elapsed=0;animate();Bmp[Guy.Zustand].Phase=0;
        const auto message=L"正在"+Name(product)+L"……";Expedition::Notify(message.c_str());return true;
    }
    void Update(std::int64_t micros,bool running)
    {
        if(!Busy())return;
        if(Guy.Aktion!=Action::CRAFT||Spielzustand!=State::GAME||Nacht){Cancel(false);return;}
        if(!running||micros<=0)return;
        elapsed+=std::uint32_t(std::min<std::int64_t>(micros,250000));
        if(UsingSupply())
        {
            const int frames=Bmp[Guy.Zustand].Anzahl;
            if(frames>0)Bmp[Guy.Zustand].Phase=short((elapsed/(pendingProduct==DrinkWater?240000u:220000u))%frames);
        }
        if(elapsed<DurationMicros(pendingProduct))return;
        const int product=pendingProduct;Cancel(false);
        const auto problem=Problem(product);if(!problem.empty()){Expedition::Notify(problem.c_str());return;}
        if(IsSupplyAction(product)){Expedition::CompleteSupplyAction(product);return;}
        if(product==Waterskin){if(Expedition::CraftWaterskin())DayJournal::Mark(Tag,DayJournal::Craft);return;}
        if(product==Bandage){if(Expedition::CraftBandage())DayJournal::Mark(Tag,DayJournal::Craft);return;}
        const auto* r=Find(product);for(const auto& cost:r->costs)Guy.Inventar[cost.item]-=cost.count;
        Guy.Inventar[product]=1;SyncUnlocks();DayJournal::Mark(Tag,DayJournal::Craft);
        const auto message=L"制作完成："+Name(product)+L"。"+r->purpose+L"。";Expedition::Notify(message.c_str());
    }
    int MissingForAction(int button)
    {
        switch(button){case BUTTFAELLEN:case BUTTBOOT:case BUTTROHR:return Owned(ROHAXT)?-1:ROHAXT;case BUTTFELD:return Owned(ROHEGGE)?-1:ROHEGGE;case BUTTANGELN:return Owned(ROHANGEL)?-1:ROHANGEL;
        case BUTTSCHLEUDER:return Owned(ROHSCHLEUDER)?-1:ROHSCHLEUDER;case BUTTHAUS1:case BUTTHAUS2:case BUTTHAUS3:return Owned(ROHHAMMER)?-1:ROHHAMMER;case BUTTANZUENDEN:return Owned(ROHSTREICHHOLZ)?-1:ROHSTREICHHOLZ;
        case BUTTAUSSCHAU:return Owned(ROHFERNROHR)?-1:ROHFERNROHR;case BUTTSCHATZKARTE:return Owned(ROHKARTE)?-1:ROHKARTE;case BUTTSCHATZ:return !Owned(ROHSCHAUFEL)?ROHSCHAUFEL:!Owned(ROHKARTE)?ROHKARTE:-1;default:return -1;}
    }
    std::wstring Requirement(int button)
    {
        const int item=MissingForAction(button);if(item<0)return L"工具已就绪，仍需走到合适地点。";
        return L"需要"+Name(item)+L" · 点击该动作查看"+std::wstring(Find(item)->category==2?L"获取方式。":L"制作配方。");
    }
}
