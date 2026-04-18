#include <RasterizerExplorer/RasterizerExplorerState.h>

#include <RasterizerExplorer/RasterizerOverlayComponent.h>
#include <Core/Input/InputManager.h>
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Random.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/RasterizerView.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(RasterizerExplorerState, 1, ezRTTIDefaultAllocator<RasterizerExplorerState>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

static void RegisterInputAction(const char* szSet, const char* szAction, const char* szKey1, const char* szKey2 = nullptr)
{
  ezInputActionConfig cfg;
  cfg.m_bApplyTimeScaling = true;
  cfg.m_sInputSlotTrigger[0] = szKey1;
  cfg.m_sInputSlotTrigger[1] = szKey2;
  ezInputManager::SetInputActionConfig(szSet, szAction, cfg, true);
}

static void RegisterInputActionNoTimeScale(const char* szSet, const char* szAction, const char* szKey1, const char* szKey2 = nullptr)
{
  ezInputActionConfig cfg;
  cfg.m_bApplyTimeScaling = false;
  cfg.m_sInputSlotTrigger[0] = szKey1;
  cfg.m_sInputSlotTrigger[1] = szKey2;
  ezInputManager::SetInputActionConfig(szSet, szAction, cfg, true);
}

RasterizerExplorerState::RasterizerExplorerState() = default;

void RasterizerExplorerState::OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  if (pWorld == nullptr)
  {
    ezWorldDesc desc("RasterizerExplorer");
    m_pWorld = EZ_DEFAULT_NEW(ezWorld, desc);
    pWorld = m_pWorld.Borrow();
  }

  SUPER::OnActivation(pWorld, sStartPosition, startPositionOffset);

  m_MainCamera.LookAt(ezVec3(-1, 0, 40), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
  m_MainCamera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 70.0f, 0.1f, 200.0f);

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (ezImgui::GetSingleton() == nullptr)
  {
    EZ_DEFAULT_NEW(ezImgui);
  }
#endif

  m_pRasterizerViewOptimized = EZ_DEFAULT_NEW(ezRasterizerView, true);
  m_pRasterizerViewGeneric = EZ_DEFAULT_NEW(ezRasterizerView, false);
  m_pRasterizerViewMOC = EZ_DEFAULT_NEW(ezRasterizerView, ezRasterizerImplementation::MaskedOcclusionCulling);
  m_pRasterizerViewGenericMOC = EZ_DEFAULT_NEW(ezRasterizerView, ezRasterizerImplementation::GenericMOC);

  CreateOverlayObject();

  GenerateCity();

  LoadSettings();
}

void RasterizerExplorerState::OnDeactivation()
{
  DestroyOverlayObject();

  m_pRasterizerViewOptimized.Clear();
  m_pRasterizerViewGeneric.Clear();
  m_pRasterizerViewMOC.Clear();
  m_pRasterizerViewGenericMOC.Clear();

  SUPER::OnDeactivation();

  m_pWorld.Clear();
}

void RasterizerExplorerState::ConfigureInputActions()
{
  SUPER::ConfigureInputActions();

  RegisterInputAction("Game", "MoveForwards", ezInputSlot_KeyW);
  RegisterInputAction("Game", "MoveBackwards", ezInputSlot_KeyS);
  RegisterInputAction("Game", "MoveLeft", ezInputSlot_KeyA);
  RegisterInputAction("Game", "MoveRight", ezInputSlot_KeyD);
  RegisterInputAction("Game", "MoveUp", ezInputSlot_KeyE);
  RegisterInputAction("Game", "MoveDown", ezInputSlot_KeyQ);
  RegisterInputActionNoTimeScale("Game", "Run", ezInputSlot_KeyLeftShift);

  RegisterInputAction("Game", "TurnLeft", ezInputSlot_MouseMoveNegX);
  RegisterInputAction("Game", "TurnRight", ezInputSlot_MouseMovePosX);
  RegisterInputAction("Game", "TurnUp", ezInputSlot_MouseMoveNegY);
  RegisterInputAction("Game", "TurnDown", ezInputSlot_MouseMovePosY);

  RegisterInputActionNoTimeScale("Game", "Look", ezInputSlot_MouseButton1);

  RegisterInputActionNoTimeScale("Rasterizer", "NextCube", ezInputSlot_KeyDown);
  RegisterInputActionNoTimeScale("Rasterizer", "PrevCube", ezInputSlot_KeyUp);
  RegisterInputActionNoTimeScale("Rasterizer", "OverlayOptimized", ezInputSlot_Key1);
  RegisterInputActionNoTimeScale("Rasterizer", "OverlayGeneric", ezInputSlot_Key2);
  RegisterInputActionNoTimeScale("Rasterizer", "OverlayMOC", ezInputSlot_Key3);
  RegisterInputActionNoTimeScale("Rasterizer", "OverlayGenericMOC", ezInputSlot_Key4);
  RegisterInputActionNoTimeScale("Rasterizer", "OverlayDiff", ezInputSlot_Key5);
}

