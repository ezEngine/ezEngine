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

/// Descriptor for creating material resources.
///
/// Materials can inherit from a base material and override shader, parameters, and textures.
/// Supports shader permutation variables, numeric/color parameters, and 2D/cube texture bindings.
struct ezMaterialResourceDescriptor
{
  /// A shader parameter name-value pair.
  struct Parameter
  {
    ezHashedString m_Name; ///< Parameter name (must match shader constant).
    ezVariant m_Value;     ///< Parameter value (numeric or color).

    EZ_FORCE_INLINE bool operator==(const Parameter& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  /// A 2D texture binding.
  struct Texture2DBinding
  {
    ezHashedString m_Name;             ///< Texture slot name (must match shader resource).
    ezTexture2DResourceHandle m_Value; ///< Texture resource handle.

    EZ_FORCE_INLINE bool operator==(const Texture2DBinding& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  /// A cube texture binding.
  struct TextureCubeBinding
  {
    ezHashedString m_Name;               ///< Texture slot name (must match shader resource).
    ezTextureCubeResourceHandle m_Value; ///< Cube map resource handle.

    EZ_FORCE_INLINE bool operator==(const TextureCubeBinding& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  void Clear();

  bool operator==(const ezMaterialResourceDescriptor& other) const;
  EZ_FORCE_INLINE bool operator!=(const ezMaterialResourceDescriptor& other) const { return !(*this == other); }

  ezMaterialResourceHandle m_hBaseMaterial;                 ///< Base material to inherit from (optional).
  ezHashedString m_sSurface;                                ///< Surface type for physics/collision properties.
  ezShaderResourceHandle m_hShader;                         ///< Shader used for rendering.
  ezDynamicArray<ezPermutationVar> m_PermutationVars;       ///< Shader permutation variable values.
  ezDynamicArray<Parameter> m_Parameters;                   ///< Shader constant parameters.
  ezDynamicArray<Texture2DBinding> m_Texture2DBindings;     ///< 2D texture bindings.
  ezDynamicArray<TextureCubeBinding> m_TextureCubeBindings; ///< Cube texture bindings.
  ezRenderData::Category m_RenderDataCategory;              ///< Render data category (opaque, transparent, etc.).
};

/// Resource representing a material with shader, parameters, and textures.
///
/// Materials define the visual appearance of rendered objects. They reference a shader and provide
/// values for shader parameters and texture slots. Supports material inheritance through base materials.
class EZ_RENDERERCORE_DLL ezMaterialResource final : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezMaterialResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezMaterialResource, ezMaterialResourceDescriptor);

public:
  /// Default material types with pre-defined shaders.
  ///
  /// Use with GetDefaultMaterialFileName() to get file paths for these materials.
  enum class DefaultMaterialType
  {
    Fullbright,          ///< Unlit material without lighting calculations.
    FullbrightAlphaTest, ///< Unlit material with alpha testing.
    Lit,                 ///< Standard lit material.
    LitAlphaTest,        ///< Lit material with alpha testing.
    Sky,                 ///< Sky material for skyboxes.
    MissingMaterial      ///< Placeholder for missing materials.
  };

  using ezMaterialId = ezGenericId<24, 8>;

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
  static ezRenderData::Category GetRenderDataCategory(const ezMaterialResourceHandle& hMaterial, bool* out_pWasFallback = nullptr, ezRenderData::Category fallbackCategory = ezDefaultRenderDataCategories::LitOpaque);

  /// \brief Copies current desc to original desc so the material is not modified on reset
  void PreserveCurrentDesc();
  virtual void ResetResource() override;

  const ezMaterialResourceDescriptor& GetCurrentDesc() const;

  /// \brief In case the renderer uses structured buffers to store materials, this is the index into the buffer returns by ezMaterialManager::GetMaterialData.
  ///
  /// You only need to call this if you want to persist the index in some other storage as ezMaterialManager::GetMaterialData will return the index as well.
  /// Important: The index can change if the shader changes. This can only be done by unloading and reloading the material in which case the [] event is fired.
  ezMaterialId GetMaterialId() const { return m_MaterialId; }

  /// \brief Returns the default material file name for the given type (materials in Data/Base/Materials/BaseMaterials).
  static const char* GetDefaultMaterialFileName(DefaultMaterialType materialType);

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

public:
  struct DirtyFlags
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      Parameter = EZ_BIT(0),
      Texture2D = EZ_BIT(1),
      TextureCube = EZ_BIT(2),
      PermutationVar = EZ_BIT(3),
      ShaderAndId = EZ_BIT(4),
      ResourceReset = Parameter | Texture2D | TextureCube | PermutationVar,
      ResourceCreation = ResourceReset | ShaderAndId,
      Default = 0
    };

    struct Bits
    {
      StorageType Parameter : 1;
      StorageType Texture2D : 1;
      StorageType TextureCube : 1;
      StorageType PermutationVar : 1;
      StorageType ShaderAndId : 1;
    };
  };

private:
  friend class ezRenderContext;
  friend class ezMaterialManager;

  ezEvent<const ezMaterialResource*, ezMutex> m_ModifiedEvent;

  void AddPermutationVar(ezStringView sName, ezStringView sValue);
  void SetModified(DirtyFlags::Enum flag);
  void FlattenOriginalDescHierarchy();
  void ComputeRenderDataCategory();

private:
  ezMaterialResourceDescriptor m_mOriginalDesc; // stores the state at loading, such that SetParameter etc. calls can be reset later

  // Dynamic data
  ezMaterialResourceDescriptor m_mDesc; // Current desc of the material. Contains any changes done after loading.
  ezBitflags<DirtyFlags> m_DirtyFlags;  // Flags indicating what has changed in m_mDesc this frame.

  // ezMaterialManager registration
  ezShaderResourceHandle m_hShader;
  ezMaterialId m_MaterialId;
};

EZ_DECLARE_FLAGS_OPERATORS(ezMaterialResource::DirtyFlags);
