#include <BakingPluginPCH.h>

#include <BakingPlugin/BakingScene.h>
#include <BakingPlugin/Components/BakingSettingsComponent.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

struct ezBakingSettingsComponent::RenderDebugViewTask : public ezTask
{
  RenderDebugViewTask()
    : ezTask("BakingDebugView")
  {
  }

  virtual void Execute() override
  {
    EZ_ASSERT_DEV(m_PixelData.GetCount() == m_uiWidth * m_uiHeight, "Pixel data must be pre-allocated");

    ezProgress progress;
    progress.m_Events.AddEventHandler([this](const ezProgressEvent& e) {
      if (e.m_Type != ezProgressEvent::Type::CancelClicked)
      {
        if (HasBeenCanceled())
        {
          e.m_pProgressbar->UserClickedCancel();
        }
        m_bHasNewData = true;
      }
    });

    m_pBakingScene->RenderDebugView(m_InverseViewProjection, m_uiWidth, m_uiHeight, m_PixelData, progress);

    m_bHasNewData = true;
  }

  ezBakingScene* m_pBakingScene = nullptr;

  ezMat4 m_InverseViewProjection = ezMat4::IdentityMatrix();
  ezUInt32 m_uiWidth = 0;
  ezUInt32 m_uiHeight = 0;
  ezDynamicArray<ezColorGammaUB> m_PixelData;

  bool m_bHasNewData = false;
};

//////////////////////////////////////////////////////////////////////////


ezBakingSettingsComponentManager::ezBakingSettingsComponentManager(ezWorld* pWorld)
  : ezSettingsComponentManager<ezBakingSettingsComponent>(pWorld)
{
}

ezBakingSettingsComponentManager::~ezBakingSettingsComponentManager() = default;

void ezBakingSettingsComponentManager::Initialize()
{
  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezBakingSettingsComponentManager::RenderDebug, this);

    this->RegisterUpdateFunction(desc);
  }

  ezRenderWorld::s_BeginRenderEvent.AddEventHandler(ezMakeDelegate(&ezBakingSettingsComponentManager::OnBeginRender, this));

  CreateDebugResources();
}

void ezBakingSettingsComponentManager::Deinitialize()
{
  ezRenderWorld::s_BeginRenderEvent.RemoveEventHandler(ezMakeDelegate(&ezBakingSettingsComponentManager::OnBeginRender, this));
}

void ezBakingSettingsComponentManager::RenderDebug(const ezWorldModule::UpdateContext& updateContext)
{
  if (ezBakingSettingsComponent* pComponent = GetSingletonComponent())
  {
    if (pComponent->GetShowDebugOverlay())
    {
      pComponent->RenderDebugOverlay();
    }
  }
}

void ezBakingSettingsComponentManager::OnBeginRender(ezUInt64 uiFrameCounter)
{
  if (ezBakingSettingsComponent* pComponent = GetSingletonComponent())
  {
    auto& task = pComponent->m_pRenderDebugViewTask;
    if (task != nullptr && task->m_bHasNewData)
    {
      task->m_bHasNewData = false;

      ezGALContext* pContext = ezGALDevice::GetDefaultDevice()->GetPrimaryContext();

      ezBoundingBoxu32 destBox;
      destBox.m_vMin.SetZero();
      destBox.m_vMax = ezVec3U32(task->m_uiWidth, task->m_uiHeight, 1);

      ezGALSystemMemoryDescription sourceData;
      sourceData.m_pData = task->m_PixelData.GetData();
      sourceData.m_uiRowPitch = task->m_uiWidth * sizeof(ezColorGammaUB);

      pContext->UpdateTexture(pComponent->m_hDebugViewTexture, ezGALTextureSubresource(), destBox, sourceData);
    }
  }
}

void ezBakingSettingsComponentManager::CreateDebugResources()
{
  if (!m_hDebugSphere.IsValid())
  {
    ezGeometry geom;
    geom.AddSphere(0.3f, 32, 16, ezColor::White);

    const char* szBufferResourceName = "ReflectionProbeDebugSphereBuffer";
    ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szBufferResourceName);
    if (!hMeshBuffer.IsValid())
    {
      ezMeshBufferResourceDescriptor desc;
      desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
    }

    const char* szMeshResourceName = "ReflectionProbeDebugSphere";
    m_hDebugSphere = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshResourceName);
    if (!m_hDebugSphere.IsValid())
    {
      ezMeshResourceDescriptor desc;
      desc.UseExistingMeshBuffer(hMeshBuffer);
      desc.AddSubMesh(geom.CalculateTriangleCount(), 0, 0);
      desc.ComputeBounds();

      m_hDebugSphere = ezResourceManager::CreateResource<ezMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
    }
  }

  if (!m_hDebugMaterial.IsValid())
  {
    m_hDebugMaterial = ezResourceManager::LoadResource<ezMaterialResource>(
      "{ 4d15c716-a8e9-43d4-9424-43174403fb94 }"); // IrradianceProbeVisualization.ezMaterialAsset
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezBakingSettingsComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("ShowDebugOverlay", GetShowDebugOverlay, SetShowDebugOverlay),
    EZ_ACCESSOR_PROPERTY("ShowDebugProbes", GetShowDebugProbes, SetShowDebugProbes),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering/Baking"),
    new ezLongOpAttribute("ezLongOpProxy_BakeScene")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezBakingSettingsComponent::ezBakingSettingsComponent() = default;
ezBakingSettingsComponent::~ezBakingSettingsComponent() = default;

void ezBakingSettingsComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();

  SUPER::OnActivated();
}

