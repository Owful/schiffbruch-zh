#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <istream>
#include <ostream>

// Platform-independent rules and a fixed-width, optional save extension.
namespace ExpeditionRules
{
    enum class Weather { Clear, Rain, Storm };
    inline Weather weather(int day)
    {
        constexpr Weather week[] = {Weather::Clear, Weather::Clear, Weather::Rain,
            Weather::Clear, Weather::Storm, Weather::Clear, Weather::Rain};
        return week[static_cast<unsigned>(std::max(1, day) - 1) % 7];
    }
    inline float visibility(Weather w)
    {
        return w == Weather::Clear ? 1.f : w == Weather::Rain ? .65f : .3f;
    }
    inline float growth(Weather w) { return w == Weather::Clear ? 1.f : 1.4f; }
    inline float fireDecay(Weather w) { return w == Weather::Clear ? 1.f : w == Weather::Rain ? 1.2f : 1.6f; }
    inline float waterChange(Weather w, int hour, int minutes, bool sheltered)
    {
        if (minutes <= 0 || sheltered) return 0.f;
        if (w != Weather::Clear) return minutes * .02f;
        return hour >= 4 && hour < 8 ? -minutes * .025f : 0.f;
    }
    inline int signalGain(Weather w, int minutes)
    {
        return std::max(0, minutes) * (w == Weather::Clear ? 1000 : w == Weather::Rain ? 650 : 300);
    }
    constexpr std::uint32_t SignalTarget = 360000; // Six clear-weather hours with a complete signal setup.
    struct Cache
    {
        std::uint8_t x = 255, y = 255, kind = 0;
    };
    struct Progress
    {
        bool waterskin = false, waterVisited = false;
        std::uint8_t water = 0, bandages = 0, claimed = 0;
        std::uint32_t signal = 0;
        std::array<Cache, 6> caches{};
    };
    inline bool valid(const Progress& p)
    {
        if (p.water > 3 || p.bandages > 3 || p.claimed > 63 || p.signal > SignalTarget || (!p.waterskin && p.water)) return false;
        for (unsigned i = 0; i < p.caches.size(); ++i)
        {
            const auto& c = p.caches[i];
            if (c.kind > 2) return false;
            if (c.x == 255 && c.y == 255) continue;
            if (c.x >= 60 || c.y >= 60) return false;
            for (unsigned j = 0; j < i; ++j)
                if (c.x == p.caches[j].x && c.y == p.caches[j].y) return false;
        }
        return true;
    }
    inline void write(std::ostream& out, const Progress& p)
    {
        const std::array<std::uint8_t, 9> fields = {static_cast<std::uint8_t>(p.waterskin),
            static_cast<std::uint8_t>(p.waterVisited), p.water, p.bandages, p.claimed,
            static_cast<std::uint8_t>(p.signal), static_cast<std::uint8_t>(p.signal >> 8),
            static_cast<std::uint8_t>(p.signal >> 16), static_cast<std::uint8_t>(p.signal >> 24)};
        out.write("EXP1", 4);
        out.write(reinterpret_cast<const char*>(fields.data()), fields.size());
        for (const auto& c : p.caches)
        {
            const std::uint8_t bytes[] = {c.x, c.y, c.kind};
            out.write(reinterpret_cast<const char*>(bytes), 3);
        }
    }
    // Legacy saves have no footer. On missing/corrupt data, leave the caller untouched.
    inline bool read(std::istream& in, Progress& result)
    {
        std::array<char, 4> magic{};
        std::array<std::uint8_t, 9> f{};
        if (!in.read(magic.data(), magic.size()) || magic != std::array<char,4>{'E','X','P','1'} ||
            !in.read(reinterpret_cast<char*>(f.data()), f.size()) || f[0] > 1 || f[1] > 1) return false;
        Progress p;
        p.waterskin = f[0] != 0; p.waterVisited = f[1] != 0;
        p.water = f[2]; p.bandages = f[3]; p.claimed = f[4];
        p.signal = std::uint32_t(f[5]) | (std::uint32_t(f[6]) << 8) |
            (std::uint32_t(f[7]) << 16) | (std::uint32_t(f[8]) << 24);
        for (auto& c : p.caches)
        {
            std::uint8_t bytes[3]{};
            if (!in.read(reinterpret_cast<char*>(bytes), 3)) return false;
            c = {bytes[0], bytes[1], bytes[2]};
        }
        if (!valid(p)) return false;
        result = p;
        return true;
    }
    inline float rescueProbability(float chance, Weather w)
    {
        if (!std::isfinite(chance)) return 0.f;
        return std::clamp(chance * visibility(w) / 72000.f, 0.f, 1.f);
    }
}
