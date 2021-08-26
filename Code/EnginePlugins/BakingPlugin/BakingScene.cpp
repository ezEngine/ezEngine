#include <BakingPluginPCH.h>

#include <BakingPlugin/BakingScene.h>
#include <BakingPlugin/Tasks/PlaceProbesTask.h>
#include <BakingPlugin/Tasks/SkyVisibilityTask.h>
#include <BakingPlugin/Tracer/TracerEmbree.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/BakedProbes/BakedProbesComponent.h>

ezResult ezBakingScene::Extract()
{
  m_MeshObjects.Clear();
  m_BoundingBox.SetInvalid();
  m_bIsBaked = false;

  const ezWorld* pWorld = ezWorld::GetWorld(m_uiWorldIndex);
  if (pWorld == nullptr)
  {
    return EZ_FAILURE;
  }

  const ezWorld& world = *pWorld;
  EZ_LOCK(world.GetReadMarker());

  // settings
  {
    auto pManager = world.GetComponentManager<ezBakedProbesComponentManager>();
    auto pComponent = pManager->GetSingletonComponent();

    m_Settings = pComponent->m_Settings;
  }

  const ezTag& tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

  for (auto it = world.GetObjects(); it.IsValid(); ++it)
  {
    // Exclude editor only objects like gizmos etc.
    if (it->GetTags().IsSet(tagEditor))
      continue;

    // Only active static objects
    if (!it->IsStatic() || !it->IsActive())
      continue;

    const ezMeshComponentBase* pMeshComponent = nullptr;
    if (!it->TryGetComponentOfBaseType(pMeshComponent) || !pMeshComponent->GetMesh().IsValid())
      continue;

    auto& meshObject = m_MeshObjects.ExpandAndGetRef();
    meshObject.m_GlobalTransform = it->GetGlobalTransformSimd();
    meshObject.m_MeshResourceId.Assign(pMeshComponent->GetMeshFile());

    auto globalBounds = it->GetGlobalBounds();
    if (globalBounds.IsValid())
    {
      m_BoundingBox.ExpandToInclude(globalBounds.GetBox());
    }
  }

  return EZ_SUCCESS;
}

ezResult ezBakingScene::Bake(const ezStringView& sOutputPath, ezProgress& progress)
{
  EZ_ASSERT_DEV(!ezThreadUtils::IsMainThread(), "BakeScene must be executed on a worker thread");

  if (m_pTracer == nullptr)
  {
    m_pTracer = EZ_DEFAULT_NEW(ezTracerEmbree);
  }

  ezProgressRange pgRange("Baking Scene", 2, true, &progress);
  pgRange.SetStepWeighting(0, 0.95f);
  pgRange.SetStepWeighting(1, 0.05f);

  if (!pgRange.BeginNextStep("Building Scene"))
    return EZ_FAILURE;

  EZ_SUCCEED_OR_RETURN(m_pTracer->BuildScene(*this));

  ezBakingInternal::PlaceProbesTask placeProbesTask(m_Settings, m_BoundingBox);
  placeProbesTask.Execute();

  ezBakingInternal::SkyVisibilityTask skyVisibilityTask(m_Settings, *m_pTracer, placeProbesTask.GetProbePositions());
  skyVisibilityTask.Execute();

  if (!pgRange.BeginNextStep("Writing Result"))
    return EZ_FAILURE;

  ezStringBuilder sFullOutputPath = sOutputPath;
  sFullOutputPath.Append("_Global.ezProbeTreeSector");

  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(sFullOutputPath));

  ezAssetFileHeader header;
  header.SetFileHashAndVersion(1, 1);
  EZ_SUCCEED_OR_RETURN(header.Write(file));

  ezProbeTreeSectorResourceDescriptor desc;
  desc.m_vGridOrigin = placeProbesTask.GetGridOrigin();
  desc.m_vProbeSpacing = m_Settings.m_vProbeSpacing;
  desc.m_vProbeCount = placeProbesTask.GetProbeCount();
  desc.m_ProbePositions = placeProbesTask.GetProbePositions();
  desc.m_SkyVisibility = skyVisibilityTask.GetSkyVisibility();

  EZ_SUCCEED_OR_RETURN(desc.Serialize(file));

  m_bIsBaked = true;

  return EZ_SUCCESS;
}

