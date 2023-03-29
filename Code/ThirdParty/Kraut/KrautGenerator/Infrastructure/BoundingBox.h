#pragma once

#include <KrautFoundation/Math/Vec3.h>
#include <KrautGenerator/KrautGeneratorDLL.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct KRAUT_DLL BoundingBox
  {
    BoundingBox();

    void SetInvalid();
    bool IsInvalid() const;
    aeVec3 GetSize() const;
    void AddBoundary(const aeVec3& vAdd);

    void ExpandToInclude(const aeVec3& v);
    aeVec3 GetCenter() const;

    aeVec3 m_vMin;
    aeVec3 m_vMax;
  };

} // namespace Kraut
