#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/GameObject.h>
#include <Core/World/SpatialSystem.h>
#include <Core/World/World.h>

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

void ezSpatialSystem::FindObjectsInSphere(const ezBoundingSphere& sphere, const QueryParams& queryParams, ezDynamicArray<ezGameObject*>& out_objects) const
{
  out_objects.Clear();

  FindObjectsInSphere(
    sphere, queryParams,
    [&](ezGameObject* pObject)
    {
      out_objects.PushBack(pObject);

      return ezVisitorExecution::Continue;
    });
}

void ezSpatialSystem::FindObjectsInBox(const ezBoundingBox& box, const QueryParams& queryParams, ezDynamicArray<ezGameObject*>& out_objects) const
{
  out_objects.Clear();

  FindObjectsInBox(
    box, queryParams,
    [&](ezGameObject* pObject)
    {
      out_objects.PushBack(pObject);

      return ezVisitorExecution::Continue;
    });
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
void ezSpatialSystem::GetInternalStats(ezStringBuilder& ref_sSb) const
{
  ref_sSb.Clear();
}
#endif

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptExtensionClass_Spatial, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(FindClosestObjectInSphere, In, "World", In, "Category", In, "Center", In, "Radius"),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezScriptExtensionAttribute("Spatial"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on


ezGameObject* ezScriptExtensionClass_Spatial::FindClosestObjectInSphere(ezWorld* pWorld, ezStringView sCategory, const ezVec3& vCenter, float fRadius)
{
  ezGameObject* pClosest = nullptr;

  auto category = ezSpatialData::FindCategory(sCategory);
  if (category != ezInvalidSpatialDataCategory)
  {
    ezSpatialSystem::QueryParams params;
    params.m_uiCategoryBitmask = category.GetBitmask();

    float fDistanceSqr = ezMath::HighValue<float>();

    pWorld->GetSpatialSystem()->FindObjectsInSphere(ezBoundingSphere::MakeFromCenterAndRadius(vCenter, fRadius), params, [&](ezGameObject* go) -> ezVisitorExecution::Enum
      {
        const float fSqr = go->GetGlobalPosition().GetSquaredDistanceTo(vCenter);

        if (fSqr < fDistanceSqr)
        {
          fDistanceSqr = fSqr;
          pClosest = go;
        }

        return ezVisitorExecution::Continue;
        //
      });
  }

  return pClosest;
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem);
