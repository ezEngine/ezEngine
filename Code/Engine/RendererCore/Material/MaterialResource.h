#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererCore/Shader/ShaderResource.h>

typedef ezResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;
typedef ezResourceHandle<class ezTextureResource> ezTextureResourceHandle;

struct ezMaterialResourceDescriptor
{
  struct PermutationVar
  {
    ezHashedString m_Name;
    ezHashedString m_Value;
  };

  struct ShaderConstant
  {
    ezUInt32 m_NameHash;
    ezVariant m_Value;
  };

  struct TextureBinding
  {
    ezUInt32 m_NameHash;
    ezTextureResourceHandle m_Value;
  };

  void Clear()
  {
    m_hBaseMaterial.Invalidate();
    m_hShader.Invalidate();
    m_PermutationVars.Clear();
    m_ShaderConstants.Clear();
    m_TextureBindings.Clear();
  }

  ezMaterialResourceHandle m_hBaseMaterial;
  ezShaderResourceHandle m_hShader;
  ezDynamicArray<PermutationVar> m_PermutationVars;
  ezDynamicArray<ShaderConstant> m_ShaderConstants;
  ezDynamicArray<TextureBinding> m_TextureBindings;
};

class EZ_RENDERERCORE_DLL ezMaterialResource : public ezResource<ezMaterialResource, ezMaterialResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialResource);

public:
  ezMaterialResource();

  EZ_FORCE_INLINE const ezMaterialResourceDescriptor& GetDescriptor()
  {
    return m_Desc;
  }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReaderBase* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezMaterialResourceDescriptor& descriptor) override;

private:
  ezMaterialResourceDescriptor m_Desc;
};


