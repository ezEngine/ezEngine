#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/BakedProbes/BakedProbesComponent.h>
#include <RendererCore/BakedProbes/BakedProbesWorldModule.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

struct ezBakedProbesComponent::RenderDebugViewTask : public ezTask
{
  RenderDebugViewTask()
  {
    ConfigureTask("BakingDebugView", ezTaskNesting::Never);
  }

  virtual void Execute() override
  {
    EZ_ASSERT_DEV(m_PixelData.GetCount() == m_uiWidth * m_uiHeight, "Pixel data must be pre-allocated");

    ezProgress progress;
    progress.m_Events.AddEventHandler([this](const ezProgressEvent& e)
      {
      if (e.m_Type != ezProgressEvent::Type::CancelClicked)
      {
        if (HasBeenCanceled())
        {
          e.m_pProgressbar->UserClickedCancel();
        }
        m_bHasNewData = true;
      } });

    if (m_pBakingInterface->RenderDebugView(*m_pWorld, m_InverseViewProjection, m_uiWidth, m_uiHeight, m_PixelData, progress).Succeeded())
    {
      m_bHasNewData = true;
    }
  }

  ezBakingInterface* m_pBakingInterface = nullptr;

  const ezWorld* m_pWorld = nullptr;
  ezMat4 m_InverseViewProjection = ezMat4::MakeIdentity();
  ezUInt32 m_uiWidth = 0;
  ezUInt32 m_uiHeight = 0;
  ezDynamicArray<ezColorGammaUB> m_PixelData;

  bool m_bHasNewData = false;
};

//////////////////////////////////////////////////////////////////////////


ezBakedProbesComponentManager::ezBakedProbesComponentManager(ezWorld* pWorld)
  : ezSettingsComponentManager<ezBakedProbesComponent>(pWorld)
{
}

ezBakedProbesComponentManager::~ezBakedProbesComponentManager() = default;

void ezBakedProbesComponentManager::Initialize()
{
  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezBakedProbesComponentManager::RenderDebug, this);

    this->RegisterUpdateFunction(desc);
  }

  CreateDebugResources();
}

void ezBakedProbesComponentManager::RenderDebug(const ezWorldModule::UpdateContext& updateContext)
{
  if (ezBakedProbesComponent* pComponent = GetSingletonComponent())
  {
    if (pComponent->GetShowDebugOverlay())
    {
      pComponent->RenderDebugOverlay();
    }
  }
}

