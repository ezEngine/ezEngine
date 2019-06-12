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

  if (!m_SkinningMatrices.IsEmpty())
  {
    pRenderData->m_hSkinningMatrices = m_hSkinningTransformsBuffer;
    pRenderData->m_pNewSkinningMatricesData = ezArrayPtr<const ezUInt8>(
      reinterpret_cast<const ezUInt8*>(m_SkinningMatrices.GetPtr()), m_SkinningMatrices.GetCount() * sizeof(ezMat4));
  }

  return pRenderData;
}

void ezSkinnedMeshComponent::CreateSkinningTransformBuffer(ezArrayPtr<const ezMat4> skinningMatrices)
{
  EZ_ASSERT_DEBUG(m_hSkinningTransformsBuffer.IsInvalidated(), "The skinning buffer should not exist at this time");

  ezGALBufferCreationDescription BufferDesc;
  BufferDesc.m_uiStructSize = sizeof(ezMat4);
  BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * skinningMatrices.GetCount();
  BufferDesc.m_bUseAsStructuredBuffer = true;
  BufferDesc.m_bAllowShaderResourceView = true;
  BufferDesc.m_ResourceAccess.m_bImmutable = false;

  m_hSkinningTransformsBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(BufferDesc, skinningMatrices.ToByteArray());
}

void ezSkinnedMeshComponent::UpdateSkinningTransformBuffer(ezArrayPtr<const ezMat4> skinningMatrices)
{
  ezArrayPtr<ezMat4> pRenderMatrices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezMat4, skinningMatrices.GetCount());
  pRenderMatrices.CopyFrom(skinningMatrices);

  m_SkinningMatrices = pRenderMatrices;
}
