#include <RmlUiPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Implementation/Extractor.h>
#include <RmlUiPlugin/RmlUiContext.h>

namespace
{
  static const char* s_szEzKeys[] = {
    ezInputSlot_KeyTab,
    ezInputSlot_KeyLeft,
    ezInputSlot_KeyUp,
    ezInputSlot_KeyRight,
    ezInputSlot_KeyDown,
    ezInputSlot_KeyPageUp,
    ezInputSlot_KeyPageDown,
    ezInputSlot_KeyHome,
    ezInputSlot_KeyEnd,
    ezInputSlot_KeyDelete,
    ezInputSlot_KeyBackspace,
    ezInputSlot_KeyReturn,
    ezInputSlot_KeyNumpadEnter,
    ezInputSlot_KeyEscape};

  static Rml::Core::Input::KeyIdentifier s_rmlKeys[] = {
    Rml::Core::Input::KI_TAB,
    Rml::Core::Input::KI_LEFT,
    Rml::Core::Input::KI_UP,
    Rml::Core::Input::KI_RIGHT,
    Rml::Core::Input::KI_DOWN,
    Rml::Core::Input::KI_PRIOR,
    Rml::Core::Input::KI_NEXT,
    Rml::Core::Input::KI_HOME,
    Rml::Core::Input::KI_END,
    Rml::Core::Input::KI_DELETE,
    Rml::Core::Input::KI_BACK,
    Rml::Core::Input::KI_RETURN,
    Rml::Core::Input::KI_RETURN,
    Rml::Core::Input::KI_ESCAPE};

  EZ_CHECK_AT_COMPILETIME(EZ_ARRAY_SIZE(s_szEzKeys) == EZ_ARRAY_SIZE(s_rmlKeys));
} // namespace

ezRmlUiContext::ezRmlUiContext(const Rml::Core::String& name)
  : Rml::Core::Context(name)
{
}

ezRmlUiContext::~ezRmlUiContext() = default;

ezResult ezRmlUiContext::LoadDocumentFromFile(const char* szFile)
{
  if (HasDocument())
  {
    UnloadDocument(GetDocument(0));
  }

  if (ezStringUtils::IsNullOrEmpty(szFile) == false)
  {
    LoadDocument(szFile);
  }

  return HasDocument() ? EZ_SUCCESS : EZ_FAILURE;
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

void ezRmlUiContext::UpdateInput(const ezVec2& mousePos)
{
  const bool bCtrlPressed = ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftCtrl) >= ezKeyState::Pressed ||
                            ezInputManager::GetInputSlotState(ezInputSlot_KeyRightCtrl) >= ezKeyState::Pressed;
  const bool bShiftPressed = ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftShift) >= ezKeyState::Pressed ||
                             ezInputManager::GetInputSlotState(ezInputSlot_KeyRightShift) >= ezKeyState::Pressed;
  const bool bAltPressed = ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftAlt) >= ezKeyState::Pressed ||
                           ezInputManager::GetInputSlotState(ezInputSlot_KeyRightAlt) >= ezKeyState::Pressed;

  int modifierState = 0;
  modifierState |= bCtrlPressed ? Rml::Core::Input::KM_CTRL : 0;
  modifierState |= bShiftPressed ? Rml::Core::Input::KM_SHIFT : 0;
  modifierState |= bAltPressed ? Rml::Core::Input::KM_ALT : 0;

  // Mouse
  {
    ProcessMouseMove(mousePos.x, mousePos.y, modifierState);

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
        ProcessTextInput(szUtf8);
      }
    }

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_szEzKeys); ++i)
    {
      ezKeyState::Enum state = ezInputManager::GetInputSlotState(s_szEzKeys[i]);
      if (state == ezKeyState::Pressed)
      {
        ProcessKeyDown(s_rmlKeys[i], modifierState);
      }
      else if (state == ezKeyState::Released)
      {
        ProcessKeyUp(s_rmlKeys[i], modifierState);
      }
    }
  }
}

void ezRmlUiContext::SetOffset(const ezVec2I32& offset)
{
  m_Offset = offset;
}

void ezRmlUiContext::SetSize(const ezVec2U32& size)
{
  SetDimensions(Rml::Core::Vector2i(size.x, size.y));
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
    extractor.BeginExtraction(m_Offset);

    Render();

    extractor.EndExtraction();

    m_uiExtractedFrame = ezRenderWorld::GetFrameCounter();
    m_pRenderData = extractor.GetRenderData();
  }
}

void ezRmlUiContext::ProcessEvent(const ezHashedString& sIdentifier, Rml::Core::Event& event)
{
  EventHandler* pEventHandler = nullptr;
  if (m_EventHandler.TryGetValue(sIdentifier, pEventHandler))
  {
    (*pEventHandler)(event);
  }
}

//////////////////////////////////////////////////////////////////////////

Rml::Core::ContextPtr ezRmlUiInternal::ContextInstancer::InstanceContext(const Rml::Core::String& name)
{
  return Rml::Core::ContextPtr(EZ_DEFAULT_NEW(ezRmlUiContext, name));
}

void ezRmlUiInternal::ContextInstancer::ReleaseContext(Rml::Core::Context* context)
{
  EZ_DEFAULT_DELETE(context);
}

void ezRmlUiInternal::ContextInstancer::Release()
{
  // nothing to do here
}
