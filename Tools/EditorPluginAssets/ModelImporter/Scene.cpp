#include <PCH.h>
#include <EditorPluginAssets/ModelImporter/Scene.h>
#include <EditorPluginAssets/ModelImporter/Node.h>
#include <EditorPluginAssets/ModelImporter/Mesh.h>
#include <EditorPluginAssets/ModelImporter/Material.h>

#include <Foundation/Containers/HashSet.h>


namespace ezModelImporter
{
  Scene::Scene()
  {
  }

  Scene::~Scene()
  {
  }

  const HierarchyObject* Scene::GetObject(ObjectHandle handle) const
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

  HierarchyObject* Scene::GetObject(ObjectHandle handle)
  {
    return const_cast<HierarchyObject*>(static_cast<const Scene*>(this)->GetObject(handle));
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
    ezHashSet<ObjectHandle> elementsInRootListIndirect;

    m_RootObjects.Clear();
    for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
    {
      ObjectHandle handle(ObjectHandle::NODE, it.Id());
      if (!it.Value()->GetParent().IsValid() && !elementsInRootListIndirect.Contains(handle))
      {
        m_RootObjects.PushBack(it.Value().Borrow());

        // Recursively put all children into the indirect list.
        ezHybridArray<ObjectHandle, 32> childHandles;
        childHandles.PushBack(handle);
        while(!childHandles.IsEmpty())
        {
          handle = childHandles.PeekBack();
          childHandles.PopBack();
          elementsInRootListIndirect.Insert(handle);

          if (handle.GetType() == ObjectHandle::NODE)
          {
            Node* node = GetObject<Node>(handle);
            childHandles.PushBackRange(node->m_Children);
          }
        }
      }
    }

    // Add all meshes that are not indirectly in the list already.
    for (auto it = m_Meshes.GetIterator(); it.IsValid(); ++it)
    {
      ObjectHandle handle(ObjectHandle::MESH, it.Id());
      if (!elementsInRootListIndirect.Contains(handle))
        m_RootObjects.PushBack(it.Value().Borrow());
    }
  }

  Mesh* Scene::MergeAllMeshes(bool mergeSubmeshesWithSameMaterials)
  {
    ezUniquePtr<Mesh> mergedMesh = EZ_DEFAULT_NEW(Mesh);

    // Get all meshes and their transform.
    for (auto meshIt = m_Meshes.GetIterator(); meshIt.IsValid(); ++meshIt)
    {
      // Apply transform if any.
      if (meshIt.Value()->GetParent().IsValid())
      {
        // Computing transform like this awnew every time is a bit wasteful, but simple to implement.
        ezTransform transform = ezTransform::Identity();
        GetObject<Node>(meshIt.Value()->GetParent())->ComputeAbsoluteTransform(*this, transform);
        meshIt.Value()->ApplyTransform(transform);
      }

      // Merge.
      mergedMesh->AddData(*meshIt.Value());

      // Remove handles & pointers to this mesh.
      for (auto nodeIt = m_Nodes.GetIterator(); nodeIt.IsValid(); ++nodeIt)
      {
        auto& children = nodeIt.Value()->m_Children;
        for (int i = children.GetCount()-1; i >= 0; --i)
        {
          if (children[i] == ObjectHandle(ObjectHandle::MESH, meshIt.Id()))
          {
            children.RemoveAt(i);
          }
        }
      }
    }

    if (mergeSubmeshesWithSameMaterials)
      mergedMesh->MergeSubMeshesWithSameMaterials();

    // Destroy all old meshes.
    m_Meshes.Clear();
    m_RootObjects.Clear();

    Mesh* outMesh = mergedMesh.Borrow();
    m_Meshes.Insert(std::move(mergedMesh));
    return outMesh;
  }

  void Scene::RemoveEmptyNodesRec(ObjectId nodeId)
  {

  }
}
