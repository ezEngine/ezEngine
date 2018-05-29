#pragma once

#include <fmod_studio.hpp>

#define EZ_FMOD_ASSERT(func) { auto fmodErrorCode = func; if (fmodErrorCode != FMOD_OK) ezLog::Error("FMOD call failed: '" EZ_STRINGIZE(func) "' - Error code {0}", fmodErrorCode); }

