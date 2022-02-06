#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <memory>

struct ezMsgExtractRenderData;
struct ezMsgSetColor;
struct ezMsgSetMeshMaterial;
struct ezMsgRopePoseUpdated;
class ezShaderTransform;

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
  virtual void OnActivated() override;
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

  void SetThickness(float fThickness);                // [ property ]
  float GetThickness() const { return m_fThickness; } // [ property ]

  void SetDetail(ezUInt32 uiDetail);                // [ property ]
  ezUInt32 GetDetail() const { return m_uiDetail; } // [ property ]

  void SetSubdivide(bool bSubdivide);                // [ property ]
  bool GetSubdivide() const { return m_bSubdivide; } // [ property ]

  void SetUScale(float fUScale);                // [ property ]
  float GetUScale() const { return m_fUScale; } // [ property ]

  void OnMsgSetColor(ezMsgSetColor& msg);               // [ msg handler ]
  void OnMsgSetMeshMaterial(ezMsgSetMeshMaterial& msg); // [ msg handler ]

private:
  void OnRopePoseUpdated(ezMsgRopePoseUpdated& msg); // [ msg handler ]

  void GenerateRenderMesh(ezUInt32 uiNumRopePieces);

  void UpdateSkinningTransformBuffer(ezArrayPtr<const ezTransform> skinningTransforms);

  ezBoundingBoxSphere m_LocalBounds;

  ezSkinningState m_SkinningState;

  ezMeshResourceHandle m_hMesh;
  ezMaterialResourceHandle m_hMaterial;

  float m_fThickness = 0.05f;
  ezUInt32 m_uiDetail = 6;
  bool m_bSubdivide = false;

  float m_fUScale = 1.0f;
};