void RasterizerExplorerState::ConfigureMainCamera()
{
  // Only let the base class update from camera components; initial setup is in OnActivation.
  SUPER::ConfigureMainCamera();
}

void RasterizerExplorerState::SetupMainView(ezGALSwapChainHandle hSwapChain, ezSizeU32 viewportSize)
{
  ezView* pView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hMainView, pView))
    return;

  // Use the default render pipeline (MainRenderPipeline.ezRenderPipelineAsset)
  auto hPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }");
  pView->SetRenderPipelineResource(hPipeline);
  pView->SetSwapChain(hSwapChain);
  pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)viewportSize.width, (float)viewportSize.height));
  pView->ForceUpdate();
}

void RasterizerExplorerState::ProcessInput()
{
#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (ezImgui::GetSingleton())
  {
    ezImgui::GetSingleton()->SetPassInputToImgui(true);

    if (ezImgui::GetSingleton()->WantsInput())
      return;
  }
#endif

  SUPER::ProcessInput();

  if (ezInputManager::GetInputActionState("Rasterizer", "NextCube") == ezKeyState::Pressed)
  {
    m_iSelectedBuilding = (m_iSelectedBuilding + 1) % (ezInt32)m_Buildings.GetCount();
  }

  if (ezInputManager::GetInputActionState("Rasterizer", "PrevCube") == ezKeyState::Pressed)
  {
    m_iSelectedBuilding = (m_iSelectedBuilding - 1 + (ezInt32)m_Buildings.GetCount()) % (ezInt32)m_Buildings.GetCount();
  }

  if (ezInputManager::GetInputActionState("Rasterizer", "OverlayOptimized") == ezKeyState::Pressed)
    m_Overlay = (m_Overlay == RasterizerOverlay::Optimized) ? RasterizerOverlay::None : RasterizerOverlay::Optimized;
  if (ezInputManager::GetInputActionState("Rasterizer", "OverlayGeneric") == ezKeyState::Pressed)
    m_Overlay = (m_Overlay == RasterizerOverlay::Generic) ? RasterizerOverlay::None : RasterizerOverlay::Generic;
  if (ezInputManager::GetInputActionState("Rasterizer", "OverlayMOC") == ezKeyState::Pressed)
    m_Overlay = (m_Overlay == RasterizerOverlay::MOC) ? RasterizerOverlay::None : RasterizerOverlay::MOC;
  if (ezInputManager::GetInputActionState("Rasterizer", "OverlayGenericMOC") == ezKeyState::Pressed)
    m_Overlay = (m_Overlay == RasterizerOverlay::GenericMOC) ? RasterizerOverlay::None : RasterizerOverlay::GenericMOC;
  if (ezInputManager::GetInputActionState("Rasterizer", "OverlayDiff") == ezKeyState::Pressed)
    m_Overlay = (m_Overlay == RasterizerOverlay::Diff) ? RasterizerOverlay::None : RasterizerOverlay::Diff;

  float fRotateSpeed = 180.0f;
  float fMoveSpeed = 10.0f;
  float fInput = 0.0f;

  if (ezInputManager::GetInputActionState("Game", "Run", &fInput) != ezKeyState::Up)
    fMoveSpeed *= 10.0f;

  if (ezInputManager::GetInputActionState("Game", "MoveForwards", &fInput) != ezKeyState::Up)
    m_MainCamera.MoveLocally(fInput * fMoveSpeed, 0, 0);
  if (ezInputManager::GetInputActionState("Game", "MoveBackwards", &fInput) != ezKeyState::Up)
    m_MainCamera.MoveLocally(-fInput * fMoveSpeed, 0, 0);
  if (ezInputManager::GetInputActionState("Game", "MoveLeft", &fInput) != ezKeyState::Up)
    m_MainCamera.MoveLocally(0, -fInput * fMoveSpeed, 0);
  if (ezInputManager::GetInputActionState("Game", "MoveRight", &fInput) != ezKeyState::Up)
    m_MainCamera.MoveLocally(0, fInput * fMoveSpeed, 0);
  if (ezInputManager::GetInputActionState("Game", "MoveUp", &fInput) != ezKeyState::Up)
    m_MainCamera.MoveGlobally(0, 0, fInput * fMoveSpeed);
  if (ezInputManager::GetInputActionState("Game", "MoveDown", &fInput) != ezKeyState::Up)
    m_MainCamera.MoveGlobally(0, 0, -fInput * fMoveSpeed);

  if (ezInputManager::GetInputActionState("Game", "Look") == ezKeyState::Down)
  {
    if (ezInputManager::GetInputActionState("Game", "TurnLeft", &fInput) != ezKeyState::Up)
      m_MainCamera.RotateGlobally(ezAngle(), ezAngle(), ezAngle::MakeFromDegree(-fRotateSpeed * fInput));
    if (ezInputManager::GetInputActionState("Game", "TurnRight", &fInput) != ezKeyState::Up)
      m_MainCamera.RotateGlobally(ezAngle(), ezAngle(), ezAngle::MakeFromDegree(fRotateSpeed * fInput));
    if (ezInputManager::GetInputActionState("Game", "TurnUp", &fInput) != ezKeyState::Up)
      m_MainCamera.RotateLocally(ezAngle(), ezAngle::MakeFromDegree(fRotateSpeed * fInput), ezAngle());
    if (ezInputManager::GetInputActionState("Game", "TurnDown", &fInput) != ezKeyState::Up)
      m_MainCamera.RotateLocally(ezAngle(), ezAngle::MakeFromDegree(-fRotateSpeed * fInput), ezAngle());
  }
}

