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
    return failures?1:0;
}
