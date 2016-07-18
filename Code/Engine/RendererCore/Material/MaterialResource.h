#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <Foundation/Containers/HashTable.h>
#include <Core/ResourceManager/Resource.h>

typedef ezTypedResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;
typedef ezTypedResourceHandle<class ezTextureResource> ezTextureResourceHandle;

struct ezMaterialResourceDescriptor
{
  struct Parameter
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
    m_Parameters.Clear();
    m_TextureBindings.Clear();
  }

  ezMaterialResourceHandle m_hBaseMaterial;
  ezShaderResourceHandle m_hShader;
  ezDynamicArray<ezPermutationVar> m_PermutationVars;
  ezDynamicArray<Parameter> m_Parameters;
  ezDynamicArray<TextureBinding> m_TextureBindings;
};

class EZ_RENDERERCORE_DLL ezMaterialResource : public ezResource<ezMaterialResource, ezMaterialResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialResource, ezResourceBase);

public:
  ezMaterialResource();

  ezTempHashedString GetPermutationValue(const ezTempHashedString& sName);
  
  void SetParameter(const char* szName, const ezVariant& value);
  void SetTextureBinding(const char* szName, ezTextureResourceHandle value);

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

  friend class ezRenderContext;

  void UpdateConstantBuffer(ezHashTable<ezHashedString, ezVariant>& parameters, ezShaderPermutationResource* pShaderPermutation, ezUInt64 uiLastModified);

  ezUInt64 m_uiLastModified;
  ezUInt64 m_uiLastUpdated;
  ezConstantBufferStorageHandle m_hConstantBufferStorage;
};


