#include "Campfires.hpp"
#include <iostream>
#include <sstream>
#include <set>
int main()
{
    int errors=0;const auto check=[&](bool ok,const char* name){if(!ok){std::cerr<<"FAIL: "<<name<<'\n';++errors;}};
    for(auto& column:Scape)for(auto& t:column){t={};t.Objekt=-1;t.Entdeckt=true;}
    Tag=1;Stunden=0;Campfires::Reset();
    Scape[10][10].Objekt=FEUER;Scape[10][10].Timer=.2f;Campfires::Register(10,10);
    Scape[12][10].Objekt=FEUER;Scape[12][10].Timer=.7f;Campfires::Register(12,10);
    auto fires=Campfires::All();check(fires.size()==2&&fires[0].id!=fires[1].id&&fires[0].name!=fires[1].name,"separate fires have distinct persistent identities");
    const float loss=Campfires::Rate()/Campfires::CapacitySeconds;
    for(int i=0;i<10;++i)Campfires::Advance(100000,true);
    check(std::abs(Scape[10][10].Timer-.2f-loss)<.00001f&&std::abs(Scape[12][10].Timer-.7f-loss)<.00001f,"one real second burns each fire independently");
    const float held=Scape[10][10].Timer;Campfires::Advance(10000000,false);check(Scape[10][10].Timer==held,"pause and focus loss do not consume fuel or accumulate time");
    Tag=3;const float rain=Campfires::Rate();check(Campfires::Affected()&&rain>1,"rain changes countdown and exposes weather details");
    Tag=5;check(Campfires::Wind()>.7f&&Campfires::Rate()>rain,"strong wind and storm increase consumption further");
    std::stringstream save(std::ios::in|std::ios::out|std::ios::binary);Campfires::Save(save);const auto bytes=save.str();
    Campfires::Reset();save.seekg(0);check(Campfires::Load(save)&&Campfires::All()[0].name==fires[0].name,"fire identity survives save and load");
    for(std::size_t i=0;i<bytes.size();++i){std::istringstream truncated(bytes.substr(0,i));check(!Campfires::Load(truncated),"truncated footer is rejected without partially assigning ids");}
    const auto otherId=Campfires::All()[1].id;Scape[10][10].Timer=.99999f;Chance=100;
    Campfires::Advance(250000,true);check(Scape[10][10].Objekt==FEUERSTELLE&&Campfires::All().size()==1&&Campfires::All()[0].id==otherId,"one extinguished fire leaves a reusable hearth and does not remove another");
    const auto chance=Chance;Campfires::Advance(250000,true);check(Chance==chance,"extinguishing subtracts rescue chance only once");
    Scape[10][10].Objekt=FEUER;Campfires::Register(10,10);check(Campfires::All().back().id>otherId,"relit fire receives a new distinguishable number");
    Bmp[FEUER].Breite=30;Bmp[FEUER].Hoehe=50;auto& t=Scape[10][10];t.xScreen=400;t.yScreen=300;
    int lastHeight=100;std::set<int> heights;
    for(float spent:{0.f,.21f,.41f,.61f,.81f}){t.Timer=spent;const auto b=Campfires::Sprite(t);check(b.h<lastHeight&&b.y+b.h==350,"five fuel stages shrink from a fixed base");lastHeight=b.h;heights.insert(b.h);}
    check(heights.size()==5,"all five fire stages have different sizes");
    check(Campfires::Countdown(75)==L"1:15","countdown uses real minutes and seconds");
    return errors?1:0;
}