void RasterizerExplorerState::GenerateCity()
{
  ezRandom rng;
  rng.Initialize(123);

  m_Buildings.Clear();

  for (ezUInt32 gx = 0; gx < GridSize; ++gx)
  {
    for (ezUInt32 gz = 0; gz < GridSize; ++gz)
    {
      const float fHeight = 2.0f + (float)rng.DoubleMinMax(0.0, 8.0);
      const float fWidth = BlockSize * (0.6f + (float)rng.DoubleMinMax(0.0, 0.4));
      const float fDepth = BlockSize * (0.6f + (float)rng.DoubleMinMax(0.0, 0.4));

      const float x = -GroundHalf + StreetWidth + gx * CellSize + BlockSize * 0.5f;
      const float y = -GroundHalf + StreetWidth + gz * CellSize + BlockSize * 0.5f;
      const float z = fHeight * 0.5f;

      auto& b = m_Buildings.ExpandAndGetRef();
      b.m_vExtents = ezVec3(fWidth, fDepth, fHeight);
      b.m_Transform = ezTransform(ezVec3(x, y, z));
    }
  }
}

void RasterizerExplorerState::RunRasterizers()
{
  auto pGround = ezRasterizerObject::CreateBox(ezVec3(GroundSize, GroundSize, 0.5f));

  // Use the main view's viewport size so the rasterizer matches the screen
  ezUInt32 uiWidth = RasterizerSize;
  ezUInt32 uiHeight = RasterizerSize;
  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hMainView, pView))
  {
    const auto& vp = pView->GetViewport();
    uiWidth = ezMath::Max(1u, (ezUInt32)vp.width);
    uiHeight = ezMath::Max(1u, (ezUInt32)vp.height);
  }

  auto RunSingle = [&](ezRasterizerView* pRasterView) -> ezTime
  {
    pRasterView->SetResolution(uiWidth, uiHeight, 0.0f);
    pRasterView->SetCamera(&m_MainCamera);

    const ezTime tStart = ezTime::Now();

    pRasterView->BeginScene();

    // Ground plane
    pRasterView->AddObject(pGround.Borrow(), ezTransform(ezVec3(0, 0, -0.25f)));

    // Buildings
    for (ezUInt32 i = 0; i < m_Buildings.GetCount(); ++i)
    {
      auto pBuilding = ezRasterizerObject::CreateBox(m_Buildings[i].m_vExtents);
      pRasterView->AddObject(pBuilding.Borrow(), m_Buildings[i].m_Transform);
    }

    pRasterView->EndScene();

    return ezTime::Now() - tStart;
  };

  m_Perf.m_LastOptimized = RunSingle(m_pRasterizerViewOptimized.Borrow());
  m_Perf.m_LastGeneric = RunSingle(m_pRasterizerViewGeneric.Borrow());
  m_Perf.m_LastMOC = RunSingle(m_pRasterizerViewMOC.Borrow());
  m_Perf.m_LastGenericMOC = RunSingle(m_pRasterizerViewGenericMOC.Borrow());

  // Exponential moving average (smoothing factor ~0.05)
  constexpr double fAlpha = 0.05;
  m_Perf.m_fAvgOptimizedMs += (m_Perf.m_LastOptimized.GetMilliseconds() - m_Perf.m_fAvgOptimizedMs) * fAlpha;
  m_Perf.m_fAvgGenericMs += (m_Perf.m_LastGeneric.GetMilliseconds() - m_Perf.m_fAvgGenericMs) * fAlpha;
  m_Perf.m_fAvgMOCMs += (m_Perf.m_LastMOC.GetMilliseconds() - m_Perf.m_fAvgMOCMs) * fAlpha;
  m_Perf.m_fAvgGenericMOCMs += (m_Perf.m_LastGenericMOC.GetMilliseconds() - m_Perf.m_fAvgGenericMOCMs) * fAlpha;
}

