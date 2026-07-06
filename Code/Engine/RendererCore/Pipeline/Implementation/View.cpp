#include <RendererCore/RendererCorePCH.h>

#include <Core/Utils/Blackboard.h>
#include <Core/World/World.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezCameraUsageHint, 1)
  EZ_ENUM_CONSTANT(ezCameraUsageHint::None),
  EZ_ENUM_CONSTANT(ezCameraUsageHint::MainView),
  EZ_ENUM_CONSTANT(ezCameraUsageHint::EditorView),
  EZ_ENUM_CONSTANT(ezCameraUsageHint::RenderTarget),
  EZ_ENUM_CONSTANT(ezCameraUsageHint::Culling),
  EZ_ENUM_CONSTANT(ezCameraUsageHint::Thumbnail),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezView, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RenderTarget0", m_PinRenderTarget0),
    EZ_MEMBER_PROPERTY("RenderTarget1", m_PinRenderTarget1),
    EZ_MEMBER_PROPERTY("RenderTarget2", m_PinRenderTarget2),
    EZ_MEMBER_PROPERTY("RenderTarget3", m_PinRenderTarget3),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezView::ezView()
{
  m_pExtractTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "", ezTaskNesting::Never, ezMakeDelegate(&ezView::ExtractData, this));
}

ezView::~ezView() = default;

void ezView::SetName(ezStringView sName)
{
  m_Data.m_sName.Assign(sName);

  ezStringBuilder sb = sName;
  sb.Append(".ExtractData");
  m_pExtractTask->ConfigureTask(sb, ezTaskNesting::Maybe);
}

void ezView::SetWorld(ezWorld* pWorld)
{
  if (m_pWorld != pWorld)
  {
    m_pWorld = pWorld;
    m_Data.m_uiSkyIrradianceIndex = pWorld == nullptr ? 0 : pWorld->GetIndex();
    ezRenderWorld::ResetRenderDataCache(*this);

    m_pWorldBlackboard = pWorld != nullptr ? pWorld->GetBlackboard() : nullptr;
    m_BlackboardChangeCounter[SourceBlackboard::World] = {};
  }
}

void ezView::SetSwapChain(ezGALSwapChainHandle hSwapChain)
{
  if (m_Data.m_hSwapChain != hSwapChain)
  {
    // Swap chain and render target setup are mutually exclusive.
    m_Data.m_hSwapChain = hSwapChain;
    m_Data.m_RenderTargets = ezGALRenderTargets();
    if (m_pRenderPipeline)
    {
      ezRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
    }
  }
}

void ezView::SetRenderTargets(const ezGALRenderTargets& renderTargets)
{
  if (m_Data.m_RenderTargets != renderTargets)
  {
    // Swap chain and render target setup are mutually exclusive.
    m_Data.m_hSwapChain = ezGALSwapChainHandle();
    m_Data.m_RenderTargets = renderTargets;
    if (m_pRenderPipeline)
    {
      ezRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
    }
  }
}

const ezGALRenderTargets& ezViewData::GetActiveRenderTargets() const
{
  if (const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain))
  {
    return pSwapChain->GetRenderTargets();
  }
  return m_RenderTargets;
}

const ezGALRenderTargets& ezView::GetActiveRenderTargets() const
{
  return m_Data.GetActiveRenderTargets();
}

void ezView::SetRenderPipelineResource(ezRenderPipelineResourceHandle hPipeline)
{
  if (hPipeline == m_hRenderPipeline)
  {
    return;
  }

  m_uiRenderPipelineResourceDescriptionCounter = 0;
  m_hRenderPipeline = hPipeline;

  if (m_pRenderPipeline == nullptr)
  {
    EnsureUpToDate();
  }
}

ezRenderPipelineResourceHandle ezView::GetRenderPipelineResource() const
{
  return m_hRenderPipeline;
}

