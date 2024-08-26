#include <GameEngine/GameEnginePCH.h>

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Core/Input/InputManager.h>
#  include <Foundation/Configuration/Startup.h>
#  include <Foundation/Time/Clock.h>
#  include <GameEngine/DearImgui/DearImgui.h>
#  include <GameEngine/GameApplication/GameApplication.h>
#  include <RendererCore/Pipeline/View.h>
#  include <RendererCore/RenderWorld/RenderWorld.h>
#  include <RendererCore/Textures/Texture2DResource.h>

namespace
{
  void* ezImguiAllocate(size_t uiSize, void* pUserData)
  {
    ezAllocator* pAllocator = static_cast<ezAllocator*>(pUserData);
    return pAllocator->Allocate(uiSize, EZ_ALIGNMENT_MINIMUM);
  }

  void ezImguiDeallocate(void* pPtr, void* pUserData)
  {
    if (pPtr != nullptr)
    {
      ezAllocator* pAllocator = static_cast<ezAllocator*>(pUserData);
      pAllocator->Deallocate(pPtr);
    }
  }
} // namespace

EZ_IMPLEMENT_SINGLETON(ezImgui);

ezImgui::ezImgui(ezImguiConfigFontCallback configFontCallback, ezImguiConfigStyleCallback configStyleCallback)
  : m_SingletonRegistrar(this)
  , m_Allocator("ImGui", ezFoundation::GetDefaultAllocator())
  , m_ConfigStyleCallback(configStyleCallback)
{
  Startup(configFontCallback);
}

ezImgui::~ezImgui()
{
  Shutdown();
}

void ezImgui::SetCurrentContextForView(const ezViewHandle& hView)
{
  EZ_LOCK(m_ViewToContextTableMutex);

  Context& context = m_ViewToContextTable[hView];
  if (context.m_pImGuiContext == nullptr)
  {
    context.m_pImGuiContext = CreateContext();
  }

  ImGui::SetCurrentContext(context.m_pImGuiContext);

  ezUInt64 uiCurrentFrameCounter = ezRenderWorld::GetFrameCounter();
  if (context.m_uiFrameBeginCounter != uiCurrentFrameCounter)
  {
    // Last frame was not rendered. This can happen if a render pipeline with dear imgui renderer is used.
    if (context.m_uiFrameRenderCounter != context.m_uiFrameBeginCounter)
    {
      ImGui::EndFrame();
    }

    BeginFrame(hView);
    context.m_uiFrameBeginCounter = uiCurrentFrameCounter;
  }
}

void ezImgui::Startup(ezImguiConfigFontCallback configFontCallback)
{
  ImGui::SetAllocatorFunctions(&ezImguiAllocate, &ezImguiDeallocate, &m_Allocator);

  m_pSharedFontAtlas = EZ_DEFAULT_NEW(ImFontAtlas);

  if (configFontCallback.IsValid())
  {
    configFontCallback(*m_pSharedFontAtlas);
  }

  unsigned char* pixels;
  int width, height;
  m_pSharedFontAtlas->GetTexDataAsRGBA32(&pixels, &width, &height); // Load as RGBA 32-bits (75% of the memory is wasted, but default font
                                                                    // is so small) because it is more likely to be compatible with user's
                                                                    // existing shaders. If your ImTextureId represent a higher-level
                                                                    // concept than just a GL texture id, consider calling
                                                                    // GetTexDataAsAlpha8() instead to save on GPU memory.

  ezTexture2DResourceHandle hFont = ezResourceManager::GetExistingResource<ezTexture2DResource>("ImguiFont");

  if (!hFont.IsValid())
  {
    ezGALSystemMemoryDescription memoryDesc;
    memoryDesc.m_pData = pixels;
    memoryDesc.m_uiRowPitch = width * 4;
    memoryDesc.m_uiSlicePitch = width * height * 4;

    ezTexture2DResourceDescriptor desc;
    desc.m_DescGAL.m_uiWidth = width;
    desc.m_DescGAL.m_uiHeight = height;
    desc.m_DescGAL.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    desc.m_InitialContent = ezMakeArrayPtr(&memoryDesc, 1);

    hFont = ezResourceManager::GetOrCreateResource<ezTexture2DResource>("ImguiFont", std::move(desc));
  }

  m_Textures.PushBack(hFont);

  const size_t id = (size_t)m_Textures.GetCount() - 1;
  m_pSharedFontAtlas->TexID = reinterpret_cast<void*>(id);
}

void ezImgui::Shutdown()
{
  m_Textures.Clear();

  m_pSharedFontAtlas = nullptr;

  for (auto it = m_ViewToContextTable.GetIterator(); it.IsValid(); ++it)
  {
    Context& context = it.Value();
    ImGui::DestroyContext(context.m_pImGuiContext);
    context.m_pImGuiContext = nullptr;
  }
  m_ViewToContextTable.Clear();
}

ImGuiContext* ezImgui::CreateContext()
{
  // imgui reads the global context pointer WHILE creating a new context
  // so if we don't reset it to null here, it will try to access it, and crash
  // if imgui was active on the same thread before
  ImGui::SetCurrentContext(nullptr);
  ImGuiContext* context = ImGui::CreateContext(m_pSharedFontAtlas.Borrow());
  ImGui::SetCurrentContext(context);

  ImGuiIO& cfg = ImGui::GetIO();

  cfg.DisplaySize.x = 1650;
  cfg.DisplaySize.y = 1080;

  if (m_ConfigStyleCallback.IsValid())
  {
    m_ConfigStyleCallback(ImGui::GetStyle());
  }

  return context;
}

