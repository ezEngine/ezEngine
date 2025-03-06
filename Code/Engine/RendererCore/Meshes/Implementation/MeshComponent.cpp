#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezMeshComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("Mesh", GetMesh, SetMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new ezDefaultValueAttribute(ezVec4(0, 1, 0, 1))),
    EZ_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
    EZ_ACCESSOR_PROPERTY("SortingDepthOffset", GetSortingDepthOffset, SetSortingDepthOffset),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnMsgExtractGeometry)
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezMeshComponent::ezMeshComponent() = default;
ezMeshComponent::~ezMeshComponent() = default;

void ezMeshComponent::OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
    return;

  // ignore invalid and created resources
  {
    ezMeshResourceHandle hRenderMesh = GetMesh();
    if (!hRenderMesh.IsValid())
      return;

    ezResourceLock<ezMeshResource> pRenderMesh(hRenderMesh, ezResourceAcquireMode::PointerOnly);
    if (pRenderMesh->GetBaseResourceFlags().IsAnySet(ezResourceFlags::IsCreatedResource))
      return;
  }

  ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), ezResourceManager::LoadResource<ezCpuMeshResource>(GetMesh().GetResourceID()));
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezMeshImportTransform, 1)
  EZ_ENUM_CONSTANT(ezMeshImportTransform::Blender_YUp),
    EZ_ENUM_CONSTANT(ezMeshImportTransform::Blender_ZUp),
    EZ_ENUM_CONSTANT(ezMeshImportTransform::Custom),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezBasisAxis::Enum ezMeshImportTransform::GetRightDir(ezMeshImportTransform::Enum transform, ezBasisAxis::Enum dir)
{
  switch (transform)
  {
    case ezMeshImportTransform::Blender_YUp:
      return ezBasisAxis::NegativeX;
    case ezMeshImportTransform::Blender_ZUp:
      return ezBasisAxis::NegativeX;
    case ezMeshImportTransform::Custom:
      return dir;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return dir;
}

ezBasisAxis::Enum ezMeshImportTransform::GetUpDir(ezMeshImportTransform::Enum transform, ezBasisAxis::Enum dir)
{
  switch (transform)
  {
    case ezMeshImportTransform::Blender_YUp:
      return ezBasisAxis::PositiveY;
    case ezMeshImportTransform::Blender_ZUp:
      return ezBasisAxis::PositiveZ;
    case ezMeshImportTransform::Custom:
      return dir;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return dir;
}

bool ezMeshImportTransform::GetFlipForward(ezMeshImportTransform::Enum transform, bool bFlip)
{
  switch (transform)
  {
    case ezMeshImportTransform::Blender_YUp:
      return false;
    case ezMeshImportTransform::Blender_ZUp:
      return false;
    case ezMeshImportTransform::Custom:
      return bFlip;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return bFlip;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponent);
