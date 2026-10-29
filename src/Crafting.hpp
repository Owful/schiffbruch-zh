#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <iosfwd>
namespace Crafting
{
    constexpr int Waterskin=10000,Bandage=10001;
    constexpr int FillWater=10002,DrinkWater=10003,ApplyBandage=10004;
    constexpr int OpenCache=10005,FeedFire=10006;
    struct Ingredient{int item,count;};
    struct Recipe{int product,category;const wchar_t* name;const wchar_t* purpose;std::vector<Ingredient> costs;int prerequisite=-1,minutes=0;};
    const std::vector<Recipe>& Recipes();const Recipe* Find(int product);
    std::wstring Name(int product);std::wstring Materials(const Recipe& recipe);std::wstring Problem(int product);
    bool Owned(int product);bool CanMake(int product);bool Make(int product);
    struct JobInfo{int product=-1;float fraction=0;int seconds=0;};
    JobInfo Job();bool Busy();int DurationSeconds(int product);
    bool IsSupplyAction(int product);bool UsingSupply();bool StartSupplyAction(int product);
    std::uint32_t DurationMicros(int product);
    void Update(std::int64_t micros,bool running);void Cancel(bool notify=true);void Reset();
    void Save(std::ostream& out);bool Load(std::istream& in);
    void SyncUnlocks();int MissingForAction(int button);std::wstring Requirement(int button);
}
