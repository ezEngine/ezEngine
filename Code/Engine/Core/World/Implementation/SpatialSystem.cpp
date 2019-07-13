#include <CorePCH.h>

#include <Core/World/SpatialSystem.h>
#include <Foundation/Time/Stopwatch.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpatialSystem, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSpatialSystem::ezSpatialSystem()
  : m_Allocator("Spatial System", ezFoundation::GetDefaultAllocator())
  , m_AllocatorWrapper(&m_Allocator)
  , m_BlockAllocator("Spatial System Blocks", &m_Allocator)
  , m_DataTable(&m_Allocator)
  , m_DataStorage(&m_BlockAllocator, &m_Allocator)
  , m_DataAlwaysVisible(&m_Allocator)
{
}

ezSpatialSystem::~ezSpatialSystem() = default;

ezSpatialDataHandle ezSpatialSystem::CreateSpatialData(const ezSimdBBoxSphere& bounds, ezGameObject* pObject, ezUInt32 uiCategoryBitmask)
{
  ezSpatialData* pData = m_DataStorage.Create();

  pData->m_pObject = pObject;
  pData->m_Bounds = bounds;

  SpatialDataAdded(pData);

  return ezSpatialDataHandle(m_DataTable.Insert(pData));
}

ezSpatialDataHandle ezSpatialSystem::CreateSpatialDataAlwaysVisible(ezGameObject* pObject, ezUInt32 uiCategoryBitmask)
{
  ezSpatialData* pData = m_DataStorage.Create();

  pData->m_pObject = pObject;
  pData->m_Flags.Add(ezSpatialData::Flags::AlwaysVisible);
  pData->m_Bounds.SetInvalid();

  m_DataAlwaysVisible.PushBack(pData);

  return ezSpatialDataHandle(m_DataTable.Insert(pData));
}

void ezSpatialSystem::DeleteSpatialData(const ezSpatialDataHandle& hData)
{
  ezSpatialData* pData = nullptr;
  if (!m_DataTable.Remove(hData.GetInternalID(), &pData))
    return;

  if (pData->m_Flags.IsSet(ezSpatialData::Flags::AlwaysVisible))
  {
    m_DataAlwaysVisible.RemoveAndSwap(pData);
  }
  else
  {
    SpatialDataRemoved(pData);
  }

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

void ezSpatialSystem::UpdateSpatialData(const ezSpatialDataHandle& hData, const ezSimdBBoxSphere& bounds,
  ezGameObject* pObject, ezUInt32 uiCategoryBitmask)
{
  ezSpatialData* pData = nullptr;
  if (!m_DataTable.TryGetValue(hData.GetInternalID(), pData))
    return;

  pData->m_pObject = pObject;

  if (!pData->m_Flags.IsSet(ezSpatialData::Flags::AlwaysVisible))
  {
    ezUInt32 uiOldCategoryBitmask = pData->m_uiCategoryBitmask;
    ezSimdBBoxSphere oldBounds = pData->m_Bounds;

    pData->m_uiCategoryBitmask = uiCategoryBitmask;
    pData->m_Bounds = bounds;

    if (uiCategoryBitmask != uiOldCategoryBitmask || bounds != oldBounds)
    {
      SpatialDataChanged(pData, oldBounds, uiOldCategoryBitmask);
    }
  }
  else
  {
    pData->m_uiCategoryBitmask = uiCategoryBitmask;
  }
}

void ezSpatialSystem::FindObjectsInSphere(const ezBoundingSphere& sphere, ezUInt32 uiCategoryBitmask, ezDynamicArray<ezGameObject*>& out_Objects,
  QueryStats* pStats /*= nullptr*/) const
{
  FindObjectsInSphere(sphere, uiCategoryBitmask,
    [&](ezGameObject* pObject) {
      out_Objects.PushBack(pObject);

      return ezVisitorExecution::Continue;
    },
    pStats);
}

void ezSpatialSystem::FindObjectsInSphere(const ezBoundingSphere& sphere, ezUInt32 uiCategoryBitmask, QueryCallback callback, QueryStats* pStats /*= nullptr*/) const
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (pStats != nullptr)
  {
    pStats->m_uiTotalNumObjects = m_DataTable.GetCount();
    pStats->m_uiNumObjectsTested += m_DataAlwaysVisible.GetCount();
    pStats->m_uiNumObjectsPassed += m_DataAlwaysVisible.GetCount();
  }
#endif

  FindObjectsInSphereInternal(sphere, uiCategoryBitmask, callback, pStats);

  for (auto pData : m_DataAlwaysVisible)
  {
    if ((pData->m_uiCategoryBitmask & uiCategoryBitmask) != 0)
    {
      callback(pData->m_pObject);
    }
  }
}

void ezSpatialSystem::FindObjectsInBox(const ezBoundingBox& box, ezUInt32 uiCategoryBitmask, ezDynamicArray<ezGameObject*>& out_Objects,
  QueryStats* pStats /*= nullptr*/) const
{
  FindObjectsInBox(box, uiCategoryBitmask,
    [&](ezGameObject* pObject) {
      out_Objects.PushBack(pObject);

      return ezVisitorExecution::Continue;
    },
    pStats);
}

void ezSpatialSystem::FindObjectsInBox(const ezBoundingBox& box, ezUInt32 uiCategoryBitmask, QueryCallback callback, QueryStats* pStats /*= nullptr*/) const
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (pStats != nullptr)
  {
    pStats->m_uiTotalNumObjects = m_DataTable.GetCount();
    pStats->m_uiNumObjectsTested += m_DataAlwaysVisible.GetCount();
    pStats->m_uiNumObjectsPassed += m_DataAlwaysVisible.GetCount();
  }
#endif

  FindObjectsInBoxInternal(box, uiCategoryBitmask, callback, pStats);

  for (auto pData : m_DataAlwaysVisible)
  {
    if ((pData->m_uiCategoryBitmask & uiCategoryBitmask) != 0)
    {
      callback(pData->m_pObject);
    }
  }
}

void ezSpatialSystem::FindVisibleObjects(const ezFrustum& frustum, ezUInt32 uiCategoryBitmask, ezDynamicArray<const ezGameObject*>& out_Objects,
  QueryStats* pStats /*= nullptr*/) const
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezStopwatch timer;

  if (pStats != nullptr)
  {
    pStats->m_uiTotalNumObjects = m_DataTable.GetCount();
    pStats->m_uiNumObjectsTested += m_DataAlwaysVisible.GetCount();
    pStats->m_uiNumObjectsPassed += m_DataAlwaysVisible.GetCount();
  }
#endif

  FindVisibleObjectsInternal(frustum, uiCategoryBitmask, out_Objects, pStats);

  for (auto pData : m_DataAlwaysVisible)
  {
    if ((pData->m_uiCategoryBitmask & uiCategoryBitmask) != 0)
    {
      out_Objects.PushBack(pData->m_pObject);
    }    
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (pStats != nullptr)
  {
    pStats->m_TimeTaken = timer.GetRunningTotal();
  }
#endif
}



EZ_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem);
