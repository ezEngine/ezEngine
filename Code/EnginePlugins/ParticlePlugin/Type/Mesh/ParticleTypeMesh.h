#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererFoundation/RendererFoundationDLL.h>

typedef ezTypedResourceHandle<class ezMeshResource> ezMeshResourceHandle;
typedef ezTypedResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeMeshFactory final : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeMeshFactory, ezParticleTypeFactory);

public:
  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezString m_sMesh;
  ezString m_sMaterial;
  ezString m_sTintColorParameter;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeMesh final : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeMesh, ezParticleType);

public:
  ezParticleTypeMesh();
  ~ezParticleTypeMesh();

  virtual void CreateRequiredStreams() override;

  ezMeshResourceHandle m_hMesh;
  mutable ezMaterialResourceHandle m_hMaterial;
  ezTempHashedString m_sTintColorParameter;

  virtual void ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override {}

  bool QueryMeshAndMaterialInfo() const;

  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamSize = nullptr;
  ezProcessingStream* m_pStreamColor = nullptr;
  ezProcessingStream* m_pStreamRotationSpeed = nullptr;
  ezProcessingStream* m_pStreamRotationOffset = nullptr;
  ezProcessingStream* m_pStreamAxis = nullptr;

  mutable bool m_bRenderDataCached = false;
  mutable ezBoundingBoxSphere m_Bounds;
  mutable ezRenderData::Category m_RenderCategory;
};
