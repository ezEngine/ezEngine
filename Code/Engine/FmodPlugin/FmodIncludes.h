#pragma once

#include <fmod_studio.hpp>

#define EZ_FMOD_ASSERT(func) { auto fmodErrorCode = func; EZ_VERIFY(fmodErrorCode == FMOD_OK, "Fmod failed with error code {0}", fmodErrorCode); }

