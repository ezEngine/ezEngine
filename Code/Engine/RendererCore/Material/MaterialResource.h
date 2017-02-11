#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <Foundation/Containers/HashTable.h>
#include <Core/ResourceManager/Resource.h>

typedef ezTypedResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;
typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;
typedef ezTypedResourceHandle<class ezTextureCubeResource> ezTextureCubeResourceHandle;

struct ezMaterialResourceDescriptor
{
  struct Parameter
  {
    ezHashedString m_Name;
    ezVariant m_Value;

    EZ_FORCE_INLINE bool operator==(const Parameter& other) const
    {
      return m_Name == other.m_Name && m_Value == other.m_Value;
    }
  };

  struct Texture2DBinding
  {
    ezHashedString m_Name;
    ezTexture2DResourceHandle m_Value;

    EZ_FORCE_INLINE bool operator==(const Texture2DBinding& other) const
    {
      return m_Name == other.m_Name && m_Value == other.m_Value;
    }
  };

  struct TextureCubeBinding
  {
    ezHashedString m_Name;
    ezTextureCubeResourceHandle m_Value;

    EZ_FORCE_INLINE bool operator==(const TextureCubeBinding& other) const
    {
      return m_Name == other.m_Name && m_Value == other.m_Value;
    }
  };

  void Clear();

  bool operator==(const ezMaterialResourceDescriptor& other) const;
  EZ_FORCE_INLINE bool operator!=(const ezMaterialResourceDescriptor& other) const
  {
    return !(*this == other);
  }

  ezMaterialResourceHandle m_hBaseMaterial;
  ezShaderResourceHandle m_hShader;
  ezDynamicArray<ezPermutationVar> m_PermutationVars;
  ezDynamicArray<Parameter> m_Parameters;
  ezDynamicArray<Texture2DBinding> m_Texture2DBindings;
  ezDynamicArray<TextureCubeBinding> m_TextureCubeBindings;
};

class EZ_RENDERERCORE_DLL ezMaterialResource : public ezResource<ezMaterialResource, ezMaterialResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialResource, ezResourceBase);

public:
  ezMaterialResource();
  ~ezMaterialResource();

  ezHashedString GetPermutationValue(const ezTempHashedString& sName);

  void SetParameter(const ezHashedString& sName, const ezVariant& value);
  void SetParameter(const char* szName, const ezVariant& value);
  ezVariant GetParameter(const ezTempHashedString& sName);

  void SetTexture2DBinding(const ezHashedString& sName, const ezTexture2DResourceHandle& value);
  void SetTexture2DBinding(const char* szName, const ezTexture2DResourceHandle& value);
  ezTexture2DResourceHandle GetTexture2DBinding(const ezTempHashedString& sName);

  void SetTextureCubeBinding(const ezHashedString& sName, const ezTextureCubeResourceHandle& value);
  void SetTextureCubeBinding(const char* szName, const ezTextureCubeResourceHandle& value);
  ezTextureCubeResourceHandle GetTextureCubeBinding(const ezTempHashedString& sName);

  /// \brief Copies current desc to original desc so the material is not modified on reset
  void PreserveCurrentDesc();
  virtual void ResetResource() override;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezMaterialResourceDescriptor& descriptor) override;

private:
  ezMaterialResourceDescriptor m_OriginalDesc; // stores the state at loading, such that SetParameter etc. calls can be reset later
  ezMaterialResourceDescriptor m_Desc;

  friend class ezRenderContext;

  ezEvent<const ezMaterialResource*> m_ModifiedEvent;
  void OnBaseMaterialModified(const ezMaterialResource* pModifiedMaterial);
  void OnResourceEvent(const ezResourceEvent& resourceEvent);

  ezAtomicInteger32 m_iLastModified;
  ezAtomicInteger32 m_iLastConstantsModified;
  ezInt32 m_iLastUpdated;
  ezInt32 m_iLastConstantsUpdated;

  bool IsModified();
  bool AreContantsModified();

  void UpdateCaches();
  void UpdateConstantBuffer(ezShaderPermutationResource* pShaderPermutation);

  ezConstantBufferStorageHandle m_hConstantBufferStorage;

  ezMutex m_CacheMutex;
  ezShaderResourceHandle m_hCachedShader;
  ezHashTable<ezHashedString, ezHashedString> m_CachedPermutationVars;
  ezHashTable<ezHashedString, ezVariant> m_CachedParameters;
  ezHashTable<ezHashedString, ezTexture2DResourceHandle> m_CachedTexture2DBindings;
  ezHashTable<ezHashedString, ezTextureCubeResourceHandle> m_CachedTextureCubeBindings;
};