ezResult ezBakingScene::RenderDebugView(const ezMat4& InverseViewProjection, ezUInt32 uiWidth, ezUInt32 uiHeight, ezDynamicArray<ezColorGammaUB>& out_Pixels,
  ezProgress& progress) const
{
  if (!m_bIsBaked)
    return EZ_FAILURE;

  const ezUInt32 uiNumPixel = uiWidth * uiHeight;
  out_Pixels.SetCountUninitialized(uiNumPixel);

  ezHybridArray<ezTracerInterface::Ray, 128> rays;
  rays.SetCountUninitialized(128);

  ezHybridArray<ezTracerInterface::Hit, 128> hits;
  hits.SetCountUninitialized(128);

  ezUInt32 uiStartPixel = 0;
  ezUInt32 uiPixelPerBatch = rays.GetCount();
  while (uiStartPixel < uiNumPixel)
  {
    uiPixelPerBatch = ezMath::Min(uiPixelPerBatch, uiNumPixel - uiStartPixel);

    for (ezUInt32 i = 0; i < uiPixelPerBatch; ++i)
    {
      ezUInt32 uiPixelIndex = uiStartPixel + i;
      ezUInt32 x = uiPixelIndex % uiWidth;
      ezUInt32 y = uiHeight - (uiPixelIndex / uiWidth) - 1;

      auto& ray = rays[i];
      ezGraphicsUtils::ConvertScreenPosToWorldPos(InverseViewProjection, 0, 0, uiWidth, uiHeight, ezVec3(float(x), float(y), 0), ray.m_vStartPos, &ray.m_vDir).IgnoreResult();
      ray.m_fDistance = 1000.0f;
    }

    m_pTracer->TraceRays(rays, hits);

    for (ezUInt32 i = 0; i < uiPixelPerBatch; ++i)
    {
      ezUInt32 uiPixelIndex = uiStartPixel + i;

      auto& hit = hits[i];
      if (hit.m_fDistance >= 0.0f)
      {
        ezVec3 normal = hit.m_vNormal * 0.5f + ezVec3(0.5f);
        out_Pixels[uiPixelIndex] = ezColorGammaUB(ezMath::ColorFloatToByte(normal.x), ezMath::ColorFloatToByte(normal.y), ezMath::ColorFloatToByte(normal.z));
      }
      else
      {
        out_Pixels[uiPixelIndex] = ezColorGammaUB(0, 0, 0);
      }
    }

    uiStartPixel += uiPixelPerBatch;

    progress.SetCompletion((float)uiStartPixel / uiNumPixel);
    if (progress.WasCanceled())
      break;
  }

  return EZ_SUCCESS;
}

ezBakingScene::ezBakingScene() = default;
ezBakingScene::~ezBakingScene() = default;

//////////////////////////////////////////////////////////////////////////

namespace
{
  static ezDynamicArray<ezUniquePtr<ezBakingScene>, ezStaticAllocatorWrapper> s_BakingScenes;
}

EZ_IMPLEMENT_SINGLETON(ezBaking);

ezBaking::ezBaking()
  : m_SingletonRegistrar(this)
{
}

void ezBaking::Startup()
{
}

void ezBaking::Shutdown()
{
  s_BakingScenes.Clear();
}

ezBakingScene* ezBaking::GetOrCreateScene(const ezWorld& world)
{
  const ezUInt32 uiWorldIndex = world.GetIndex();

  s_BakingScenes.EnsureCount(uiWorldIndex + 1);
  if (s_BakingScenes[uiWorldIndex] == nullptr)
  {
    auto pScene = EZ_DEFAULT_NEW(ezBakingScene);
    pScene->m_uiWorldIndex = uiWorldIndex;

    s_BakingScenes[uiWorldIndex] = pScene;
  }

  return s_BakingScenes[uiWorldIndex].Borrow();
}

ezBakingScene* ezBaking::GetScene(const ezWorld& world)
{
  const ezUInt32 uiWorldIndex = world.GetIndex();

  if (uiWorldIndex < s_BakingScenes.GetCount())
  {
    return s_BakingScenes[uiWorldIndex].Borrow();
  }

  return nullptr;
}

const ezBakingScene* ezBaking::GetScene(const ezWorld& world) const
{
  const ezUInt32 uiWorldIndex = world.GetIndex();

  if (uiWorldIndex < s_BakingScenes.GetCount())
  {
    return s_BakingScenes[uiWorldIndex].Borrow();
  }

  return nullptr;
}

ezResult ezBaking::RenderDebugView(const ezWorld& world, const ezMat4& InverseViewProjection, ezUInt32 uiWidth, ezUInt32 uiHeight, ezDynamicArray<ezColorGammaUB>& out_Pixels, ezProgress& progress) const
{
  if (const ezBakingScene* pScene = GetScene(world))
  {
    return pScene->RenderDebugView(InverseViewProjection, uiWidth, uiHeight, out_Pixels, progress);
  }

  return EZ_FAILURE;
}
