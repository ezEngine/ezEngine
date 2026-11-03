#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using ezMeshResourceHandle = ezTypedResourceHandle<class ezMeshResource>;
using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;

/// Renders particles as instanced 3D meshes.
///
/// Each particle renders a full 3D mesh at its position, oriented according to
/// its rotation axis. Materials can be overridden globally or use the mesh's
/// default materials. Uses instanced rendering for performance when many particles
/// share the same mesh.
class EZ_PARTICLEPLUGIN_DLL ezParticleTypeMesh final : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeMesh, ezParticleType);

public:

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void OnDeinitialize(const ezParticleModuleContext& ctx) const override;
  virtual void InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;
  virtual void ExtractTypeRenderData(const ezParticleModuleContext& ctx, ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const override;
  virtual void RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule) const override;

  ezHashedString m_sMesh;
  ezHashedString m_sMaterial;
  float m_fScale = 1.0f;
  ezHashedString m_sTintColorParameter;

  ezMeshResourceHandle m_hMesh;
  ezMaterialResourceHandle m_hMaterial;

private:
  struct InstanceData
  {
    ezRenderDataManager* m_pRenderDataManager = nullptr;

    ezProcessingStream* m_pStreamPosition = nullptr;
    ezProcessingStream* m_pStreamSize = nullptr;
    ezProcessingStream* m_pStreamColor = nullptr;
    ezProcessingStream* m_pStreamRotationSpeed = nullptr;
    ezProcessingStream* m_pStreamRotationOffset = nullptr;
    ezProcessingStream* m_pStreamAxis = nullptr;
    ezProcessingStream* m_pStreamVariation = nullptr;

    ezMaterialResourceHandle m_hMaterial; ///< The material of the module, or the first material of the mesh, if there is none.
    bool m_bRenderDataCached = false;
    ezRenderData::Category m_RenderCategory;
    ezInstanceDataOffset m_InstanceDataOffset;
    ezUInt8 m_uiNumSubMeshes = 0;
    ezSmallArray<ezMaterialResourceHandle, 1> m_CachedSubMeshMaterials;
  };

  /// Queries and caches mesh and material information from resources.
  bool QueryMeshAndMaterialInfo(InstanceData& data) const;
};
