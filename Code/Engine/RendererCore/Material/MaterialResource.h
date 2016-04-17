#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <Foundation/Containers/HashTable.h>
#include <Core/ResourceManager/Resource.h>

typedef ezTypedResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;
typedef ezTypedResourceHandle<class ezTextureResource> ezTextureResourceHandle;

struct ezMaterialResourceDescriptor
{
  struct ShaderConstant
  {
    ezHashedString m_Name;
    ezVariant m_Value;
  };

  struct TextureBinding
  {
    ezHashedString m_Name;
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
  ezDynamicArray<ezPermutationVar> m_PermutationVars;
  ezDynamicArray<ShaderConstant> m_ShaderConstants;
  ezDynamicArray<TextureBinding> m_TextureBindings;
};

class EZ_RENDERERCORE_DLL ezMaterialResource : public ezResource<ezMaterialResource, ezMaterialResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialResource, ezResourceBase);

public:
  ezMaterialResource();

  EZ_FORCE_INLINE const ezMaterialResourceDescriptor& GetDescriptor()
  {
    return m_Desc;
  }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezMaterialResourceDescriptor& descriptor) override;

private:
  ezMaterialResourceDescriptor m_Desc;
};


