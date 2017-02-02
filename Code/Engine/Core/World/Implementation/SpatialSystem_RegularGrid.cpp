#include <Core/PCH.h>
#include <Core/World/SpatialSystem_RegularGrid.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpatialSystem_RegularGrid, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSpatialSystem_RegularGrid::ezSpatialSystem_RegularGrid(ezUInt32 uiCellSize /* = 128 */)
{
}

ezSpatialSystem_RegularGrid::~ezSpatialSystem_RegularGrid()
{

}

void ezSpatialSystem_RegularGrid::FindObjectsInSphere(const ezBoundingSphere& sphere, ezDynamicArray<ezGameObject *>& out_Objects)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezSpatialSystem_RegularGrid::FindObjectsInSphere(const ezBoundingSphere& sphere, QueryCallback& callback)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezSpatialSystem_RegularGrid::FindObjectsInBox(const ezBoundingBox& box, ezDynamicArray<ezGameObject *>& out_Objects)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezSpatialSystem_RegularGrid::FindObjectsInBox(const ezBoundingBox& box, QueryCallback& callback)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezSpatialSystem_RegularGrid::FindVisibleObjects(const ezFrustum& frustum, ezDynamicArray<const ezGameObject *>& out_Objects)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezSpatialSystem_RegularGrid::SpatialDataBoundsChanged(ezSpatialData* pData, const ezBoundingBoxSphere& oldBounds, const ezBoundingBoxSphere& newBounds)
{
  // not implemented
}
