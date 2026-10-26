#pragma once
#include <cstdint>
namespace GameClock
{
    bool Running(bool focused);
    void Advance(std::int64_t micros,bool focused);
}
