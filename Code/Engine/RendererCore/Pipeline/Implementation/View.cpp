#include <RendererCore/RendererCorePCH.h>

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
  m_sName.Assign(sName);

  ezStringBuilder sb = sName;
  sb.Append(".ExtractData");
  m_pExtractTask->ConfigureTask(sb, ezTaskNesting::Maybe);
}

void ezView::SetWorld(ezWorld* pWorld)
{
  if (m_pWorld != pWorld)
  {
    m_pWorld = pWorld;

    ezRenderWorld::ResetRenderDataCache(*this);
  }
}

void ezView::SetSwapChain(ezGALSwapChainHandle hSwapChain)
{
  if (m_Data.m_hSwapChain != hSwapChain)
  {
    // Swap chain and render target setup are mutually exclusive.
    m_Data.m_hSwapChain = hSwapChain;
    m_Data.m_renderTargets = ezGALRenderTargets();
    if (m_pRenderPipeline)
    {
      ezRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
    }
  }
}

void ezView::SetRenderTargets(const ezGALRenderTargets& renderTargets)
{
  if (m_Data.m_renderTargets != renderTargets)
  {
    // Swap chain and render target setup are mutually exclusive.
    m_Data.m_hSwapChain = ezGALSwapChainHandle();
    m_Data.m_renderTargets = renderTargets;
    if (m_pRenderPipeline)
    {
      ezRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
    }
  }
}

const ezGALRenderTargets& ezView::GetActiveRenderTargets() const
{
  if (const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(m_Data.m_hSwapChain))
  {
    return pSwapChain->GetRenderTargets();
  }
  return m_Data.m_renderTargets;
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

  ezRenderWorldExtractionEvent extractionEvent;
  extractionEvent.m_Type = ezRenderWorldExtractionEvent::Type::BeforeViewExtraction;
  extractionEvent.m_pView = this;
  extractionEvent.m_uiFrameCounter = ezRenderWorld::GetFrameCounter();
  ezRenderWorld::s_ExtractionEvent.Broadcast(extractionEvent);


  m_pRenderPipeline->m_sName = m_sName;
  m_pRenderPipeline->ExtractData(*this);

  extractionEvent.m_Type = ezRenderWorldExtractionEvent::Type::AfterViewExtraction;
  ezRenderWorld::s_ExtractionEvent.Broadcast(extractionEvent);
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

void ezView::SetRenderPassProperty(const char* szPassName, const char* szPropertyName, const ezVariant& value)
{
  SetProperty(m_PassProperties, szPassName, szPropertyName, value);
}

void ezView::SetExtractorProperty(const char* szPassName, const char* szPropertyName, const ezVariant& value)
{
  SetProperty(m_ExtractorProperties, szPassName, szPropertyName, value);
}

void ezView::ResetRenderPassProperties()
{
  for (auto it : m_PassProperties)
  {
    auto& prop = it.Value();
    if (prop.m_bIsValid)
    {
      prop.m_CurrentValue = prop.m_DefaultValue;
      prop.m_bIsDirty;
    }
  }
}

void ezView::ResetExtractorProperties()
{
  for (auto it : m_ExtractorProperties)
  {
    auto& prop = it.Value();
    if (prop.m_bIsValid)
    {
      prop.m_CurrentValue = prop.m_DefaultValue;
      prop.m_bIsDirty;
    }
  }
}

void ezView::SetRenderPassReadBackProperty(const char* szPassName, const char* szPropertyName, const ezVariant& value)
{
  SetReadBackProperty(m_PassReadBackProperties, szPassName, szPropertyName, value);
}

ezVariant ezView::GetRenderPassReadBackProperty(const char* szPassName, const char* szPropertyName)
{
  ezStringBuilder sKey(szPassName, "::", szPropertyName);

  auto it = m_PassReadBackProperties.Find(sKey);
  if (it.IsValid())
  {
    return it.Value().m_CurrentValue;
  }

  ezLog::Warning("Unknown read-back property '{0}::{1}'", szPassName, szPropertyName);
  return ezVariant();
}


bool ezView::IsRenderPassReadBackPropertyExisting(const char* szPassName, const char* szPropertyName) const
{
  ezStringBuilder sKey(szPassName, "::", szPropertyName);

  auto it = m_PassReadBackProperties.Find(sKey);
  return it.IsValid();
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
      ezRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());

      m_bPermutationVarsDirty = true;
      ResetAllPropertyStates(m_PassProperties);
      ResetAllPropertyStates(m_ExtractorProperties);
    }

    ApplyPermutationVars();
    ApplyRenderPassProperties();
    ApplyExtractorProperties();
  }
}

void ezView::ApplyPermutationVars()
{
  if (!m_bPermutationVarsDirty)
    return;

  m_pRenderPipeline->m_PermutationVars = m_PermutationVars;
  m_bPermutationVarsDirty = false;
}

