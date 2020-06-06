#include <RmlUiPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRmlUiCanvas2DComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("RmlFile", GetRmlFile, SetRmlFile)->AddAttributes(new ezFileBrowserAttribute("Rml File", "*.rml")),
    EZ_ACCESSOR_PROPERTY("Offset", GetOffset, SetOffset)->AddAttributes(new ezDefaultValueAttribute(ezVec2::ZeroVector()), new ezSuffixAttribute("px")),
    EZ_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new ezDefaultValueAttribute(ezVec2U32(100)), new ezSuffixAttribute("px")),
    EZ_ACCESSOR_PROPERTY("PassInput", GetPassInput, SetPassInput)->AddAttributes(new ezDefaultValueAttribute(true)),
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

void ezRmlUiCanvas2DComponent::Initialize()
{
  SUPER::Initialize();

  ezStringBuilder sName = m_sRmlFile;
  sName = sName.GetFileName();

  m_pContext = ezRmlUi::GetSingleton()->CreateContext(sName, m_Size);
  m_pContext->LoadDocumentFromFile(m_sRmlFile);
  m_pContext->SetOffset(m_Offset);
}

void ezRmlUiCanvas2DComponent::Deinitialize()
{
  SUPER::Deinitialize();

  ezRmlUi::GetSingleton()->DeleteContext(m_pContext);
  m_pContext = nullptr;
}

void ezRmlUiCanvas2DComponent::OnActivated()
{
  SUPER::OnActivated();

  m_pContext->ShowDocument();
}

void ezRmlUiCanvas2DComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  m_pContext->HideDocument();
}

void ezRmlUiCanvas2DComponent::Update()
{
  if (m_pContext != nullptr)
  {
    float fViewWidth = 1.0f;
    float fViewHeight = 1.0f;

    if (ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView, GetWorld()))
    {
      fViewWidth = pView->GetViewport().width;
      fViewHeight = pView->GetViewport().height;
    }

    if (false)
    {
      float fScale = fViewHeight / 1000.0f;
      m_pContext->SetDpiScale(fScale);
    }

    if (m_bPassInput)
    {
      float mouseX, mouseY;
      ezInputManager::GetInputSlotState(ezInputSlot_MousePositionX, &mouseX);
      ezInputManager::GetInputSlotState(ezInputSlot_MousePositionY, &mouseY);

      ezVec2 mousePos = ezVec2(mouseX * fViewWidth, mouseY * fViewHeight) - ezVec2(m_Offset.x, m_Offset.y);
      m_pContext->UpdateInput(mousePos);
    }

    m_pContext->Update();
  }
}

void ezRmlUiCanvas2DComponent::SetRmlFile(const char* szFile)
{
  if (m_sRmlFile != szFile)
  {
    m_sRmlFile = szFile;

    if (m_pContext != nullptr)
    {
      m_pContext->LoadDocumentFromFile(m_sRmlFile);
    }
  }
}

void ezRmlUiCanvas2DComponent::SetOffset(const ezVec2I32& offset)
{
  if (m_Offset != offset)
  {
    m_Offset = offset;

    if (m_pContext != nullptr)
    {
      m_pContext->SetOffset(m_Offset);
    }
  }
}

void ezRmlUiCanvas2DComponent::SetSize(const ezVec2U32& size)
{
  if (m_Size != size)
  {
    m_Size = size;

    if (m_pContext != nullptr)
    {
      m_pContext->SetSize(m_Size);
    }
  }
}

void ezRmlUiCanvas2DComponent::SetPassInput(bool bPassInput)
{
  if (m_bPassInput != bPassInput)
  {
    m_bPassInput = bPassInput;
  }
}

void ezRmlUiCanvas2DComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_sRmlFile;
  s << m_Offset;
  s << m_Size;
  s << m_bPassInput;
}

void ezRmlUiCanvas2DComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_sRmlFile;
  s >> m_Offset;
  s >> m_Size;
  s >> m_bPassInput;
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
    ezRmlUi::GetSingleton()->ExtractContext(*m_pContext, msg);
  }
}
