#pragma once

#include <Core/World/Declarations.h>

struct ezGameObjectDesc
{
  EZ_DECLARE_POD_TYPE();

  ezGameObjectDesc()
  {
    m_Flags.Add(ezObjectFlags::Default);
    m_uiPersistentId = -1;
    m_szName = NULL;

    m_LocalPosition.SetZero();
    m_LocalRotation.SetIdentity();
    m_LocalScaling.Set(1.0f);
  }

  ezBitflags<ezObjectFlags> m_Flags;
  ezUInt64 m_uiPersistentId;

  const char* m_szName;

  ezGameObjectHandle m_Parent;

  ezVec3 m_LocalPosition;
  ezQuat m_LocalRotation;
  ezVec3 m_LocalScaling;
};
