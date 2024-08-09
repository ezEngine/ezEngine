#include <CppProjectPlugin/CppProjectPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/System/Window.h>
#include <Core/World/World.h>
#include <CppProjectPlugin/GameState/CppProjectGameState.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Logging/Log.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>

ezCVarBool cvar_DebugDisplay("CppProject.DebugDisplay", false, ezCVarFlags::Default, "Whether the game should display debug geometry.");

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(CppProjectGameState, 1, ezRTTIDefaultAllocator<CppProjectGameState>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

CppProjectGameState::CppProjectGameState() = default;
CppProjectGameState::~CppProjectGameState() = default;

void CppProjectGameState::OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  EZ_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld, sStartPosition, startPositionOffset);

  // the main entry point when the game starts
  // could do some setup here, but in a lot of cases it is better to leave this as is
  // and instead override the various other virtual functions that the game state provides
  // see below and see ezGameState for additional details
}

void CppProjectGameState::AfterWorldUpdate()
{
  SUPER::AfterWorldUpdate();

  if (cvar_DebugDisplay)
  {
    ezDebugRenderer::DrawLineSphere(m_pMainWorld, ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), 1.0f), ezColor::Orange);
  }

  ezDebugRenderer::Draw2DText(m_pMainWorld, "Press 'O' to spawn objects", ezVec2I32(10, 10), ezColor::White);
  ezDebugRenderer::Draw2DText(m_pMainWorld, "Press 'P' to remove objects", ezVec2I32(10, 30), ezColor::White);
}

void CppProjectGameState::BeforeWorldUpdate()
{
  SUPER::BeforeWorldUpdate();

  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  // if you need to modify the world, this is a good place to do it
}

ezResult CppProjectGameState::SpawnPlayer(ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  // replace this to create a custom player object or load a prefab
  return SUPER::SpawnPlayer(sStartPosition, startPositionOffset);
}

void CppProjectGameState::OnChangedMainWorld(ezWorld* pPrevWorld, ezWorld* pNewWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  SUPER::OnChangedMainWorld(pPrevWorld, pNewWorld, sStartPosition, startPositionOffset);

  // called whenever the main world is changed, ie when transitioning between levels
  // may need to update references to the world here or reset some state
}

ezString CppProjectGameState::GetStartupSceneFile()
{
  // replace this to load a certain scene at startup
  // the default implementation looks at the command line "-scene" argument
  return SUPER::GetStartupSceneFile();
}

static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  ezInputActionConfig cfg;
  cfg.m_bApplyTimeScaling = true;
  cfg.m_sInputSlotTrigger[0] = szKey1;
  cfg.m_sInputSlotTrigger[1] = szKey2;
  cfg.m_sInputSlotTrigger[2] = szKey3;

  ezInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void CppProjectGameState::ConfigureInputActions()
{
  SUPER::ConfigureInputActions();

  RegisterInputAction("CppProjectPlugin", "SpawnObject", ezInputSlot_KeyO, ezInputSlot_Controller0_ButtonA, ezInputSlot_MouseButton2);
  RegisterInputAction("CppProjectPlugin", "DeleteObject", ezInputSlot_KeyP, ezInputSlot_Controller0_ButtonB);
}

void CppProjectGameState::ProcessInput()
{
  SUPER::ProcessInput();

  ezWorld* pWorld = m_pMainWorld;

  if (ezInputManager::GetInputActionState("CppProjectPlugin", "SpawnObject") == ezKeyState::Pressed)
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
    ezMeshComponent* pMesh;
    pWorld->GetOrCreateComponentManager<ezMeshComponentManager>()->CreateComponent(pObject, pMesh);

    // Set the mesh to use.
    // Here we use a path relative to the project directory.
    // We have to reference the 'transformed' file, not the source file.
    // This would break if the source asset is moved or renamed.
    pMesh->SetMeshFile("AssetCache/Common/Meshes/Sphere.ezMesh");

    // here we use the asset GUID to reference the transformed asset
    // we can copy the GUID from the asset browser
    // the GUID is stable even if the source asset gets moved or renamed
    // using asset collections we could also give a nice name like 'Blue Material' to this asset
    ezMaterialResourceHandle hMaterial = ezResourceManager::LoadResource<ezMaterialResource>("{ aa1c5601-bc43-fbf8-4e07-6a3df3af51e7 }");

    // override the mesh material in the first slot with something different
    pMesh->SetMaterial(0, hMaterial);
  }

  if (ezInputManager::GetInputActionState("CppProjectPlugin", "DeleteObject") == ezKeyState::Pressed)
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
        ezMeshComponent* pMesh = nullptr;
        if (pObject->TryGetComponentOfBaseType(pMesh))
        {
          pMesh->DeleteComponent();
        }
      }

      // delete the object, all its children and attached components
      pWorld->DeleteObjectDelayed(hObject);
    }
  }
}

void CppProjectGameState::ConfigureMainCamera()
{
  SUPER::ConfigureMainCamera();

  // do custom camera setup here
}