void ezView::SetCameraUsageHint(ezEnum<ezCameraUsageHint> val)
{
  m_Data.m_CameraUsageHint = val;
}

void ezView::SetViewRenderMode(ezEnum<ezViewRenderMode> value)
{
  m_Data.m_ViewRenderMode = value;
}

void ezView::SetViewport(const ezRectFloat& viewport)
{
  m_Data.m_ViewPortRect = viewport;

  UpdateViewData(ezRenderWorld::GetDataIndexForExtraction());
}

void ezView::ForceUpdate()
{
  if (m_pRenderPipeline)
  {
    ezRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
  }
}

void ezView::ExtractData()
{
  EZ_ASSERT_DEV(IsValid(), "Cannot extract data from an invalid view");

  m_pRenderPipeline->m_sName = m_Data.m_sName;
  m_pRenderPipeline->ExtractData(*this);
}

void ezView::ComputeCullingFrustum(ezFrustum& out_frustum) const
{
  const ezCamera* pCamera = GetCullingCamera();
  const float fViewportAspectRatio = m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height;

  ezMat4 viewMatrix = pCamera->GetViewMatrix();

  ezMat4 projectionMatrix;
  pCamera->GetProjectionMatrix(fViewportAspectRatio, projectionMatrix);

  out_frustum = ezFrustum::MakeFromMVP(projectionMatrix * viewMatrix);
}

void ezView::SetShaderPermutationVariable(const char* szName, const char* szValue)
{
  ezHashedString sName;
  sName.Assign(szName);

  for (auto& var : m_PermutationVars)
  {
    if (var.m_sName == sName)
    {
      if (var.m_sValue != szValue)
      {
        var.m_sValue.Assign(szValue);
        m_bPermutationVarsDirty = true;
      }
      return;
    }
  }

  auto& var = m_PermutationVars.ExpandAndGetRef();
  var.m_sName = sName;
  var.m_sValue.Assign(szValue);
  m_bPermutationVarsDirty = true;
}

void ezView::SetBlackboard(const ezSharedPtr<ezBlackboard>& pBlackboard)
{
  if (m_pViewBlackboard != pBlackboard)
  {
    m_pViewBlackboard = pBlackboard;
    m_BlackboardChangeCounter[SourceBlackboard::View] = {};
  }
}

const ezSharedPtr<ezBlackboard>& ezView::GetBlackboard() const
{
  return m_pViewBlackboard;
}

void ezView::UpdateViewData(ezUInt32 uiDataIndex)
{
  if (m_pRenderPipeline != nullptr)
  {
    m_pRenderPipeline->UpdateViewData(*this, uiDataIndex);
  }
}

void ezView::UpdateCachedMatrices() const
{
  const ezCamera* pCamera = GetCamera();

  bool bUpdateVP = false;

  if (m_uiLastCameraOrientationModification != pCamera->GetOrientationModificationCounter())
  {
    bUpdateVP = true;
    m_uiLastCameraOrientationModification = pCamera->GetOrientationModificationCounter();

    m_Data.m_ViewMatrix[0] = pCamera->GetViewMatrix(ezCameraEye::Left);
    m_Data.m_ViewMatrix[1] = pCamera->GetViewMatrix(ezCameraEye::Right);

    // Some of our matrices contain very small values so that the matrix inversion will fall below the default epsilon.
    // We pass zero as epsilon here since all view and projection matrices are invertible.
    m_Data.m_InverseViewMatrix[0] = m_Data.m_ViewMatrix[0].GetInverse(0.0f);
    m_Data.m_InverseViewMatrix[1] = m_Data.m_ViewMatrix[1].GetInverse(0.0f);
  }

  const float fViewportAspectRatio = m_Data.m_ViewPortRect.HasNonZeroArea() ? m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height : 1.0f;
  if (m_uiLastCameraSettingsModification != pCamera->GetSettingsModificationCounter() || m_fLastViewportAspectRatio != fViewportAspectRatio)
  {
    bUpdateVP = true;
    m_uiLastCameraSettingsModification = pCamera->GetSettingsModificationCounter();
    m_fLastViewportAspectRatio = fViewportAspectRatio;


    pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_Data.m_ProjectionMatrix[0], ezCameraEye::Left);
    m_Data.m_InverseProjectionMatrix[0] = m_Data.m_ProjectionMatrix[0].GetInverse(0.0f);

    pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_Data.m_ProjectionMatrix[1], ezCameraEye::Right);
    m_Data.m_InverseProjectionMatrix[1] = m_Data.m_ProjectionMatrix[1].GetInverse(0.0f);
  }

  if (bUpdateVP)
  {
    for (int i = 0; i < 2; ++i)
    {
      m_Data.m_ViewProjectionMatrix[i] = m_Data.m_ProjectionMatrix[i] * m_Data.m_ViewMatrix[i];
      m_Data.m_InverseViewProjectionMatrix[i] = m_Data.m_ViewProjectionMatrix[i].GetInverse(0.0f);
    }
  }
}

