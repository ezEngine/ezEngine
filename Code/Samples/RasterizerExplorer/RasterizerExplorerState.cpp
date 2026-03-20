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

  m_MainCamera.LookAt(ezVec3(-15, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
  m_MainCamera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (ezImgui::GetSingleton() == nullptr)
  {
    EZ_DEFAULT_NEW(ezImgui);
  }
#endif

  m_pRasterizerViewOptimized = EZ_DEFAULT_NEW(ezRasterizerView, true);
  m_pRasterizerViewGeneric = EZ_DEFAULT_NEW(ezRasterizerView, false);

  CreateOverlayObject();

  GenerateTransforms();

  LoadSettings();
}

void RasterizerExplorerState::OnDeactivation()
{
  DestroyOverlayObject();

  m_pRasterizerViewOptimized.Clear();
  m_pRasterizerViewGeneric.Clear();

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
  RegisterInputActionNoTimeScale("Rasterizer", "OverlayDiff", ezInputSlot_Key3);
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
    m_iSelectedCube = (m_iSelectedCube + 1) % (ezInt32)NumBoxes;
  }

  if (ezInputManager::GetInputActionState("Rasterizer", "PrevCube") == ezKeyState::Pressed)
  {
    m_iSelectedCube = (m_iSelectedCube - 1 + (ezInt32)NumBoxes) % (ezInt32)NumBoxes;
  }

  if (ezInputManager::GetInputActionState("Rasterizer", "OverlayOptimized") == ezKeyState::Pressed)
    m_Overlay = (m_Overlay == RasterizerOverlay::Optimized) ? RasterizerOverlay::None : RasterizerOverlay::Optimized;
  if (ezInputManager::GetInputActionState("Rasterizer", "OverlayGeneric") == ezKeyState::Pressed)
    m_Overlay = (m_Overlay == RasterizerOverlay::Generic) ? RasterizerOverlay::None : RasterizerOverlay::Generic;
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

void RasterizerExplorerState::GenerateTransforms()
{
  ezRandom rng;
  rng.Initialize(42);

  m_Transforms.SetCount(NumBoxes);

  for (ezUInt32 i = 0; i < NumBoxes; ++i)
  {
    const float x = (float)rng.DoubleMinMax(-8.0, 8.0);
    const float y = (float)rng.DoubleMinMax(-8.0, 8.0);
    const float z = (float)rng.DoubleMinMax(-5.0, 5.0);

    const float ax = (float)rng.DoubleMinMax(0.0, 360.0);
    const float ay = (float)rng.DoubleMinMax(0.0, 360.0);
    const float az = (float)rng.DoubleMinMax(0.0, 360.0);

    ezQuat rot = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(ax), ezAngle::MakeFromDegree(ay), ezAngle::MakeFromDegree(az));
    m_Transforms[i] = ezTransform(ezVec3(x, y, z), rot);
  }
}

bool RasterizerExplorerState::IsCubeVisible(ezUInt32 uiIndex) const
{
  if (!m_bCullNearby || m_iSelectedCube < 0)
    return true;

  const ezVec3 vSelected = m_Transforms[m_iSelectedCube].m_vPosition;
  const ezVec3 vCube = m_Transforms[uiIndex].m_vPosition;
  return (vCube - vSelected).GetLengthSquared() <= m_fCullDistance * m_fCullDistance;
}

void RasterizerExplorerState::RunRasterizers()
{
  auto pBox = ezRasterizerObject::CreateBox(ezVec3(2, 2, 2));

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

  ezRasterizerView* rasterViews[] = {m_pRasterizerViewOptimized.Borrow(), m_pRasterizerViewGeneric.Borrow()};

  for (ezRasterizerView* pRasterView : rasterViews)
  {
    pRasterView->SetResolution(uiWidth, uiHeight, 0.0f);
    pRasterView->SetCamera(&m_MainCamera);

    pRasterView->BeginScene();
    for (ezUInt32 i = 0; i < NumBoxes; ++i)
    {
      if (IsCubeVisible(i))
      {
        pRasterView->AddObject(pBox.Borrow(), m_Transforms[i]);
      }
    }
    pRasterView->EndScene();
  }
}

