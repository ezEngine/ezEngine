#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Implementation/Extractor.h>
#include <RmlUiPlugin/RmlUiContext.h>

namespace
{
  static const char* s_szEzKeys[] = {ezInputSlot_KeyTab, ezInputSlot_KeyLeft, ezInputSlot_KeyUp, ezInputSlot_KeyRight, ezInputSlot_KeyDown,
    ezInputSlot_KeyPageUp, ezInputSlot_KeyPageDown, ezInputSlot_KeyHome, ezInputSlot_KeyEnd, ezInputSlot_KeyDelete, ezInputSlot_KeyBackspace,
    ezInputSlot_KeyReturn, ezInputSlot_KeyNumpadEnter, ezInputSlot_KeyEscape};

  static Rml::Input::KeyIdentifier s_rmlKeys[] = {Rml::Input::KI_TAB, Rml::Input::KI_LEFT, Rml::Input::KI_UP,
    Rml::Input::KI_RIGHT, Rml::Input::KI_DOWN, Rml::Input::KI_PRIOR, Rml::Input::KI_NEXT, Rml::Input::KI_HOME,
    Rml::Input::KI_END, Rml::Input::KI_DELETE, Rml::Input::KI_BACK, Rml::Input::KI_RETURN, Rml::Input::KI_RETURN,
    Rml::Input::KI_ESCAPE};

  static_assert(EZ_ARRAY_SIZE(s_szEzKeys) == EZ_ARRAY_SIZE(s_rmlKeys));
} // namespace

ezRmlUiContext::ezRmlUiContext(const Rml::String& sName, Rml::RenderManager* render_manager, Rml::TextInputHandler* text_input_handler)
  : Rml::Context(sName, render_manager, text_input_handler)
{
}

ezRmlUiContext::~ezRmlUiContext() = default;

ezResult ezRmlUiContext::LoadDocumentFromResource(const ezRmlUiResourceHandle& hResource)
{
  UnloadDocument();

  if (hResource.IsValid())
  {
    ezResourceLock<ezRmlUiResource> pResource(hResource, ezResourceAcquireMode::BlockTillLoaded);
    if (pResource.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      LoadDocument(pResource->GetRmlFile().GetData());
    }
  }

  return HasDocument() ? EZ_SUCCESS : EZ_FAILURE;
}

ezResult ezRmlUiContext::LoadDocumentFromString(const ezStringView& sContent)
{
  UnloadDocument();

  if (!sContent.IsEmpty())
  {
    Rml::String sRmlContent = Rml::String(sContent.GetStartPointer(), sContent.GetElementCount());

    LoadDocumentFromMemory(sRmlContent);
  }

  return HasDocument() ? EZ_SUCCESS : EZ_FAILURE;
}

void ezRmlUiContext::UnloadDocument()
{
  if (HasDocument())
  {
    Rml::Context::UnloadDocument(GetDocument(0));
  }
}

ezResult ezRmlUiContext::ReloadDocumentFromResource(const ezRmlUiResourceHandle& hResource)
{
  Rml::Factory::ClearStyleSheetCache();
  Rml::Factory::ClearTemplateCache();

  return LoadDocumentFromResource(hResource);
}

void ezRmlUiContext::ShowDocument()
{
  if (HasDocument())
  {
    GetDocument(0)->Show();
  }
}

void ezRmlUiContext::HideDocument()
{
  if (HasDocument())
  {
    GetDocument(0)->Hide();
  }
}

