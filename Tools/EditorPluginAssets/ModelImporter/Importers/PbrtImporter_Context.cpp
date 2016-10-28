#include <PCH.h>
#include <EditorPluginAssets/ModelImporter/Importers/PbrtImporter_Context.h>
#include <EditorPluginAssets/ModelImporter/Material.h>
#include <EditorPluginAssets/ModelImporter/Scene.h>
#include <EditorPluginAssets/ModelImporter/Node.h>
#include <EditorPluginAssets/ModelImporter/Mesh.h>
#include <Foundation/Logging/Log.h>

namespace ezModelImporter
{
  namespace Pbrt
  {
    Object::Object() : m_Transform(ezTransform::Identity())
    {}

    Object::~Object()
    {}

    Object::Object(Object&& object)
    {
      *this = std::move(object);
    }

    void Object::operator = (Object&& object)
    {
      m_Transform = object.m_Transform;
      m_MeshesHandles = std::move(object.m_MeshesHandles);
      m_MeshesData = std::move(object.m_MeshesData);
    }

    void Object::AddMesh(ezUniquePtr<Mesh> mesh)
    {
      EZ_ASSERT_DEBUG(m_MeshesHandles.IsEmpty(), "Can't add meshes to Pbrt::Object after it was initialized once.");
      m_MeshesData.PushBack(std::move(mesh));
    }

    ezArrayPtr<ObjectHandle> Object::InstantiateMeshes(Scene& scene)
    {
      if(m_MeshesHandles.IsEmpty())
      {
        m_MeshesHandles.SetCountUninitialized(m_MeshesData.GetCount());
        for(ezUInt32 i=0; i<m_MeshesData.GetCount(); ++i)
        {
          m_MeshesHandles[i] = scene.AddMesh(std::move(m_MeshesData[i]));
        }
        m_MeshesData.Clear();
      }

      return m_MeshesHandles;
    }

    ParseContext::ParseContext(const char* modelFilename)
      : m_inWorld(false)
      , m_modelFilename(modelFilename)
      , m_activeObject(nullptr)
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
      if (m_activeObject)
        return m_activeObject->m_Transform;
      else
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

    void ParseContext::ObjectBegin(const char* name)
    {
      m_activeObject = nullptr;
      m_objects.TryGetValue(name, m_activeObject);
      if (!m_activeObject)
      {
        m_objects.Insert(name, Object());
        m_activeObject = &m_objects[name];
      }
    }

    Object* ParseContext::GetActiveObject() const
    {
      return m_activeObject;
    }

    void ParseContext::ObjectEnd()
    {
      m_activeObject = nullptr;
    }

    Object* ParseContext::LookUpObject(const char* name)
    {
      Object* out = nullptr;
      m_objects.TryGetValue(name, out);
      return out;
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
