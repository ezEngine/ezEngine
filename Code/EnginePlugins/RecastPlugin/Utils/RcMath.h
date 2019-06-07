#pragma once

#include <RecastPlugin/RecastPluginDLL.h>
#include <Foundation/Math/Declarations.h>

/// \brief Helper class to convert between Recast's convention (float[3] and Y is up) and ezVec3 (Z up)
///
/// Will automatically swap Y and Z when assigning between the different types.
struct EZ_RECASTPLUGIN_DLL ezRcPos
{
  float m_Pos[3];

  ezRcPos();
  ezRcPos(const float* pos);
  ezRcPos(const ezVec3& v);

  void operator=(const ezVec3& v);
  void operator=(const float* pos);

  operator const float*() const;
  operator float*() ;
  operator ezVec3() const;
};
