#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>
#include <MiniAudio/miniaudio.h>
#include <MiniAudioPlugin/MiniAudioPluginDLL.h>

EZ_DEFINE_AS_POD_TYPE(ma_sound);

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library

#define EZ_MA_CHECK(x)                               \
  do                                                 \
  {                                                  \
    if (int res = (x); res != MA_SUCCESS)            \
    {                                                \
      EZ_REPORT_FAILURE("MiniAudio error: {}", res); \
    }                                                \
  } while (false)\
