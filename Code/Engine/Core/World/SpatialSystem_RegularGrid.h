#pragma once

#include <Core/World/SpatialSystem.h>

class EZ_CORE_DLL ezSpatialSystem_RegularGrid : public ezSpatialSystem
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpatialSystem_RegularGrid, ezSpatialSystem);

public:
  ezSpatialSystem_RegularGrid(ezUInt32 uiCellSize = 128);
  ~ezSpatialSystem_RegularGrid();

private:
  // ezSpatialSystem implementation
  virtual void FindObjectsInSphere(const ezBoundingSphere& sphere, ezDynamicArray<ezGameObject *>& out_Objects) override;
  virtual void FindObjectsInSphere(const ezBoundingSphere& sphere, QueryCallback& callback) override;

  virtual void FindObjectsInBox(const ezBoundingBox& box, ezDynamicArray<ezGameObject *>& out_Objects) override;
  virtual void FindObjectsInBox(const ezBoundingBox& box, QueryCallback& callback) override;

  virtual void FindVisibleObjects(const ezFrustum& frustum, ezDynamicArray<const ezGameObject *>& out_Objects) override;

  virtual void SpatialDataBoundsChanged(ezSpatialData* pData, const ezBoundingBoxSphere& oldBounds, const ezBoundingBoxSphere& newBounds) override;
};

