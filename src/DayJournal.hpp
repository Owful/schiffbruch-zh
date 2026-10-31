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
    enum Event : std::uint32_t { Explore=1, Search=2, Food=4, Water=8, Timber=16, Craft=32, Build=64, Cache=128, Care=256, Fill=512, Fish=1024, Hunt=2048, Light=4096, Feed=8192 };
    enum Loot : unsigned { Branch,Log,Stone,Leaf,Vine,Telescope,Map,Shovel,Bandage,Firestarter,LootCount };
    enum Made : unsigned { MadeAxe,MadeHoe,MadeRod,MadeSling,MadeHammer,MadeSpade,MadeStarter,MadeWaterskin,MadeBandage,MadeCount };
    inline std::array<std::uint16_t,MadeCount> made{};
    inline std::uint16_t worked=0,finished=0;
    struct Context {int water=-1,food=-1,health=-1,shelter=-1,weather=0;};
    inline Context context;
    struct LootEntry {std::wstring name;unsigned count;};
    inline int day=0;
    inline std::uint32_t events=0;
    inline std::int64_t elapsed=0;
    inline bool active=false;
    inline std::array<std::uint16_t,LootCount> loot{};
    constexpr std::int64_t Duration=36000000, SkipAfter=4000000;
    inline void Reset(){day=0;events=0;loot.fill(0);made.fill(0);worked=finished=0;context={};elapsed=0;active=false;}
    inline void NewDay(int today){day=today;events=0;loot.fill(0);made.fill(0);worked=finished=0;context={};elapsed=0;active=false;}
    inline void Mark(int today,Event event){if(today<1)return;if(day!=today)NewDay(today);events|=event;}
    inline void RecordLoot(int today,Loot kind,int amount){if(today<1||kind>=LootCount||amount<=0)return;if(day!=today)NewDay(today);loot[kind]=std::uint16_t(std::min(65535,int(loot[kind])+amount));}
    inline void RecordMade(int today,Made kind){if(today<1||kind>=MadeCount)return;Mark(today,Craft);made[kind]=std::uint16_t(std::min(65535,int(made[kind])+1));}
    inline void Project(int today,unsigned kind,bool complete){if(today<1||kind>=9)return;Mark(today,Build);worked|=std::uint16_t(1u<<kind);if(complete)finished|=std::uint16_t(1u<<kind);}
    inline void SetContext(int water,int food,int health,int shelter,int weather){context={water,food,health,shelter,weather};}
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
    template<std::size_t N> inline std::wstring Voice(const wchar_t* const (&lines)[N],unsigned salt)
    {unsigned h=events*13u+worked*7u+finished*3u;for(unsigned i=0;i<LootCount;++i)h+=loot[i]*(i+1);const auto d=unsigned(std::max(1,day)-1);const auto shift=N>2?1+salt%(N-2):1;return lines[(d+(d/N)*shift+salt+h)%N];}
    inline std::wstring Names(unsigned mask,const wchar_t* const* names,unsigned count)
    {std::wstring out;unsigned added=0;for(unsigned i=0;i<count;++i)if(mask&(1u<<i)){if(added==2){out+=L"等";break;}if(added)out+=L"、";out+=names[i];++added;}return out;}
    inline std::vector<std::wstring> Summary()
    {
        static const wchar_t* explore[]={L"我今天往没走过的地方探了探。岛还是那座岛，熟路倒多了几条。",L"今天又认了几条新路。腿说下次别带它，眼睛说再看一眼。",L"我把陌生的地方走成了认识的地方。脚底板对此保留意见。",L"今天在岛上扩充了我的活动范围。房产没有，地理知识倒涨了。",L"我又去未知处转了转。还没找到出口，先攒了点方向感。",L"新路走过了，胆子也跟着长了一点。蚊子最好别跟着长。",L"我今天又探了一片地方。地图负责变清楚，我负责走到腿酸。"};
        static const wchar_t* search[]={L"我翻了翻四周。岛上的物资没有标签，只能靠眼神和一点不嫌麻烦。",L"今天搜得挺仔细。以前弯腰是掉了东西，现在弯腰是盼着有东西。",L"我又在周围找了找。海岛没有便利店，只好由我担任采购部。",L"我今天认真翻找了周围。石头不会自己进包，这点它们坚持得很好。",L"今天的搜索工作完成。没有收银员，也没有人替我拎袋子。"};
        static const wchar_t* cache[]={L"漂流箱今天开了张。海上快递没写收件人，我决定先不计较。",L"我打开了漂来的箱子。寄件人不详，收件人目前只有我比较积极。",L"今天从漂流箱里拿到了补给。感谢大海，这次没给我寄账单。",L"漂流箱总算把东西交出来了。包装结实得像里面藏着房契。",L"今天的意外收获来自一个箱子。没有客服，好在也不用填满意度。",L"我打开了漂流补给箱。暂时还没读到信，先把能用的东西收好。"};
        static const wchar_t* wood[]={L"我今天跟树较了较劲。树没说话，我的胳膊倒很有意见。",L"砍树这事完成了。木头有了，双手的震感也附赠了。",L"今天动了斧头。以前觉得树荫不错，现在连树干都看着实用。",L"我砍下了木材。斧头负责咚咚响，我负责别砸到自己的脚。",L"今天和木头打过交道。它们能搭屋，我先负责把它们搬回来。"};
        static const wchar_t* craft[]={L"今天做成了{items}。成品未必好看，能用就算我手艺过关。",L"我亲手做出了{items}。荒岛生活没有保修，只好对自己客气一点。",L"{items}今天做好了。工具多一点，我的办法也就多一点。",L"今天多了{items}。没有流水线，只有一个边做边琢磨的我。",L"我把材料变成了{items}。以前只会说“这个我也能做”，今天算交了作业。",L"{items}完工。我决定暂时不研究外观，先研究怎么用得顺手。",L"今天做出了{items}。手艺是练的，材料是真的，成就感也是。"};
        static const wchar_t* craft_unknown[]={L"我今天做了些东西。没有说明书的日子，也得想办法把生活凑齐。",L"今天动手制作了物品。做出来的那一刻，比空想靠谱多了。",L"我把一些材料做成了能用的东西。手忙了，心里反而踏实一点。",L"今天有东西从我手里做出来。岛上还没开工厂，一个人也能慢慢来。",L"制作这项工作今天没白忙。手艺未必精，至少没有只停在嘴上。"};
        static const wchar_t* build[]={L"今天推进了{items}的施工。岛上没有监工，我已经学会自己催自己。",L"我又折腾了一阵{items}。工程有了进展，腰也有了感想。",L"今天给{items}添了些进度。岛上工期可以商量，手上的活总得一点点做。",L"{items}今天又修了一阵。没有施工队，只有施工的我。",L"我今天在忙{items}。每多做一点，这里就少一点“临时凑合”的样子。"};
        static const wchar_t* finish[]={L"{items}今天有工程完工了。我宣布给自己一个不用鼓掌的剪彩。",L"今天把{items}做到了完工。没有验收队，我先认真高兴一下。",L"{items}有一处完工了。辛苦终于有了形状，值得我多看两眼。",L"今天有{items}完工。我这支单人施工队，效率居然还行。",L"我完成了{items}的工程。先不谈下一项，允许我叉腰满意一会儿。",L"{items}有工程收尾了。今天的汗总算有了一个像样的去处。",L"今天完工的{items}，是我给自己攒出来的一点底气。"};
        static const wchar_t* food[]={L"今天吃过东西。胃的会议终于散了，耳根清静不少。",L"我给肚子补了点货。味道先不排名，能让我接着走就很好。",L"今天有好好补充食物。所谓荒岛餐厅，主厨、食客、洗碗的都是我。",L"我今天照顾过自己的胃。它很实际，不接受“再坚持一下”当主食。",L"食物补上了。至少今天，我和肚子站在同一阵线。"};
        static const wchar_t* water[]={L"今天喝了水。嗓子终于同意继续配合我的生存计划。",L"我给自己补过水。海这么大，能入口的那几口还是得认真找。",L"今天有补水。脑子里的海浪声小了一点，也可能是我终于没那么渴了。",L"我今天记得喝水。嘴上逞强没用，身体只认这几口实在的。",L"水喝上了。没有杯垫也没关系，能润嗓子就是好服务。",L"今天补了水，先给自己记一分。这分不能吃，但水能喝。",L"我今天照顾了口渴这回事。岛上没有送水工，还好我没把自己忘了。"};
        static const wchar_t* care[]={L"今天包扎了伤口。手法未必专业，对自己负责这件事倒得认真。",L"我今天给伤口上了绷带。岛上医生暂时只有我，请多包涵。",L"今天处理了伤口。逞强又不能当药使，这个道理我算记住了。",L"我给自己做了包扎。先把人照顾好，明天的计划才有执行人。",L"今天把伤口照料了一下。身体没有备件，还是省着折腾。"};
        static const wchar_t* quiet[]={L"今天没留下多少忙碌的记录。我没有偷懒到消失，只是把一天慢慢过完了。",L"今天没有记下什么大动作。海风照常营业，我也还在。",L"今天的记录很安静。不是每天都要轰轰烈烈，先把自己好好留到明天。",L"今天没记下什么特别的事。我决定不给自己硬编一份工作报告。",L"今天的手记有点短。人还在，计划可以明天再写。",L"今天没留下多少行动记录。岛没跑，我也不必跟它比耐心。",L"今天没有值得列一大串的事。空白先留着，明天说不定正好用得上。"};
        static const wchar_t* both[]={L"今天吃过东西，也补过水。胃和嗓子都安排上了，这个后勤部长算没白当。",L"我今天把吃喝都照顾到了。食谱未必讲究，肚子和嗓子倒没再联名抗议。",L"今天有吃有喝。海岛套餐没有菜单，全靠我自己把日子凑起来。",L"我今天没忘记吃，也没忘记喝。身体这位老搭档，得给它点实在的待遇。",L"吃喝今天都补上了。我宣布后勤工作暂时合格，奖品是今晚能歇口气。",L"今天补了食物和水。计划可以很宏大，人还是得靠这两样撑着。",L"我今天给胃和嗓子都交了作业。两位满意不满意，明天再观察。"};
        static const wchar_t* fill[]={L"我还装满过水壶。出门带水，总比一路惦记水好。",L"今天也给水壶补过货。移动饮水点，算是有着落了。",L"我把水壶装满了。别的先不说，路上的口渴得有个着落。",L"今天给水壶存了水。手边多点准备，脚下就敢多走两步。",L"水壶今天装满过。带着这点踏实，比光带着决心强。"};
        static const wchar_t* fish[]={L"我今天去了钓鱼。鱼竿负责伸出去，耐心还得我自己掏。",L"今天在水边钓了鱼。发呆总算有了一个正经的项目名称。",L"我今天练了练钓鱼。没有观众，也不用向谁解释我蹲了多久。",L"今天安排了钓鱼这项活。岛上没有外卖，水边也得试着营业。",L"我今天去水边钓过鱼。水面看着安静，我心里倒挺忙。"};
        static const wchar_t* hunt[]={L"今天拿投石索捕了鸟。晚饭这件事，得靠我自己想办法。",L"我今天用投石索捕了鸟。手艺又多练了一项，手腕也多记了一笔。",L"今天去捕了鸟。荒岛生活不讲摆盘，讲究一个自己动手。",L"我今天拿投石索打过猎。岛上没有现成饭，只好多学一门本事。",L"今天试了试捕鸟这项生计。工具在手里，办法得在脑子里。"};
        static const wchar_t* light[]={L"我今天点起了篝火。岛上总算有一处，是我亲手点亮的。",L"今天生起了火。还不能叫灯火通明，好歹不再全靠月亮。",L"我今天把篝火点起来了。一小团亮光，看着还挺争气。",L"今天有火亮起来了。原来安稳这件事，也能一点点点出来。",L"我今天生起了篝火。夜色再大，我也有一点自己的光。"};
        static const wchar_t* feed[]={L"也给篝火添过柴。它吃木头，我吃别的，分工很明确。",L"今天还喂过篝火。这位室友不说话，木头倒是一口不落。",L"我给篝火续过柴。照顾它的胃口，也算照顾今晚的亮光。",L"篝火今天添过木材。它肯继续亮着，我搬这趟就不算白忙。",L"我今天给火添过柴。营地这点暖意，得有人记着照料。"};
        std::vector<std::wstring> out;
        const auto voice=[&](const auto& lines,unsigned salt){return Voice(lines,salt);};
        std::wstring travel;
        if(events&Explore)travel=voice(explore,1);
        if(events&Cache)travel+=voice(cache,3);else if(events&Search)travel+=voice(search,5);
        if(!travel.empty())out.push_back(travel);
        std::wstring work;
        if(events&Timber){work=voice(wood,7);if(loot[Log])work+=L"今天入包的原木一共"+std::to_wstring(loot[Log])+L"根。";}
        static constexpr const wchar_t* tools[]={L"石斧",L"石锄",L"钓竿",L"投石索",L"木石锤",L"木铲",L"生火工具",L"水壶",L"绷带"};
        static constexpr const wchar_t* projects[]={L"农田",L"帐篷",L"独木舟",L"水管",L"求救标记",L"树屋梯子",L"树屋平台",L"树屋",L"篝火堆"};
        const auto named=[&](std::wstring text,const std::wstring& names){const auto at=text.find(L"{items}");if(at!=std::wstring::npos)text.replace(at,7,names);return text;};
        unsigned mask=0;for(unsigned i=0;i<MadeCount;++i)if(made[i])mask|=1u<<i;
        if(events&Craft)work+=mask?named(voice(craft,11),Names(mask,tools,MadeCount)):voice(craft_unknown,11);
        if(finished)work+=named(voice(finish,13),Names(finished,projects,9));
        const unsigned unfinished=worked&~finished;
        if(unfinished)work+=named(voice(build,17),Names(unfinished,projects,9));
        else if((events&Build)&&!finished)work+=L"今天营地的施工有了进展。没干完的留给明天，我先别把自己也当建材用。";
        if(work.size()>140)
        {
            work=L"今天的活不少。";
            if(events&Timber)work+=L"我砍了木材；";
            if(mask)work+=L"做成了"+Names(mask,tools,MadeCount)+L"；";
            if(finished)work+=Names(finished,projects,9)+L"有工程完工；";
            if(unfinished)work+=Names(unfinished,projects,9)+L"还在施工；";
            work+=L"这支单人施工队，先给自己记一功。";
        }
        if(!work.empty())out.push_back(work);
        std::wstring living;
        if((events&(Food|Water))==(Food|Water))living=voice(both,37);
        else if(events&Food)living=voice(food,19);else if(events&Water)living=voice(water,23);
        const auto addLiving=[&](std::wstring line){if(living.size()+line.size()<=100)living+=line;};
        if(events&Care)addLiving(voice(care,29));
        if(events&Fill)addLiving(voice(fill,41));
        if(events&Light)addLiving(voice(light,43));else if(events&Feed)addLiving(voice(feed,47));
        if(events&Fish)addLiving(voice(fish,53));if(events&Hunt)addLiving(voice(hunt,59));
        if(!living.empty())out.push_back(living);
        if(loot[Telescope]||loot[Map])
        {std::wstring rare=L"今天还找到了";if(loot[Telescope])rare+=L"望远镜";if(loot[Telescope]&&loot[Map])rare+=L"和";if(loot[Map])rare+=L"藏宝图";rare+=L"。岛上原来真的会藏点让我眼睛一亮的东西。";out.push_back(rare);}
        if(out.empty())out.push_back(voice(quiet,31));
        return out;
    }
    inline std::vector<LootEntry> LootSummary()
    {
        static constexpr const wchar_t* names[]={L"树枝",L"原木",L"石头",L"棕叶",L"藤蔓",L"望远镜",L"藏宝图",L"木铲",L"绷带",L"生火工具"};
        std::vector<LootEntry> out;for(unsigned i=0;i<LootCount;++i)if(loot[i])out.push_back({names[i],loot[i]});return out;
    }
    inline std::wstring Encouragement(int today)
    {
        static const wchar_t* lines[]={L"今天的我还在。海岛生存这份工作，明天继续打卡。",L"先睡一觉。再周密的计划，也得有个睡醒的人来执行。",L"今天到这里就算收工。我不是永动机，肚子已经证明过很多次。",L"明天的麻烦明天认领。今晚这点安静，先算我的。",L"能把今天过下来，算我有点本事。这个结论睡醒也有效。",L"今晚给自己放个小假。时长一夜，福利是终于不用再搬东西。",L"海浪负责值夜班，我负责把精神睡回来。合作愉快。",L"今天先不跟命运讲道理了。它要真想聊，明天排队。",L"能在这里多安稳一点，都是我一点点攒的。今晚值得睡个好觉。",L"今晚的任务只剩睡觉。这个我有经验，争取一次完成。",L"今天的难处已经熬过去一份了。剩下的，我明天再拆。",L"我的办法还没用完。先睡，灵感说不定也需要充电。",L"今天的成绩不必惊天动地。轮到我睡个踏实觉，才是正经事。",L"我还在学怎么和这座岛相处。今天没学会的，明天补课。",L"不管白天折腾成什么样，今晚先给自己留点耐心。",L"明天我还会有新主意。要是没有，就先把水和饭安排明白。",L"我承认今天有点累。但累的是我，继续想办法的也是我。",L"今天到此为止。生活不是一口气搭好的，帐篷也不是。",L"我给自己留了一点明天的可能。听起来不多，其实挺够用。",L"今晚不加班。海岛老板要有意见，也得先找得到我。",L"先把眼睛闭上。等太阳上班，我再考虑跟着开工。",L"今天能做到的已经落在岛上了。明天，我再添一点。",L"我没有把所有事都做好，不过也没打算就此认输。先歇会儿。",L"要是哪天回去了，这些日子够我吹很久。今晚先攒点精神。"};
        static const wchar_t* tent[]={L"今晚能钻进帐篷，地面这位老熟人先少见一晚。",L"今晚睡帐篷。屋子小归小，至少是我安顿自己的地方。",L"有帐篷过夜，心里踏实一截。豪华谈不上，实用我很认可。",L"今晚有帐篷收留我。单人间，海景声效，倒也齐全。",L"帐篷今晚归我住。面积可以让步，能歇下来就很好。"};
        static const wchar_t* house[]={L"今晚住树屋。楼层不高，心里倒踏实些。",L"今晚能进树屋歇着，算我在岛上给自己争取到一点高度。",L"树屋今晚有住客：还是我。住得稳当，比挂牌子重要。",L"今晚睡树屋。白天搭出来的安稳，晚上总算轮到我享受。",L"有树屋过夜，今天这点疲惫也算有地方放了。"};
        static const wchar_t* outside[]={L"今晚还得露宿，明天得把住处安排得稳妥些。",L"今晚睡在外头，不能把这当成长久打算。明天得再想想住处。",L"露宿这一晚先撑过去。明天给住处多留点工夫。",L"今晚还没有合适的住处，明天得把这件事往前排。",L"今晚得在外头歇着。看星星归看星星，明天还是得找个稳当住处。"};
        std::wstring lead;
        if(context.health>=0&&context.health<=35)lead=L"今天有点透支，明天先照顾好伤口和吃喝。";
        else if(context.water>=0&&context.water<50)lead=L"明天先去补水。嗓子不同意我再拿毅力解渴。";
        else if(context.food>=0&&context.food<50)lead=L"明天先找吃的。肚子已经提前报了名。";
        else if(worked&~finished)lead=L"没收尾的工程还在，明天接着来，不必把今天推倒重做。";
        else if(context.weather==2)lead=L"风雨今天挺有存在感。我不跟天气争最后一句。";
        else if(context.weather==1)lead=L"雨天也算过完了。衣服能晾，精神也得晾一晾。";
        if(context.shelter==1)lead+=Voice(tent,71);
        else if(context.shelter==2)lead+=Voice(house,71);
        else if(context.shelter==0)lead+=Voice(outside,71);
        return lead+lines[(unsigned(std::max(1,today)-1)*7u)%std::size(lines)];
    }
    inline void Save(std::ostream& out)
    {
        const std::uint32_t d=static_cast<std::uint32_t>(day);
        const std::array<std::uint8_t,8> data={std::uint8_t(d),std::uint8_t(d>>8),std::uint8_t(d>>16),std::uint8_t(d>>24),std::uint8_t(events),std::uint8_t(events>>8),std::uint8_t(events>>16),std::uint8_t(events>>24)};
        out.write("DAY3",4);out.write(reinterpret_cast<const char*>(data.data()),data.size());
        const auto write=[&](unsigned n){const std::uint8_t bytes[]={std::uint8_t(n),std::uint8_t(n>>8)};out.write(reinterpret_cast<const char*>(bytes),2);};
        for(auto n:loot)write(n);for(auto n:made)write(n);write(worked);write(finished);
    }
    inline bool Load(std::istream& in,int today)
    {
        std::array<char,4> magic{};std::array<std::uint8_t,8> b{};
        if(!in.read(magic.data(),4)||(magic!=std::array<char,4>{'D','A','Y','1'}&&magic!=std::array<char,4>{'D','A','Y','2'}&&magic!=std::array<char,4>{'D','A','Y','3'})||!in.read(reinterpret_cast<char*>(b.data()),b.size()))return false;
        const std::uint32_t d=std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);
        const std::uint32_t e=std::uint32_t(b[4])|(std::uint32_t(b[5])<<8)|(std::uint32_t(b[6])<<16)|(std::uint32_t(b[7])<<24);
        if(d!=std::uint32_t(today)||(e&~(magic[3]=='3'?16383u:511u)))return false;
        std::array<std::uint16_t,LootCount> counts{};
        if(magic[3]!='1')for(auto& count:counts){std::uint8_t v[2]{};if(!in.read(reinterpret_cast<char*>(v),2))return false;count=std::uint16_t(v[0]|(unsigned(v[1])<<8));}
        std::array<std::uint16_t,MadeCount> crafted{};unsigned work=0,done=0;
        if(magic[3]=='3')
        {const auto read=[&](){std::uint8_t v[2]{};in.read(reinterpret_cast<char*>(v),2);return unsigned(v[0])|(unsigned(v[1])<<8);};
         for(auto& n:crafted)n=std::uint16_t(read());work=read();done=read();if(!in||(work&~511u)||(done&~work))return false;}
        day=today;events=e;loot=counts;made=crafted;worked=std::uint16_t(work);finished=std::uint16_t(done);context={};elapsed=0;active=false;return true;
    }
}
