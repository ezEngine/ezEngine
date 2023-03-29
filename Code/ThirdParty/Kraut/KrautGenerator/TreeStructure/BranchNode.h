#pragma once

#include <KrautGenerator/KrautGeneratorDLL.h>

#include <KrautFoundation/Math/Vec3.h>
#include <KrautFoundation/Streams/Streams.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct KRAUT_DLL BranchNode
  {
    aeVec3 m_vPosition = aeVec3::ZeroVector();
    float m_fThickness = 0.1f;
    bool m_bHasChildBranches = false;
    float m_fTexCoordV = 0.0f;
  };

} // namespace Kraut
