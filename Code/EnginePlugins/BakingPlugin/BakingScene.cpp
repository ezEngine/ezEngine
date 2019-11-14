#include <BakingPluginPCH.h>

#include <BakingPlugin/BakingScene.h>
#include <BakingPlugin/Tracer/TracerEmbree.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

namespace
{
  static ezDynamicArray<ezUniquePtr<ezBakingScene>, ezStaticAllocatorWrapper> s_BakingScenes;
}

// static
ezBakingScene* ezBakingScene::GetOrCreate(const ezWorld& world)
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

// static
ezBakingScene* ezBakingScene::Get(const ezWorld& world)
{
  const ezUInt32 uiWorldIndex = world.GetIndex();

  if (uiWorldIndex < s_BakingScenes.GetCount())
  {
    return s_BakingScenes[uiWorldIndex].Borrow();
  }

  return nullptr;
}

ezResult ezBakingScene::Extract()
{
  if (m_pTracer == nullptr)
  {
    m_pTracer = EZ_DEFAULT_NEW(ezTracerEmbree);
  }

  m_MeshObjects.Clear();
  m_BoundingBox.SetInvalid();
  m_bTracerReady = false;

  const ezWorld* pWorld = ezWorld::GetWorld(m_uiWorldIndex);
  if (pWorld == nullptr)
  {
    return EZ_FAILURE;
  }

  const ezWorld& world = *pWorld;
  EZ_LOCK(world.GetReadMarker());

  for (auto it = world.GetObjects(); it.IsValid(); ++it)
  {
    const ezMeshComponentBase* pMeshComponent = nullptr;
    if (!it->TryGetComponentOfBaseType(pMeshComponent) || !pMeshComponent->GetMesh().IsValid())
      continue;

    auto& meshObject = m_MeshObjects.ExpandAndGetRef();
    meshObject.m_GlobalTransform = it->GetGlobalTransformSimd();
    meshObject.m_MeshResourceId.Assign(pMeshComponent->GetMeshFile());

    m_BoundingBox.ExpandToInclude(it->GetGlobalBounds().GetBox());
  }

  return EZ_SUCCESS;
}

ezResult ezBakingScene::Bake(const ezStringView& sOutputPath, ezProgress& progress)
{
  EZ_ASSERT_DEV(!ezThreadUtils::IsMainThread(), "BakeScene must be executed on a worker thread");

  EZ_SUCCEED_OR_RETURN(m_pTracer->BuildScene(*this));
  m_bTracerReady = true;

  return EZ_SUCCESS;
}

ezResult ezBakingScene::RenderDebugView(const ezMat4& InverseViewProjection, ezUInt32 uiWidth, ezUInt32 uiHeight, ezDynamicArray<ezColorGammaUB>& out_Pixels,
  ezProgress& progress) const
{
  if (!m_bTracerReady)
    return EZ_FAILURE;

  const ezUInt32 uiNumPixel = uiWidth * uiHeight;
  out_Pixels.SetCountUninitialized(uiNumPixel);

  ezUInt32 uiStartPixel = 0;
  ezUInt32 uiPixelPerBatch = 128;
  while (uiStartPixel < uiNumPixel)
  {
    uiPixelPerBatch = ezMath::Min(uiPixelPerBatch, uiNumPixel - uiStartPixel);

    for (ezUInt32 i = 0; i < uiPixelPerBatch; ++i)
    {
      out_Pixels[uiStartPixel + i] = ezColor((float)i / uiPixelPerBatch, 0, 0);
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
