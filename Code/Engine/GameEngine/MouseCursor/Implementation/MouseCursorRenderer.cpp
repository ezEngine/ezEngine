#include <GameEngine/GameEnginePCH.h>

#include "../../../../../Data/Base/Shaders/MouseCursor/MouseCursorConstants.h"
#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <GameEngine/MouseCursor/MouseCursorRenderer.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_IMPLEMENT_SINGLETON(ezMouseCursorRenderer);

EZ_BEGIN_SUBSYSTEM_DECLARATION(GameEngine, MouseCursorRenderer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezMouseCursorRenderer);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezMouseCursorRenderer* pDummy = ezMouseCursorRenderer::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezMouseCursorRenderer::ezMouseCursorRenderer()
  : m_SingletonRegistrar(this)
{
  // Only subscribe here, don't create any GPU resources yet. The GAL device may not even exist,
  // and an application that never sets a cursor shouldn't pay for this at all.
  ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&ezMouseCursorRenderer::OnGALDeviceEvent, this));
}

ezMouseCursorRenderer::~ezMouseCursorRenderer()
{
  ezGALDevice::s_Events.RemoveEventHandler(ezMakeDelegate(&ezMouseCursorRenderer::OnGALDeviceEvent, this));

  // Without this renderer nothing would draw the custom cursor anymore, but the ezInputManager would
  // still consider one to be set and thus keep the OS cursor hidden. Clearing it restores the OS cursor.
  ezInputManager::ClearMouseCursor();

  ReleaseGpuResources();
}

void ezMouseCursorRenderer::OnGALDeviceEvent(const ezGALDeviceEvent& e)
{
  if (e.m_Type == ezGALDeviceEvent::BeforeShutdown)
  {
    ReleaseGpuResources();
    return;
  }

  const bool bCustomCursorActive = ezInputManager::IsCustomMouseCursorActive();

  if (e.m_Type == ezGALDeviceEvent::BeforeBeginFrame)
  {
    if (!bCustomCursorActive || m_hSwapChain.IsInvalidated())
      return;

    // ezRenderWorld only enqueues swap-chains that a valid view renders into, and a view requires a
    // world. Enqueue it ourselves, so that the cursor is also rendered when no world is active.
    // ezGALDevice::EnqueueFrameSwapChain ignores duplicates.
    e.m_pDevice->EnqueueFrameSwapChain(m_hSwapChain);
    return;
  }

  if (e.m_Type != ezGALDeviceEvent::AfterBeginFrame)
    return;

  if (!bCustomCursorActive || m_hSwapChain.IsInvalidated())
    return;

  ezGALDevice* pDevice = e.m_pDevice;

  const ezGALSwapChain* pSwapChain = pDevice->GetSwapChain(m_hSwapChain);
  if (pSwapChain == nullptr)
    return;

  const ezGALTextureHandle hBackBuffer = pSwapChain->GetBackBufferTexture();
  if (hBackBuffer.IsInvalidated())
    return;

  if (EnsureGpuResourcesExist().Failed())
    return;

  // Take a copy of the description. The execute callback may run on another thread, while the
  // game code is already modifying the cursor for the next frame.
  ezMouseCursorDesc desc = ezInputManager::GetMouseCursor();

  // Using the OS cursor size as the base is what makes a custom cursor look right independent of
  // resolution, window size, monitor and DPI scaling.
  const float fCursorSize = (float)ezInputManager::GetHardwareCursorSize() * desc.m_fSize;
  const ezVec2 vCursorSize(fCursorSize, fCursorSize);

  if (!m_bIdentifierResolved || ezInputManager::GetMouseCursorIdentifierChangeCounter() != m_uiLastIdentifierChangeCounter)
  {
    m_uiLastIdentifierChangeCounter = ezInputManager::GetMouseCursorIdentifierChangeCounter();
    m_bIdentifierResolved = true;
    ResolveCursor(desc.m_sCursor);
  }

  if (!m_Current.m_hMaterial.IsValid() && !m_Current.m_hTexture.IsValid())
    return;

  float fMouseX = 0.0f;
  float fMouseY = 0.0f;
  ezInputManager::GetInputSlotState(ezInputSlot_MousePositionX, &fMouseX);
  ezInputManager::GetInputSlotState(ezInputSlot_MousePositionY, &fMouseY);

  // The mouse position is normalized and relative to the window, so this means it left the window.
  if (fMouseX < 0.0f || fMouseX > 1.0f || fMouseY < 0.0f || fMouseY > 1.0f)
    return;

  const ezGALTexture* pBackBuffer = pDevice->GetTexture(hBackBuffer);
  const ezVec2 vTargetSize((float)pBackBuffer->GetDescription().m_uiWidth, (float)pBackBuffer->GetDescription().m_uiHeight);
  const ezVec2 vMousePosition(fMouseX * vTargetSize.x, fMouseY * vTargetSize.y);

  m_pRenderGraph->Reset();

  const ezRenderGraphTextureHandle hTarget = m_pRenderGraph->ImportTexture(hBackBuffer);

  {
    auto pass = m_pRenderGraph->AddGraphicsPass("MouseCursor");
    pass.AddColorTarget(hTarget);
    // Nothing reads our output, so without this the pass would be culled.
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, desc, vMousePosition, vCursorSize, vTargetSize](const ezRenderGraphContext& ctx)
      {
      ezRenderContext* pRenderContext = ctx.GetRenderContext();

      // A single quad, generated from SV_VertexID. No vertex or index buffer needed.
      pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 2);

      auto* pConstants = ezRenderContext::GetConstantBufferData<ezMouseCursorConstants>(m_hConstantBuffer);
      pConstants->CursorPositionAndSize.Set(vMousePosition.x, vMousePosition.y, vCursorSize.x, vCursorSize.y);
      pConstants->CursorHotspotAndRotation.Set(desc.m_vHotspot.x, desc.m_vHotspot.y, ezMath::Sin(desc.m_Rotation), ezMath::Cos(desc.m_Rotation));
      pConstants->CursorUvRect.Set(desc.m_vUvTopLeft.x, desc.m_vUvTopLeft.y, desc.m_vUvBottomRight.x, desc.m_vUvBottomRight.y);
      pConstants->CursorTargetSize.Set(vTargetSize.x, vTargetSize.y, 1.0f / vTargetSize.x, 1.0f / vTargetSize.y);
      pConstants->CursorColor = desc.m_Color;

      // Most of ezGlobalConstants is undefined outside of a render pipeline, but the time constants
      // are cheap to provide and make time based effects in custom cursor materials work.
      // There is no world here, so WorldTime stays zero, only DeltaTime and GlobalTime are usable.
      pRenderContext->SetGlobalAndWorldTimeConstants(ezTime::MakeZero());

      // The render graph resets the render pass, material and draw call bind groups for every pass,
      // but not the frame bind group, which is why the cursor constants live in that one.
      ezBindGroupBuilder& bindGroup = pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_FRAME);
      bindGroup.BindBuffer("ezMouseCursorConstants", m_hConstantBuffer);

      if (m_Current.m_hMaterial.IsValid())
      {
        pRenderContext->BindMaterial(m_Current.m_hMaterial);
      }
      else
      {
        pRenderContext->BindShader(m_hDefaultShader);
        bindGroup.BindTexture("CursorTexture", m_Current.m_hTexture);
      }

      pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
}

