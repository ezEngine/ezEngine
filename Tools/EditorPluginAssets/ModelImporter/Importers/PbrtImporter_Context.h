#pragma once

#include <Foundation/Containers/HashTable.h>
#include <EditorPluginAssets/ModelImporter/Handles.h>

namespace ezModelImporter
{
  class MaterialHandle;

  namespace Pbrt
  {
    class ParseContext
    {
    public:
      ParseContext(const char* modelFilename);

      void PopActiveTransform();
      void PopActiveMaterial();

      ezTransform& PeekActiveTransform();
      MaterialHandle* PeekActiveMaterial();

      const ezTransform& PushActiveTransform();
      void PushActiveMaterial(MaterialHandle handle);

      void AddNamedMaterial(const char* name, MaterialHandle handle);
      /// Pops the current material (if any) and pushes the named material with the given name if any.
      ezResult MakeNamedMaterialActive(const char* name);

      void EnterWorld();
      void ExitWorld();
      bool IsInWorld() const;

      const char* GetModelFilename();

      void AddTexture(const char* name, const char* filename);
      const char* LookUpTextureFilename(const char* textureName) const;

    private:
      ezDynamicArray<ezTransform> m_transformStack;
      ezDynamicArray<MaterialHandle> m_activeMaterialStack;
      ezHashTable<ezString, MaterialHandle> m_namedMaterials;

      // Objects created with "ObjectBegin"/"ObjectEnd". Can be instanticated with "ObjectInstance"
      ezHashTable<ezString, ObjectHandle> m_objects;

      bool m_inWorld;

      const char* const m_modelFilename;

      ezHashTable<ezString, ezString> m_textureFilenames;
    };
  }
}
