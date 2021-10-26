#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>

namespace ezBakingInternal
{
  struct Volume
  {
    ezSimdMat4f m_GlobalToLocalTransform;
  };
} // namespace ezBakingInternal
