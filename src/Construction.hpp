#pragma once
#include "extern.hpp"
#include <algorithm>
#include <string>
namespace Construction
{
    inline bool Unfinished(const SCAPE& tile){return tile.Objekt>=FELD&&tile.Objekt<=FEUERSTELLE&&tile.Phase>=Bmp[tile.Objekt].Anzahl;}
    inline const wchar_t* Name(int object)
    {
        switch(object){case FELD:return L"农田";case ZELT:return L"帐篷";case BOOT:return L"独木舟";case ROHR:return L"水管";case SOS:return L"求救标记";case HAUS1:return L"树屋梯子";case HAUS2:return L"树屋平台";case HAUS3:return L"树屋";case FEUERSTELLE:return L"篝火堆";default:return L"工程";}
    }
    inline const wchar_t* Material(int item)
    {switch(item){case ROHAST:return L"树枝";case ROHSTAMM:return L"原木";case ROHSTEIN:return L"石头";case ROHBLATT:return L"棕叶";case ROHLIANE:return L"藤蔓";default:return L"材料";}}
    inline std::wstring Missing(const SCAPE& tile)
    {
        std::wstring text;
        for(int item=ROHAST;item<=ROHSCHLEUDER;++item)
        {
            const int amount=std::max(0,int(tile.Rohstoff[item])-Guy.Inventar[item]);if(!amount)continue;
            if(!text.empty())text+=L"、";text+=Material(item)+std::to_wstring(amount);
        }
        return text;
    }
    inline std::wstring Rows(const SCAPE& tile)
    {
        std::wstring text;
        for(int item=ROHAST;item<=ROHSCHLEUDER;++item)
        {
            const int remaining=tile.Rohstoff[item];if(remaining<=0)continue;
            if(!text.empty())text+=L"\n";
            text+=std::wstring(Material(item))+L"：持有"+std::to_wstring(Guy.Inventar[item])+L" / 还需"+std::to_wstring(remaining);
            const int missing=std::max(0,remaining-Guy.Inventar[item]);text+=missing?L"，缺"+std::to_wstring(missing):L"，已齐";
        }
        return text.empty()?L"剩余材料已经投入，可继续施工。":text;
    }
    inline std::wstring Message(const SCAPE& tile)
    {
        const auto missing=Missing(tile);return missing.empty()?L"材料已齐，走近后可继续施工。":L"继续"+std::wstring(Name(tile.Objekt))+L"还缺："+missing+L"。之前的施工进度保留。";
    }
}
