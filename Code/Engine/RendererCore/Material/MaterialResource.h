#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>

using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;
using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;
using ezTextureCubeResourceHandle = ezTypedResourceHandle<class ezTextureCubeResource>;

struct ezMaterialResourceDescriptor
{
  struct Parameter
  {
    ezHashedString m_Name;
    ezVariant m_Value;

    EZ_FORCE_INLINE bool operator==(const Parameter& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  struct Texture2DBinding
  {
    ezHashedString m_Name;
    ezTexture2DResourceHandle m_Value;

    EZ_FORCE_INLINE bool operator==(const Texture2DBinding& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  struct TextureCubeBinding
  {
    ezHashedString m_Name;
    ezTextureCubeResourceHandle m_Value;

    EZ_FORCE_INLINE bool operator==(const TextureCubeBinding& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  void Clear();

  bool operator==(const ezMaterialResourceDescriptor& other) const;
  EZ_FORCE_INLINE bool operator!=(const ezMaterialResourceDescriptor& other) const { return !(*this == other); }

  ezMaterialResourceHandle m_hBaseMaterial;
  // ezSurfaceResource is not linked into this project (not true anymore -> could be changed)
  // this is not used for game purposes but rather for automatic collision mesh generation, so we only store the asset ID here
  ezHashedString m_sSurface;
  ezShaderResourceHandle m_hShader;
  ezDynamicArray<ezPermutationVar> m_PermutationVars;
  ezDynamicArray<Parameter> m_Parameters;
  ezDynamicArray<Texture2DBinding> m_Texture2DBindings;
  ezDynamicArray<TextureCubeBinding> m_TextureCubeBindings;
  ezRenderData::Category m_RenderDataCategory;
};

class EZ_RENDERERCORE_DLL ezMaterialResource final : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezMaterialResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezMaterialResource, ezMaterialResourceDescriptor);

public:
  ezMaterialResource();
  ~ezMaterialResource();

  ezHashedString GetPermutationValue(const ezTempHashedString& sName);
  ezHashedString GetSurface() const;

  void SetParameter(const ezHashedString& sName, const ezVariant& value);
  void SetParameter(const char* szName, const ezVariant& value);
  ezVariant GetParameter(const ezTempHashedString& sName);

  void SetTexture2DBinding(const ezHashedString& sName, const ezTexture2DResourceHandle& value);
  void SetTexture2DBinding(const char* szName, const ezTexture2DResourceHandle& value);
  ezTexture2DResourceHandle GetTexture2DBinding(const ezTempHashedString& sName);

  void SetTextureCubeBinding(const ezHashedString& sName, const ezTextureCubeResourceHandle& value);
  void SetTextureCubeBinding(const char* szName, const ezTextureCubeResourceHandle& value);
  ezTextureCubeResourceHandle GetTextureCubeBinding(const ezTempHashedString& sName);

  ezRenderData::Category GetRenderDataCategory();

  /// \brief Copies current desc to original desc so the material is not modified on reset
  void PreserveCurrentDesc();
  virtual void ResetResource() override;

  const ezMaterialResourceDescriptor& GetCurrentDesc() const;

  /// \brief Use these enum values together with GetDefaultMaterialFileName() to get the default file names for these material types.
  enum class DefaultMaterialType
  {
    Fullbright,
    FullbrightAlphaTest,
    Lit,
    LitAlphaTest,
    Sky,
    MissingMaterial
  };

  /// \brief Returns the default material file name for the given type (materials in Data/Base/Materials/BaseMaterials).
  static const char* GetDefaultMaterialFileName(DefaultMaterialType materialType);

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezMaterialResourceDescriptor m_mOriginalDesc; // stores the state at loading, such that SetParameter etc. calls can be reset later
  ezMaterialResourceDescriptor m_mDesc;

  friend class ezRenderContext;
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, MaterialResource);

  ezEvent<const ezMaterialResource*, ezMutex> m_ModifiedEvent;
  void OnBaseMaterialModified(const ezMaterialResource* pModifiedMaterial);
  void OnResourceEvent(const ezResourceEvent& resourceEvent);

  void AddPermutationVar(ezStringView sName, ezStringView sValue);

  ezAtomicInteger32 m_iLastModified;
  ezAtomicInteger32 m_iLastConstantsModified;
  ezInt32 m_iLastUpdated;
  ezInt32 m_iLastConstantsUpdated;

  bool IsModified();
  bool AreConstantsModified();

  void UpdateConstantBuffer(ezShaderPermutationResource* pShaderPermutation);

  ezConstantBufferStorageHandle m_hConstantBufferStorage;

  struct CachedValues
  {
    ezShaderResourceHandle m_hShader;
    ezHashTable<ezHashedString, ezHashedString> m_PermutationVars;
    ezHashTable<ezHashedString, ezVariant> m_Parameters;
    ezHashTable<ezHashedString, ezTexture2DResourceHandle> m_Texture2DBindings;
    ezHashTable<ezHashedString, ezTextureCubeResourceHandle> m_TextureCubeBindings;
    ezRenderData::Category m_RenderDataCategory;

    void Reset();
  };

  ezUInt32 m_uiCacheIndex;
  CachedValues* m_pCachedValues;

  CachedValues* GetOrUpdateCachedValues();
  static CachedValues* AllocateCache(ezUInt32& inout_uiCacheIndex);
  static void DeallocateCache(ezUInt32 uiCacheIndex);

  ezMutex m_UpdateCacheMutex;
  static ezDeque<ezMaterialResource::CachedValues> s_CachedValues;

  static void ClearCache();
};
