#pragma once

#include "headers.hpp"
#include "extern.hpp"
#include "SoundTypes.hpp"
#include <cstdint>

namespace Sound
{
    void Init();
    void LoadSound(short Sound); // Lädt einen Sound in den Speicher		
    void PlaySound(short Sound, short Volume); // Spiel eine Wavedatei mit Prozentualer Lautstärke
    void StopSound(short Sound); // Stoppt ein Sound

    void Update(std::int64_t micros,bool focused,bool paused);
    int GetMix(Bus bus);void SetMix(Bus bus,int level);void SetEnabled(bool enabled);
    struct Snapshot {int loaded=0,playing=0;float music=0,ambience=0;};
    Snapshot Inspect();
    float CurrentVolume(short id);bool IsPlaying(short id);
} // namespace Sound
