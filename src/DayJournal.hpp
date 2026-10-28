#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

// Optional save footer; facts are recorded when an action actually has an effect.
namespace DayJournal
{
    enum Event : std::uint32_t { Explore=1, Search=2, Food=4, Water=8, Timber=16, Craft=32, Build=64, Cache=128, Care=256 };
    enum Loot : unsigned { Branch,Log,Stone,Leaf,Vine,Telescope,Map,Shovel,Bandage,Firestarter,LootCount };
    struct LootEntry {std::wstring name;unsigned count;};
    inline int day=0;
    inline std::uint32_t events=0;
    inline std::int64_t elapsed=0;
    inline bool active=false;
    inline std::array<std::uint16_t,LootCount> loot{};
    constexpr std::int64_t Duration=36000000, SkipAfter=4000000;
    inline void Reset(){day=0;events=0;loot.fill(0);elapsed=0;active=false;}
    inline void NewDay(int today){day=today;events=0;loot.fill(0);elapsed=0;active=false;}
    inline void Mark(int today,Event event){if(today<1)return;if(day!=today)NewDay(today);events|=event;}
    inline void RecordLoot(int today,Loot kind,int amount){if(today<1||kind>=LootCount||amount<=0)return;if(day!=today)NewDay(today);loot[kind]=std::uint16_t(std::min(65535,int(loot[kind])+amount));}
    inline void Begin(int today){if(day!=today)NewDay(today);elapsed=0;active=true;}
    inline bool Active(){return active;}
    inline bool CanSkip(){return active&&elapsed>=SkipAfter;}
    inline void Skip(){if(CanSkip())elapsed=Duration;}
    inline bool Advance(std::int64_t micros,bool running)
    {
        if(!active||!running)return false;
        elapsed+=std::clamp<std::int64_t>(micros,0,250000);
        if(elapsed<Duration)return false;
        active=false;return true;
    }
    inline std::vector<std::wstring> Summary()
    {
        std::vector<std::wstring> out;
        if(events&Explore)out.emplace_back(L"你向未知的地方走了走，岛屿又多了一些熟悉的模样。");
        if(events&Cache)out.emplace_back(L"你发现了漂流的补给，旅途中有了一份意外的收获。");
        else if(events&Search)out.emplace_back(L"你仔细搜索了周围，为接下来的日子寻找可能。");
        if(events&Timber)out.emplace_back(L"你收集了木材，为以后的生活积攒力量。");
        if(events&Craft)out.emplace_back(L"你亲手制作了物品，让岛上的生活多了一分可能。");
        if(events&Build)out.emplace_back(L"你为营地的建设付出了时间，一点一点安顿自己。");
        if((events&(Food|Water))==(Food|Water))out.emplace_back(L"你找到了食物和水，也认真照顾了自己。");
        else if(events&Food)out.emplace_back(L"你补充了食物，给自己留住继续前行的力气。");
        else if(events&Water)out.emplace_back(L"你喝了水，记得照顾在这座岛上努力生活的自己。");
        if(events&Care)out.emplace_back(L"你处理了伤口，给自己留下一点恢复的时间。");
        if(out.empty())out.emplace_back(L"今天，你在这座岛上度过了又一天。");
        return out;
    }
    inline std::vector<LootEntry> LootSummary()
    {
        static constexpr const wchar_t* names[]={L"树枝",L"原木",L"石头",L"棕叶",L"藤蔓",L"望远镜",L"藏宝图",L"木铲",L"绷带",L"生火工具"};
        std::vector<LootEntry> out;for(unsigned i=0;i<LootCount;++i)if(loot[i])out.push_back({names[i],loot[i]});return out;
    }
    inline std::wstring Encouragement(int today)
    {
        static constexpr const wchar_t* lines[]={
            L"你不必每天都走很远。愿意再走一步，就已经很勇敢。",
            L"今天的你已经尽力了。剩下的事，等天亮再说。",
            L"海很大，但你也在一点点找到自己的方向。",
            L"慢慢来。每一个平安度过的夜晚，都值得被珍惜。",
            L"那些看似微小的努力，正在为明天铺路。",
            L"就算今天有些不顺利，你也依然值得好好休息。",
            L"你已经走到了这里，请相信自己还有继续前行的力量。",
            L"先把疲惫交给夜色。明天，我们再一起想办法。",
            L"没有人规定你必须很快。照自己的节奏，已经很好。",
            L"能在陌生的地方照顾自己，是一件了不起的事。",
            L"今天留下的每一点收获，都是你亲手点亮的希望。",
            L"即使暂时看不见彼岸，你的努力也从未白费。",
            L"允许自己停下来。休息，也是为了走向想去的地方。",
            L"风浪会过去，而你正在学会和它们一起生活。",
            L"你不需要一个人解决所有事情。今晚，先让自己安心。",
            L"也许改变很小，但你已经比昨天更熟悉这座岛了。",
            L"今天能做到的，你都认真做了。现在，请温柔地休息。",
            L"有些答案不用急着找到，明天的阳光会陪你继续寻找。",
            L"无论今天走得顺不顺，你都值得一句：辛苦了。",
            L"这座岛还很陌生，但你已经为自己创造了一点点安稳。",
            L"每一天都有自己的重量。你把今天好好走完了。",
            L"不必责怪自己的迟疑，在不确定中前行本就需要勇气。",
            L"把未完成的事留给明天，把今晚留给自己。",
            L"愿海浪带走疲惫，也愿你醒来时仍有期待。"
        };
        const auto index=(static_cast<unsigned>(std::max(1,today)-1)*7u)%std::size(lines);
        return lines[index];
    }
    inline void Save(std::ostream& out)
    {
        const std::uint32_t d=static_cast<std::uint32_t>(day);
        const std::array<std::uint8_t,8> data={std::uint8_t(d),std::uint8_t(d>>8),std::uint8_t(d>>16),std::uint8_t(d>>24),std::uint8_t(events),std::uint8_t(events>>8),std::uint8_t(events>>16),std::uint8_t(events>>24)};
        out.write("DAY2",4);out.write(reinterpret_cast<const char*>(data.data()),data.size());
        for(auto count:loot){const std::uint8_t bytes[]={std::uint8_t(count),std::uint8_t(count>>8)};out.write(reinterpret_cast<const char*>(bytes),2);}
    }
    inline bool Load(std::istream& in,int today)
    {
        std::array<char,4> magic{};std::array<std::uint8_t,8> b{};
        if(!in.read(magic.data(),4)||(magic!=std::array<char,4>{'D','A','Y','1'}&&magic!=std::array<char,4>{'D','A','Y','2'})||!in.read(reinterpret_cast<char*>(b.data()),b.size()))return false;
        const std::uint32_t d=std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);
        const std::uint32_t e=std::uint32_t(b[4])|(std::uint32_t(b[5])<<8)|(std::uint32_t(b[6])<<16)|(std::uint32_t(b[7])<<24);
        if(d!=std::uint32_t(today)||(e&~511u))return false;
        std::array<std::uint16_t,LootCount> counts{};
        if(magic[3]=='2')for(auto& count:counts){std::uint8_t v[2]{};if(!in.read(reinterpret_cast<char*>(v),2))return false;count=std::uint16_t(v[0]|(unsigned(v[1])<<8));}
        day=today;events=e;loot=counts;elapsed=0;active=false;return true;
    }
}