void ezBakedProbesComponentManager::CreateDebugResources()
{
  if (!m_hDebugSphere.IsValid())
  {
    ezGeometry geom;
    geom.AddStackedSphere(0.3f, 32, 16);

    const char* szBufferResourceName = "IrradianceProbeDebugSphereBuffer";
    ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szBufferResourceName);
    if (!hMeshBuffer.IsValid())
    {
      ezMeshBufferResourceDescriptor desc;
      desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      hMeshBuffer = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
    }

    const char* szMeshResourceName = "IrradianceProbeDebugSphere";
    m_hDebugSphere = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshResourceName);
    if (!m_hDebugSphere.IsValid())
    {
      ezMeshResourceDescriptor desc;
      desc.UseExistingMeshBuffer(hMeshBuffer);
      desc.AddSubMesh(geom.CalculateTriangleCount(), 0, 0);
      desc.ComputeBounds();

      m_hDebugSphere = ezResourceManager::GetOrCreateResource<ezMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
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
EZ_BEGIN_COMPONENT_TYPE(ezBakedProbesComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Settings", m_Settings),
    EZ_ACCESSOR_PROPERTY("ShowDebugOverlay", GetShowDebugOverlay, SetShowDebugOverlay)->AddAttributes(new ezGroupAttribute("Debug")),
    EZ_ACCESSOR_PROPERTY("ShowDebugProbes", GetShowDebugProbes, SetShowDebugProbes),
    EZ_ACCESSOR_PROPERTY("UseTestPosition", GetUseTestPosition, SetUseTestPosition),
    EZ_ACCESSOR_PROPERTY("TestPosition", GetTestPosition, SetTestPosition)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_FUNCTION_PROPERTY(OnObjectCreated),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Lighting/Baking"),
    new ezLongOpAttribute("ezLongOpProxy_BakeScene"),
    new ezTransformManipulatorAttribute("TestPosition"),
    new ezInDevelopmentAttribute(ezInDevelopmentAttribute::Phase::Beta),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezBakedProbesComponent::ezBakedProbesComponent() = default;
ezBakedProbesComponent::~ezBakedProbesComponent() = default;

void ezBakedProbesComponent::OnActivated()
{
  auto pModule = GetWorld()->GetOrCreateModule<ezBakedProbesWorldModule>();
  pModule->SetProbeTreeResourcePrefix(m_sProbeTreeResourcePrefix);

  GetOwner()->UpdateLocalBounds();

  SUPER::OnActivated();
}

void ezBakedProbesComponent::OnDeactivated()
{
  if (m_pRenderDebugViewTask != nullptr)
  {
    ezTaskSystem::CancelTask(m_pRenderDebugViewTask).IgnoreResult();
  }

  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void ezBakedProbesComponent::SetShowDebugOverlay(bool bShow)
{
  m_bShowDebugOverlay = bShow;

  if (bShow && m_pRenderDebugViewTask == nullptr)
  {
    m_pRenderDebugViewTask = EZ_DEFAULT_NEW(RenderDebugViewTask);
  }
}

void ezBakedProbesComponent::SetShowDebugProbes(bool bShow)
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

void ezBakedProbesComponent::SetUseTestPosition(bool bUse)
{
  if (m_bUseTestPosition != bUse)
  {
    m_bUseTestPosition = bUse;

    if (IsActiveAndInitialized())
    {
      ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
    }
  }
}

void ezBakedProbesComponent::SetTestPosition(const ezVec3& vPos)
{
  m_vTestPosition = vPos;

  if (IsActiveAndInitialized())
  {
    ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void ezBakedProbesComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg)
{
  ref_msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? ezDefaultSpatialDataCategories::RenderDynamic : ezDefaultSpatialDataCategories::RenderStatic);
}

void ezBakedProbesComponent::OnExtractRenderData(ezMsgExtractRenderData& ref_msg) const
{
  if (!m_bShowDebugProbes)
    return;

  // Don't trigger probe rendering in shadow or reflection views.
  if (ref_msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow ||
      ref_msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Reflection)
    return;

  auto pModule = GetWorld()->GetModule<ezBakedProbesWorldModule>();
  if (!pModule->HasProbeData())
    return;

  const ezGameObject* pOwner = GetOwner();
  auto pManager = static_cast<const ezBakedProbesComponentManager*>(GetOwningManager());

  auto addProbeRenderData = [&](const ezVec3& vPosition, ezCompressedSkyVisibility skyVisibility, ezRenderData::Caching::Enum caching)
  {
    ezTransform transform = ezTransform::MakeIdentity();
    transform.m_vPosition = vPosition;

    ezColor encodedSkyVisibility = ezColor::Black;
    encodedSkyVisibility.r = *reinterpret_cast<const float*>(&skyVisibility);

    ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(pOwner);
    {
      pRenderData->m_GlobalTransform = transform;
      pRenderData->m_GlobalBounds = ezBoundingBoxSphere::MakeInvalid();
      pRenderData->m_hMesh = pManager->m_hDebugSphere;
      pRenderData->m_hMaterial = pManager->m_hDebugMaterial;
      pRenderData->m_Color = encodedSkyVisibility;
      pRenderData->m_uiSubMeshIndex = 0;
      pRenderData->m_uiUniqueID = ezRenderComponent::GetUniqueIdForRendering(*this, 0);

      pRenderData->FillBatchIdAndSortingKey();
    }

    ref_msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::SimpleOpaque, caching);
  };

  if (m_bUseTestPosition)
  {
    ezBakedProbesWorldModule::ProbeIndexData indexData;
    if (pModule->GetProbeIndexData(m_vTestPosition, ezVec3::MakeAxisZ(), indexData).Failed())
      return;

    if (true)
    {
      ezResourceLock<ezProbeTreeSectorResource> pProbeTree(pModule->m_hProbeTree, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pProbeTree.GetAcquireResult() != ezResourceAcquireResult::Final)
        return;

      for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(indexData.m_probeIndices); ++i)
      {
        ezVec3 pos = pProbeTree->GetProbePositions()[indexData.m_probeIndices[i]];
        ezDebugRenderer::DrawCross(ref_msg.m_pView->GetHandle(), pos, 0.5f, ezColor::Yellow);

        pos.z += 0.5f;
        ezDebugRenderer::Draw3DText(ref_msg.m_pView->GetHandle(), ezFmt("Weight: {}", indexData.m_probeWeights[i]), pos, ezColor::Yellow);
      }
    }

    ezCompressedSkyVisibility skyVisibility = ezBakingUtils::CompressSkyVisibility(pModule->GetSkyVisibility(indexData));

    addProbeRenderData(m_vTestPosition, skyVisibility, ezRenderData::Caching::Never);
  }
  else
  {
    ezResourceLock<ezProbeTreeSectorResource> pProbeTree(pModule->m_hProbeTree, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pProbeTree.GetAcquireResult() != ezResourceAcquireResult::Final)
      return;

    auto probePositions = pProbeTree->GetProbePositions();
    auto skyVisibility = pProbeTree->GetSkyVisibility();

    for (ezUInt32 uiProbeIndex = 0; uiProbeIndex < probePositions.GetCount(); ++uiProbeIndex)
    {
      addProbeRenderData(probePositions[uiProbeIndex], skyVisibility[uiProbeIndex], ezRenderData::Caching::IfStatic);
    }
  }
}

void ezBakedProbesComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  if (m_Settings.Serialize(s).Failed())
    return;

  s << m_sProbeTreeResourcePrefix;
  s << m_bShowDebugOverlay;
  s << m_bShowDebugProbes;
  s << m_bUseTestPosition;
  s << m_vTestPosition;
}

void ezBakedProbesComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  if (m_Settings.Deserialize(s).Failed())
    return;

  s >> m_sProbeTreeResourcePrefix;
  s >> m_bShowDebugOverlay;
  s >> m_bShowDebugProbes;
  s >> m_bUseTestPosition;
  s >> m_vTestPosition;
}

void ezBakedProbesComponent::RenderDebugOverlay()
{
  ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView);
  if (pView == nullptr)
    return;

  ezBakingInterface* pBakingInterface = ezSingletonRegistry::GetSingletonInstance<ezBakingInterface>();
  if (pBakingInterface == nullptr)
  {
    ezDebugRenderer::Draw2DText(pView->GetHandle(), "Baking Plugin not loaded", ezVec2I32(10, 10), ezColor::OrangeRed);
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezRectFloat viewport = pView->GetViewport();
  ezUInt32 uiWidth = static_cast<ezUInt32>(ezMath::Ceil(viewport.width / 3.0f));
  ezUInt32 uiHeight = static_cast<ezUInt32>(ezMath::Ceil(viewport.height / 3.0f));

  ezMat4 inverseViewProjection = pView->GetInverseViewProjectionMatrix(ezCameraEye::Left);

  if (m_pRenderDebugViewTask->m_InverseViewProjection != inverseViewProjection ||
      m_pRenderDebugViewTask->m_uiWidth != uiWidth || m_pRenderDebugViewTask->m_uiHeight != uiHeight)
  {
    ezTaskSystem::CancelTask(m_pRenderDebugViewTask).IgnoreResult();

    m_pRenderDebugViewTask->m_pBakingInterface = pBakingInterface;
    m_pRenderDebugViewTask->m_pWorld = GetWorld();
    m_pRenderDebugViewTask->m_InverseViewProjection = inverseViewProjection;
    m_pRenderDebugViewTask->m_uiWidth = uiWidth;
    m_pRenderDebugViewTask->m_uiHeight = uiHeight;
    m_pRenderDebugViewTask->m_PixelData.SetCount(uiWidth * uiHeight, ezColor::Red);
    m_pRenderDebugViewTask->m_bHasNewData = false;

    ezTaskSystem::StartSingleTask(m_pRenderDebugViewTask, ezTaskPriority::LongRunning);
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

  auto& task = m_pRenderDebugViewTask;
  if (task != nullptr && task->m_bHasNewData)
  {
    task->m_bHasNewData = false;

    ezGALSystemMemoryDescription sourceData;
    sourceData.m_pData = task->m_PixelData.GetByteArrayPtr();
    sourceData.m_uiRowPitch = task->m_uiWidth * sizeof(ezColorGammaUB);

    pDevice->UpdateTextureForNextFrame(m_hDebugViewTexture, sourceData);
  }

  ezRectFloat rectInPixel = ezRectFloat(10.0f, 10.0f, static_cast<float>(uiWidth), static_cast<float>(uiHeight));

  ezDebugRenderer::Draw2DRectangle(pView->GetHandle(), rectInPixel, 0.0f, ezColor::White, pDevice->GetDefaultResourceView(m_hDebugViewTexture));
}

void ezBakedProbesComponent::OnObjectCreated(const ezAbstractObjectNode& node)
{
  ezStringBuilder sPrefix;
  sPrefix.SetFormat(":project/AssetCache/Generated/{0}", node.GetGuid());

  m_sProbeTreeResourcePrefix.Assign(sPrefix);
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakedProbesComponent);