void RasterizerExplorerState::DrawDebugGeometry()
{
  // Draw ground plane outline
  const ezBoundingBox groundBox = ezBoundingBox::MakeFromMinMax(
    ezVec3(-GroundHalf, -GroundHalf, -0.5f),
    ezVec3(GroundHalf, GroundHalf, 0.0f));
  ezDebugRenderer::DrawLineBox(m_pMainWorld, groundBox, ezColor::DarkGray);

  // Draw buildings
  for (ezUInt32 i = 0; i < m_Buildings.GetCount(); ++i)
  {
    const auto& b = m_Buildings[i];
    const ezVec3 half = b.m_vExtents * 0.5f;
    const ezBoundingBox localBox = ezBoundingBox::MakeFromMinMax(-half, half);

    ezColor color = (m_iSelectedBuilding == (ezInt32)i) ? ezColor::Orange : ezColor::GreenYellow;
    ezDebugRenderer::DrawLineBox(m_pMainWorld, localBox, color, b.m_Transform);
  }

  ezDebugRenderer::Draw2DText(m_pMainWorld,
    ezFmt("City Scene - Buildings: {}", m_Buildings.GetCount()),
    ezVec2I32(10, 10), ezColor::White);

  ezDebugRenderer::Draw2DText(m_pMainWorld,
    "WASD = move, RMB+mouse = look, Up/Down = select building, ESC = quit",
    ezVec2I32(10, 30), ezColor::LightGray);

  // Query visibility of the selected building against all rasterizers
  if (m_iSelectedBuilding >= 0 && m_iSelectedBuilding < (ezInt32)m_Buildings.GetCount())
  {
    const auto& b = m_Buildings[m_iSelectedBuilding];
    const ezVec3 half = b.m_vExtents * 0.5f;
    const ezBoundingBox localBox2 = ezBoundingBox::MakeFromMinMax(-half, half);
    const ezMat4 mTransform = b.m_Transform.GetAsMat4();

    ezVec3 corners[8];
    localBox2.GetCorners(corners);

    ezBoundingBox worldBox = ezBoundingBox::MakeInvalid();
    for (ezUInt32 c = 0; c < 8; ++c)
    {
      worldBox.ExpandToInclude(mTransform.TransformPosition(corners[c]));
    }

    ezSimdBBox simdBox;
    simdBox.m_Min.Load<3>(worldBox.m_vMin.GetData());
    simdBox.m_Max.Load<3>(worldBox.m_vMax.GetData());
    simdBox.m_Min.SetW(1);
    simdBox.m_Max.SetW(1);

    const bool bVisOpt = m_pRasterizerViewOptimized->IsVisible(simdBox);
    const bool bVisGen = m_pRasterizerViewGeneric->IsVisible(simdBox);
    const bool bVisMOC = m_pRasterizerViewMOC->IsVisible(simdBox);
    const bool bVisGMOC = m_pRasterizerViewGenericMOC->IsVisible(simdBox);

    const bool bAllAgree = (bVisOpt == bVisGen) && (bVisGen == bVisMOC) && (bVisMOC == bVisGMOC);

    ezDebugRenderer::Draw2DText(m_pMainWorld,
      ezFmt("Building {} - Opt: {}  Gen: {}  MOC: {}  GMOC: {}", m_iSelectedBuilding,
        bVisOpt ? "VIS" : "OCC",
        bVisGen ? "VIS" : "OCC",
        bVisMOC ? "VIS" : "OCC",
        bVisGMOC ? "VIS" : "OCC"),
      ezVec2I32(10, 50), bAllAgree ? ezColor::White : ezColor::Red);
  }
}