void ezMouseCursorRenderer::ResolveCursor(ezStringView sIdentifier)
{
  m_Current = ResolvedCursor();

  if (sIdentifier.IsEmpty())
    return;

  if (const ResolvedCursor* pCached = m_ResolvedCursors.GetValue(sIdentifier))
  {
    m_Current = *pCached;
    return;
  }

  // Turns an asset GUID into something like 'AssetCache/Common/Materials/Foo.ezBinMaterial'.
  // Plain paths are passed through unchanged (the function returns false, but still writes them).
  ezStringBuilder sResolved;
  ezFileSystem::ResolveAssetRedirection(sIdentifier, sResolved);

  ResolvedCursor cursor;

  if (sResolved.HasExtension("ezBinMaterial") || sResolved.HasExtension("ezMaterial"))
  {
    // Load with the original identifier, the resource loader does its own redirection.
    cursor.m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(sIdentifier);
  }
  else if (sResolved.HasExtension("ezBinTexture2D") || sResolved.HasExtension("dds") || sResolved.HasExtension("png") ||
           sResolved.HasExtension("jpg") || sResolved.HasExtension("jpeg") || sResolved.HasExtension("tga"))
  {
    cursor.m_hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(sIdentifier);
  }
  else
  {
    ezLog::Warning("Mouse cursor '{}' resolves to '{}', which is neither a material nor a 2D texture.", sIdentifier, sResolved);
  }

  m_ResolvedCursors.Insert(sIdentifier, cursor);
  m_Current = cursor;
}

ezResult ezMouseCursorRenderer::EnsureGpuResourcesExist()
{
  if (m_pRenderGraph == nullptr)
  {
    m_pRenderGraph = ezRenderGraphManager::CreateRenderGraph("MouseCursor", ezRenderGraphPhase::PostRender);

    if (m_pRenderGraph == nullptr)
      return EZ_FAILURE;
  }

  if (m_hConstantBuffer.IsInvalidated())
  {
    m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezMouseCursorConstants>();
  }

  if (!m_hDefaultShader.IsValid())
  {
    m_hDefaultShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/MouseCursor/MouseCursor.ezShader");
  }

  return EZ_SUCCESS;
}

void ezMouseCursorRenderer::ReleaseGpuResources()
{
  if (!m_hConstantBuffer.IsInvalidated())
  {
    ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  }

  m_pRenderGraph = nullptr;
  m_hDefaultShader.Invalidate();
  m_ResolvedCursors.Clear();
  m_Current = ResolvedCursor();
  m_bIdentifierResolved = false;
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_MouseCursor_Implementation_MouseCursorRenderer);