void ezView::SetProperty(ezMap<ezString, PropertyValue>& map, const char* szPassName, const char* szPropertyName, const ezVariant& value)
{
  ezStringBuilder sKey(szPassName, "::", szPropertyName);

  bool bExisted = false;
  auto& prop = map.FindOrAdd(sKey, &bExisted).Value();

  if (!bExisted)
  {
    prop.m_sObjectName = szPassName;
    prop.m_sPropertyName = szPropertyName;
    prop.m_bIsValid = true;
  }

  prop.m_bIsDirty = true;
  prop.m_CurrentValue = value;
}


void ezView::SetReadBackProperty(ezMap<ezString, PropertyValue>& map, const char* szPassName, const char* szPropertyName, const ezVariant& value)
{
  ezStringBuilder sKey(szPassName, "::", szPropertyName);

  bool bExisted = false;
  auto& prop = map.FindOrAdd(sKey, &bExisted).Value();

  if (!bExisted)
  {
    prop.m_sObjectName = szPassName;
    prop.m_sPropertyName = szPropertyName;
    prop.m_bIsValid = true;
  }

  prop.m_bIsDirty = false;
  prop.m_CurrentValue = value;
}

void ezView::ReadBackPassProperties()
{
  ezHybridArray<ezRenderPipelinePass*, 16> passes;
  m_pRenderPipeline->GetPasses(passes);

  for (auto pPass : passes)
  {
    pPass->ReadBackProperties(this);
  }
}

void ezView::ResetAllPropertyStates(ezMap<ezString, PropertyValue>& map)
{
  for (auto it = map.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_bIsDirty = true;
    it.Value().m_bIsValid = true;
  }
}

void ezView::ApplyRenderPassProperties()
{
  for (auto it = m_PassProperties.GetIterator(); it.IsValid(); ++it)
  {
    auto& propertyValue = it.Value();

    if (!propertyValue.m_bIsValid || !propertyValue.m_bIsDirty)
      continue;

    propertyValue.m_bIsDirty = false;

    ezReflectedClass* pObject = nullptr;
    const char* szDot = propertyValue.m_sObjectName.FindSubString(".");
    if (szDot != nullptr)
    {
      EZ_REPORT_FAILURE("Setting renderer properties is not possible anymore");
    }
    else
    {
      pObject = m_pRenderPipeline->GetPassByName(propertyValue.m_sObjectName);
    }

    if (pObject == nullptr)
    {
      ezLog::Error("The render pass '{0}' does not exist. Property '{1}' cannot be applied.", propertyValue.m_sObjectName, propertyValue.m_sPropertyName);

      propertyValue.m_bIsValid = false;
      continue;
    }

    ApplyProperty(pObject, propertyValue, "render pass");
  }
}

void ezView::ApplyExtractorProperties()
{
  for (auto it = m_ExtractorProperties.GetIterator(); it.IsValid(); ++it)
  {
    if (!it.Value().m_bIsValid || !it.Value().m_bIsDirty)
      continue;

    it.Value().m_bIsDirty = false;

    ezExtractor* pExtractor = m_pRenderPipeline->GetExtractorByName(it.Value().m_sObjectName);
    if (pExtractor == nullptr)
    {
      ezLog::Error("The extractor '{0}' does not exist. Property '{1}' cannot be applied.", it.Value().m_sObjectName, it.Value().m_sPropertyName);

      it.Value().m_bIsValid = false;
      continue;
    }

    ApplyProperty(pExtractor, it.Value(), "extractor");
  }
}

void ezView::ApplyProperty(ezReflectedClass* pObject, PropertyValue& data, const char* szTypeName)
{
  ezAbstractProperty* pAbstractProperty = pObject->GetDynamicRTTI()->FindPropertyByName(data.m_sPropertyName);
  if (pAbstractProperty == nullptr)
  {
    ezLog::Error("The {0} '{1}' does not have a property called '{2}', it cannot be applied.", szTypeName, data.m_sObjectName, data.m_sPropertyName);

    data.m_bIsValid = false;
    return;
  }

  if (pAbstractProperty->GetCategory() != ezPropertyCategory::Member)
  {
    ezLog::Error("The {0} property '{1}::{2}' is not a member property, it cannot be applied.", szTypeName, data.m_sObjectName, data.m_sPropertyName);

    data.m_bIsValid = false;
    return;
  }

  auto pMemberProperty = static_cast<ezAbstractMemberProperty*>(pAbstractProperty);
  if (data.m_DefaultValue.IsValid() == false)
  {
    data.m_DefaultValue = ezReflectionUtils::GetMemberPropertyValue(pMemberProperty, pObject);
  }

  ezReflectionUtils::SetMemberPropertyValue(pMemberProperty, pObject, data.m_CurrentValue);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_View);
