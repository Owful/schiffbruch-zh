#pragma once

#include <iosfwd>
#include <string>

namespace Expedition
{
    struct Info {int water=0,bandages=0,signal=0,explored=0;bool stages[5]{};bool waterskin=false;};
    Info GetInfo();
    void Reset();
    void Save(std::ostream& out);
    void Load(std::istream& in);
    void OnTime(int minutes, int startingHour);
    void OnSearch();
    int CacheAt(int x,int y);
    void CacheHint(int x,int y);
    void UseWaterskin();
    bool NearFreshwater();void FillWaterskin();void DrinkWaterskin();
    void UseBandage();
    std::wstring SupplyProblem(int action);void CompleteSupplyAction(int action);
    bool CraftWaterskin();
    bool CraftBandage();
    void ToggleJournal();
    void ToggleEffects();
    void ToggleHud();
    void DrawAtmosphere();
    void DrawCaches(bool inFrontOfPlayer);
    void DrawHud();
    void Notify(const wchar_t* text);
    void CollectTimber();
    std::wstring CurrentNotice();
}
