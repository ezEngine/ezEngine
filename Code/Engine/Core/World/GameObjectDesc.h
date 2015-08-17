#pragma once

#include <Foundation/Strings/HashedString.h>

#include <Core/World/Declarations.h>

/// \brief Describes the initial state of a game object.
struct ezGameObjectDesc
{
  EZ_DECLARE_POD_TYPE();

  ezGameObjectDesc()
  {
    m_Flags.Add(ezObjectFlags::Default);
        
    m_LocalPosition.SetZero();
    m_LocalRotation.SetIdentity();
    m_LocalScaling.Set(1.0f);
  }

  ezBitflags<ezObjectFlags> m_Flags;
  
  ezHashedString m_sName;

  ezGameObjectHandle m_hParent;

  ezVec3 m_LocalPosition;
  ezQuat m_LocalRotation;
  ezVec3 m_LocalScaling;
};

