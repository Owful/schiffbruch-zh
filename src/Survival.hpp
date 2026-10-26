#pragma once
#include <string>
namespace Survival
{
    void Reset();
    void Update();
    float ProtectHealth(float previous,float next);
    int TakeAlert();
    void Acknowledge();
    std::wstring Advice();
    std::wstring DeathReason();
}
