#include <Core/CorePCH.h>

#include <Core/World/SpatialSystem.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpatialSystem, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSpatialSystem::ezSpatialSystem()
  : m_Allocator("Spatial System", ezFoundation::GetDefaultAllocator())
{
}

ezSpatialSystem::~ezSpatialSystem() = default;

void ezSpatialSystem::StartNewFrame()
{
  ++m_uiFrameCounter;
}

void ezSpatialSystem::FindObjectsInSphere(const ezBoundingSphere& sphere, const QueryParams& queryParams, ezDynamicArray<ezGameObject*>& out_Objects) const
{
  FindObjectsInSphere(
    sphere, queryParams,
    [&](ezGameObject* pObject) {
      out_Objects.PushBack(pObject);

      return ezVisitorExecution::Continue;
    });
}

void ezSpatialSystem::FindObjectsInBox(  const ezBoundingBox& box, const QueryParams& queryParams, ezDynamicArray<ezGameObject*>& out_Objects) const
{
  FindObjectsInBox(
    box, queryParams,
    [&](ezGameObject* pObject) {
      out_Objects.PushBack(pObject);

      return ezVisitorExecution::Continue;
    });
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem);
