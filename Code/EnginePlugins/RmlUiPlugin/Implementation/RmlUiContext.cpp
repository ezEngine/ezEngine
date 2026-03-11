#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Implementation/RenderInterface.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>


ezRmlUiContext::ezRmlUiContext(const Rml::String& sName, Rml::RenderManager* pRenderManager, Rml::TextInputHandler* pTextInputHandler)
  : Rml::Context(sName, pRenderManager, pTextInputHandler)
{
}

ezRmlUiContext::~ezRmlUiContext() = default;

ezResult ezRmlUiContext::LoadDocumentFromResource(const ezRmlUiResourceHandle& hResource)
{
  return ezRmlUi::GetSingleton()->LoadDocumentFromResource(*this, hResource);
}

ezResult ezRmlUiContext::LoadDocumentFromString(const ezStringView& sContent)
{
  return ezRmlUi::GetSingleton()->LoadDocumentFromString(*this, sContent);
}

void ezRmlUiContext::UnloadDocument()
{
  ezRmlUi::GetSingleton()->UnloadDocument(*this);
}

ezResult ezRmlUiContext::ReloadDocumentFromResource(const ezRmlUiResourceHandle& hResource)
{
  ezRmlUi::GetSingleton()->ClearCaches();

  EZ_SUCCEED_OR_RETURN(LoadDocumentFromResource(hResource));

  RequestNextUpdate(0.0);

  return EZ_SUCCESS;
}

void ezRmlUiContext::ShowDocument()
{
  if (HasDocument())
  {
    EZ_LOCK(ezRmlUi::GetSingleton()->GetContextMutex());
    GetDocument(0)->Show();
  }
}

void ezRmlUiContext::HideDocument()
{
  if (HasDocument())
  {
    EZ_LOCK(ezRmlUi::GetSingleton()->GetContextMutex());
    GetDocument(0)->Hide();
  }

  m_bWantsInput = false;
}

bool ezRmlUiContext::UpdateInput(const ezVec2& vMousePos, const ezRmlUiInputProvider& input)
{
  bool bMouseInputConsumed = false;
  bool bKeyboardInputConsumed = false;

  int modifierState = 0;
  modifierState |= input.IsButtonDown(ezRmlUiInputButtons::Alt) ? Rml::Input::KM_ALT : 0;
  modifierState |= input.IsButtonDown(ezRmlUiInputButtons::Ctrl) ? Rml::Input::KM_CTRL : 0;
  modifierState |= input.IsButtonDown(ezRmlUiInputButtons::Shift) ? Rml::Input::KM_SHIFT : 0;

  // Mouse
  {
    bMouseInputConsumed |= !ProcessMouseMove(static_cast<int>(vMousePos.x), static_cast<int>(vMousePos.y), modifierState);

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(ezRmlUiInputButtons::s_MouseButtonMappings); ++i)
    {
      ezRmlUiInputButtons::MouseButtonMapping mbm = ezRmlUiInputButtons::s_MouseButtonMappings[i];
      ezKeyState::Enum state = input.GetButtonState(mbm.uiEzButton);
      if (state == ezKeyState::Pressed)
      {
        bMouseInputConsumed |= !ProcessMouseButtonDown(mbm.uiRmlButton, modifierState);
      }
      else if (state == ezKeyState::Released)
      {
        bMouseInputConsumed |= !ProcessMouseButtonUp(mbm.uiRmlButton, modifierState);
      }
    }

    if (input.IsButtonDown(ezRmlUiInputButtons::MouseWheelDown))
    {
      bKeyboardInputConsumed |= !ProcessMouseWheel(1.0f, modifierState);
    }
    if (input.IsButtonDown(ezRmlUiInputButtons::MouseWheelUp))
    {
      bKeyboardInputConsumed |= !ProcessMouseWheel(-1.0f, modifierState);
    }
  }

  // Keyboard
  {
    ezUInt32 uiLastChar = input.m_uiLastCharacter;
    if (uiLastChar >= 32 || uiLastChar == '\n') // >= space (+ enter/return)
    {
      char szUtf8[8] = "";
      char* pChar = szUtf8;
      ezUnicodeUtils::EncodeUtf32ToUtf8(uiLastChar, pChar);
      if (!ezStringUtils::IsNullOrEmpty(szUtf8))
      {
        bKeyboardInputConsumed |= !ProcessTextInput(szUtf8);
      }
    }

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(ezRmlUiInputButtons::s_KeyMappings); ++i)
    {
      ezRmlUiInputButtons::KeyMapping km = ezRmlUiInputButtons::s_KeyMappings[i];
      ezKeyState::Enum state = input.GetButtonState(km.uiEzKey);
      if (state == ezKeyState::Pressed)
      {
        bKeyboardInputConsumed |= !ProcessKeyDown(km.uiRmlKey, modifierState);
      }
      else if (state == ezKeyState::Released)
      {
        bKeyboardInputConsumed |= !ProcessKeyUp(km.uiRmlKey, modifierState);
      }
    }
  }

  m_bWantsInput = bMouseInputConsumed || bKeyboardInputConsumed;

  return bMouseInputConsumed || bKeyboardInputConsumed;
}

