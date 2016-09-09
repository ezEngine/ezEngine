#include <PCH.h>
#include <EditorPluginAssets/ModelImporter/Scene.h>
#include <EditorPluginAssets/ModelImporter/Node.h>
#include <EditorPluginAssets/ModelImporter/Mesh.h>
#include <EditorPluginAssets/ModelImporter/Material.h>


namespace ezModelImporter
{
  Scene::Scene()
  {
  }

  Scene::~Scene()
  {
  }

  HierarchyObject* Scene::GetObject(ObjectHandle handle)
  {
    switch (handle.GetType())
    {
    case ObjectHandle::NODE:
      return m_Nodes[handle.m_Id].Borrow();

    case ObjectHandle::MESH:
      return m_Meshes[handle.m_Id].Borrow();

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
    }

    return nullptr;
  }

  const Material* Scene::GetMaterial(MaterialHandle handle) const
  {
    return m_Materials[handle.GetInternalID()].Borrow();
  }

  ObjectHandle Scene::AddNode(ezUniquePtr<Node> node)
  {
    return ObjectHandle(ObjectHandle::NODE, m_Nodes.Insert(std::move(node)));
  }

  ObjectHandle Scene::AddMesh(ezUniquePtr<Mesh> mesh)
  {
    return ObjectHandle(ObjectHandle::MESH, m_Meshes.Insert(std::move(mesh)));
  }

  MaterialHandle Scene::AddMaterial(ezUniquePtr<Material> material)
  {
    return MaterialHandle(m_Materials.Insert(std::move(material)));
  }

  void Scene::RefreshRootList()
  {
    m_RootObjects.Clear();
    for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
    {
      if (!it.Value()->GetParent().IsValid())
        m_RootObjects.PushBack(it.Value().Borrow());
    }
    for (auto it = m_Meshes.GetIterator(); it.IsValid(); ++it)
    {
      if (!it.Value()->GetParent().IsValid())
        m_RootObjects.PushBack(it.Value().Borrow());
    }
  }

  Mesh* Scene::MergeAllMeshes()
  {
    ezUniquePtr<Mesh> mergedMesh = EZ_DEFAULT_NEW(Mesh);

    for (auto meshIt = m_Meshes.GetIterator(); meshIt.IsValid(); ++meshIt)
    {
      mergedMesh->AddData(*meshIt.Value());

      // Todo: Apply transforms from nodes.
      // It is easier to apply the transform to the source mesh which will be copied and deleted anyways.

      // Remove handles & pointers to this mesh.
      for (auto nodeIt = m_Nodes.GetIterator(); nodeIt.IsValid(); ++nodeIt)
      {
        auto& children = nodeIt.Value()->m_Children;
        for (int i = children.GetCount(); i >= 0; ++i)
        {
          if (children[i] == ObjectHandle(ObjectHandle::MESH, meshIt.Id()))
          {
            children.RemoveAt(i);
          }
        }

        if (children.IsEmpty())
        {
          // TODO: Remove empty node.
        }
      }
    }


    // Destroy all old meshes.
    m_Meshes.Clear();
    m_RootObjects.Clear();


    Mesh* outMesh = mergedMesh.Borrow();
    m_Meshes.Insert(std::move(mergedMesh));
    return outMesh;
  }
}
