#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/SkinnedMeshRenderData.h>
#include <RendererCore/Pipeline/RenderDataManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkinnedMeshRenderData, 1, ezRTTIDefaultAllocator<ezSkinnedMeshRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool ezSkinnedMeshRenderData::CanBatch(const ezRenderData& other0) const
{
  const auto& other = ezStaticCast<const ezSkinnedMeshRenderData&>(other0);

  return m_hSkinningBuffer == other.m_hSkinningBuffer && SUPER::CanBatch(other0);
}

//////////////////////////////////////////////////////////////////////////

ezSkinningState::ezSkinningState() = default;

ezSkinningState::~ezSkinningState()
{
  Clear();
}

void ezSkinningState::Clear()
{
  if (m_pWorld == nullptr)
    return;

  if (auto pRenderDataManager = m_pWorld->GetModule<ezRenderDataManager>())
  {
    pRenderDataManager->DeleteSkinningData(m_DataOffset);
  }

  m_uiNumBones = 0;
  m_pWorld = nullptr;
}

ezArrayPtr<ezShaderTransform> ezSkinningState::GetOrCreateBoneTransformsForWriting(ezComponent& ref_ownerComponent, ezUInt32 uiNumBones)
{
  EZ_ASSERT_DEV(ref_ownerComponent.HandlesMessage(ezMsgCustomInstanceDataOffsetChanged()), "Owner component must handle ezMsgCustomInstanceDataOffsetChanged.");

  auto pWorld = ref_ownerComponent.GetWorld();
  EZ_ASSERT_DEV(m_pWorld == nullptr || m_pWorld == pWorld, "ezSkinningState used with different worlds simultaneously, which is not supported.");
  m_pWorld = pWorld;

  auto pRenderDataManager = m_pWorld->GetModuleReadOnly<ezRenderDataManager>();

  if (m_uiNumBones > 0 && m_uiNumBones != uiNumBones)
  {
    pRenderDataManager->DeleteSkinningData(m_DataOffset);
  }
  m_uiNumBones = uiNumBones;

  return pRenderDataManager->GetOrCreateSkinningData(&ref_ownerComponent, m_DataOffset, uiNumBones);
}

ezArrayPtr<const ezShaderTransform> ezSkinningState::GetBoneTransformsForReading() const
{
  if (m_pWorld != nullptr && m_uiNumBones > 0)
  {
    if (auto pRenderDataManager = m_pWorld->GetModuleReadOnly<ezRenderDataManager>())
    {
      return pRenderDataManager->GetSkinningData(m_DataOffset);
    }
  }

  return ezArrayPtr<const ezShaderTransform>();
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_SkinnedMeshRenderData);
