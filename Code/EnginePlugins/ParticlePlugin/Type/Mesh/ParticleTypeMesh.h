#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using ezMeshResourceHandle = ezTypedResourceHandle<class ezMeshResource>;
using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;

/// Factory for creating mesh particle types.
class EZ_PARTICLEPLUGIN_DLL ezParticleTypeMeshFactory final : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeMeshFactory, ezParticleTypeFactory);

public:
  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  ezString m_sMesh;
  ezString m_sMaterial;
  float m_fScale = 1.0f;
  ezString m_sTintColorParameter;
};

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
  ezParticleTypeMesh();
  ~ezParticleTypeMesh();

  virtual void CreateRequiredStreams() override;

  ezMeshResourceHandle m_hMesh;
  mutable ezMaterialResourceHandle m_hMaterial;
  float m_fScale = 1.0f;
  ezTempHashedString m_sTintColorParameter;

  virtual void ExtractTypeRenderData(ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const override;

protected:
  friend class ezParticleTypeMeshFactory;

  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override {}

  /// Queries and caches mesh and material information from resources.
  bool QueryMeshAndMaterialInfo() const;

  void RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule) override;

  ezRenderDataManager* m_pRenderDataManager = nullptr;

  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamSize = nullptr;
  ezProcessingStream* m_pStreamColor = nullptr;
  ezProcessingStream* m_pStreamRotationSpeed = nullptr;
  ezProcessingStream* m_pStreamRotationOffset = nullptr;
  ezProcessingStream* m_pStreamAxis = nullptr;
  ezProcessingStream* m_pStreamVariation = nullptr;

  mutable bool m_bRenderDataCached = false;
  mutable ezRenderData::Category m_RenderCategory;
  mutable ezInstanceDataOffset m_InstanceDataOffset;
  mutable ezUInt8 m_uiNumSubMeshes = 0;
  mutable ezDynamicArray<ezMaterialResourceHandle> m_CachedSubMeshMaterials;
  bool m_bMaterialOverride = false;
};
