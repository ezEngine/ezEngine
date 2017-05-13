#pragma once

#include <ModelImporter/HierarchyObject.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/RefCounted.h>

namespace ezModelImporter
{
  class HierarchyObject;
  class Mesh;
  class Node;
  struct Material;

  /// Data representation of an imported scene.
  ///
  /// A scene contains all data that has been imported from a single file.
  /// All data is as raw as possible, however we apply basic preprocessing during import already to fit into our data structure.
  /// \see ezModelImporter::Importer
  class EZ_MODELIMPORTER_DLL Scene : public ezRefCounted
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(Scene);

  public:
    Scene();
    ~Scene();

    // Data access.
  public:

    /// Returns a list of all root objects.
    ezArrayPtr<const HierarchyObject* const> GetRootObjects() const { return ezMakeArrayPtr(m_RootObjects); }
    ezArrayPtr<HierarchyObject*> GetRootObjects() { return ezMakeArrayPtr(m_RootObjects); }

    //const HierarchyObject* GetObject(ObjectReference handle) const;
    const HierarchyObject* GetObject(ObjectHandle handle) const;
    HierarchyObject* GetObject(ObjectHandle handle);
    template<typename T>
    const T* GetObject(ObjectHandle handle) const { auto obj = GetObject(handle); return obj ? obj->Cast<T>() : nullptr; }
    template<typename T>
    T* GetObject(ObjectHandle handle) { auto obj = GetObject(handle); return obj ? obj->Cast<T>() : nullptr; }

    const Material* GetMaterial(MaterialHandle handle) const;

    const ezIdTable<ObjectId, ezUniquePtr<Node>>& GetNodes() const           { return m_Nodes; }
    const ezIdTable<ObjectId, ezUniquePtr<Mesh>>& GetMeshes() const          { return m_Meshes; }
    const ezIdTable<MaterialId, ezUniquePtr<Material>>& GetMaterials() const { return m_Materials; }
    ezIdTable<ObjectId, ezUniquePtr<Node>>& GetNodes()                       { return m_Nodes; }
    ezIdTable<ObjectId, ezUniquePtr<Mesh>>& GetMeshes()                      { return m_Meshes; }
    ezIdTable<MaterialId, ezUniquePtr<Material>>& GetMaterials()             { return m_Materials; }

    // Manipulation methods for importer implementations.
  public:

    ObjectHandle AddNode(ezUniquePtr<Node> node);
    ObjectHandle AddMesh(ezUniquePtr<Mesh> mesh);
    MaterialHandle AddMaterial(ezUniquePtr<Material> material);

    /// Adds all objects without a parent to the list of root objects.
    ///
    /// Called by Importer after ImporterImplementation is done.
    void RefreshRootList();

    /// Ensures that every hierarchy object has an unique name.
    ///
    /// Does not change the name of materials
    /// Called by Importer after ImporterImplementation is done.
    void CreateUniqueNames();

    // Postprocessing
  public:
    /// Merges all meshes into a single one.
    ///
    /// Assumes that the root list is up to date.
    /// Transformations from nodes will be applied. The resulting mesh will be stored in the list of root objects.
    /// Note that instanced meshes will be duplicated in this process!
    /// \param mergeSubmeshesWithIdenticalMaterials
    ///   If true, all submeshes that use the same material will be merged in the resulting mesh.
    /// \return
    ///   Pointer to the newly created meshnode.
    Mesh* MergeAllMeshes(bool mergeSubmeshesWithSameMaterials = true);


  private:

    ezDynamicArray<HierarchyObject*> m_RootObjects;

    ezIdTable<ObjectId, ezUniquePtr<ezModelImporter::Node>> m_Nodes;
    ezIdTable<ObjectId, ezUniquePtr<Mesh>> m_Meshes;
    ezIdTable<MaterialId, ezUniquePtr<Material>> m_Materials;
  };
}
