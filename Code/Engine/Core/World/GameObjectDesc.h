#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/TagSet.h>
#include <Foundation/Types/Uuid.h>

/// \brief Describes the initial state of a game object.
struct EZ_CORE_DLL ezGameObjectDesc
{
  EZ_DECLARE_POD_TYPE();

  bool m_bActiveFlag = true;
  bool m_bDynamic = false;
  ezUInt16 m_uiTeamID = 0;

  ezHashedString m_sName;

  ezGameObjectHandle m_hParent;

  ezVec3 m_LocalPosition = ezVec3::ZeroVector();
  ezQuat m_LocalRotation = ezQuat::IdentityQuaternion();
  ezVec3 m_LocalScaling = ezVec3(1, 1, 1);
  float m_LocalUniformScaling = 1.0f;
  ezTagSet m_Tags;
};
