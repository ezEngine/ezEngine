#include <BakingPluginPCH.h>

#include <BakingPlugin/Baking.h>
#include <BakingPlugin/Tracer/TracerEmbree.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

ezResult ezBaking::ExtractScene(const ezWorld& world, Scene& out_Scene)
{
  out_Scene.m_BoundingBox.SetInvalid();

  for (auto it = world.GetObjects(); it.IsValid(); ++it)
  {
    const ezMeshComponentBase* pMeshComponent = nullptr;
    if (!it->TryGetComponentOfBaseType(pMeshComponent) || !pMeshComponent->GetMesh().IsValid())
      continue;

    auto& meshObject = out_Scene.m_MeshObjects.ExpandAndGetRef();
    meshObject.m_GlobalTransform = it->GetGlobalTransformSimd();
    meshObject.m_MeshResourceId.Assign(pMeshComponent->GetMeshFile());

    out_Scene.m_BoundingBox.ExpandToInclude(it->GetGlobalBounds().GetBox());
  }

  return EZ_SUCCESS;
}

ezResult ezBaking::BakeScene(const Scene& scene, const ezStringView& sOutputPath, ezProgress& progress)
{
  EZ_ASSERT_DEV(!ezThreadUtils::IsMainThread(), "BakeScene must be executed on a worker thread");

  ezTracerEmbree tracer;
  EZ_SUCCEED_OR_RETURN(tracer.BuildScene(scene));


  return EZ_SUCCESS;
}