void RasterizerExplorerState::BeforeWorldUpdate()
{
  SUPER::BeforeWorldUpdate();

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  DrawImGuiPanel();
  DrawPerfPanel();
#endif
}

void RasterizerExplorerState::AfterWorldUpdate()
{
  SUPER::AfterWorldUpdate();

  RunRasterizers();
  UpdateOverlayTexture();
  DrawDebugGeometry();
}

void RasterizerExplorerState::DrawImGuiPanel()
{
#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (ezImgui::GetSingleton() == nullptr)
    return;

  ezImgui::GetSingleton()->SetCurrentContextForView(m_hMainView);

  ImGui::SetNextWindowSize(ImVec2(280, 500), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowPos(ImVec2(10, 60), ImGuiCond_FirstUseEver);

  static const char* szOverlayNames[] = {"None", "Optimized (1)", "Generic (2)", "MOC (3)", "GenericMOC (4)", "Diff (5)"};

  if (ImGui::Begin("City"))
  {
    // Overlay controls
    ImGui::Text("Overlay:");
    int iOverlay = (int)m_Overlay;
    for (int n = 0; n < (int)RasterizerOverlay::COUNT; ++n)
    {
      if (n > 0)
        ImGui::SameLine();
      if (ImGui::RadioButton(szOverlayNames[n], iOverlay == n))
        m_Overlay = (RasterizerOverlay)n;
    }

    if (ImGui::Button("Save Settings"))
    {
      SaveSettings();
    }
    ImGui::SameLine();
    if (ImGui::Button("Print LookAt"))
    {
      const ezVec3 vPos = m_MainCamera.GetCenterPosition();
      const ezVec3 vDir = m_MainCamera.GetCenterDirForwards();
      const ezVec3 vUp = m_MainCamera.GetCenterDirUp();
      const ezVec3 vTarget = vPos + vDir;
      ezLog::Info("camera.LookAt(ezVec3({}f, {}f, {}f), ezVec3({}f, {}f, {}f), ezVec3({}f, {}f, {}f));",
        vPos.x, vPos.y, vPos.z, vTarget.x, vTarget.y, vTarget.z, vUp.x, vUp.y, vUp.z);
    }

    // Max occluders CVar
    if (auto pCVar = static_cast<ezCVarInt*>(ezCVar::FindCVarByName("Spatial.Occlusion.MaxOccluders")))
    {
      int iMaxOccluders = pCVar->GetValue();
      if (ImGui::SliderInt("Max Occluders", &iMaxOccluders, 1, 200))
      {
        *pCVar = iMaxOccluders;
      }
    }

    ImGui::Separator();

    // Cull controls
    ImGui::Checkbox("Cull Nearby Objects", &m_bCullNearby);
    if (m_bCullNearby)
    {
      ImGui::SliderFloat("Cull Distance", &m_fCullDistance, 0.5f, 30.0f, "%.1f");
    }

    if (m_iSelectedBuilding >= 0)
    {
      ImGui::Text("Selected: Building %d", m_iSelectedBuilding);
    }
    else
    {
      ImGui::TextDisabled("No building selected");
    }

    ImGui::Separator();

    if (ImGui::BeginChild("BuildingList", ImVec2(0, 0), ImGuiChildFlags_None))
    {
      for (ezUInt32 i = 0; i < m_Buildings.GetCount(); ++i)
      {
        ezStringBuilder sLabel;
        sLabel.SetFormat("Building {}", i);

        bool bIsSelected = (m_iSelectedBuilding == (ezInt32)i);

        if (ImGui::Selectable(sLabel, bIsSelected))
        {
          m_iSelectedBuilding = bIsSelected ? -1 : (ezInt32)i;
        }
      }
    }
    ImGui::EndChild();
  }
  ImGui::End();
#endif
}

void RasterizerExplorerState::DrawPerfPanel()
{
#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (ezImgui::GetSingleton() == nullptr)
    return;

  ezImgui::GetSingleton()->SetCurrentContextForView(m_hMainView);

  ImGui::SetNextWindowSize(ImVec2(260, 0), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowPos(ImVec2(300, 60), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Rasterizer Performance"))
  {
    const ezUInt32 uiW = m_pRasterizerViewOptimized->GetResolutionX();
    const ezUInt32 uiH = m_pRasterizerViewOptimized->GetResolutionY();
    ImGui::Text("Resolution: %u x %u", uiW, uiH);
    ImGui::Text("Occluders:  %u", m_Buildings.GetCount() + 1); // buildings + ground
    ImGui::Separator();

    ImGui::Columns(3, "perf_cols");
    ImGui::SetColumnWidth(0, 90);
    ImGui::SetColumnWidth(1, 80);
    ImGui::SetColumnWidth(2, 80);

    ImGui::Text("Impl");
    ImGui::NextColumn();
    ImGui::Text("Last (ms)");
    ImGui::NextColumn();
    ImGui::Text("Avg (ms)");
    ImGui::NextColumn();
    ImGui::Separator();

    ImGui::Text("Optimized");
    ImGui::NextColumn();
    ImGui::Text("%.3f", m_Perf.m_LastOptimized.GetMilliseconds());
    ImGui::NextColumn();
    ImGui::Text("%.3f", m_Perf.m_fAvgOptimizedMs);
    ImGui::NextColumn();

    ImGui::Text("Generic");
    ImGui::NextColumn();
    ImGui::Text("%.3f", m_Perf.m_LastGeneric.GetMilliseconds());
    ImGui::NextColumn();
    ImGui::Text("%.3f", m_Perf.m_fAvgGenericMs);
    ImGui::NextColumn();

    ImGui::Text("MOC");
    ImGui::NextColumn();
    ImGui::Text("%.3f", m_Perf.m_LastMOC.GetMilliseconds());
    ImGui::NextColumn();
    ImGui::Text("%.3f", m_Perf.m_fAvgMOCMs);
    ImGui::NextColumn();

    ImGui::Text("GenericMOC");
    ImGui::NextColumn();
    ImGui::Text("%.3f", m_Perf.m_LastGenericMOC.GetMilliseconds());
    ImGui::NextColumn();
    ImGui::Text("%.3f", m_Perf.m_fAvgGenericMOCMs);
    ImGui::NextColumn();

    ImGui::Columns(1);

    if (m_Perf.m_fAvgOptimizedMs > 0.0001)
    {
      ImGui::Separator();
      ImGui::Text("Relative (vs Optimized):");
      ImGui::Text("  Generic:    %.2fx", m_Perf.m_fAvgGenericMs / m_Perf.m_fAvgOptimizedMs);
      ImGui::Text("  MOC:        %.2fx", m_Perf.m_fAvgMOCMs / m_Perf.m_fAvgOptimizedMs);
      ImGui::Text("  GenericMOC: %.2fx", m_Perf.m_fAvgGenericMOCMs / m_Perf.m_fAvgOptimizedMs);
    }
  }
  ImGui::End();
#endif
}

void RasterizerExplorerState::CreateOverlayObject()
{
  // Create a game object with the overlay component (texture created lazily in UpdateOverlayTexture)
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  ezGameObjectDesc objDesc;
  objDesc.m_sName.Assign("RasterizerOverlay");

  ezGameObject* pObject = nullptr;
  m_hOverlayObject = m_pMainWorld->CreateObject(objDesc, pObject);

  ezRasterizerOverlayComponent* pComp = nullptr;
  m_hOverlayComponent = ezRasterizerOverlayComponent::CreateComponent(pObject, pComp);
}

void RasterizerExplorerState::DestroyOverlayObject()
{
  if (!m_hOverlayObject.IsInvalidated() && m_pMainWorld)
  {
    EZ_LOCK(m_pMainWorld->GetWriteMarker());
    m_pMainWorld->DeleteObjectNow(m_hOverlayObject);
    m_hOverlayObject.Invalidate();
  }

  auto pDevice = ezGALDevice::GetDefaultDevice();
  if (!m_hOverlayTexture.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hOverlayTexture);
    m_hOverlayTexture.Invalidate();
  }
}

void RasterizerExplorerState::UpdateOverlayTexture()
{
  // Get the rasterizer's actual resolution
  const ezUInt32 uiWidth = m_pRasterizerViewOptimized->GetResolutionX();
  const ezUInt32 uiHeight = m_pRasterizerViewOptimized->GetResolutionY();

  if (uiWidth == 0 || uiHeight == 0)
    return;

  auto pDevice = ezGALDevice::GetDefaultDevice();

  // Recreate the overlay texture if the resolution changed
  if (!m_hOverlayTexture.IsInvalidated())
  {
    const ezGALTexture* pTex = pDevice->GetTexture(m_hOverlayTexture);
    if (pTex == nullptr || pTex->GetDescription().m_uiWidth != uiWidth || pTex->GetDescription().m_uiHeight != uiHeight)
    {
      pDevice->DestroyTexture(m_hOverlayTexture);
      m_hOverlayTexture.Invalidate();
    }
  }

  if (m_hOverlayTexture.IsInvalidated())
  {
    ezGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth = uiWidth;
    texDesc.m_uiHeight = uiHeight;
    texDesc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    texDesc.m_ResourceAccess.m_bImmutable = false;
    m_hOverlayTexture = pDevice->CreateTexture(texDesc);
  }

  // Update the component's texture handle
  {
    EZ_LOCK(m_pMainWorld->GetWriteMarker());

    ezRasterizerOverlayComponent* pComp = nullptr;
    if (m_pMainWorld->TryGetComponent(m_hOverlayComponent, pComp))
    {
      pComp->SetActiveFlag(m_Overlay != RasterizerOverlay::None);
      pComp->SetTextureHandle(m_hOverlayTexture);
    }
  }

  if (m_Overlay == RasterizerOverlay::None)
    return;

  const ezUInt32 uiPixelCount = uiWidth * uiHeight;
  m_OverlayBuffer.SetCountUninitialized(uiPixelCount);

  if (m_Overlay == RasterizerOverlay::Optimized)
  {
    m_pRasterizerViewOptimized->ReadBackFrame(m_OverlayBuffer);
  }
  else if (m_Overlay == RasterizerOverlay::Generic)
  {
    m_pRasterizerViewGeneric->ReadBackFrame(m_OverlayBuffer);
  }
  else if (m_Overlay == RasterizerOverlay::MOC)
  {
    m_pRasterizerViewMOC->ReadBackFrame(m_OverlayBuffer);
  }
  else if (m_Overlay == RasterizerOverlay::GenericMOC)
  {
    m_pRasterizerViewGenericMOC->ReadBackFrame(m_OverlayBuffer);
  }
  else if (m_Overlay == RasterizerOverlay::Diff)
  {
    ezDynamicArray<ezColorLinearUB> bufA, bufB;
    bufA.SetCountUninitialized(uiPixelCount);
    bufB.SetCountUninitialized(uiPixelCount);
    m_pRasterizerViewOptimized->ReadBackFrame(bufA);
    m_pRasterizerViewGeneric->ReadBackFrame(bufB);

    ezImageHeader header;
    header.SetWidth(uiWidth);
    header.SetHeight(uiHeight);
    header.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);

    ezImageView viewA, viewB;
    viewA.ResetAndViewExternalStorage(header, ezConstByteBlobPtr(reinterpret_cast<const ezUInt8*>(bufA.GetData()), bufA.GetCount() * sizeof(ezColorLinearUB)));
    viewB.ResetAndViewExternalStorage(header, ezConstByteBlobPtr(reinterpret_cast<const ezUInt8*>(bufB.GetData()), bufB.GetCount() * sizeof(ezColorLinearUB)));

    ezImage diffImage;
    ezImageUtils::ComputeImageDifferenceABS(viewA, viewB, diffImage);

    auto diffData = diffImage.GetBlobPtr<ezColorLinearUB>();
    ezMemoryUtils::Copy(m_OverlayBuffer.GetData(), diffData.GetPtr(), m_OverlayBuffer.GetCount());
  }

  // Upload to GPU
  ezGALSystemMemoryDescription sourceData;
  sourceData.m_pData = ezMakeArrayPtr(reinterpret_cast<ezUInt8*>(m_OverlayBuffer.GetData()), m_OverlayBuffer.GetCount() * sizeof(ezColorLinearUB));
  sourceData.m_uiRowPitch = uiWidth * sizeof(ezColorLinearUB);
  pDevice->UpdateTextureForNextFrame(m_hOverlayTexture, sourceData);
}

void RasterizerExplorerState::SaveSettings()
{
  ezFileWriter file;
  if (file.Open(":appdata/RasterizerExplorer.ddl").Failed())
  {
    ezLog::Warning("Failed to save settings.");
    return;
  }

  ezOpenDdlWriter writer;
  writer.SetOutputStream(&file);

  writer.BeginObject("Settings");
  {
    const ezVec3 vPos = m_MainCamera.GetCenterPosition();
    const ezVec3 vDir = m_MainCamera.GetCenterDirForwards();
    const ezVec3 vUp = m_MainCamera.GetCenterDirUp();

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float, "CamPosX");
    writer.WriteFloat(&vPos.x);
    writer.EndPrimitiveList();

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float, "CamPosY");
    writer.WriteFloat(&vPos.y);
    writer.EndPrimitiveList();

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float, "CamPosZ");
    writer.WriteFloat(&vPos.z);
    writer.EndPrimitiveList();

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float, "CamDirX");
    writer.WriteFloat(&vDir.x);
    writer.EndPrimitiveList();

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float, "CamDirY");
    writer.WriteFloat(&vDir.y);
    writer.EndPrimitiveList();

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float, "CamDirZ");
    writer.WriteFloat(&vDir.z);
    writer.EndPrimitiveList();

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float, "CamUpX");
    writer.WriteFloat(&vUp.x);
    writer.EndPrimitiveList();

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float, "CamUpY");
    writer.WriteFloat(&vUp.y);
    writer.EndPrimitiveList();

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float, "CamUpZ");
    writer.WriteFloat(&vUp.z);
    writer.EndPrimitiveList();

    ezOpenDdlUtils::StoreInt32(writer, m_iSelectedBuilding, "SelectedCube");
    ezOpenDdlUtils::StoreBool(writer, m_bCullNearby, "CullNearby");
    ezOpenDdlUtils::StoreFloat(writer, m_fCullDistance, "CullDistance");
    ezOpenDdlUtils::StoreInt32(writer, (ezInt32)m_Overlay, "Overlay");
  }
  writer.EndObject();

  ezLog::Info("Settings saved.");
}

