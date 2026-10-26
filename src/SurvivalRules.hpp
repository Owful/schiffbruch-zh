#pragma once
namespace SurvivalRules
{
    struct Warnings
    {
        bool criticalShown=false,lastChanceUsed=false,protectedUntilAcknowledged=false;
        int duskDay=-1,pending=0;
        float protect(float previous,float next)
        {
            if(previous<=0||next>0)return next;
            if(protectedUntilAcknowledged)return 1;
            if(lastChanceUsed)return next;
            lastChanceUsed=true;protectedUntilAcknowledged=true;criticalShown=true;pending=1;
            return 1;
        }
        void update(float health,int day,int hour,bool nightRisk)
        {
            if(health>=45){criticalShown=false;lastChanceUsed=false;}
            if(health>0&&health<=25&&!criticalShown){criticalShown=true;pending=1;}
            if(nightRisk&&hour>=10&&duskDay!=day&&pending==0){duskDay=day;pending=2;}
        }
    };
}