void ezBakingSettingsComponent::OnDeactivated()
{
  if (m_pRenderDebugViewTask != nullptr)
  {
    ezTaskSystem::WaitForTask(m_pRenderDebugViewTask.Borrow());
  }

  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void ezBakingSettingsComponent::SetShowDebugOverlay(bool bShow)
{
  m_bShowDebugOverlay = bShow;

  if (bShow && m_pRenderDebugViewTask == nullptr)
  {
    m_pRenderDebugViewTask = EZ_DEFAULT_NEW(RenderDebugViewTask);
  }
}

void ezBakingSettingsComponent::SetShowDebugProbes(bool bShow)
{
  if (m_bShowDebugProbes != bShow)
  {
    m_bShowDebugProbes = bShow;

    if (IsActiveAndInitialized())
    {
      ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
    }
  }
}

void ezBakingSettingsComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? ezDefaultSpatialDataCategories::RenderDynamic : ezDefaultSpatialDataCategories::RenderStatic);
}

void ezBakingSettingsComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_bShowDebugProbes)
    return;

  // Don't trigger probe rendering in shadow or reflection views.
  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow ||
      msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Reflection)
    return;

  ezBakingScene* pBakingScene = ezBakingScene::Get(*GetWorld());
  if (pBakingScene != nullptr && pBakingScene->IsBaked())
  {
    auto probePositions = pBakingScene->GetProbePositions();
    auto skyVisibility = pBakingScene->GetSkyVisibility();

    ezTransform transform = ezTransform::IdentityTransform();
    ezColor encodedSkyVisibility = ezColor::Black;

    const ezGameObject* pOwner = GetOwner();
    auto pManager = static_cast<const ezBakingSettingsComponentManager*>(GetOwningManager());

    for (ezUInt32 uiProbeIndex = 0; uiProbeIndex < probePositions.GetCount(); ++uiProbeIndex)
    {
      transform.m_vPosition = probePositions[uiProbeIndex];

      auto& skyVis = skyVisibility[uiProbeIndex];
      encodedSkyVisibility.r = *reinterpret_cast<float*>(&ezColorLinearUB(skyVis.m_Values[0], skyVis.m_Values[1], skyVis.m_Values[2], 0));
      encodedSkyVisibility.g = *reinterpret_cast<float*>(&ezColorLinearUB(skyVis.m_Values[3], skyVis.m_Values[4], skyVis.m_Values[5], 0));

      ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(pOwner);
      {
        pRenderData->m_GlobalTransform = transform;
        pRenderData->m_GlobalBounds.SetInvalid();
        pRenderData->m_hMesh = pManager->m_hDebugSphere;
        pRenderData->m_hMaterial = pManager->m_hDebugMaterial;
        pRenderData->m_Color = encodedSkyVisibility;
        pRenderData->m_uiSubMeshIndex = 0;
        pRenderData->m_uiUniqueID = ezRenderComponent::GetUniqueIdForRendering(this, 0);

        pRenderData->FillBatchIdAndSortingKey();
      }

      msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::SimpleOpaque, ezRenderData::Caching::IfStatic);
    }
  }
}

void ezBakingSettingsComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();
}

void ezBakingSettingsComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();
}

void ezBakingSettingsComponent::RenderDebugOverlay()
{
  ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView);
  if (pView == nullptr)
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezRectFloat viewport = pView->GetViewport();
  ezUInt32 uiWidth = ezMath::Ceil(viewport.width / 3.0f);
  ezUInt32 uiHeight = ezMath::Ceil(viewport.height / 3.0f);

  ezBakingScene* pBakingScene = ezBakingScene::Get(*GetWorld());
  if (pBakingScene != nullptr && pBakingScene->IsBaked())
  {
    ezMat4 inverseViewProjection = pView->GetInverseViewProjectionMatrix(ezCameraEye::Left);

    if (m_pRenderDebugViewTask->m_InverseViewProjection != inverseViewProjection ||
        m_pRenderDebugViewTask->m_uiWidth != uiWidth || m_pRenderDebugViewTask->m_uiHeight != uiHeight)
    {
      ezTaskSystem::CancelTask(m_pRenderDebugViewTask.Borrow());

      m_pRenderDebugViewTask->m_pBakingScene = pBakingScene;
      m_pRenderDebugViewTask->m_InverseViewProjection = inverseViewProjection;
      m_pRenderDebugViewTask->m_uiWidth = uiWidth;
      m_pRenderDebugViewTask->m_uiHeight = uiHeight;
      m_pRenderDebugViewTask->m_PixelData.SetCount(uiWidth * uiHeight, ezColor::Red);
      m_pRenderDebugViewTask->m_bHasNewData = false;

      ezTaskSystem::StartSingleTask(m_pRenderDebugViewTask.Borrow(), ezTaskPriority::LongRunning);
    }
  }

  ezUInt32 uiTextureWidth = 0;
  ezUInt32 uiTextureHeight = 0;
  if (const ezGALTexture* pTexture = pDevice->GetTexture(m_hDebugViewTexture))
  {
    uiTextureWidth = pTexture->GetDescription().m_uiWidth;
    uiTextureHeight = pTexture->GetDescription().m_uiHeight;
  }

  if (uiTextureWidth != uiWidth || uiTextureHeight != uiHeight)
  {
    if (!m_hDebugViewTexture.IsInvalidated())
    {
      pDevice->DestroyTexture(m_hDebugViewTexture);
    }

    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = uiWidth;
    desc.m_uiHeight = uiHeight;
    desc.m_Format = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hDebugViewTexture = pDevice->CreateTexture(desc);
  }

  ezRectFloat rectInPixel = ezRectFloat(10.0f, 10.0f, uiWidth, uiHeight);

  ezDebugRenderer::Draw2DRectangle(pView->GetHandle(), rectInPixel, 0.0f, ezColor::White, pDevice->GetDefaultResourceView(m_hDebugViewTexture));
}
