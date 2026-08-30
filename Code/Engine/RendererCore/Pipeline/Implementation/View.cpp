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
    m_bBlackboardMappingsDirty = true;
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
    m_bBlackboardMappingsDirty = true;
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
      m_PropertyMappings.Clear();
      m_bBlackboardMappingsDirty = true;
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
  bool blackboardValuesChanged[SourceBlackboard::COUNT] = {};

  static_assert(EZ_ARRAY_SIZE(m_BlackboardChangeCounter) == EZ_ARRAY_SIZE(pBlackboards));
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(pBlackboards); ++i)
  {
    const ezBlackboard* pBlackboard = pBlackboards[i];
    if (pBlackboard == nullptr)
      continue;

    ChangeCounter& changeCounter = m_BlackboardChangeCounter[i];

    bAnyStructureChanged |= changeCounter.m_uiStructure != pBlackboard->GetBlackboardChangeCounter();
    changeCounter.m_uiStructure = pBlackboard->GetBlackboardChangeCounter();

    blackboardValuesChanged[i] = changeCounter.m_uiValue != pBlackboard->GetBlackboardEntryChangeCounter();
    bAnyValuesChanged |= blackboardValuesChanged[i];

    changeCounter.m_uiValue = pBlackboard->GetBlackboardEntryChangeCounter();
  }

  // Adding or removing an entry can change which blackboard provides a mapping, so both have to be resolved together.
  bool bSwitchChanged = false;
  if (m_bBlackboardMappingsDirty || bAnyStructureChanged)
  {
    RebuildPropertyMappings(pBlackboards);
    bSwitchChanged = RebuildSwitchMappings(pBlackboards);

    m_bBlackboardMappingsDirty = false;
  }
  else if (bAnyValuesChanged)
  {
    UpdatePropertyMappings(blackboardValuesChanged);
    bSwitchChanged = UpdateSwitchValues(blackboardValuesChanged);
  }

  if (bSwitchChanged)
  {
    ezRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
  }
}

bool ezView::RebuildSwitchMappings(const ezBlackboard* const* pBlackboards)
{
  const ezArrayPtr<const ezRenderPipelinePassGraph::SwitchInfo> switches = m_pRenderPipeline->GetSwitches();
  m_SwitchMappings.SetCount(switches.GetCount());

  bool bSwitchChanged = false;
  for (ezUInt32 i = 0; i < switches.GetCount(); ++i)
  {
    SwitchMapping& mapping = m_SwitchMappings[i];
    mapping = {};

    // The view blackboard overrides the world blackboard when both provide the same entry.
    for (ezUInt32 uiSource = SourceBlackboard::COUNT; uiSource-- > 0;)
    {
      const ezBlackboard* pBlackboard = pBlackboards[uiSource];
      if (pBlackboard == nullptr)
        continue;

      if (const ezBlackboard::Entry* pEntry = pBlackboard->GetEntry(switches[i].m_sBlackboardProperty))
      {
        mapping.m_pEntry = pEntry;
        mapping.m_uiEntryChangeCounter = pEntry->m_uiChangeCounter;
        mapping.m_SourceIndex = static_cast<SourceBlackboard>(uiSource);
        break;
      }
    }

    if (mapping.m_pEntry != nullptr && mapping.m_pEntry->m_Value.CanConvertTo<ezInt32>())
    {
      bSwitchChanged |= m_pRenderPipeline->SetSwitchValue(i, mapping.m_pEntry->m_Value.ConvertTo<ezInt32>());
    }
    else
    {
      if (mapping.m_pEntry != nullptr)
      {
        ezLog::Warning("Blackboard entry '{}' for switch '{}' is not an integer.", switches[i].m_sBlackboardProperty, switches[i].m_pSwitch->GetName());
      }
      bSwitchChanged |= m_pRenderPipeline->SetSwitchToDefault(i);
    }
  }

  return bSwitchChanged;
}

