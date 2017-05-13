#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Types/UniquePtr.h>
#include <ModelImporter/Handles.h>

namespace ezModelImporter
{
  class MaterialHandle;
  class Mesh;

  namespace Pbrt
  {
    class EZ_MODELIMPORTER_DLL Object
    {
    public:
      Object();
      ~Object();

      // Old object looses data in copy. (workaround to make hash table work)
      Object(Object&& object);
      void operator = (Object&& object);

      ezTransform m_Transform;

      void AddMesh(ezUniquePtr<Mesh> mesh);
      ezArrayPtr<ObjectHandle> InstantiateMeshes(Scene& scene);

    private:
      ezHybridArray<ObjectHandle, 2> m_MeshesHandles;
      ezDynamicArray<ezUniquePtr<Mesh>> m_MeshesData;
    };

    class EZ_MODELIMPORTER_DLL ParseContext
    {
      EZ_DISALLOW_COPY_AND_ASSIGN(ParseContext);

    public:
      ParseContext(const char* modelFilename);

      void PopActiveTransform();
      void PopActiveMaterial();

      /// Returns active transform.
      /// Redirects to the active object node if any.
      ezTransform& PeekActiveTransform();
      const ezTransform& PushActiveTransform();

      MaterialHandle* PeekActiveMaterial();
      void PushActiveMaterial(MaterialHandle handle);

      void AddNamedMaterial(const char* name, MaterialHandle handle);
      /// Pops the current material (if any) and pushes the named material with the given name if any.
      ezResult MakeNamedMaterialActive(const char* name);

      void ObjectBegin(const char* name);
      Object* GetActiveObject() const;
      void ObjectEnd();
      Object* LookUpObject(const char* name);

      void EnterWorld();
      void ExitWorld();
      bool IsInWorld() const;

      void AddTexture(const char* name, const char* filename);
      const char* LookUpTextureFilename(const char* textureName) const;

      const char* GetModelFilename();

    private:
      ezDynamicArray<ezTransform> m_transformStack;
      ezDynamicArray<MaterialHandle> m_activeMaterialStack;
      ezHashTable<ezString, MaterialHandle> m_namedMaterials;

      // Objects created with "ObjectBegin"/"ObjectEnd". Can be instanticated with "ObjectInstance"
      ezHashTable<ezString, Object> m_objects;
      Object* m_activeObject;

      bool m_inWorld;

      const char* const m_modelFilename;

      ezHashTable<ezString, ezString> m_textureFilenames;
    };
  }
}
