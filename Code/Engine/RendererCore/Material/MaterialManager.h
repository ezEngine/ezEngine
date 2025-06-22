#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

class ezGALCommandEncoder;
struct ezGALDeviceEvent;

/// \brief Fired by ezMaterialManager::s_MaterialShaderChangedEvent in case a material is reloaded and now occupies another index / shader.
struct EZ_RENDERERCORE_DLL ezMaterialShaderChanged
{
  ezMaterialResourceHandle m_hMaterial;
  ezShaderResourceHandle m_hOldShader;
  ezMaterialResource::ezMaterialId m_OldId;
  ezShaderResourceHandle m_hNewShader;
  ezMaterialResource::ezMaterialId m_NewId;
};

class EZ_RENDERERCORE_DLL ezMaterialManager
{
  EZ_DECLARE_SINGLETON(ezMaterialManager);

public:
  /// \brief The material's render data. Accessed via ezMaterialManager::GetMaterialData.
  struct EZ_RENDERERCORE_DLL MaterialData
  {
    ezShaderResourceHandle m_hShader;
    ezMaterialResource::ezMaterialId m_MaterialId; ///< Index into m_hStructuredBufferView

    ezDynamicArray<ezPermutationVar> m_PermutationVars;

    // Depending on the capabilities of the GAL device, materials are either stored in a constant or structured buffer.
    ezGALBufferHandle m_hStructuredBuffer;
    ezGALBufferHandle m_hConstantBuffer;

    ezDynamicArray<ezMaterialResourceDescriptor::Parameter> m_Parameters; // Builds constant buffer
    ezDynamicArray<ezMaterialResourceDescriptor::Texture2DBinding> m_Texture2DBindings;
    ezDynamicArray<ezMaterialResourceDescriptor::TextureCubeBinding> m_TextureCubeBindings;
  };

public:
  /// \brief Called by ezMaterialResource::UpdateContent and ezMaterialResource::CreateResource to add the material to the manager.
  static void MaterialAdded(ezMaterialResource* pMaterial);
  /// \brief Called by ezMaterialResource::SetModified to inform the material manager of changes.
  static void MaterialModified(ezMaterialResourceHandle hMaterial);
  /// \brief Called by ezMaterialResource destructor to remove the material from the manager.
  /// Note that we don't call this during unload to maintain the material index during reloading of materials.
  static void MaterialRemoved(ezMaterialResource* pMaterial);
  /// \brief Returns the render data for a material resource. Can be nullptr if the material is not loaded yet or extraction + update was not executed yet.
  static const MaterialData* GetMaterialData(const ezMaterialResource* pMaterial);
  /// \brief Fired in case a material is reloaded and now occupies another index / shader.
  static ezEvent<const ezMaterialShaderChanged&, ezMutex> s_MaterialShaderChangedEvent;

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, MaterialManager);
  friend class ezMemoryUtils;

  /// \brief How MaterialShaderConstants will store the material data.
  enum class MaterialStorageMode
  {
    MultipleConstantBuffer,
    MultipleStructuredBuffer,
    SingleStructuredBuffer,
  };

  /// \brief Manages the constants for all materials of a single shader resource.
  class MaterialShaderConstants
  {
  public:
    MaterialShaderConstants(ezShaderResourceHandle hShader, ezMaterialManager* pParent);
    ~MaterialShaderConstants();

    ezMaterialResource::ezMaterialId AddMaterial(ezMaterialResourceHandle hMaterial);
    void RemoveMaterial(ezMaterialResource::ezMaterialId id);
    bool IsEmpty() const;

    void MarkDirty(ezMaterialResource::ezMaterialId id);
    bool RequiresUpdates() const;
    void UpdateConstantBuffers();

    void DestroyGpuResources();

  private:
    void OnResourceEvent(const ezResourceEvent& e);
    void OnShaderChanged(ezShaderResource* pShader);
    bool UpdateMaterialLayout();
    void UpdateMaterial(ezMaterialResource::ezMaterialId id, ezMaterialResourceHandle hMaterial, ezGALCommandEncoder* pEncoder);

  private:
    // Material data
    mutable ezMutex m_MaterialsMutex;
    ezIdTable<ezMaterialResource::ezMaterialId, ezMaterialResourceHandle> m_Materials;

    ezDynamicArray<ezUInt8> m_MaterialsData;
    // #TODO_MATERIAL Right now, there are individual structured buffers for each material until we have refactored the high level renderer to actually have a place to store the material index.
    ezGALBufferHandle m_hStructuredBuffer;
    ezDynamicArray<ezGALBufferHandle> m_MaterialBuffers;

    // Shader data
    bool m_bShaderDirty = true;
    bool m_bShaderHasNoConstants = false;
    ezSet<ezMaterialResource::ezMaterialId> m_DirtyMaterials;
    ezSharedPtr<ezShaderConstantBufferLayout> m_pLayout;
    ezHashTable<ezHashedString, ezUInt32> m_ParameterNameToLayoutIndex;

    // Static data
    MaterialStorageMode m_Mode = MaterialStorageMode::MultipleConstantBuffer;
    const ezShaderResourceHandle m_hShader;
    ezMaterialManager* m_pParent = nullptr;
    ezEvent<const ezResourceEvent&, ezMutex>::Unsubscriber m_ShaderResourceEventSubscriber;
  };

  struct ExtractedMaterial
  {
    // Not using the frame allocator on the member arrays as we std::move these.
    ezMaterialResourceHandle m_hMaterial;
    ezShaderResourceHandle m_hShader;
    ezMaterialResource::ezMaterialId m_MaterialId;
    ezBitflags<ezMaterialResource::DirtyFlags> m_DirtyFlags;
    ezDynamicArray<ezMaterialResourceDescriptor::Parameter> m_Parameters;
    ezDynamicArray<ezMaterialResourceDescriptor::Texture2DBinding> m_Texture2DBindings;
    ezDynamicArray<ezMaterialResourceDescriptor::TextureCubeBinding> m_TextureCubeBindings;
    ezDynamicArray<ezPermutationVar> m_PermutationVars;
  };

  struct PendingChanges
  {
    PendingChanges();

    ezDynamicArray<const void*> m_RemovedMaterials;
    ezDynamicArray<ExtractedMaterial> m_AddedOrModifiedMaterials;
  };