void ezView::EnsureUpToDate()
{
  if (m_hRenderPipeline.IsValid())
  {
    ezResourceLock<ezRenderPipelineResource> pPipeline(m_hRenderPipeline, ezResourceAcquireMode::BlockTillLoaded);

    ezUInt32 uiCounter = pPipeline->GetCurrentResourceChangeCounter();

    if (m_uiRenderPipelineResourceDescriptionCounter != uiCounter)
    {
      m_uiRenderPipelineResourceDescriptionCounter = uiCounter;

      m_pRenderPipeline = pPipeline->CreateRenderPipeline();
      if (m_pRenderPipeline != nullptr)
      {
        ezRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
      }

      m_bPermutationVarsDirty = true;

      // Re-evaluate all blackboards since the render pipeline has changed and the property mappings are not valid anymore.
      m_BlackboardChangeCounter[SourceBlackboard::World] = {};
      m_BlackboardChangeCounter[SourceBlackboard::View] = {};
    }

    ApplyPermutationVars();
    ApplyPropertiesFromBlackboard();
  }
}

void ezView::ReadBackPassProperties()
{
  EZ_PROFILE_SCOPE("ViewReadBackPassProperties");

  ezTempHybridArray<ezRenderPipelinePass*, 32> passes;
  m_pRenderPipeline->GetPasses(passes);

  for (auto pPass : passes)
  {
    EZ_PROFILE_SCOPE(pPass->GetName());

    pPass->ReadBackProperties(this);
  }
}

void ezView::ApplyPermutationVars()
{
  if (!m_bPermutationVarsDirty)
    return;

  if (m_pRenderPipeline == nullptr)
    return;

  m_pRenderPipeline->m_PermutationVars = m_PermutationVars;
  m_bPermutationVarsDirty = false;
}