void ezRmlUiContext::SetSize(const ezVec2U32& vSize)
{
  SetDimensions(Rml::Vector2i(vSize.x, vSize.y));
}

void ezRmlUiContext::SetDpiScale(float fScale)
{
  SetDensityIndependentPixelRatio(fScale);
}

void ezRmlUiContext::RegisterEventHandler(const char* szIdentifier, EventHandler handler)
{
  ezHashedString sIdentifier;
  sIdentifier.Assign(szIdentifier);

  m_EventHandler.Insert(sIdentifier, std::move(handler));
}

void ezRmlUiContext::DeregisterEventHandler(const char* szIdentifier)
{
  m_EventHandler.Remove(ezTempHashedString(szIdentifier));
}

void ezRmlUiContext::RegisterFallbackEventHandler(FallbackEventHandler handler)
{
  m_FallbackEventHandler = std::move(handler);
}

void ezRmlUiContext::DeregisterFallbackEventHandler()
{
  m_FallbackEventHandler.Invalidate();
}

void ezRmlUiContext::Update()
{
  EZ_LOCK(ezRmlUi::GetSingleton()->GetContextMutex());

  Rml::Context::Update();

  m_uiUpdatedFrame = ezRenderWorld::GetFrameCounter();
}

void ezRmlUiContext::ExtractRenderData(ezRmlUiInternal::RenderInterface& renderInterface, ezGALTextureHandle hTexture)
{
  if (m_uiExtractedFrame != m_uiUpdatedFrame)
  {
    ezHashedString sName;
    sName.Assign(ezRmlUiConversionUtils::ToStringView(GetName()));

    renderInterface.BeginExtraction(sName, hTexture);

    Render();

    renderInterface.EndExtraction();

    m_uiExtractedFrame = m_uiUpdatedFrame;
  }
}

void ezRmlUiContext::ProcessEvent(const ezHashedString& sIdentifier, Rml::Event& event)
{
  EventHandler* pEventHandler = nullptr;
  if (m_EventHandler.TryGetValue(sIdentifier, pEventHandler))
  {
    (*pEventHandler)(event);
  }
  else if (m_FallbackEventHandler.IsValid())
  {
    m_FallbackEventHandler(sIdentifier, event);
  }
}

//////////////////////////////////////////////////////////////////////////

Rml::ContextPtr ezRmlUiInternal::ContextInstancer::InstanceContext(const Rml::String& sName, Rml::RenderManager* pRenderManager, Rml::TextInputHandler* pTextInputHandler)
{
  return Rml::ContextPtr(EZ_DEFAULT_NEW(ezRmlUiContext, sName, pRenderManager, pTextInputHandler));
}

void ezRmlUiInternal::ContextInstancer::ReleaseContext(Rml::Context* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

void ezRmlUiInternal::ContextInstancer::Release()
{
  // nothing to do here
}
