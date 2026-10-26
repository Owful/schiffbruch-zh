#pragma once
#include <array>
#include <algorithm>
#include <cstdint>

namespace ArtPalette
{
    // Grade terrain at load time, preserving source assets and their transparency key.
    inline std::array<std::uint8_t,3> grade(unsigned r,unsigned g,unsigned b)
    {
        if (b > r*5/4 && b > g*6/5)
            return {static_cast<std::uint8_t>(26+r*18/100),static_cast<std::uint8_t>(75+g*32/100+b*10/100),static_cast<std::uint8_t>(87+b*25/100)};
        if (g > r*6/5 && g > b*6/5)
            return {static_cast<std::uint8_t>(42+r*34/100),static_cast<std::uint8_t>(47+g*48/100),static_cast<std::uint8_t>(43+b*31/100)};
        if (r > 150 && g > 140 && b < std::min(r,g)*55/100)
            return {static_cast<std::uint8_t>(111+r*37/100),static_cast<std::uint8_t>(106+g*32/100),static_cast<std::uint8_t>(111+b*31/100)};
        return {static_cast<std::uint8_t>(r),static_cast<std::uint8_t>(g),static_cast<std::uint8_t>(b)};
    }
}