void ezView::ApplyPropertiesFromBlackboard()
{
  if (m_pRenderPipeline == nullptr)
    return;

  const ezBlackboard* pBlackboards[SourceBlackboard::COUNT];
  pBlackboards[SourceBlackboard::World] = m_pWorldBlackboard.Borrow();
  pBlackboards[SourceBlackboard::View] = m_pViewBlackboard.Borrow();

  bool bAnyStructureChanged = false;
  bool bAnyValuesChanged = false;

  static_assert(EZ_ARRAY_SIZE(m_BlackboardChangeCounter) == EZ_ARRAY_SIZE(pBlackboards));
  for (ezUInt32 i = EZ_ARRAY_SIZE(pBlackboards); i-- > 0;)
  {
    const ezBlackboard* pBlackboard = pBlackboards[i];
    if (pBlackboard == nullptr)
      continue;

    auto& changeCounter = m_BlackboardChangeCounter[i];

    const bool bStructureChanged = changeCounter.m_uiStructure != pBlackboard->GetBlackboardChangeCounter();
    if (bStructureChanged)
    {
      UpdatePropertyMappings(*pBlackboard, i);

      changeCounter.m_uiStructure = pBlackboard->GetBlackboardChangeCounter();
    }

    bAnyStructureChanged |= bStructureChanged;
    bAnyValuesChanged |= changeCounter.m_uiValue != pBlackboard->GetBlackboardEntryChangeCounter();

    changeCounter.m_uiValue = pBlackboard->GetBlackboardEntryChangeCounter();
  }

  if (bAnyStructureChanged || bAnyValuesChanged)
  {
    for (auto& mapping : m_PropertyMappings)
    {
      const ezBlackboard* pBlackboard = pBlackboards[static_cast<ezUInt32>(mapping.Value().m_SourceIndex)];
      EZ_ASSERT_DEBUG(pBlackboard != nullptr, "Property mapping has invalid source index.");

      auto pEntry = pBlackboard->GetEntry(mapping.Key());
      if (pEntry != nullptr && (mapping.Value().m_uiChangeCounter != pEntry->m_uiChangeCounter || bAnyStructureChanged))
      {
        ezReflectionUtils::SetMemberPropertyValue(mapping.Value().m_pProperty, mapping.Value().m_pObject, pEntry->m_Value);
        mapping.Value().m_uiChangeCounter = pEntry->m_uiChangeCounter;
      }
    }
  }
}

void ezView::UpdatePropertyMappings(const ezBlackboard& blackboard, ezUInt32 uiSourceIndex)
{
  EZ_ASSERT_DEV(m_pRenderPipeline != nullptr, "Can only update mappings with a valid render pipeline");

  // Reset the property value to the default value if the blackboard entry was removed
  ezHybridArray<ezHashedString, 32> keysToRemove;
  for (auto it = m_PropertyMappings.GetIterator(); it.IsValid(); ++it)
  {
    auto& mapping = it.Value();
    if (mapping.m_SourceIndex == uiSourceIndex && blackboard.GetEntry(it.Key()) == nullptr)
    {
      ezReflectionUtils::SetMemberPropertyValue(mapping.m_pProperty, mapping.m_pObject, mapping.m_DefaultValue);
      keysToRemove.PushBack(it.Key());
    }
  }

  // Remove old mappings
  for (auto& key : keysToRemove)
  {
    m_PropertyMappings.Remove(key);
  }

  // Add or update mappings
  for (auto it = blackboard.GetAllEntries().GetIterator(); it.IsValid(); ++it)
  {
    const ezStringView sName = it.Key();
    const char* szDot = sName.FindSubString(".");
    if (szDot == nullptr)
      continue;

    const ezStringView sObjectName = ezStringView(sName.GetStartPointer(), szDot);

    ezReflectedClass* pObject = m_pRenderPipeline->GetPassByName(sObjectName);
    if (pObject == nullptr)
    {
      pObject = m_pRenderPipeline->GetExtractorByName(sObjectName);
    }

    if (pObject == nullptr)
      continue;

    const ezStringView sPropertyName = ezStringView(szDot + 1, sName.GetEndPointer());
    const ezAbstractProperty* pAbstractProperty = pObject->GetDynamicRTTI()->FindPropertyByName(sPropertyName);
    if (pAbstractProperty == nullptr || pAbstractProperty->GetCategory() != ezPropertyCategory::Member)
      continue;

    auto& mapping = m_PropertyMappings[it.Key()];
    mapping.m_pObject = pObject;
    mapping.m_pProperty = static_cast<const ezAbstractMemberProperty*>(pAbstractProperty);
    mapping.m_uiChangeCounter = 0;
    mapping.m_SourceIndex = ezMath::Max(mapping.m_SourceIndex, static_cast<SourceBlackboard>(uiSourceIndex));
    mapping.m_DefaultValue = ezReflectionUtils::GetMemberPropertyValue(mapping.m_pProperty, mapping.m_pObject);
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_View);
