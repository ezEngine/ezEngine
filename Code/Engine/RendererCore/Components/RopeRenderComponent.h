#pragma once

#include <RendererCore/Components/RenderComponent.h>

struct ezMsgExtractRenderData;
struct ezMsgSetColor;
struct ezMsgSetMeshMaterial;
struct ezMsgRopePoseUpdated;

using ezRopeRenderComponentManager = ezComponentManager<class ezRopeRenderComponent, ezBlockStorageType::Compact>;

class EZ_RENDERERCORE_DLL ezRopeRenderComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRopeRenderComponent, ezRenderComponent, ezRopeRenderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

protected:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const; // [ msg handler ]

  //////////////////////////////////////////////////////////////////////////
  // ezRopeRenderComponent

public:
  ezRopeRenderComponent();
  ~ezRopeRenderComponent();

  ezColor m_Color = ezColor::White; // [ property ]

  void SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;      // [ property ]

  void SetMaterial(const ezMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  ezMaterialResourceHandle GetMaterial() const { return m_hMaterial; }

  void OnMsgSetColor(ezMsgSetColor& msg);               // [ msg handler ]
  void OnMsgSetMeshMaterial(ezMsgSetMeshMaterial& msg); // [ msg handler ]

private:
  void OnRopePoseUpdated(ezMsgRopePoseUpdated& msg); // [ msg handler ]

  void GenerateRenderMesh();

  void UpdateSkinningTransformBuffer(ezArrayPtr<const ezTransform> skinningTransforms);

  ezBoundingBoxSphere m_LocalBounds;

  ezGALBufferHandle m_hSkinningTransformsBuffer;
  ezArrayPtr<const ezMat4> m_SkinningMatrices;
  ezUInt64 m_uiSkinningMatricesValidFrame = 0;

  ezMeshResourceHandle m_hMesh;
  ezMaterialResourceHandle m_hMaterial;

  ezUInt32 m_uiNumRopePieces = 0;
};
