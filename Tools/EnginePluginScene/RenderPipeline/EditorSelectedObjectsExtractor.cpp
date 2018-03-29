#include <PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <EnginePluginScene/RenderPipeline/EditorSelectedObjectsExtractor.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Debug/DebugRenderer.h>

class ezCameraViewRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCameraViewRenderData, ezRenderData);

public:

  ezTexture2DResourceHandle m_hRenderTarget;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCameraViewRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

struct EZ_ALIGN_16(ezCamViewVertex)
{
  EZ_DECLARE_POD_TYPE();

  ezVec3 m_Position;
  ezVec2 m_TexCoord;
};

class ezCameraViewRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCameraViewRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezCameraViewRenderer);

public:
  ezCameraViewRenderer() {}
  ~ezCameraViewRenderer()
  {
    m_hShader.Invalidate();

    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexBuffer);
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hIndexBuffer);

    m_hVertexBuffer.Invalidate();
    m_hIndexBuffer.Invalidate();
  }

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override
  {
    types.PushBack(ezGetStaticRTTI<ezCameraViewRenderData>());
  }

  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override
  {
    SetupRenderer();

    ezRenderContext* pRenderContext = renderContext.m_pRenderContext;

    ezTexture2DResourceHandle hTexture = ezResourceManager::LoadResource<ezTexture2DResource>("Textures/MissingTexture_D.dds");

    for (auto it = batch.GetIterator<ezCameraViewRenderData>(); it.IsValid(); ++it)
    {
      const ezCameraViewRenderData* pRenderData = pRenderData = it;

      pRenderContext->BindShader(m_hShader);
      pRenderContext->BindTexture2D("BaseTexture", pRenderData->m_hRenderTarget);

      pRenderContext->BindMeshBuffer(m_hVertexBuffer, m_hIndexBuffer, &m_VertexDeclarationInfo, ezGALPrimitiveTopology::Triangles, 2);

      pRenderContext->DrawMeshBuffer(2);
    }
  }

protected:
  void SetupRenderer()
  {
    if (!m_hVertexBuffer.IsInvalidated())
      return;

    // load the shader
    {
      m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Editor/ViewportOverlay.ezShader");
    }

    // Create the vertex buffer
    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(ezCamViewVertex);
      desc.m_uiTotalSize = 4 * desc.m_uiStructSize;
      desc.m_BufferType = ezGALBufferType::VertexBuffer;
      desc.m_ResourceAccess.m_bImmutable = true;

      ezCamViewVertex data[4] =
      {
        ezCamViewVertex{ ezVec3(20, 20, 0), ezVec2(0, 0) },
        ezCamViewVertex{ ezVec3(270, 20, 0), ezVec2(1, 0) },
        ezCamViewVertex{ ezVec3(270, 155, 0), ezVec2(1, 1) },
        ezCamViewVertex{ ezVec3(20, 155, 0), ezVec2(0, 1) }
      };

      m_hVertexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc, ezArrayPtr<const ezUInt8>((const ezUInt8*)data, sizeof(ezCamViewVertex) * 4));
    }

    // Create the index buffer
    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(ezUInt16);
      desc.m_uiTotalSize = 6 * desc.m_uiStructSize;
      desc.m_BufferType = ezGALBufferType::IndexBuffer;
      desc.m_ResourceAccess.m_bImmutable = true;

      ezUInt16 data[6] =
      {
        0, 1, 2, 0, 2, 3
      };

      m_hIndexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc, ezArrayPtr<const ezUInt8>((const ezUInt8*)data, EZ_ARRAY_SIZE(data) * sizeof(ezUInt16)));
    }

    // Setup the vertex declaration
    {
      {
        ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
        si.m_Semantic = ezGALVertexAttributeSemantic::Position;
        si.m_Format = ezGALResourceFormat::XYZFloat;
        si.m_uiOffset = 0;
        si.m_uiElementSize = 12;
      }

      {
        ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
        si.m_Semantic = ezGALVertexAttributeSemantic::TexCoord0;
        si.m_Format = ezGALResourceFormat::UVFloat;
        si.m_uiOffset = 12;
        si.m_uiElementSize = 8;
      }
    }
  }

  ezShaderResourceHandle m_hShader;
  ezGALBufferHandle m_hVertexBuffer;
  ezGALBufferHandle m_hIndexBuffer;
  ezVertexDeclarationInfo m_VertexDeclarationInfo;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCameraViewRenderer, 1, ezRTTIDefaultAllocator<ezCameraViewRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////



EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorSelectedObjectsExtractor, 1, ezRTTIDefaultAllocator<ezEditorSelectedObjectsExtractor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE


ezEditorSelectedObjectsExtractor::ezEditorSelectedObjectsExtractor()
{
  m_pSceneContext = nullptr;
}

const ezDeque<ezGameObjectHandle>* ezEditorSelectedObjectsExtractor::GetSelection()
{
  if (m_pSceneContext == nullptr)
    return nullptr;

  return &m_pSceneContext->GetSelectionWithChildren();
}

