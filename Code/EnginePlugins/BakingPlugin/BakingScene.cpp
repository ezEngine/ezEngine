#include <BakingPluginPCH.h>

#include <BakingPlugin/BakingScene.h>
#include <BakingPlugin/Tracer/TracerEmbree.h>
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

  return EZ_SUCCESS;
}

ezBakingScene::ezBakingScene() = default;
ezBakingScene::~ezBakingScene() = default;
