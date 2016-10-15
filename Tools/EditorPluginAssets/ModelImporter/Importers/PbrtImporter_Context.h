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

      void EnterWorld();
      void ExitWorld();
      bool IsInWorld() const;

      const char* GetModelFilename();

    private:
      ezDynamicArray<ezTransform> m_transformStack;
      ezDynamicArray<MaterialHandle> m_activeMaterialStack;

      // Objects created with "ObjectBegin"/"ObjectEnd". Can be instanticated with "ObjectInstance"
      ezHashTable<ezString, ObjectHandle> m_objects;

      bool m_inWorld;

      const char* const m_modelFilename;
    };
  }
}
