#pragma once
#include <cstdint>

namespace DuskNotice
{
    constexpr std::uint64_t DurationMilliseconds=5000;
    struct State
    {
        std::uint64_t until=0;
        bool atSea=false;
        void show(std::uint64_t now,bool sea){until=now+DurationMilliseconds;atSea=sea;}
        bool visible(std::uint64_t now)const{return now<until;}
        void reset(){until=0;atSea=false;}
    };
}
