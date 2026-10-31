#include "DayJournal.hpp"
#include <iostream>
#include <set>
#include <sstream>
int main()
{
    int failures=0;const auto check=[&](bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';++failures;}};
    DayJournal::Reset();DayJournal::Begin(1);
    check(DayJournal::Summary().size()==1&&DayJournal::events==0,"quiet day has an honest neutral summary");
    DayJournal::Mark(1,DayJournal::Food);DayJournal::Mark(1,DayJournal::Craft);
    DayJournal::RecordLoot(1,DayJournal::Branch,3);DayJournal::RecordLoot(1,DayJournal::Branch,2);DayJournal::RecordLoot(1,DayJournal::Stone,0);
    check(DayJournal::loot[DayJournal::Branch]==5&&DayJournal::LootSummary().size()==1,"daily loot sums actual gains and omits zero quantities");
    check(DayJournal::Summary().size()==2,"only recorded actions appear in summary");
    for(int i=0;i<60;++i)DayJournal::Advance(250000,false);
    check(DayJournal::elapsed==0&&!DayJournal::CanSkip(),"night and skip timer pause when unfocused");
    DayJournal::Skip();check(DayJournal::elapsed==0,"skip is unavailable before the minimum reading time");
    for(int i=0;i<16;++i)check(!DayJournal::Advance(250000,true),"night does not end early");
    check(DayJournal::CanSkip(),"skip becomes available after four seconds");
    std::stringstream saved(std::ios::in|std::ios::out|std::ios::binary);DayJournal::Save(saved);const auto bytes=saved.str();
    DayJournal::Reset();saved.seekg(0);check(DayJournal::Load(saved,1)&&DayJournal::events==(DayJournal::Food|DayJournal::Craft)&&DayJournal::loot[DayJournal::Branch]==5,"daily facts and counts survive save and load");
    for(std::size_t n=0;n<bytes.size();++n){std::istringstream partial(bytes.substr(0,n));check(!DayJournal::Load(partial,1),"truncated journal footer is ignored");}
    std::istringstream oldSave("");check(!DayJournal::Load(oldSave,1),"old saves need no journal footer");
    auto oldFooter=bytes.substr(0,12);oldFooter[3]='1';std::istringstream prior(oldFooter);check(DayJournal::Load(prior,1)&&DayJournal::loot[DayJournal::Branch]==0,"previous journal footer loads without invented quantities");
    DayJournal::Begin(1);for(int i=0;i<143;++i)check(!DayJournal::Advance(250000,true),"night lasts at least thirty-six seconds");
    check(DayJournal::Advance(250000,true)&&!DayJournal::Active(),"night ends automatically at thirty-six seconds");
    DayJournal::NewDay(2);check(DayJournal::events==0&&DayJournal::LootSummary().empty(),"next morning clears yesterday's facts and quantities");
    std::set<std::wstring> messages;for(int day=1;day<=24;++day)messages.insert(DayJournal::Encouragement(day));
    check(messages.size()==24,"the first 24 nights have distinct encouragement");
    const auto text=[](){std::wstring out;for(const auto& line:DayJournal::Summary())out+=line;return out;};
    DayJournal::Reset();DayJournal::Mark(8,DayJournal::Timber);DayJournal::RecordLoot(8,DayJournal::Log,7);
    DayJournal::RecordMade(8,DayJournal::MadeAxe);DayJournal::RecordMade(8,DayJournal::MadeBandage);
    DayJournal::Project(8,1,false);
    check(text().find(L"石斧")!=std::wstring::npos&&text().find(L"帐篷")!=std::wstring::npos&&text().find(L"7根")!=std::wstring::npos,"narration uses actual crafted items, project and timber quantity");
    check(text().find(L"你")==std::wstring::npos&&text().find(L"完工了。")==std::wstring::npos,"unfinished project is not narrated as completed or in second person");
    DayJournal::Project(8,1,true);std::stringstream rich(std::ios::in|std::ios::out|std::ios::binary);DayJournal::Save(rich);const auto richText=text();
    DayJournal::Reset();rich.seekg(0);check(DayJournal::Load(rich,8)&&DayJournal::made[DayJournal::MadeAxe]==1&&DayJournal::finished==2&&text()==richText,"DAY3 facts and exact narration survive reload");
    std::string old2=rich.str().substr(0,12+2*DayJournal::LootCount);old2[3]='2';std::istringstream day2(old2);
    check(DayJournal::Load(day2,8)&&DayJournal::made[DayJournal::MadeAxe]==0&&DayJournal::worked==0,"DAY2 journal loads without inventing detailed crafts or completed projects");
    DayJournal::NewDay(9);check(text().find(L"石斧")==std::wstring::npos&&text().find(L"帐篷")==std::wstring::npos,"yesterday's crafts and projects do not leak into a quiet day");
    std::set<std::wstring> versions;
    for(int d=1;d<=14;++d){DayJournal::NewDay(d);DayJournal::Mark(d,DayJournal::Explore);DayJournal::Mark(d,DayJournal::Food);DayJournal::Mark(d,DayJournal::Water);DayJournal::RecordMade(d,DayJournal::MadeAxe);versions.insert(text());}
    check(versions.size()>=12,"repeating the same daily activities still varies the narrator's voice");
    DayJournal::SetContext(20,90,90,1,0);check(DayJournal::Encouragement(14).find(L"补水")!=std::wstring::npos,"low water changes tomorrow's inner-monologue plan");
    DayJournal::SetContext(90,90,20,0,2);check(DayJournal::Encouragement(14).find(L"伤口")!=std::wstring::npos,"low health takes priority over weather jokes");
    DayJournal::NewDay(20);for(unsigned i=0;i<14;++i)DayJournal::Mark(20,static_cast<DayJournal::Event>(1u<<i));
    for(unsigned i=0;i<DayJournal::MadeCount;++i)DayJournal::RecordMade(20,static_cast<DayJournal::Made>(i));
    for(unsigned i=0;i<9;++i)DayJournal::Project(20,i,i%2==0);
    DayJournal::RecordLoot(20,DayJournal::Telescope,1);DayJournal::RecordLoot(20,DayJournal::Map,1);
    check(DayJournal::Summary().size()<=4&&text().size()<400,"busy days remain concise enough for the journal column");
    for(int d=1;d<=24;++d)for(int shelter=0;shelter<3;++shelter)
    {DayJournal::SetContext(20,20,20,shelter,2);check(DayJournal::Encouragement(d).size()<=110,"closing monologue fits two lines at the journal width");}
    std::stringstream mixed(std::ios::in|std::ios::out|std::ios::binary);DayJournal::Save(mixed);mixed.write("FIR1",4);mixed.seekg(0);
    check(DayJournal::Load(mixed,20),"rich journal reloads");char next[4]{};mixed.read(next,4);check(std::string(next,4)=="FIR1","DAY3 does not consume the following campfire footer");
    return failures?1:0;
}
