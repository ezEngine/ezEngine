#pragma once

#include <Foundation/Math/Quat.h>
#include <Foundation/Strings/HashedString.h>

#include <Core/World/Declarations.h>
#include <Foundation/Types/Uuid.h>

/// \brief Describes the initial state of a game object.
struct EZ_CORE_DLL ezGameObjectDesc
{
  EZ_DECLARE_POD_TYPE();

  ezGameObjectDesc()
  {
    m_LocalPosition.SetZero();
    m_LocalRotation.SetIdentity();
    m_LocalScaling.Set(1.0f);
  }

  bool m_bActive = true;
  bool m_bDynamic = false;
  ezUInt16 m_uiTeamID = 0;

  ezHashedString m_sName;

  ezGameObjectHandle m_hParent;

  ezVec3 m_LocalPosition;
  ezQuat m_LocalRotation;
  ezVec3 m_LocalScaling;
  float m_LocalUniformScaling = 1.0f;
  ezTagSet m_Tags;
};

