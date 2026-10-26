#include "ExpeditionRules.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>

namespace
{
    int failures = 0;
    void check(bool condition, const char* message)
    {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    }
}
int main()
{
    using namespace ExpeditionRules;
    check(weather(1) == Weather::Clear && weather(2) == Weather::Clear, "new players get two clear days");
    check(weather(3) == Weather::Rain && weather(5) == Weather::Storm && weather(8) == weather(1), "repeatable forecast");
    check(waterChange(Weather::Clear, 4, 60, false) == -1.5f, "midday heat consumes water");
    check(waterChange(Weather::Clear, 3, 60, false) == 0.f, "morning has no heat penalty");
    check(waterChange(Weather::Storm, 4, 60, true) == 0.f, "shelter blocks rain collection");
    check(waterChange(Weather::Rain, 4, 60, false) > 0.f, "rain replenishes a little water");
    check(fireDecay(Weather::Storm) > fireDecay(Weather::Clear), "storms shorten fire lifetime");
    check(growth(Weather::Rain) > growth(Weather::Clear), "rain helps crops grow");
    std::uint32_t progress = 0;
    for (int i = 0; i < 360; ++i) progress += signalGain(Weather::Clear, 1);
    check(progress == SignalTarget, "rescue is reachable in six clear hours");
    check(signalGain(Weather::Storm, 60) < signalGain(Weather::Rain, 60), "weather affects signal visibility");
    check(signalGain(Weather::Clear, -3) == 0, "negative time cannot reverse signal progress");
    check(rescueProbability(-5, Weather::Clear) == 0.f, "negative chance does not produce invalid modulo");
    check(rescueProbability(1000000, Weather::Clear) == 1.f, "very large rescue chance is bounded");
    check(rescueProbability(std::numeric_limits<float>::infinity(), Weather::Clear) == 0.f, "invalid chance is ignored");
    Progress source;
    source.waterskin = true; source.waterVisited = true; source.water = 3;
    source.bandages = 2; source.claimed = 5; source.signal = 259750;
    source.caches[0] = {12, 23, 1}; source.caches[1] = {15, 40, 2};
    std::ostringstream out(std::ios::binary); write(out, source);
    const auto bytes = out.str();
    check(bytes.size() == 31, "footer has a stable 31-byte format");
    Progress loaded;
    std::istringstream in(bytes, std::ios::binary);
    check(read(in, loaded) && loaded.water == 3 && loaded.signal == source.signal &&
        loaded.claimed == 5 && loaded.caches[1].y == 40, "save round trip retains consumables, exploration and rescue");
    for (std::size_t n = 0; n < bytes.size(); ++n)
    {
        Progress sentinel; sentinel.bandages = 1;
        std::istringstream truncated(bytes.substr(0,n), std::ios::binary);
        check(!read(truncated, sentinel) && sentinel.bandages == 1, "truncated extension does not mutate game state");
    }
    auto corrupt = bytes; corrupt[6] = 99;
    std::istringstream bad(corrupt, std::ios::binary);
    check(!read(bad, loaded), "impossible water count is rejected");
    corrupt = bytes; corrupt[13] = 60;
    std::istringstream badPosition(corrupt, std::ios::binary);
    check(!read(badPosition, loaded), "out-of-map cache coordinate is rejected");
    std::istringstream legacy(""); Progress legacyState;
    check(!read(legacy,legacyState) && legacyState.water == 0, "legacy saves start with default expedition state");
    if (failures) return EXIT_FAILURE;
    std::cout << "Expedition rules: all checks passed.\n";
}
