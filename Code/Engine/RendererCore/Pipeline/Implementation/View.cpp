#include <PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Pipeline/Extractor.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezCameraUsageHint, 1)
  EZ_ENUM_CONSTANT(ezCameraUsageHint::None),
  EZ_ENUM_CONSTANT(ezCameraUsageHint::MainView),
  EZ_ENUM_CONSTANT(ezCameraUsageHint::EditorView),
  EZ_ENUM_CONSTANT(ezCameraUsageHint::Thumbnail),
EZ_END_STATIC_REFLECTED_ENUM();

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
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezView::ezView()
  : m_ExtractTask("", ezMakeDelegate(&ezView::ExtractData, this))
{
  m_pWorld = nullptr;
  m_pLogicCamera = nullptr;
  m_pRenderCamera = nullptr;

  m_uiLastCameraSettingsModification = 0;
  m_uiLastCameraOrientationModification = 0;
  m_fLastViewportAspectRatio = 1.0f;

  m_uiRenderPipelineResourceDescriptionCounter = 0;
}

ezView::~ezView()
{

}

void ezView::SetName(const char* szName)
{
  m_sName.Assign(szName);

  ezStringBuilder sb = szName;
  sb.Append(".ExtractData");
  m_ExtractTask.SetTaskName(sb);
}

void ezView::SetRenderTargetSetup(ezGALRenderTagetSetup& renderTargetSetup)
{
  m_RenderTargetSetup = renderTargetSetup;
  if (m_pRenderPipeline)
  {
    ezRenderLoop::AddRenderPipelineToRebuild(m_pRenderPipeline, this);
    m_pRenderPipeline->ResetPipelineState();
  }
}

void ezView::SetRenderPipelineResource(ezRenderPipelineResourceHandle hPipeline)
{
  if (hPipeline == m_hRenderPipeline)
  {
    return;
  }
  m_uiRenderPipelineResourceDescriptionCounter = 0;
  m_hRenderPipeline = hPipeline;
}

ezRenderPipelineResourceHandle ezView::GetRenderPipelineResource() const
{
  return m_hRenderPipeline;
}


ezEnum<ezCameraUsageHint> ezView::GetCameraUsageHint() const
{
  return m_CameraUsageHint;
}


void ezView::SetCameraUsageHint(ezEnum<ezCameraUsageHint> val)
{
  m_CameraUsageHint = val;
}

void ezView::EnsureUpToDate()
{
  EZ_ASSERT_DEBUG(m_hRenderPipeline.IsValid(), "Renderpipeline has not been set on this view");

  ezResourceLock<ezRenderPipelineResource> pPipeline(m_hRenderPipeline, ezResourceAcquireMode::NoFallback);

  ezUInt32 uiCounter = pPipeline->GetCurrentResourceChangeCounter();

  if (m_uiRenderPipelineResourceDescriptionCounter != uiCounter)
  {
    m_uiRenderPipelineResourceDescriptionCounter = uiCounter;

    m_pRenderPipeline = pPipeline->CreateRenderPipeline();
    ezRenderLoop::AddRenderPipelineToRebuild(m_pRenderPipeline, this);

    ezStringBuilder sb = m_sName.GetString();
    sb.Append(".RenderPipeline");
    m_pRenderPipeline->m_sName.Assign(sb.GetData());

    ResetAllPropertyStates(m_PassProperties);
    ResetAllPropertyStates(m_ExtractorProperties);
  }

  ApplyRenderPassProperties();
  ApplyExtractorProperties();
}