void ezEditorSelectedObjectsExtractor::Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData* pExtractedRenderData)
{
  const bool bShowCameraOverlays = view.GetCameraUsageHint() == ezCameraUsageHint::EditorView /*&& view.GetCamera()->IsPerspective()*/;

  if (bShowCameraOverlays)
  {
    const ezDeque<ezGameObjectHandle>* pSelection = GetSelection();
    if (pSelection == nullptr)
      return;

    ezCameraComponent* pCamComp = nullptr;

    CreateRenderTargetTexture(view);

    EZ_LOCK(view.GetWorld()->GetReadMarker());

    for (const auto& hObj : *pSelection)
    {
      const ezGameObject* pObject = nullptr;
      if (!view.GetWorld()->TryGetObject(hObj, pObject))
        continue;

      if (FilterByViewTags(view, pObject))
        continue;

      if (pObject->TryGetComponentOfBaseType(pCamComp))
      {
        UpdateRenderTargetCamera(pCamComp);

        if (true)
        {
          ezCameraViewRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezCameraViewRenderData>(nullptr, 0);
          pRenderData->m_GlobalTransform = pObject->GetGlobalTransform();
          pRenderData->m_GlobalBounds = ezBoundingBoxSphere(pRenderData->m_GlobalTransform.m_vPosition, ezVec3::ZeroVector(), 0.1f);
          pRenderData->m_hRenderTarget = m_hRenderTarget;

          pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::GUI, 0);
        }
        else
        {
          // TODO: this code path creates abstract art

          // TODO: use aspect ratio of camera render target, if available
          ezDebugRenderer::Draw2DRectangle(view.GetWorld(), ezRectFloat(20, 20, 250, 135), 0, ezColor::White, m_hRenderTarget);
        }

        // TODO: if the camera renders to a texture anyway, use its view + render target instead

        ezRenderWorld::AddViewToRender(m_hRenderTargetView);

        break;
      }
    }
  }

  ezSelectedObjectsExtractor::Extract(view, visibleObjects, pExtractedRenderData);
}

void ezEditorSelectedObjectsExtractor::CreateRenderTargetTexture(const ezView& view)
{
  if (m_hRenderTarget.IsValid())
    return;

  m_hRenderTarget = ezResourceManager::GetExistingResource<ezTexture2DResource>("EditorCameraRT");

  if (!m_hRenderTarget.IsValid())
  {
    const float fAspect = 9.0f / 16.0f;
    const ezUInt32 uiWidth = 256;

    ezTexture2DResourceDescriptor d;
    d.m_DescGAL.SetAsRenderTarget(uiWidth, (ezUInt32)(uiWidth * fAspect), ezGALResourceFormat::RGBAUByteNormalizedsRGB);

    m_hRenderTarget = ezResourceManager::CreateResource<ezTexture2DResource>("EditorCameraRT", d);
  }

  CreateRenderTargetView(view);
}

void ezEditorSelectedObjectsExtractor::CreateRenderTargetView(const ezView& view)
{
  EZ_ASSERT_DEV(m_hRenderTargetView.IsInvalidated(), "Render target view is already created");

  ezResourceLock<ezTexture2DResource> pRenderTarget(m_hRenderTarget, ezResourceAcquireMode::NoFallback);

  ezStringBuilder name("EditorCameraRT");

  ezView* pRenderTargetView = nullptr;
  m_hRenderTargetView = ezRenderWorld::CreateView(name, pRenderTargetView);

  // MainRenderPipeline.ezRenderPipelineAsset
  auto hRenderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }");
  pRenderTargetView->SetRenderPipelineResource(hRenderPipeline);

  // TODO: get rid of const cast ?
  pRenderTargetView->SetWorld(const_cast<ezWorld*>(view.GetWorld()));
  pRenderTargetView->SetCamera(&m_RenderTargetCamera);

  m_RenderTargetCamera.SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 45, 0.1f, 100.0f);

  ezGALRenderTagetSetup renderTargetSetup;
  renderTargetSetup.SetRenderTarget(0, pRenderTarget->GetRenderTargetView());
  pRenderTargetView->SetRenderTargetSetup(renderTargetSetup);

  const float resX = (float)pRenderTarget->GetWidth();
  const float resY = (float)pRenderTarget->GetHeight();

  pRenderTargetView->SetViewport(ezRectFloat(0, 0, resX, resY));
}

void ezEditorSelectedObjectsExtractor::UpdateRenderTargetCamera(const ezCameraComponent* pCamComp)
{
  if (pCamComp->GetCameraMode() == ezCameraMode::OrthoFixedHeight || pCamComp->GetCameraMode() == ezCameraMode::OrthoFixedWidth)
  {
    m_RenderTargetCamera.SetCameraMode(pCamComp->GetCameraMode(), pCamComp->GetOrthoDimension(), pCamComp->GetNearPlane(), pCamComp->GetFarPlane());
  }
  else
  {
    m_RenderTargetCamera.SetCameraMode(pCamComp->GetCameraMode(), pCamComp->GetFieldOfView(), pCamComp->GetNearPlane(), pCamComp->GetFarPlane());
  }
  
  ezView* pRenderTargetView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hRenderTargetView, pRenderTargetView))
    return;

  pRenderTargetView->m_IncludeTags = pCamComp->m_IncludeTags;
  pRenderTargetView->m_ExcludeTags = pCamComp->m_ExcludeTags;

  if (pCamComp->GetRenderPipeline().IsValid())
  {
    pRenderTargetView->SetRenderPipelineResource(pCamComp->GetRenderPipeline());
  }
  else
  {
    // MainRenderPipeline.ezRenderPipelineAsset
    auto hRenderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }");
    pRenderTargetView->SetRenderPipelineResource(hRenderPipeline);
  }

  const ezVec3 pos = pCamComp->GetOwner()->GetGlobalPosition();
  const ezVec3 dir = pCamComp->GetOwner()->GetGlobalDirForwards();
  const ezVec3 up = pCamComp->GetOwner()->GetGlobalDirUp();

  m_RenderTargetCamera.LookAt(pos, pos + dir, up);
  m_RenderTargetCamera.SetExposure(pCamComp->GetExposure());
}
