#pragma once

#include <Core/World/Declarations.h>

struct EZ_ALIGN_16(ezSpatialData)
{
  ezGameObject* m_pObject;
  ezUInt32 m_Flags; //todo
  ezUInt32 m_uiLastFrameVisible;

  ezBoundingBoxSphere m_Bounds; //todo simd

  ///\todo might want to store local bounding box for precise culling
  ezUInt32 m_uiReserved[5];
};
