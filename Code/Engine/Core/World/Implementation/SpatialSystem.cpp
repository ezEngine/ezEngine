#include <Core/PCH.h>
#include <Core/World/SpatialSystem.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpatialSystem, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_CHECK_AT_COMPILETIME(sizeof(ezSpatialData) == 64);

ezSpatialSystem::ezSpatialSystem()
  : m_Allocator("Spatial System", ezFoundation::GetDefaultAllocator())
  , m_AllocatorWrapper(&m_Allocator)
  , m_BlockAllocator("Spatial System Blocks", &m_Allocator)
  , m_DataStorage(&m_BlockAllocator, &m_Allocator)
{

}

ezSpatialSystem::~ezSpatialSystem()
{

}

ezSpatialDataHandle ezSpatialSystem::CreateSpatialData(const ezBoundingBoxSphere& bounds, ezGameObject* pObject /*= nullptr*/)
{
  ezSpatialData* pData = m_DataStorage.Create();

  pData->m_pObject = pObject;
  pData->m_Flags = 0;
  pData->m_uiLastFrameVisible = 0;
  pData->m_Bounds = bounds;

  ezBoundingBoxSphere invalidBounds; invalidBounds.SetInvalid();
  SpatialDataBoundsChanged(pData, invalidBounds, bounds);

  return ezSpatialDataHandle(m_DataTable.Insert(pData));
}

void ezSpatialSystem::DeleteSpatialData(const ezSpatialDataHandle& hData)
{
  ezSpatialData* pData = nullptr;
  if (!m_DataTable.Remove(hData.GetInternalID(), &pData))
    return;

  ezBoundingBoxSphere invalidBounds; invalidBounds.SetInvalid();
  SpatialDataBoundsChanged(pData, pData->m_Bounds, invalidBounds);

  ezSpatialData* pMovedData = nullptr;
  m_DataStorage.Delete(pData, pMovedData);

  if (pData != pMovedData)
  {
    FixSpatialDataPointer(pMovedData, pData);
  }
}

bool ezSpatialSystem::TryGetSpatialData(const ezSpatialDataHandle& hData, const ezSpatialData*& out_pData) const
{
  ezSpatialData* pData = nullptr;
  bool res = m_DataTable.TryGetValue(hData.GetInternalID(), pData);
  out_pData = pData;
  return res;
}

void ezSpatialSystem::UpdateSpatialData(const ezSpatialDataHandle& hData, const ezBoundingBoxSphere& bounds, ezGameObject* pObject /*= nullptr*/)
{
  ezSpatialData* pData = nullptr;
  if (!m_DataTable.TryGetValue(hData.GetInternalID(), pData))
    return;

  ezBoundingBoxSphere oldBounds = pData->m_Bounds;

  pData->m_pObject = pObject;
  pData->m_Bounds = bounds;

  if (bounds != oldBounds)
  {
    SpatialDataBoundsChanged(pData, oldBounds, bounds);
  }
}

void ezSpatialSystem::FixSpatialDataPointer(ezSpatialData* pOldPtr, ezSpatialData* pNewPtr)
{

}