void RasterizerExplorerState::LoadSettings()
{
  ezFileReader file;
  if (file.Open(":appdata/RasterizerExplorer.ddl").Failed())
    return;

  ezOpenDdlReader reader;
  if (reader.ParseDocument(file).Failed())
    return;

  const ezOpenDdlReaderElement* pRoot = reader.GetRootElement();
  const ezOpenDdlReaderElement* pSettings = pRoot ? pRoot->FindChildOfType("Settings") : nullptr;
  if (pSettings == nullptr)
    return;

  auto ReadFloat = [&](const char* szName, float fDefault) -> float
  {
    if (auto p = pSettings->FindChildOfType(ezOpenDdlPrimitiveType::Float, szName))
      return p->GetPrimitivesFloat()[0];
    return fDefault;
  };

  ezVec3 vPos;
  vPos.x = ReadFloat("CamPosX", -15.0f);
  vPos.y = ReadFloat("CamPosY", 0.0f);
  vPos.z = ReadFloat("CamPosZ", 0.0f);

  ezVec3 vDir;
  vDir.x = ReadFloat("CamDirX", 1.0f);
  vDir.y = ReadFloat("CamDirY", 0.0f);
  vDir.z = ReadFloat("CamDirZ", 0.0f);

  ezVec3 vUp;
  vUp.x = ReadFloat("CamUpX", 0.0f);
  vUp.y = ReadFloat("CamUpY", 0.0f);
  vUp.z = ReadFloat("CamUpZ", 1.0f);

  m_MainCamera.LookAt(vPos, vPos + vDir, vUp);

  if (auto p = pSettings->FindChildOfType(ezOpenDdlPrimitiveType::Int32, "SelectedCube"))
    m_iSelectedBuilding = p->GetPrimitivesInt32()[0];

  if (auto p = pSettings->FindChildOfType(ezOpenDdlPrimitiveType::Bool, "CullNearby"))
    m_bCullNearby = p->GetPrimitivesBool()[0];

  if (auto p = pSettings->FindChildOfType(ezOpenDdlPrimitiveType::Float, "CullDistance"))
    m_fCullDistance = p->GetPrimitivesFloat()[0];

  if (auto p = pSettings->FindChildOfType(ezOpenDdlPrimitiveType::Int32, "Overlay"))
  {
    ezInt32 iOverlay = p->GetPrimitivesInt32()[0];
    if (iOverlay >= 0 && iOverlay < (ezInt32)RasterizerOverlay::COUNT)
      m_Overlay = (RasterizerOverlay)iOverlay;
  }

  ezLog::Info("Settings loaded.");
}