void ezRmlUiContext::UpdateInput(const ezVec2& vMousePos)
{
  float width = static_cast<float>(GetDimensions().x);
  float height = static_cast<float>(GetDimensions().y);

  m_bWantsInput = vMousePos.x >= 0.0f && vMousePos.x <= width && vMousePos.y >= 0.0f && vMousePos.y <= height;

  const bool bCtrlPressed = ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftCtrl) >= ezKeyState::Pressed ||
                            ezInputManager::GetInputSlotState(ezInputSlot_KeyRightCtrl) >= ezKeyState::Pressed;
  const bool bShiftPressed = ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftShift) >= ezKeyState::Pressed ||
                             ezInputManager::GetInputSlotState(ezInputSlot_KeyRightShift) >= ezKeyState::Pressed;
  const bool bAltPressed = ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftAlt) >= ezKeyState::Pressed ||
                           ezInputManager::GetInputSlotState(ezInputSlot_KeyRightAlt) >= ezKeyState::Pressed;

  int modifierState = 0;
  modifierState |= bCtrlPressed ? Rml::Input::KM_CTRL : 0;
  modifierState |= bShiftPressed ? Rml::Input::KM_SHIFT : 0;
  modifierState |= bAltPressed ? Rml::Input::KM_ALT : 0;

  // Mouse
  {
    ProcessMouseMove(static_cast<int>(vMousePos.x), static_cast<int>(vMousePos.y), modifierState);

    static const char* szMouseButtons[] = {ezInputSlot_MouseButton0, ezInputSlot_MouseButton1, ezInputSlot_MouseButton2};
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(szMouseButtons); ++i)
    {
      ezKeyState::Enum state = ezInputManager::GetInputSlotState(szMouseButtons[i]);
      if (state == ezKeyState::Pressed)
      {
        ProcessMouseButtonDown(i, modifierState);
      }
      else if (state == ezKeyState::Released)
      {
        ProcessMouseButtonUp(i, modifierState);
      }
    }

    if (ezInputManager::GetInputSlotState(ezInputSlot_MouseWheelDown) == ezKeyState::Pressed)
    {
      m_bWantsInput |= !ProcessMouseWheel(1.0f, modifierState);
    }
    if (ezInputManager::GetInputSlotState(ezInputSlot_MouseWheelUp) == ezKeyState::Pressed)
    {
      m_bWantsInput |= !ProcessMouseWheel(-1.0f, modifierState);
    }
  }

  // Keyboard
  {
    ezUInt32 uiLastChar = ezInputManager::RetrieveLastCharacter(false);
    if (uiLastChar >= 32) // >= space
    {
      char szUtf8[8] = "";
      char* pChar = szUtf8;
      ezUnicodeUtils::EncodeUtf32ToUtf8(uiLastChar, pChar);
      if (!ezStringUtils::IsNullOrEmpty(szUtf8))
      {
        m_bWantsInput |= !ProcessTextInput(szUtf8);
      }
    }

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_szEzKeys); ++i)
    {
      ezKeyState::Enum state = ezInputManager::GetInputSlotState(s_szEzKeys[i]);
      if (state == ezKeyState::Pressed)
      {
        m_bWantsInput |= !ProcessKeyDown(s_rmlKeys[i], modifierState);
      }
      else if (state == ezKeyState::Released)
      {
        m_bWantsInput |= !ProcessKeyUp(s_rmlKeys[i], modifierState);
      }
    }
  }
}

void ezRmlUiContext::SetOffset(const ezVec2I32& vOffset)
{
  m_vOffset = vOffset;
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

void ezRmlUiContext::ExtractRenderData(ezRmlUiInternal::Extractor& extractor)
{
  if (m_uiExtractedFrame != ezRenderWorld::GetFrameCounter())
  {
    extractor.BeginExtraction(m_vOffset);

    Render();

    extractor.EndExtraction();

    m_uiExtractedFrame = ezRenderWorld::GetFrameCounter();
    m_pRenderData = extractor.GetRenderData();
  }
}

void ezRmlUiContext::ProcessEvent(const ezHashedString& sIdentifier, Rml::Event& event)
{
  EventHandler* pEventHandler = nullptr;
  if (m_EventHandler.TryGetValue(sIdentifier, pEventHandler))
  {
    (*pEventHandler)(event);
  }
}

//////////////////////////////////////////////////////////////////////////

Rml::ContextPtr ezRmlUiInternal::ContextInstancer::InstanceContext(const Rml::String& sName, Rml::RenderManager* render_manager, Rml::TextInputHandler* text_input_handler)
{
  return Rml::ContextPtr(EZ_DEFAULT_NEW(ezRmlUiContext, sName, render_manager, text_input_handler));
}

void ezRmlUiInternal::ContextInstancer::ReleaseContext(Rml::Context* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

void ezRmlUiInternal::ContextInstancer::Release()
{
  // nothing to do here
}
