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

ezRmlUiContext::ezRmlUiContext(const char* szName, const ezVec2U32& initialSize)
{
  m_pContext = Rml::Core::CreateContext(szName, Rml::Core::Vector2i(initialSize.x, initialSize.y));
}

ezRmlUiContext::~ezRmlUiContext()
{
  Rml::Core::RemoveContext(m_pContext->GetName());
}

ezResult ezRmlUiContext::LoadDocumentFromFile(const char* szFile)
{
  if (m_pDocument != nullptr)
  {
    m_pContext->UnloadDocument(m_pDocument);
    m_pDocument = nullptr;
  }

  if (ezStringUtils::IsNullOrEmpty(szFile) == false)
  {
    m_pDocument = m_pContext->LoadDocument(szFile);
  }

  return m_pDocument != nullptr ? EZ_SUCCESS : EZ_FAILURE;
}

void ezRmlUiContext::ShowDocument()
{
  if (m_pDocument != nullptr)
  {
    m_pDocument->Show();
  }
}

void ezRmlUiContext::HideDocument()
{
  if (m_pDocument != nullptr)
  {
    m_pDocument->Hide();
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
    m_pContext->ProcessMouseMove(mousePos.x, mousePos.y, modifierState);

    static const char* szMouseButtons[] = {ezInputSlot_MouseButton0, ezInputSlot_MouseButton1, ezInputSlot_MouseButton2};
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(szMouseButtons); ++i)
    {
      ezKeyState::Enum state = ezInputManager::GetInputSlotState(szMouseButtons[i]);
      if (state == ezKeyState::Pressed)
      {
        m_pContext->ProcessMouseButtonDown(i, modifierState);
      }
      else if (state == ezKeyState::Released)
      {
        m_pContext->ProcessMouseButtonUp(i, modifierState);
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
        m_pContext->ProcessTextInput(szUtf8);
      }
    }

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_szEzKeys); ++i)
    {
      ezKeyState::Enum state = ezInputManager::GetInputSlotState(s_szEzKeys[i]);
      if (state == ezKeyState::Pressed)
      {
        m_pContext->ProcessKeyDown(s_rmlKeys[i], modifierState);
      }
      else if (state == ezKeyState::Released)
      {
        m_pContext->ProcessKeyUp(s_rmlKeys[i], modifierState);
      }
    }
  }
}

void ezRmlUiContext::Update()
{
  if (m_pDocument != nullptr)
  {
    m_pContext->Update();
  }
}

void ezRmlUiContext::SetOffset(const ezVec2I32& offset)
{
  m_Offset = offset;
}

void ezRmlUiContext::SetSize(const ezVec2U32& size)
{
  if (m_pDocument != nullptr)
  {
    m_pContext->SetDimensions(Rml::Core::Vector2i(size.x, size.y));
  }
}

void ezRmlUiContext::SetDpiScale(float fScale)
{
  if (m_pDocument != nullptr)
  {
    m_pContext->SetDensityIndependentPixelRatio(fScale);
  }
}

Rml::Core::Context* ezRmlUiContext::GetRmlContext()
{
  return m_pContext;
}

void ezRmlUiContext::ExtractRenderData(ezRmlUiInternal::Extractor& extractor)
{
  if (m_uiExtractedFrame != ezRenderWorld::GetFrameCounter())
  {
    extractor.BeginExtraction(m_Offset);

    m_pContext->Render();

    extractor.EndExtraction();

    m_uiExtractedFrame = ezRenderWorld::GetFrameCounter();
    m_pRenderData = extractor.GetRenderData();
  }
}