private:
  ezMaterialManager();
  ~ezMaterialManager();

  void OnExtractionEvent(const ezRenderWorldExtractionEvent& e);
  void OnRenderEvent(const ezGALDeviceEvent& e);

  void ExtractMaterialUpdates();
  void RegisterMaterial(ezMaterialResource* pMaterial);
  void UnregisterMaterial(ezMaterialResource* pMaterial);
  static void ExtractMaterial(ezMaterialResource* pMaterial, ExtractedMaterial& extractedMaterial);
  void ApplyMaterialChanges();
  void Cleanup();

  MaterialShaderConstants& GetShaderConstants(ezShaderResourceHandle hShader);

private:
  // Extract these materials during extraction phase.
  ezMutex m_ExtractionMutex;
  ezHashSet<ezMaterialResourceHandle> m_ModifiedMaterials;
  ezDynamicArray<const void*> m_RemovedMaterials;

  // Extraction result created by frame allocator
  ezUniquePtr<PendingChanges> m_pPendingChanges;

  // Used during material creation, deletion and updates.
  ezMutex m_MaterialShaderMutex;
  ezMap<ezShaderResourceHandle, ezUniquePtr<MaterialShaderConstants>> m_MaterialShaders;

  // Used by render thread only to map from ezMaterialResource to the MaterialData. No need for locks as only accessed by the render thread.
  // This container will hold dead pointers after material deletion until the next extraction phase + begin render loop.
  // This is better than locking this container on every material draw call.
  ezMap<const void*, MaterialData> m_Materials;
};
