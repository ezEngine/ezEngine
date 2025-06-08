#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <GameComponentsPlugin/Effects/MeshDecalComponent.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <RendererCore/Decals/Implementation/DecalManager.h>
#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/Texture2DResource.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezMeshDecalDescription, ezNoBase, 1, ezRTTIDefaultAllocator<ezMeshDecalDescription>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Index", m_uiIndex)->AddAttributes(new ezClampValueAttribute(0, 7)),
    EZ_RESOURCE_MEMBER_PROPERTY("BaseColorTexture", m_hBaseColorTexture)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezResult ezMeshDecalDescription::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_uiIndex;
  inout_stream << m_hBaseColorTexture;

  return EZ_SUCCESS;
}

ezResult ezMeshDecalDescription::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_uiIndex;
  inout_stream >> m_hBaseColorTexture;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////

ezMeshDecalComponentManager::ezMeshDecalComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezMeshDecalComponent, ezBlockStorageType::Compact>(pWorld)
{
}

ezMeshDecalComponentManager::~ezMeshDecalComponentManager() = default;

void ezMeshDecalComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezMeshDecalComponentManager::UpdateDecalVisibility, this);
  desc.m_Phase = ezWorldUpdatePhase::Async;

  this->RegisterUpdateFunction(desc);
}

void ezMeshDecalComponentManager::UpdateDecalVisibility(const ezWorldModule::UpdateContext& context)
{
  for (auto it = m_DecalUsageInfos.GetIterator(); it.IsValid(); ++it)
  {
    auto& usageInfo = it.Value();

    const float fScreenSpaceSize = ezMath::ColorShortToFloat(usageInfo.m_iMaxScreenSpaceSize);
    ezDecalManager::GetOrAddRuntimeDecal(it.Key(), fScreenSpaceSize, nullptr);

    usageInfo.m_iMaxScreenSpaceSize = ezMath::ColorFloatToShort(0.01f);
  }
}

void ezMeshDecalComponentManager::RegisterDecalUsage(const ezTexture2DResourceHandle& hDecal)
{
  if (hDecal.IsValid())
  {
    DecalUsageInfo& usageInfo = m_DecalUsageInfos[hDecal];
    usageInfo.m_iRefCount.Increment();
  }
}

void ezMeshDecalComponentManager::UnregisterDecalUsage(const ezTexture2DResourceHandle& hDecal)
{
  if (hDecal.IsValid())
  {
    DecalUsageInfo& usageInfo = m_DecalUsageInfos[hDecal];
    EZ_ASSERT_DEBUG(usageInfo.m_iRefCount > 0, "Unregistering a decal usage that was never registered.");
    if (usageInfo.m_iRefCount.Decrement() == 0)
    {
      m_DecalUsageInfos.Remove(hDecal);
    }
  }
}

void ezMeshDecalComponentManager::UpdateMaxScreenSpaceSize(const ezTexture2DResourceHandle& hDecal, float fMaxScreenSpaceSize) const
{
  if (hDecal.IsValid())
  {
    if (const DecalUsageInfo* pUsageInfo = m_DecalUsageInfos.GetValue(hDecal))
    {
      int iMaxScreenSpaceSize = ezMath::ColorFloatToShort(fMaxScreenSpaceSize);
      pUsageInfo->m_iMaxScreenSpaceSize.Max(iMaxScreenSpaceSize);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezMeshDecalComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_ACCESSOR_PROPERTY("Decals", Decals_GetCount, Decals_Get, Decals_Set, Decals_Insert, Decals_Remove),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

void ezMeshDecalComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s.WriteArray(m_DecalDescs).AssertSuccess();
}

void ezMeshDecalComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = inout_stream.GetStream();

  s.ReadArray(m_DecalDescs).AssertSuccess();
}

void ezMeshDecalComponent::OnActivated()
{
  SUPER::OnActivated();

  auto pManager = static_cast<ezMeshDecalComponentManager*>(GetOwningManager());
  for (auto& desc : m_DecalDescs)
  {
    pManager->RegisterDecalUsage(desc.m_hBaseColorTexture);
  }

  UpdateDecalIds();
}

void ezMeshDecalComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  auto pManager = static_cast<ezMeshDecalComponentManager*>(GetOwningManager());
  for (auto& desc : m_DecalDescs)
  {
    pManager->UnregisterDecalUsage(desc.m_hBaseColorTexture);
  }

  m_ActiveRuntimeDecals.Clear();
}

void ezMeshDecalComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  const float fScreenSpaceSize = ezLightComponent::CalculateScreenSpaceSize(GetOwner()->GetGlobalBounds().GetSphere(), *msg.m_pView->GetCullingCamera());

  auto pManager = static_cast<const ezMeshDecalComponentManager*>(GetOwningManager());
  for (auto& hTexture : m_ActiveRuntimeDecals)
  {
    pManager->UpdateMaxScreenSpaceSize(hTexture, fScreenSpaceSize);
  }
}

ezUInt32 ezMeshDecalComponent::Decals_GetCount() const
{
  return m_DecalDescs.GetCount();
}

const ezMeshDecalDescription& ezMeshDecalComponent::Decals_Get(ezUInt32 uiIndex) const
{
  return m_DecalDescs[uiIndex];
}

void ezMeshDecalComponent::Decals_Set(ezUInt32 uiIndex, const ezMeshDecalDescription& desc)
{
  auto pManager = static_cast<ezMeshDecalComponentManager*>(GetOwningManager());
  pManager->UnregisterDecalUsage(m_DecalDescs[uiIndex].m_hBaseColorTexture);

  m_DecalDescs[uiIndex] = desc;

  pManager->RegisterDecalUsage(desc.m_hBaseColorTexture);

  UpdateDecalIds();
}

void ezMeshDecalComponent::Decals_Insert(ezUInt32 uiIndex, const ezMeshDecalDescription& desc)
{
  auto pManager = static_cast<ezMeshDecalComponentManager*>(GetOwningManager());
  pManager->RegisterDecalUsage(desc.m_hBaseColorTexture);

  m_DecalDescs.InsertAt(uiIndex, desc);

  UpdateDecalIds();
}

void ezMeshDecalComponent::Decals_Remove(ezUInt32 uiIndex)
{
  auto pManager = static_cast<ezMeshDecalComponentManager*>(GetOwningManager());
  pManager->UnregisterDecalUsage(m_DecalDescs[uiIndex].m_hBaseColorTexture);

  m_DecalDescs.RemoveAtAndCopy(uiIndex);

  UpdateDecalIds();
}

void ezMeshDecalComponent::UpdateDecalIds()
{
  if (!IsActiveAndInitialized())
    return;

  m_ActiveRuntimeDecals.Clear();

  ezArrayMap<ezUInt32, ezTexture2DResourceHandle> indexToTexture(ezFrameAllocator::GetCurrentAllocator());
  indexToTexture.Reserve(m_DecalDescs.GetCount());

  for (auto& desc : m_DecalDescs)
  {
    indexToTexture.Insert(desc.m_uiIndex, desc.m_hBaseColorTexture);
  }

  ezUInt32 uiRandomSeed = GetOwner()->GetStableRandomSeed();
  int iRandomPos = 0;
  auto RandomIndex = [uiRandomSeed, &iRandomPos](ezUInt32 uiMin, ezUInt32 uiMax)
  {
    ezUInt32 uiIndex = static_cast<ezUInt32>(ezSimdRandom::FloatMinMax(ezSimdVec4i(iRandomPos), ezSimdVec4f(float(uiMin)), ezSimdVec4f(float(uiMax)), ezSimdVec4u(uiRandomSeed)).x());
    iRandomPos++;
    return uiIndex;
  };

  ezUInt16 decalIndices[8] = {};
  for (ezUInt32 i = 0; i < 8; ++i)
  {
    decalIndices[i] = ezSmallInvalidIndex;

    const ezUInt32 uiLowerBound = indexToTexture.LowerBound(i);
    const ezUInt32 uiUpperBound = ezMath::Min(indexToTexture.UpperBound(i), indexToTexture.GetCount());
    
    if (uiLowerBound != ezInvalidIndex && uiUpperBound > uiLowerBound)
    {
      const ezUInt32 uiIndex = RandomIndex(uiLowerBound, uiUpperBound);
      auto& hTexture = indexToTexture.GetValue(uiIndex);
      if (hTexture.IsValid())
      {
        ezDecalId decalId = ezDecalManager::GetOrAddRuntimeDecal(hTexture, 0.0f, nullptr);
        decalIndices[i] = decalId.m_InstanceIndex;

        if (!m_ActiveRuntimeDecals.Contains(hTexture))
          m_ActiveRuntimeDecals.PushBack(hTexture);
      }
    }
  }

  const float* pDecalIndicesAsFloat = reinterpret_cast<const float*>(decalIndices);
  ezMsgSetCustomData msg;
  msg.m_fData0 = pDecalIndicesAsFloat[0];
  msg.m_fData1 = pDecalIndicesAsFloat[1];
  msg.m_fData2 = pDecalIndicesAsFloat[2];
  msg.m_fData3 = pDecalIndicesAsFloat[3];

  GetOwner()->PostMessage(msg, ezTime::MakeZero(), ezObjectMsgQueueType::AfterInitialized);
}
