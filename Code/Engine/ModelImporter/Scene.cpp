#include <PCH.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Containers/IdTable.h>
#include <ModelImporter/Handles.h>
#include <Foundation/Types/UniquePtr.h>
#include <ModelImporter/Node.h>

#include <ModelImporter/Scene.h>

#include <ModelImporter/Node.h>
#include <ModelImporter/Mesh.h>
#include <ModelImporter/Material.h>

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Logging/Log.h>


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
    if (!handle.IsValid())
      return nullptr;

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
    ezUniquePtr<Material>* material = nullptr;
    m_Materials.TryGetValue(handle.GetInternalID(), material);
    return material ? material->Borrow() : nullptr;
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
    ezHashSet<ObjectHandle> elementsInChildArrays;

    m_RootObjects.Clear();
    for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
    {
      for (ObjectHandle child : it.Value()->m_Children)
      {
        elementsInChildArrays.Insert(child);
      }
    }

    // Add everything that was not in any child array.
    for (auto it = m_Meshes.GetIterator(); it.IsValid(); ++it)
    {
      ObjectHandle handle(ObjectHandle::MESH, it.Id());
      if (!elementsInChildArrays.Contains(handle))
        m_RootObjects.PushBack(it.Value().Borrow());
    }
    for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
    {
      ObjectHandle handle(ObjectHandle::NODE, it.Id());
      if (!elementsInChildArrays.Contains(handle))
        m_RootObjects.PushBack(it.Value().Borrow());
    }

    EZ_ASSERT_DEBUG((m_Nodes.IsEmpty() && m_Meshes.IsEmpty()) || !m_RootObjects.IsEmpty(), "There are objects but no root objects. The scene graph is a circle!");
  }

  namespace
  {
    template<typename HierarchyNodeCollection>
    void CreateUniqueNamesImpl(HierarchyNodeCollection& hierarchyObjectList, const char* unnamedPrefix)
    {
      ezStringBuilder tmp;

      int unnamedCounter = 0;
      ezHashSet<ezString> encounteredNames;
      for (auto it = hierarchyObjectList.GetIterator(); it.IsValid(); ++it)
      {
        auto& targetName = it.Value()->m_Name;

        if (targetName.IsEmpty())
        {
          ezStringBuilder newName = unnamedPrefix;
          newName.Append("_", ezConversionUtils::ToString(unnamedCounter, tmp));
          ++unnamedCounter;

          targetName = newName;
          encounteredNames.Insert(targetName);
        }
        else if (encounteredNames.Contains(targetName))
        {
          ezStringBuilder newName;

          int suffix = 0;
          do {
            newName = targetName;
            newName.Append("_", ezConversionUtils::ToString(suffix, tmp));
            ++suffix;
          } while (encounteredNames.Contains(newName));

          targetName = newName;
          encounteredNames.Insert(targetName);
        }
      }
    }
  }

  void Scene::CreateUniqueNames()
  {
    CreateUniqueNamesImpl(m_Nodes, "Node");
    CreateUniqueNamesImpl(m_Meshes, "Mesh");
  }

  Mesh* Scene::MergeAllMeshes(bool mergeSubmeshesWithSameMaterials)
  {
    ezStopwatch timer;
    ezUniquePtr<Mesh> mergedMesh = EZ_DEFAULT_NEW(Mesh);

    // Walk along the scene hierarchy.
    for (auto rootObject : m_RootObjects)
    {
      ezDynamicArray<HierarchyObject*> treeStack;
      ezDynamicArray<ezTransform> transformStack;
      treeStack.PushBack(rootObject);
      transformStack.PushBack(ezTransform::Identity());

      while (!treeStack.IsEmpty())
      {
        HierarchyObject* currentObject = treeStack.PeekBack();
        treeStack.PopBack();
        ezTransform currentTransform = transformStack.PeekBack();
        transformStack.PopBack();

        if (Node* node = currentObject->Cast<Node>())
        {
          currentTransform.SetGlobalTransform(currentTransform, node->m_RelativeTransform);

          for (int childIdx=node->m_Children.GetCount()-1; childIdx >=0; --childIdx)
          {
            ObjectHandle childHandle = node->m_Children[childIdx];
            if (childHandle.GetType() == ObjectHandle::MESH)
              node->m_Children.RemoveAt(childIdx);

            treeStack.PushBack(GetObject<HierarchyObject>(childHandle));
            transformStack.PushBack(currentTransform);
          }
        }
        else if (const Mesh* mesh = currentObject->Cast<Mesh>())
        {
          mergedMesh->AddData(*mesh, currentTransform);
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

    ezLog::Debug("Merged meshes in '{0}'s", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));
    return outMesh;
  }
}

