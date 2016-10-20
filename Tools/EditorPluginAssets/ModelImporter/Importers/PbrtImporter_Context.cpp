#include <PCH.h>
#include <EditorPluginAssets/ModelImporter/Importers/PbrtImporter_Context.h>
#include <EditorPluginAssets/ModelImporter/Material.h>
#include <EditorPluginAssets/ModelImporter/Scene.h>
#include <EditorPluginAssets/ModelImporter/Node.h>
#include <Foundation/Logging/Log.h>

namespace ezModelImporter
{
  namespace Pbrt
  {
    ParseContext::ParseContext(const char* modelFilename) : m_inWorld(false), m_modelFilename(modelFilename)
    {
      m_transformStack.PushBack(ezTransform::Identity());
      m_activeMaterialStack.PushBack(MaterialHandle());
    }

    void ParseContext::PopActiveTransform()
    {
      if (m_transformStack.GetCount() > 1)
        m_transformStack.PopBack();
    }
    void ParseContext::PopActiveMaterial()
    {
      if (m_activeMaterialStack.GetCount() > 1)
        m_activeMaterialStack.PopBack();
    }

    ezTransform& ParseContext::PeekActiveTransform()
    {
      return m_transformStack.PeekBack();
    }

    MaterialHandle* ParseContext::PeekActiveMaterial()
    {
      return m_activeMaterialStack.IsEmpty() ? nullptr : &m_activeMaterialStack.PeekBack();
    }

    const ezTransform& ParseContext::PushActiveTransform()
    {
      if (!m_inWorld)
        ezLog::Warning("Invalid Pbrt file: Transforms can only be pushed on the stack in the world section.");

      m_transformStack.PushBack(m_transformStack.PeekBack());
      return m_transformStack.PeekBack();
    }
    void ParseContext::PushActiveMaterial(MaterialHandle handle)
    {
      if (!m_inWorld)
        ezLog::Warning("Invalid Pbrt file: Materials can only be pushed on the stack in the world section.");

      m_activeMaterialStack.PushBack(handle);
    }

    void ParseContext::AddNamedMaterial(const char* name, MaterialHandle handle)
    {
      m_namedMaterials.Insert(name, handle);
    }

    ezResult ParseContext::MakeNamedMaterialActive(const char* name)
    {
      MaterialHandle handle;
      if (m_namedMaterials.TryGetValue(name, handle))
      {
        m_activeMaterialStack.PopBack();
        m_activeMaterialStack.PushBack(handle);
        return EZ_SUCCESS;
      }
      else
        return EZ_FAILURE;
    }

    void ParseContext::EnterWorld()
    {
      m_inWorld = true;

      // There should be only one transform and one material on the stack, but we better be failsafe here!
      m_transformStack.Clear();
      m_transformStack.PushBack(ezTransform::Identity());
      m_activeMaterialStack.Clear();
      m_activeMaterialStack.PushBack(MaterialHandle());
    }

    void ParseContext::ExitWorld()
    {
      m_inWorld = false;
    }

    bool ParseContext::IsInWorld() const
    {
      return m_inWorld;
    }

    const char* ParseContext::GetModelFilename()
    {
      return m_modelFilename;
    }

    void ParseContext::AddTexture(const char* name, const char* filename)
    {
      m_textureFilenames.Insert(name, filename);
    }

    const char* ParseContext::LookUpTextureFilename(const char* textureName) const
    {
      ezString* filename = nullptr;
      m_textureFilenames.TryGetValue(textureName, filename);
      return filename ? filename->GetData() : nullptr;
    }
  }
}
