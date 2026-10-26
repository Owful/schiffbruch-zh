#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace PixelFormat
{
    inline unsigned shift(std::uint32_t mask)
    {
        unsigned n = 0;
        if (mask) while (!(mask & 1)) { mask >>= 1; ++n; }
        return n;
    }
    inline std::uint32_t channel(std::uint8_t value, std::uint32_t mask)
    {
        const auto n = shift(mask);
        return static_cast<std::uint32_t>((std::uint64_t(value) * (mask >> n) + 127) / 255) << n;
    }
    inline std::uint8_t unpack(std::uint32_t pixel, std::uint32_t mask)
    {
        if (!mask) return 0;
        const auto n = shift(mask), max = mask >> n;
        return static_cast<std::uint8_t>((std::uint64_t((pixel & mask) >> n) * 255 + max / 2) / max);
    }
    inline std::uint32_t pack(std::uint8_t r, std::uint8_t g, std::uint8_t b,
        std::uint32_t red, std::uint32_t green, std::uint32_t blue)
    {
        return channel(r,red) | channel(g,green) | channel(b,blue);
    }
    inline std::uint32_t read(const void* surface, std::ptrdiff_t pitch, int x, int y, unsigned bits)
    {
        if (bits != 16 && bits != 24 && bits != 32) return 0;
        std::uint32_t pixel = 0;
        std::memcpy(&pixel, static_cast<const std::uint8_t*>(surface) + y*pitch + x*(bits/8), bits/8);
        return pixel;
    }
    inline void write(void* surface, std::ptrdiff_t pitch, int x, int y, unsigned bits, std::uint32_t pixel)
    {
        if (bits != 16 && bits != 24 && bits != 32) return;
        std::memcpy(static_cast<std::uint8_t*>(surface) + y*pitch + x*(bits/8), &pixel, bits/8);
    }
}
