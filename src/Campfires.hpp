#pragma once
#include "extern.hpp"
#include "ExpeditionRules.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <istream>
#include <ostream>
#include <set>
#include <string>
#include <vector>

namespace Campfires
{
    constexpr float CapacitySeconds=800.f;
    inline std::array<std::uint32_t,MAXXKACH*MAXYKACH> ids{};
    inline std::uint32_t nextId=1;
    inline void Reset(){ids.fill(0);nextId=1;}
    inline unsigned index(int x,int y){return unsigned(y)*MAXXKACH+unsigned(x);}
    inline bool valid(int x,int y){return x>=0&&y>=0&&x<MAXXKACH&&y<MAXYKACH;}
    inline void Register(int x,int y){if(valid(x,y)&&Scape[x][y].Objekt==FEUER)ids[index(x,y)]=nextId++;}
    inline void Sync()
    {
        for(int y=0;y<MAXYKACH;++y)for(int x=0;x<MAXXKACH;++x)
        {auto& id=ids[index(x,y)];if(Scape[x][y].Objekt!=FEUER)id=0;else if(!id)id=nextId++;}
    }
    inline float Wind()
    {
        const auto weather=ExpeditionRules::weather(Tag);
        const float base=weather==ExpeditionRules::Weather::Storm?.8f:weather==ExpeditionRules::Weather::Rain?.4f:.15f;
        return base+float((std::max(1,int(Tag))*7+int(Stunden)*3)%5)*.035f;
    }
    inline float Rate()
    {
        const auto weather=ExpeditionRules::weather(Tag);
        return 1.f+.5f*Wind()+(weather==ExpeditionRules::Weather::Storm?.7f:weather==ExpeditionRules::Weather::Rain?.35f:0.f);
    }
    inline bool Affected(){return Wind()>=.7f||ExpeditionRules::weather(Tag)!=ExpeditionRules::Weather::Clear;}
    inline std::wstring Effect()
    {
        return ExpeditionRules::weather(Tag)==ExpeditionRules::Weather::Storm?L"强风 + 暴雨 · 加速燃烧":ExpeditionRules::weather(Tag)==ExpeditionRules::Weather::Rain?L"风 + 阵雨 · 加速燃烧":L"微风 · 正常燃烧";
    }
    inline int Stage(float spent){return std::clamp(int(std::ceil((1.f-std::clamp(spent,0.f,1.f))*5.f)),1,5);}
    struct Bounds{int x,y,w,h;};
    inline Bounds Sprite(const SCAPE& t)
    {
        const int stage=Stage(t.Timer),w=std::max(3,int(Bmp[FEUER].Breite*(.50f+.10f*stage))),h=std::max(4,int(Bmp[FEUER].Hoehe*(.25f+.15f*stage)));
        return {t.xScreen+t.ObPos.x+(Bmp[FEUER].Breite-w)/2,t.yScreen+t.ObPos.y+Bmp[FEUER].Hoehe-h,w,h};
    }
    inline std::wstring Name(unsigned id)
    {
        static constexpr const wchar_t* names[]={L"小暖团",L"橘子糖",L"星星豆",L"栗子球",L"小太阳",L"暖呼呼",L"萤火糖",L"小火苗",L"糯米团",L"红豆包",L"晚安灯",L"棉花糖"};
        return std::wstring(names[((id? id:1)-1)*7%std::size(names)])+L" · "+std::to_wstring(id)+L"号";
    }
    struct Info{int x,y,percent,seconds,stage;unsigned id;std::wstring name;};
    inline std::vector<Info> All()
    {
        Sync();std::vector<Info> out;
        for(int y=0;y<MAXYKACH;++y)for(int x=0;x<MAXXKACH;++x)
        {const auto& t=Scape[x][y];if(t.Objekt!=FEUER||!t.Entdeckt)continue;const auto id=ids[index(x,y)];out.push_back({x,y,int(std::ceil(std::clamp(1.f-t.Timer,0.f,1.f)*100)),int(std::ceil(std::max(0.f,1.f-t.Timer)*CapacitySeconds/Rate())),Stage(t.Timer),id,Name(id)});}
        std::sort(out.begin(),out.end(),[](const auto& a,const auto& b){return a.id<b.id;});return out;
    }
    inline std::wstring Countdown(int seconds){return std::to_wstring(seconds/60)+L":"+(seconds%60<10?L"0":L"")+std::to_wstring(seconds%60);}
    inline void Advance(std::int64_t micros,bool running)
    {
        if(!running||micros<=0)return;Sync();const float loss=float(std::min<std::int64_t>(micros,250000))/1000000.f*Rate()/CapacitySeconds;
        for(int y=0;y<MAXYKACH;++y)for(int x=0;x<MAXXKACH;++x)
        {
            auto& t=Scape[x][y];if(t.Objekt!=FEUER)continue;t.Timer=std::min(1.f,t.Timer+loss);
            if(t.Timer<1.f)continue;
            t.Objekt=FEUERSTELLE;t.Phase=0;t.Timer=0;t.ObPos={short(Bmp[FEUERSTELLE].rcDes.left),short(Bmp[FEUERSTELLE].rcDes.top)};
            ids[index(x,y)]=0;Chance-=2+2*t.Hoehe;
        }
    }
    inline void Save(std::ostream& out)
    {
        Sync();out.write("FIR1",4);
        const auto write=[&](std::uint32_t n){const std::uint8_t b[]={std::uint8_t(n),std::uint8_t(n>>8),std::uint8_t(n>>16),std::uint8_t(n>>24)};out.write(reinterpret_cast<const char*>(b),4);};
        write(nextId);unsigned count=0;for(auto id:ids)if(id)++count;write(count);
        for(unsigned i=0;i<ids.size();++i)if(ids[i]){write(i);write(ids[i]);}
    }
    inline bool Load(std::istream& in)
    {
        std::array<char,4> magic{};if(!in.read(magic.data(),4)||magic!=std::array<char,4>{'F','I','R','1'})return false;
        const auto read=[&](){std::uint8_t b[4]{};in.read(reinterpret_cast<char*>(b),4);return std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);};
        const auto next=read(),count=read();if(!in||next<1||next>1000000||count>ids.size())return false;
        std::array<std::uint32_t,MAXXKACH*MAXYKACH> restored{};std::set<unsigned> seen;
        for(unsigned i=0;i<count;++i){const auto tile=read(),id=read();if(!in||tile>=restored.size()||id<1||id>=next||restored[tile]||!seen.insert(id).second)return false;restored[tile]=id;}
        ids=restored;nextId=next;Sync();return true;
    }
}
