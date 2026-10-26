#include "Survival.hpp"
#include "SurvivalRules.hpp"
#include "Action.hpp"
#include "State.hpp"

namespace Survival
{
    namespace {SurvivalRules::Warnings warnings;}
    void Reset(){warnings={};}
    void Update()
    {
        if(Spielzustand!=State::GAME||Nacht||Guy.Aktion==Action::DEATH||Guy.Aktion==Action::DAY_END)return;
        const auto& tile=Scape[Guy.Pos.x][Guy.Pos.y];
        const bool shelter=(tile.Objekt==ZELT||tile.Objekt==HAUS3)&&tile.Phase>=0&&tile.Phase<Bmp[tile.Objekt].Anzahl;
        warnings.update(Guy.Resource[GESUNDHEIT],Tag,Stunden,BootsFahrt||!shelter);
    }
    float ProtectHealth(float previous,float next)
    {
        if(Spielzustand!=State::GAME||Nacht||Guy.Aktion==Action::DAY_END)return next;
        return warnings.protect(previous,next);
    }
    int TakeAlert(){int result=warnings.pending;warnings.pending=0;return result;}
    void Acknowledge(){warnings.protectedUntilAcknowledged=false;}
    std::wstring Advice()
    {
        std::wstring message;
        if(Guy.Resource[WASSER]<50)message+=L"缺水：点击淡水或走近后按 E 饮水；有储水时按 K。";
        if(Guy.Resource[NAHRUNG]<50)message+=L"饥饿：点击成熟浆果，或走近后按 E 进食。";
        if(Guy.Resource[GESUNDHEIT]<=35)message+=L"生命危险：先停止劳动，按 J 使用绷带；补足食物和水后生命才会恢复。";
        if(message.empty())message=L"没有庇护所过夜会失去 20 点生命，帐篷过夜失去 5 点；树屋可恢复生命。夜晚留在海上会遇难，请在 18:00 前靠岸。";
        else message+=L"水分与饱食合计低于 100 时，时间流逝会持续损失生命。";
        return message;
    }
    std::wstring DeathReason()
    {
        if(Nacht&&BootsFahrt)return L"原因：入夜时仍在海上，没有安全的过夜地点。";
        if(Nacht)return L"原因：生命不足以承受过夜损耗。露宿会损失 20 点生命，帐篷会损失 5 点；树屋能提供更好的保护。";
        if(BootsFahrt&&Stunden>=10)return L"原因：入夜时仍在海上，没有安全的过夜地点。";
        if(Guy.Resource[WASSER]<50&&Guy.Resource[NAHRUNG]<50)return L"原因：长期缺水和饥饿使生命耗尽。请提前寻找淡水和成熟浆果。";
        if(Guy.Resource[WASSER]<50)return L"原因：缺水导致生命持续下降。请使用水壶或河流补水。";
        if(Guy.Resource[NAHRUNG]<50)return L"原因：饥饿导致生命持续下降。请寻找成熟浆果或农作物。";
        return L"原因：生命不足以承受这次损耗。提前包扎伤口，天黑前准备好庇护所。";
    }
}
