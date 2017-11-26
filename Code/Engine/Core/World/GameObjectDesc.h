#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Math/Quat.h>

#include <Core/World/Declarations.h>

/// \brief Describes the initial state of a game object.
struct ezGameObjectDesc
{
  EZ_DECLARE_POD_TYPE();

  ezGameObjectDesc()
  {
    m_bActive = true;
    m_bDynamic = false;

    m_LocalPosition.SetZero();
    m_LocalRotation.SetIdentity();
    m_LocalScaling.Set(1.0f);
    m_LocalUniformScaling = 1.0f;
  }

  bool m_bActive;
  bool m_bDynamic;

  ezHashedString m_sName;

  ezGameObjectHandle m_hParent;

  ezVec3 m_LocalPosition;
  ezQuat m_LocalRotation;
  ezVec3 m_LocalScaling;
  float m_LocalUniformScaling;
  ezTagSet m_Tags;
};

