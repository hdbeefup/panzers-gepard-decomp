// src/3dengine/pz/weathersound.h
// The world's rain ambient sound goes through the Panzers concert (HD
// 0x8f1c5c, the Miles concert): the world code has no Miles build switch,
// so these wrappers (weatherfx.cpp) forward to it and do nothing without
// Miles. OWNER: agent M6-WX.

#ifndef PZ_WEATHERSOUND_H
#define PZ_WEATHERSOUND_H

namespace pz {

int  WeatherPrecacheSound(const char* name);            // concert +0x24(name, 0): cache index, -1
int  WeatherCreateLoopSound(int cache);                 // concert +0x30(cache, 1, 0, 0, 1): sound id, -1
void WeatherSetSoundVolume(int sound, float volume);    // concert +0x48(id, volume, 0)
void WeatherRemoveSound(int sound);                     // concert +0x3c(id)

} // namespace pz

#endif // PZ_WEATHERSOUND_H
