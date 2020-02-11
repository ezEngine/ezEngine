#include <SampleGamePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Logging/Log.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <SampleGamePlugin/GameState/SampleGameState.h>
#include <System/Window/Window.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(SampleGameState, 1, ezRTTIDefaultAllocator<SampleGameState>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

SampleGameState::SampleGameState() {}

void SampleGameState::OnActivation(ezWorld* pWorld, const ezTransform* pStartPosition)
{
  EZ_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld, pStartPosition);

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (ezImgui::GetSingleton() == nullptr)
  {
    EZ_DEFAULT_NEW(ezImgui);
  }
#endif
}

void SampleGameState::OnDeactivation()
{
  EZ_LOG_BLOCK("GameState::Deactivate");

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (ezImgui::GetSingleton() != nullptr)
  {
    ezImgui* pImgui = ezImgui::GetSingleton();
    EZ_DEFAULT_DELETE(pImgui);
  }
#endif

  SUPER::OnDeactivation();
}

void SampleGameState::BeforeWorldUpdate()
{
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (ezImgui::GetSingleton() != nullptr)
  {
    static bool stats = false;
    static bool window = true;
    static float color[3];
    static float slider = 0.5f;

    ezImgui::GetSingleton()->SetCurrentContextForView(m_hMainView);
    ezImgui::GetSingleton()->SetPassInputToImgui(
      false); // reset this state, to deactivate input processing as long as SampleGameState::ProcessInput() isn't called again

    ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
    ImGui::Begin("Imgui Window", &window);
    ImGui::Text("Hello World!");
    ImGui::SliderFloat("Slider", &slider, 0.0f, 1.0f);
    ImGui::ColorEdit3("Color", color);


    if (ImGui::Button("Toggle Stats"))
    {
      stats = !stats;
    }

    if (stats)
    {
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }

    ImGui::End();
  }
#endif
}



ezGameStatePriority SampleGameState::DeterminePriority(ezWorld* pWorld) const
{
  return

    ezGameStatePriority::Default;
}

void SampleGameState::ConfigureMainWindowInputDevices(ezWindow* pWindow)
{
  SUPER::ConfigureMainWindowInputDevices(pWindow);

  // setup devices here
}

void SampleGameState::ConfigureInputActions()
{
  SUPER::ConfigureInputActions();

  // setup custom input actions here
}

void SampleGameState::ProcessInput()
{
#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (ezImgui::GetSingleton())
  {
    // SampleGameState::ProcessInput() isn't necessary called each frame, if the application decides that the game-state
    // should not get any input at the moment (this happens for instance, when the ezConsole is open)
    // so only enable it when the game state gets input (and reset it in BeforeWorldUpdate())
    ezImgui::GetSingleton()->SetPassInputToImgui(true);

    // if the UI wants input, do not process other game state input
    if (ezImgui::GetSingleton()->WantsInput())
      return;
  }
#endif

  SUPER::ProcessInput();

  // do game input processing here
}

void SampleGameState::ConfigureMainCamera()
{
  SUPER::ConfigureMainCamera();

  // do custom camera setup here
}
