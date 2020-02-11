#include <GameEnginePCH.h>

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#include <GameEngine/DearImgui/DearImgui.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Time/Clock.h>
#include <GameApplication/GameApplication.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/Texture2DResource.h>

namespace
{
  void* ezImguiAllocate(size_t uiSize, void* pUserData)
  {
    ezAllocatorBase* pAllocator = static_cast<ezAllocatorBase*>(pUserData);
    return pAllocator->Allocate(uiSize, EZ_ALIGNMENT_MINIMUM);
  }

  void ezImguiDeallocate(void* ptr, void* pUserData)
  {
    if (ptr != nullptr)
    {
      ezAllocatorBase* pAllocator = static_cast<ezAllocatorBase*>(pUserData);
      pAllocator->Deallocate(ptr);
    }
  }
}

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

    hFont = ezResourceManager::CreateResource<ezTexture2DResource>("ImguiFont", std::move(desc));
  }

  m_hTextures.PushBack(hFont);

  const size_t id = (size_t)m_hTextures.GetCount() - 1;
  m_pSharedFontAtlas->TexID = reinterpret_cast<void*>(id);
}

void ezImgui::Shutdown()
{
  m_hTextures.Clear();

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
  ImGuiContext* context = ImGui::CreateContext(m_pSharedFontAtlas.Borrow());
  ImGui::SetCurrentContext(context);

  ImGuiIO& cfg = ImGui::GetIO();

  cfg.DisplaySize.x = 1650;
  cfg.DisplaySize.y = 1080;

  cfg.KeyMap[ImGuiKey_Tab] = ImGuiKey_Tab;
  cfg.KeyMap[ImGuiKey_LeftArrow] = ImGuiKey_LeftArrow;
  cfg.KeyMap[ImGuiKey_RightArrow] = ImGuiKey_RightArrow;
  cfg.KeyMap[ImGuiKey_UpArrow] = ImGuiKey_UpArrow;
  cfg.KeyMap[ImGuiKey_DownArrow] = ImGuiKey_DownArrow;
  cfg.KeyMap[ImGuiKey_PageUp] = ImGuiKey_PageUp;
  cfg.KeyMap[ImGuiKey_PageDown] = ImGuiKey_PageDown;
  cfg.KeyMap[ImGuiKey_Home] = ImGuiKey_Home;
  cfg.KeyMap[ImGuiKey_End] = ImGuiKey_End;
  cfg.KeyMap[ImGuiKey_Delete] = ImGuiKey_Delete;
  cfg.KeyMap[ImGuiKey_Backspace] = ImGuiKey_Backspace;
  cfg.KeyMap[ImGuiKey_Enter] = ImGuiKey_Enter;
  cfg.KeyMap[ImGuiKey_Escape] = ImGuiKey_Escape;
  cfg.KeyMap[ImGuiKey_A] = ImGuiKey_A;
  cfg.KeyMap[ImGuiKey_C] = ImGuiKey_C;
  cfg.KeyMap[ImGuiKey_V] = ImGuiKey_V;
  cfg.KeyMap[ImGuiKey_X] = ImGuiKey_X;
  cfg.KeyMap[ImGuiKey_Y] = ImGuiKey_Y;
  cfg.KeyMap[ImGuiKey_Z] = ImGuiKey_Z;

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
    ezUnicodeUtils::EncodeUtf32ToUtf8(ezInputManager::RetrieveLastCharacter(), pChar);
    cfg.AddInputCharactersUTF8(szUtf8);

    float mousex, mousey;
    ezInputManager::GetInputSlotState(ezInputSlot_MousePositionX, &mousex);
    ezInputManager::GetInputSlotState(ezInputSlot_MousePositionY, &mousey);
    cfg.MousePos.x = cfg.DisplaySize.x * mousex;
    cfg.MousePos.y = cfg.DisplaySize.y * mousey;
    cfg.MouseDown[0] = ezInputManager::GetInputSlotState(ezInputSlot_MouseButton0) >= ezKeyState::Pressed;
    cfg.MouseDown[1] = ezInputManager::GetInputSlotState(ezInputSlot_MouseButton1) >= ezKeyState::Pressed;
    cfg.MouseDown[2] = ezInputManager::GetInputSlotState(ezInputSlot_MouseButton2) >= ezKeyState::Pressed;

    cfg.MouseWheel = 0;
    if (ezInputManager::GetInputSlotState(ezInputSlot_MouseWheelDown) == ezKeyState::Pressed)
      cfg.MouseWheel = -1;
    if (ezInputManager::GetInputSlotState(ezInputSlot_MouseWheelUp) == ezKeyState::Pressed)
      cfg.MouseWheel = +1;

    cfg.KeyAlt = ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftAlt) >= ezKeyState::Pressed ||
                 ezInputManager::GetInputSlotState(ezInputSlot_KeyRightAlt) >= ezKeyState::Pressed;
    cfg.KeyCtrl = ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftCtrl) >= ezKeyState::Pressed ||
                  ezInputManager::GetInputSlotState(ezInputSlot_KeyRightCtrl) >= ezKeyState::Pressed;
    cfg.KeyShift = ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftShift) >= ezKeyState::Pressed ||
                   ezInputManager::GetInputSlotState(ezInputSlot_KeyRightShift) >= ezKeyState::Pressed;
    cfg.KeySuper = ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftWin) >= ezKeyState::Pressed ||
                   ezInputManager::GetInputSlotState(ezInputSlot_KeyRightWin) >= ezKeyState::Pressed;

    cfg.KeysDown[ImGuiKey_Tab] = ezInputManager::GetInputSlotState(ezInputSlot_KeyTab) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_LeftArrow] = ezInputManager::GetInputSlotState(ezInputSlot_KeyLeft) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_RightArrow] = ezInputManager::GetInputSlotState(ezInputSlot_KeyRight) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_UpArrow] = ezInputManager::GetInputSlotState(ezInputSlot_KeyUp) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_DownArrow] = ezInputManager::GetInputSlotState(ezInputSlot_KeyDown) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_PageUp] = ezInputManager::GetInputSlotState(ezInputSlot_KeyPageUp) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_PageDown] = ezInputManager::GetInputSlotState(ezInputSlot_KeyPageDown) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Home] = ezInputManager::GetInputSlotState(ezInputSlot_KeyHome) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_End] = ezInputManager::GetInputSlotState(ezInputSlot_KeyEnd) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Delete] = ezInputManager::GetInputSlotState(ezInputSlot_KeyDelete) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Backspace] = ezInputManager::GetInputSlotState(ezInputSlot_KeyBackspace) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Enter] = ezInputManager::GetInputSlotState(ezInputSlot_KeyReturn) >= ezKeyState::Pressed ||
                                   ezInputManager::GetInputSlotState(ezInputSlot_KeyNumpadEnter) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Escape] = ezInputManager::GetInputSlotState(ezInputSlot_KeyEscape) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_A] = ezInputManager::GetInputSlotState(ezInputSlot_KeyA) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_C] = ezInputManager::GetInputSlotState(ezInputSlot_KeyC) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_V] = ezInputManager::GetInputSlotState(ezInputSlot_KeyV) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_X] = ezInputManager::GetInputSlotState(ezInputSlot_KeyX) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Y] = ezInputManager::GetInputSlotState(ezInputSlot_KeyY) >= ezKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Z] = ezInputManager::GetInputSlotState(ezInputSlot_KeyZ) >= ezKeyState::Pressed;
  }
  else
  {
    cfg.ClearInputCharacters();

    cfg.MousePos.x = -1;
    cfg.MousePos.y = -1;

    cfg.MouseDown[0] = false;
    cfg.MouseDown[1] = false;
    cfg.MouseDown[2] = false;

    cfg.MouseWheel = 0;

    cfg.KeyAlt = false;
    cfg.KeyCtrl = false;
    cfg.KeyShift = false;
    cfg.KeySuper = false;

    for (int i = 0; i <= ImGuiKey_COUNT; ++i)
      cfg.KeysDown[i] = false;
  }

  ImGui::NewFrame();

  m_bImguiWantsInput = cfg.WantCaptureKeyboard || cfg.WantCaptureMouse;
}

#endif

EZ_STATICLINK_FILE(GameEngine, GameEngine_DearImgui_DearImgui);