bool ezView::UpdateSwitchValues(const bool* pBlackboardValuesChanged)
{
  bool bSwitchChanged = false;
  for (ezUInt32 i = 0; i < m_SwitchMappings.GetCount(); ++i)
  {
    SwitchMapping& mapping = m_SwitchMappings[i];
    if (mapping.m_pEntry == nullptr || !pBlackboardValuesChanged[mapping.m_SourceIndex] || mapping.m_uiEntryChangeCounter == mapping.m_pEntry->m_uiChangeCounter)
      continue;

    mapping.m_uiEntryChangeCounter = mapping.m_pEntry->m_uiChangeCounter;
    if (mapping.m_pEntry->m_Value.CanConvertTo<ezInt32>())
    {
      bSwitchChanged |= m_pRenderPipeline->SetSwitchValue(i, mapping.m_pEntry->m_Value.ConvertTo<ezInt32>());
    }
    else
    {
      bSwitchChanged |= m_pRenderPipeline->SetSwitchToDefault(i);
    }
  }

  return bSwitchChanged;
}

void ezView::RebuildPropertyMappings(const ezBlackboard* const* pBlackboards)
{
  EZ_ASSERT_DEV(m_pRenderPipeline != nullptr, "Can only update mappings with a valid render pipeline");

  for (auto it = m_PropertyMappings.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_pEntry = nullptr;
  }

  // Ascending priority, so that entries of the view blackboard replace those of the world blackboard.
  for (ezUInt32 uiSource = 0; uiSource < SourceBlackboard::COUNT; ++uiSource)
  {
    const ezBlackboard* pBlackboard = pBlackboards[uiSource];
    if (pBlackboard == nullptr)
      continue;

    for (auto it = pBlackboard->GetAllEntries().GetIterator(); it.IsValid(); ++it)
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

      bool bExisted = false;
      PropertyMapping& mapping = m_PropertyMappings.FindOrAdd(it.Key(), &bExisted);
      if (!bExisted)
      {
        mapping.m_pObject = pObject;
        mapping.m_pProperty = static_cast<const ezAbstractMemberProperty*>(pAbstractProperty);
        // Read before any blackboard value was applied, so that the property can be restored once no blackboard provides this entry anymore.
        mapping.m_DefaultValue = ezReflectionUtils::GetMemberPropertyValue(mapping.m_pProperty, mapping.m_pObject);
      }

      mapping.m_pEntry = &it.Value();
      mapping.m_uiEntryChangeCounter = it.Value().m_uiChangeCounter;
      mapping.m_SourceIndex = static_cast<SourceBlackboard>(uiSource);
    }
  }

  for (auto it = m_PropertyMappings.GetIterator(); it.IsValid();)
  {
    PropertyMapping& mapping = it.Value();

    if (mapping.m_pEntry != nullptr)
    {
      ezReflectionUtils::SetMemberPropertyValue(mapping.m_pProperty, mapping.m_pObject, mapping.m_pEntry->m_Value);
      ++it;
      continue;
    }

    ezReflectionUtils::SetMemberPropertyValue(mapping.m_pProperty, mapping.m_pObject, mapping.m_DefaultValue);
    it = m_PropertyMappings.Remove(it);
  }
}

void ezView::UpdatePropertyMappings(const bool* pBlackboardValuesChanged)
{
  for (auto it = m_PropertyMappings.GetIterator(); it.IsValid(); ++it)
  {
    PropertyMapping& mapping = it.Value();
    if (!pBlackboardValuesChanged[mapping.m_SourceIndex] || mapping.m_uiEntryChangeCounter == mapping.m_pEntry->m_uiChangeCounter)
      continue;

    mapping.m_uiEntryChangeCounter = mapping.m_pEntry->m_uiChangeCounter;
    ezReflectionUtils::SetMemberPropertyValue(mapping.m_pProperty, mapping.m_pObject, mapping.m_pEntry->m_Value);
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_View);