void ezView::ExtractData()
{
  EZ_ASSERT_DEV(IsValid(), "Cannot extract data from an invalid view");

  m_pRenderPipeline->ExtractData(*this);
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
  prop.m_Value = value;
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
  prop.m_Value = value;
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

void ezView::ApplyProperty(ezReflectedClass* pClass, PropertyValue &data, const char* szTypeName)
{
  ezAbstractProperty* pAbstractProperty = pClass->GetDynamicRTTI()->FindPropertyByName(data.m_sPropertyName);
  if (pAbstractProperty == nullptr)
  {
    ezLog::Error("The {0} '{1}' does not have a property called '{2}', it cannot be applied.", szTypeName, data.m_sObjectName.GetData(), data.m_sPropertyName.GetData());

    data.m_bIsValid = false;
    return;
  }

  if (pAbstractProperty->GetCategory() != ezPropertyCategory::Member)
  {
    ezLog::Error("The {0} property '{1}::{2}' is not a member property, it cannot be applied.", szTypeName, data.m_sObjectName.GetData(), data.m_sPropertyName.GetData());

    data.m_bIsValid = false;
    return;
  }

  ezReflectionUtils::SetMemberPropertyValue(static_cast<ezAbstractMemberProperty*>(pAbstractProperty), pClass, data.m_Value);
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
      ezStringView sPassName(propertyValue.m_sObjectName.GetData(), szDot);
      ezRenderPipelinePass* pPass = m_pRenderPipeline->GetPassByName(sPassName);
      if (pPass)
        pObject = pPass->GetRendererByType(ezRTTI::FindTypeByName(szDot + 1));
    }
    else
    {
      pObject = m_pRenderPipeline->GetPassByName(propertyValue.m_sObjectName);
    }

    if (pObject == nullptr)
    {
      ezLog::Error("The render pass '{0}' does not exist. Property '{1}' cannot be applied.", propertyValue.m_sObjectName.GetData(), propertyValue.m_sPropertyName.GetData());

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
      ezLog::Error("The extractor '{0}' does not exist. Property '{1}' cannot be applied.", it.Value().m_sObjectName.GetData(), it.Value().m_sPropertyName.GetData());

      it.Value().m_bIsValid = false;
      continue;
    }

    ApplyProperty(pExtractor, it.Value(), "extractor");
  }
}

void ezView::SetRenderPassProperty(const char* szPassName, const char* szPropertyName, const ezVariant& value)
{
  SetProperty(m_PassProperties, szPassName, szPropertyName, value);
}

void ezView::SetExtractorProperty(const char* szPassName, const char* szPropertyName, const ezVariant& value)
{
  SetProperty(m_ExtractorProperties, szPassName, szPropertyName, value);
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
    return it.Value().m_Value;

  ezLog::Warning("Unknown read-back property '{0}::{1}'", szPassName, szPropertyName);
  return ezVariant();
}


bool ezView::IsRenderPassReadBackPropertyExisting(const char* szPassName, const char* szPropertyName) const
{
  ezStringBuilder sKey(szPassName, "::", szPropertyName);

  auto it = m_PassReadBackProperties.Find(sKey);
  return it.IsValid();
}

void ezView::UpdateCachedMatrices() const
{
  const ezCamera* pCamera = GetRenderCamera();

  bool bUpdateVP = false;

  if (m_uiLastCameraOrientationModification != pCamera->GetOrientationModificationCounter())
  {
    bUpdateVP = true;
    m_uiLastCameraOrientationModification = pCamera->GetOrientationModificationCounter();

    pCamera->GetViewMatrix(m_Data.m_ViewMatrix);
    m_Data.m_InverseViewMatrix = m_Data.m_ViewMatrix.GetInverse();
  }

  const float fViewportAspectRatio = m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height;
  if (m_uiLastCameraSettingsModification != pCamera->GetSettingsModificationCounter() ||
    m_fLastViewportAspectRatio != fViewportAspectRatio)
  {
    bUpdateVP = true;
    m_uiLastCameraSettingsModification = pCamera->GetSettingsModificationCounter();
    m_fLastViewportAspectRatio = fViewportAspectRatio;

    pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_Data.m_ProjectionMatrix);
    m_Data.m_InverseProjectionMatrix = m_Data.m_ProjectionMatrix.GetInverse();
  }

  if (bUpdateVP)
  {
    m_Data.m_ViewProjectionMatrix = m_Data.m_ProjectionMatrix * m_Data.m_ViewMatrix;
    m_Data.m_InverseViewProjectionMatrix = m_Data.m_ViewProjectionMatrix.GetInverse();
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_View);

