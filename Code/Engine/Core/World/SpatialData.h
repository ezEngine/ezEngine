#pragma once

#include <Foundation/SimdMath/SimdBBoxSphere.h>
#include <Core/World/Declarations.h>

struct EZ_ALIGN_16(ezSpatialData)
{
  ezGameObject* m_pObject;
  ezUInt32 m_uiRefCount : 16;
  ezUInt32 m_Flags : 16; //todo

  ezUInt32 m_uiLastFrameVisible;

  ///\todo might want to store local bounding box for precise culling
  ezUInt32 m_uiReserved[4];

  ezSimdBBoxSphere m_Bounds;
};
