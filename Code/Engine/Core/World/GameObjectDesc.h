#pragma once

#include <Core/World/Declarations.h>

struct ezGameObjectFlags
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    Dynamic = 1U << 0,
    Active = 1U << 1,

    InternalFlags = Active - 1,
    Default = Dynamic | Active
  };

  struct Bits
  {
    StorageType Dynamic : 1;
    StorageType Active : 1;
  };
};

EZ_DECLARE_FLAGS_OR_OPERATOR(ezGameObjectFlags);


struct ezGameObjectDesc
{
  EZ_DECLARE_POD_TYPE();

  ezGameObjectDesc()
  {
    m_Flags.Add(ezGameObjectFlags::Default);
    m_uiPersistentId = -1;
    m_szName = NULL;

    m_LocalPosition.SetZero();
    m_LocalRotation.SetIdentity();
    m_LocalScaling.Set(1.0f);
  }

  ezBitflags<ezGameObjectFlags> m_Flags;
  ezUInt64 m_uiPersistentId;

  const char* m_szName;

  ezVec3 m_LocalPosition;
  ezQuat m_LocalRotation;
  ezVec3 m_LocalScaling;
};
