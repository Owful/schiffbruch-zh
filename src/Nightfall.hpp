#pragma once
#include <cstdint>
#include <string>
namespace Nightfall
{
    void Reset();void Update(std::int64_t micros,bool running);
    bool Pending();bool Walking();std::wstring Line();
}
