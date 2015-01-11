#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererCore/Shader/ShaderResource.h>

typedef ezResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;

struct ezMaterialResourceDescriptor
{
  ezShaderResourceHandle m_hShader;
};

class EZ_RENDERERCORE_DLL ezMaterialResource : public ezResource<ezMaterialResource, ezMaterialResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialResource);

public:
  ezMaterialResource();

  EZ_FORCE_INLINE ezShaderResourceHandle GetShader()
  {
    return m_hShader;
  }

private:
  virtual void UnloadData(bool bFullUnload) override;
  virtual void UpdateContent(ezStreamReaderBase* Stream) override;
  virtual void UpdateMemoryUsage() override;
  virtual void CreateResource(const ezMaterialResourceDescriptor& descriptor) override;

private:
  struct PermutationVar
  {
    ezString m_Name;
    ezString m_Value;
  };

  ezDynamicArray<PermutationVar> m_PermutationVars;

  ezHashTable<ezHashedString, ezVariant> m_Parameter;
  //ezHashTable<ezHashedString, ezTextureResourceHandle> m_Textures;

  ezShaderResourceHandle m_hShader;
};


