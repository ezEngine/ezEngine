#pragma once

#include <Foundation/SimdMath/SimdBBoxSphere.h>
#include <Core/World/Declarations.h>

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
  void* m_pUserData = nullptr;
  ezBitflags<Flags> m_Flags;

  ///\todo might want to store local bounding box for precise culling
  ezUInt32 m_uiReserved[3];

  ezSimdBBoxSphere m_Bounds;
};