void ezImgui::BeginFrame(const ezViewHandle& hView)
{
  ezView* pView = nullptr;
  if (!ezRenderWorld::TryGetView(hView, pView))
  {
    return;
  }

  auto viewport = pView->GetViewport();
  m_CurrentWindowResolution = ezSizeU32(static_cast<ezUInt32>(viewport.width), static_cast<ezUInt32>(viewport.height));

  ImGuiIO& cfg = ImGui::GetIO();

  cfg.DisplaySize.x = viewport.width;
  cfg.DisplaySize.y = viewport.height;
  cfg.DeltaTime = (float)ezClock::GetGlobalClock()->GetTimeDiff().GetSeconds();

  if (m_bPassInputToImgui)
  {
    char szUtf8[8] = "";
    char* pChar = szUtf8;
    ezUnicodeUtils::EncodeUtf32ToUtf8(ezInputManager::RetrieveLastCharacter(false), pChar);
    cfg.AddInputCharactersUTF8(szUtf8);

    float mousex, mousey;
    ezInputManager::GetInputSlotState(ezInputSlot_MousePositionX, &mousex);
    ezInputManager::GetInputSlotState(ezInputSlot_MousePositionY, &mousey);
    cfg.AddMousePosEvent(cfg.DisplaySize.x * mousex, cfg.DisplaySize.y * mousey);
    cfg.AddMouseButtonEvent(0, ezInputManager::GetInputSlotState(ezInputSlot_MouseButton0) >= ezKeyState::Pressed);
    cfg.AddMouseButtonEvent(1, ezInputManager::GetInputSlotState(ezInputSlot_MouseButton1) >= ezKeyState::Pressed);
    cfg.AddMouseButtonEvent(2, ezInputManager::GetInputSlotState(ezInputSlot_MouseButton2) >= ezKeyState::Pressed);

    float fMouseWheel = 0;
    if (ezInputManager::GetInputSlotState(ezInputSlot_MouseWheelDown) == ezKeyState::Pressed)
      fMouseWheel = -1;
    if (ezInputManager::GetInputSlotState(ezInputSlot_MouseWheelUp) == ezKeyState::Pressed)
      fMouseWheel = +1;
    cfg.AddMouseWheelEvent(0, fMouseWheel);

    cfg.AddKeyEvent(ImGuiKey_LeftAlt, ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftAlt) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightAlt, ezInputManager::GetInputSlotState(ezInputSlot_KeyRightAlt) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_LeftCtrl, ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftCtrl) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightCtrl, ezInputManager::GetInputSlotState(ezInputSlot_KeyRightCtrl) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_LeftShift, ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftShift) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightShift, ezInputManager::GetInputSlotState(ezInputSlot_KeyRightShift) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_LeftSuper, ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftWin) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightSuper, ezInputManager::GetInputSlotState(ezInputSlot_KeyRightWin) >= ezKeyState::Pressed);

    cfg.AddKeyEvent(ImGuiKey_Tab, ezInputManager::GetInputSlotState(ezInputSlot_KeyTab) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_LeftArrow, ezInputManager::GetInputSlotState(ezInputSlot_KeyLeft) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_RightArrow, ezInputManager::GetInputSlotState(ezInputSlot_KeyRight) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_UpArrow, ezInputManager::GetInputSlotState(ezInputSlot_KeyUp) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_DownArrow, ezInputManager::GetInputSlotState(ezInputSlot_KeyDown) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_PageUp, ezInputManager::GetInputSlotState(ezInputSlot_KeyPageUp) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_PageDown, ezInputManager::GetInputSlotState(ezInputSlot_KeyPageDown) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Home, ezInputManager::GetInputSlotState(ezInputSlot_KeyHome) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_End, ezInputManager::GetInputSlotState(ezInputSlot_KeyEnd) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Delete, ezInputManager::GetInputSlotState(ezInputSlot_KeyDelete) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Backspace, ezInputManager::GetInputSlotState(ezInputSlot_KeyBackspace) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Enter, ezInputManager::GetInputSlotState(ezInputSlot_KeyReturn) >= ezKeyState::Pressed ||
                                      ezInputManager::GetInputSlotState(ezInputSlot_KeyNumpadEnter) >= ezKeyState::Pressed);

    cfg.AddKeyEvent(ImGuiKey_Escape, ezInputManager::GetInputSlotState(ezInputSlot_KeyEscape) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_A, ezInputManager::GetInputSlotState(ezInputSlot_KeyA) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_C, ezInputManager::GetInputSlotState(ezInputSlot_KeyC) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_V, ezInputManager::GetInputSlotState(ezInputSlot_KeyV) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_X, ezInputManager::GetInputSlotState(ezInputSlot_KeyX) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Y, ezInputManager::GetInputSlotState(ezInputSlot_KeyY) >= ezKeyState::Pressed);
    cfg.AddKeyEvent(ImGuiKey_Z, ezInputManager::GetInputSlotState(ezInputSlot_KeyZ) >= ezKeyState::Pressed);
  }
  else
  {
    cfg.ClearInputKeys();
  }

  ImGui::NewFrame();

  m_bImguiWantsInput = cfg.WantCaptureKeyboard || cfg.WantCaptureMouse;
}

#endif
