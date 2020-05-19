#include <RmlUiPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRmlUiCanvas2DComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("RmlFile", GetRmlFile, SetRmlFile)->AddAttributes(new ezFileBrowserAttribute("Rml File", "*.rml")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData)
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("RmlUi"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezRmlUiCanvas2DComponent::ezRmlUiCanvas2DComponent() = default;
ezRmlUiCanvas2DComponent::~ezRmlUiCanvas2DComponent() = default;

void ezRmlUiCanvas2DComponent::OnActivated()
{
  SUPER::OnActivated();

  m_pContext = ezRmlUi::GetSingleton()->CreateContext();
  m_pContext->LoadDocumentFromFile(m_sRmlFile);
}

void ezRmlUiCanvas2DComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  ezRmlUi::GetSingleton()->DeleteContext(m_pContext);
  m_pContext = nullptr;
}

void ezRmlUiCanvas2DComponent::Update()
{
  if (m_pContext != nullptr)
  {
    m_pContext->Update();
  }
}

void ezRmlUiCanvas2DComponent::SetRmlFile(const char* szFile)
{
  if (m_sRmlFile != szFile)
  {
    m_sRmlFile = szFile;

    if (IsActiveAndInitialized())
    {
      m_pContext->LoadDocumentFromFile(m_sRmlFile);
    }
  }
}

const char* ezRmlUiCanvas2DComponent::GetRmlFile() const
{
  return m_sRmlFile;
}

void ezRmlUiCanvas2DComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_sRmlFile;
}

void ezRmlUiCanvas2DComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_sRmlFile;
}

ezResult ezRmlUiCanvas2DComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  bAlwaysVisible = true;
  return EZ_SUCCESS;
}

void ezRmlUiCanvas2DComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::MainView &&
      msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::EditorView)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
    return;

  if (m_pContext != nullptr)
  {
    m_pContext->ExtractRenderData(msg);
  }
}
