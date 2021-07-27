#include <RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkinnedMeshRenderData, 1, ezRTTIDefaultAllocator<ezSkinnedMeshRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezSkinnedMeshRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(m_uiUniqueID);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezSkinnedMeshComponent, 1);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Animated Mesh")),
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new ezAssetBrowserAttribute("Material")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSkinnedMeshComponent::ezSkinnedMeshComponent() = default;
ezSkinnedMeshComponent::~ezSkinnedMeshComponent() = default;

void ezSkinnedMeshComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();
}

void ezSkinnedMeshComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();
}

void ezSkinnedMeshComponent::OnDeactivated()
{
  if (!m_hSkinningTransformsBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hSkinningTransformsBuffer);
    m_hSkinningTransformsBuffer.Invalidate();
  }

  SUPER::OnDeactivated();
}

ezMeshRenderData* ezSkinnedMeshComponent::CreateRenderData() const
{
  auto pRenderData = ezCreateRenderDataForThisFrame<ezSkinnedMeshRenderData>(GetOwner());

  pRenderData->m_hSkinningTransforms = m_hSkinningTransformsBuffer;
  pRenderData->m_pNewSkinningTransformData = m_SkinningTransforms.ToByteArray();

  return pRenderData;
}

void ezSkinnedMeshComponent::UpdateSkinningTransformBuffer(ezArrayPtr<const ezShaderTransform> skinningTransforms)
{
  if (m_hSkinningTransformsBuffer.IsInvalidated())
  {
    ezGALBufferCreationDescription BufferDesc;
    BufferDesc.m_uiStructSize = sizeof(ezShaderTransform);
    BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * skinningTransforms.GetCount();
    BufferDesc.m_bUseAsStructuredBuffer = true;
    BufferDesc.m_bAllowShaderResourceView = true;
    BufferDesc.m_ResourceAccess.m_bImmutable = false;

    m_hSkinningTransformsBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(BufferDesc, skinningTransforms.ToByteArray());
  }
  else
  {
    auto transformsCopy = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezShaderTransform, skinningTransforms.GetCount());
    transformsCopy.CopyFrom(skinningTransforms);

    m_SkinningTransforms = transformsCopy;
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_SkinnedMeshComponent);
