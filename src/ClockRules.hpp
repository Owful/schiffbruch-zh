#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <istream>
#include <ostream>

namespace ClockRules
{
    // A 06:00–18:00 day takes 4 minutes 48 seconds, regardless of player actions.
    constexpr std::int64_t MinuteMicros=400000;
    inline std::uint32_t remainder=0;
    inline void reset(){remainder=0;}
    inline int advance(std::int64_t micros,bool running)
    {
        if(!running||micros<=0)return 0;
        // Discard long stalls (e.g. dragging a window); never catch up a whole absence.
        const auto total=remainder+std::min<std::int64_t>(micros,250000);
        remainder=std::uint32_t(total%MinuteMicros);
        return int(total/MinuteMicros);
    }
    inline void save(std::ostream& out)
    {out.write("CLK1",4);out.write(reinterpret_cast<const char*>(&remainder),sizeof(remainder));}
    inline bool load(std::istream& in)
    {
        std::array<char,4> magic{};std::uint32_t value=0;
        if(!in.read(magic.data(),4)||magic!=std::array<char,4>{'C','L','K','1'}||!in.read(reinterpret_cast<char*>(&value),sizeof(value))||value>=MinuteMicros)return false;
        remainder=value;return true;
    }
}
