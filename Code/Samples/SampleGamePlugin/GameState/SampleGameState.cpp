#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/System/Window.h>
#include <Core/World/World.h>
#include <Foundation/Logging/Log.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <SampleGamePlugin/GameState/SampleGameState.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(SampleGameState, 1, ezRTTIDefaultAllocator<SampleGameState>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

// BEGIN-DOCS-CODE-SNIPPET: confunc-impl
SampleGameState::SampleGameState()
  : m_ConFunc_Print("Print", "(string arg1): Prints 'arg1' to the log", ezMakeDelegate(&SampleGameState::ConFunc_Print, this))
{
}

void SampleGameState::ConFunc_Print(ezString sText)
{
  ezLog::Info("Text: '{}'", sText);
}
// END-DOCS-CODE-SNIPPET

void SampleGameState::OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  EZ_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld, sStartPosition, startPositionOffset);

// BEGIN-DOCS-CODE-SNIPPET: imgui-alloc
#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (ezImgui::GetSingleton() == nullptr)
  {
    EZ_DEFAULT_NEW(ezImgui);
  }
#endif
  // END-DOCS-CODE-SNIPPET
}

void SampleGameState::OnDeactivation()
{
  EZ_LOG_BLOCK("GameState::Deactivate");

  SUPER::OnDeactivation();
}

// BEGIN-DOCS-CODE-SNIPPET: cvar-1
#include <Foundation/Configuration/CVar.h>

ezCVarBool cvar_DebugDisplay("Game.DebugDisplay", false, ezCVarFlags::Default, "Whether the game should display debug geometry.");
// END-DOCS-CODE-SNIPPET

void SampleGameState::AfterWorldUpdate()
{
  SUPER::AfterWorldUpdate();

  // BEGIN-DOCS-CODE-SNIPPET: cvar-2
  if (cvar_DebugDisplay)
  {
    ezDebugRenderer::DrawLineSphere(m_pMainWorld, ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), 1.0f), ezColor::Orange);
  }
  // END-DOCS-CODE-SNIPPET

  ezDebugRenderer::Draw2DText(m_pMainWorld, "Press 'O' to spawn objects", ezVec2I32(10, 10), ezColor::White);
  ezDebugRenderer::Draw2DText(m_pMainWorld, "Press 'P' to remove objects", ezVec2I32(10, 30), ezColor::White);
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

    // BEGIN-DOCS-CODE-SNIPPET: imgui-activate
    ezImgui::GetSingleton()->SetCurrentContextForView(m_hMainView);
    // END-DOCS-CODE-SNIPPET

    ezImgui::GetSingleton()->SetPassInputToImgui(false); // reset this state, to deactivate input processing as long as SampleGameState::ProcessInput() isn't called again

    // BEGIN-DOCS-CODE-SNIPPET: imgui-panel
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
    // END-DOCS-CODE-SNIPPET
  }
#endif
}

void SampleGameState::ConfigureMainWindowInputDevices(ezWindow* pWindow)
{
  SUPER::ConfigureMainWindowInputDevices(pWindow);

  // setup devices here
}

// BEGIN-DOCS-CODE-SNIPPET: input-config
static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  ezInputActionConfig cfg;
  cfg.m_bApplyTimeScaling = true;
  cfg.m_sInputSlotTrigger[0] = szKey1;
  cfg.m_sInputSlotTrigger[1] = szKey2;
  cfg.m_sInputSlotTrigger[2] = szKey3;

  ezInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void SampleGameState::ConfigureInputActions()
{
  SUPER::ConfigureInputActions();

  RegisterInputAction("SamplePlugin", "SpawnObject", ezInputSlot_KeyO, ezInputSlot_Controller0_ButtonA, ezInputSlot_MouseButton2);
  RegisterInputAction("SamplePlugin", "DeleteObject", ezInputSlot_KeyP, ezInputSlot_Controller0_ButtonB);
}
// END-DOCS-CODE-SNIPPET

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

  ezWorld* pWorld = m_pMainWorld;

  if (ezInputManager::GetInputActionState("SamplePlugin", "SpawnObject") == ezKeyState::Pressed)
  {
    const ezVec3 pos = GetMainCamera()->GetCenterPosition() + GetMainCamera()->GetCenterDirForwards();

    // make sure we are allowed to modify the world
    EZ_LOCK(pWorld->GetWriteMarker());

    // create a game object at the desired position
    ezGameObjectDesc desc;
    desc.m_LocalPosition = pos;

    ezGameObject* pObject = nullptr;
    ezGameObjectHandle hObject = pWorld->CreateObject(desc, pObject);

    m_SpawnedObjects.PushBack(hObject);

    // attach a mesh component to the object
    // BEGIN-DOCS-CODE-SNIPPET: create-component
    ezMeshComponent* pMesh;
    pWorld->GetOrCreateComponentManager<ezMeshComponentManager>()->CreateComponent(pObject, pMesh);
    // END-DOCS-CODE-SNIPPET

    // Set the mesh to use.
    // Here we use a path relative to the project directory.
    // We have to reference the 'transformed' file, not the source file.
    // This would break if the source asset is moved or renamed.
    pMesh->SetMeshFile("AssetCache/Common/Meshes/Sphere.ezBinMesh");

    // here we use the asset GUID to reference the transformed asset
    // we can copy the GUID from the asset browser
    // the GUID is stable even if the source asset gets moved or renamed
    // using asset collections we could also give a nice name like 'Blue Material' to this asset
    ezMaterialResourceHandle hMaterial = ezResourceManager::LoadResource<ezMaterialResource>("{ aa1c5601-bc43-fbf8-4e07-6a3df3af51e7 }");

    // override the mesh material in the first slot with something different
    pMesh->SetMaterial(0, hMaterial);
  }

  if (ezInputManager::GetInputActionState("SamplePlugin", "DeleteObject") == ezKeyState::Pressed)
  {
    if (!m_SpawnedObjects.IsEmpty())
    {
      // make sure we are allowed to modify the world
      EZ_LOCK(pWorld->GetWriteMarker());

      ezGameObjectHandle hObject = m_SpawnedObjects.PeekBack();
      m_SpawnedObjects.PopBack();

      // this is only for demonstration purposes, removing the object will delete all attached components as well
      ezGameObject* pObject = nullptr;
      if (pWorld->TryGetObject(hObject, pObject))
      {
        // BEGIN-DOCS-CODE-SNIPPET: find-component
        ezMeshComponent* pMesh = nullptr;
        if (pObject->TryGetComponentOfBaseType(pMesh))
        {
          pMesh->DeleteComponent();
        }
        // END-DOCS-CODE-SNIPPET
      }

      // delete the object, all its children and attached components
      pWorld->DeleteObjectDelayed(hObject);
    }
  }
}


void SampleGameState::ConfigureMainCamera()
{
  SUPER::ConfigureMainCamera();

  // do custom camera setup here
}