void RasterizerExplorerState::DrawDebugGeometry()
{
  const ezVec3 vBoxHalfExtents(1, 1, 1);
  const ezBoundingBox localBox = ezBoundingBox::MakeFromMinMax(-vBoxHalfExtents, vBoxHalfExtents);

  ezUInt32 uiVisibleCount = 0;
  for (ezUInt32 i = 0; i < NumBoxes; ++i)
  {
    if (!IsCubeVisible(i))
      continue;

    ++uiVisibleCount;

    ezColor color = (m_iSelectedCube == (ezInt32)i) ? ezColor::Orange : ezColor::GreenYellow;
    ezDebugRenderer::DrawLineBox(m_pMainWorld, localBox, color, m_Transforms[i]);
  }

  // Draw cull radius sphere around selected cube
  if (m_bCullNearby && m_iSelectedCube >= 0)
  {
    ezBoundingSphere sphere = ezBoundingSphere::MakeFromCenterAndRadius(m_Transforms[m_iSelectedCube].m_vPosition, m_fCullDistance);
    ezDebugRenderer::DrawLineSphere(m_pMainWorld, sphere, ezColor::CornflowerBlue);
  }

  ezDebugRenderer::Draw2DText(m_pMainWorld,
    ezFmt("Rasterizer Explorer - Visible: {} / {}", uiVisibleCount, NumBoxes),
    ezVec2I32(10, 10), ezColor::White);

  ezDebugRenderer::Draw2DText(m_pMainWorld,
    "WASD = move, RMB+mouse = look, ESC = quit",
    ezVec2I32(10, 30), ezColor::LightGray);

  // Query visibility of the selected cube against both rasterizers
  if (m_iSelectedCube >= 0 && IsCubeVisible(m_iSelectedCube))
  {
    const ezBoundingBox localBox2 = ezBoundingBox::MakeFromMinMax(-vBoxHalfExtents, vBoxHalfExtents);
    const ezTransform& t = m_Transforms[m_iSelectedCube];
    const ezMat4 mTransform = t.GetAsMat4();

    // Transform the 8 corners to get the world-space AABB
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

    ezDebugRenderer::Draw2DText(m_pMainWorld,
      ezFmt("Cube {} - Optimized: {}  Generic: {}", m_iSelectedCube, bVisOpt ? "VISIBLE" : "OCCLUDED", bVisGen ? "VISIBLE" : "OCCLUDED"),
      ezVec2I32(10, 50), bVisOpt == bVisGen ? ezColor::White : ezColor::Red);
  }
}

void RasterizerExplorerState::BeforeWorldUpdate()
{
  SUPER::BeforeWorldUpdate();

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  DrawImGuiPanel();
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

  static const char* szOverlayNames[] = {"None", "Optimized (1)", "Generic (2)", "Diff (3)"};

  if (ImGui::Begin("Cubes"))
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

    if (m_iSelectedCube >= 0)
    {
      ImGui::Text("Selected: Cube %d", m_iSelectedCube);
    }
    else
    {
      ImGui::TextDisabled("No cube selected");
    }

    ImGui::Separator();

    if (ImGui::BeginChild("CubeList", ImVec2(0, 0), ImGuiChildFlags_None))
    {
      for (ezUInt32 i = 0; i < NumBoxes; ++i)
      {
        ezStringBuilder sLabel;
        sLabel.SetFormat("Cube {}", i);

        bool bIsSelected = (m_iSelectedCube == (ezInt32)i);

        if (ImGui::Selectable(sLabel, bIsSelected))
        {
          m_iSelectedCube = bIsSelected ? -1 : (ezInt32)i;
        }
      }
    }
    ImGui::EndChild();
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

    ezOpenDdlUtils::StoreInt32(writer, m_iSelectedCube, "SelectedCube");
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
    m_iSelectedCube = p->GetPrimitivesInt32()[0];

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
