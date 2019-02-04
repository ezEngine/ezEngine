#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>

struct EZ_ALIGN_16(ezSpatialData)
{
  struct Flags
  {
    typedef ezUInt8 StorageType;

    enum Enum
    {
      None = 0,
      Dynamic = EZ_BIT(0),
      AlwaysVisible = EZ_BIT(1),

      Default = Dynamic
    };

    struct Bits
    {
      StorageType Dynamic : 1;
      StorageType AlwaysVisible : 1;
    };
  };

  ezGameObject* m_pObject = nullptr;
  ezBitflags<Flags> m_Flags;

  ezSimdBBoxSphere m_Bounds;

  ezUInt32 m_uiUserData[4] = {};
};

