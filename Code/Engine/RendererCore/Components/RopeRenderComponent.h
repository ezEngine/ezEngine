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

/// \brief Used to render a rope or cable.
///
/// This is needed to visualize the ezFakeRopeComponent or ezJoltRopeComponent.
/// The component handles the message ezMsgRopePoseUpdated to generate an animated mesh and apply the pose.
/// The component has to be attached to the same object as the rope simulation component.
class EZ_RENDERERCORE_DLL ezRopeRenderComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRopeRenderComponent, ezRenderComponent, ezRopeRenderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

protected:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible, ezMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const; // [ msg handler ]

  //////////////////////////////////////////////////////////////////////////
  // ezRopeRenderComponent

public:
  ezRopeRenderComponent();
  ~ezRopeRenderComponent();

  ezColor m_Color = ezColor::White;                                                        // [ property ]


  void SetMaterial(const ezMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; } // [ property ]
  const ezMaterialResourceHandle& GetMaterial() const { return m_hMaterial; }              // [ property ]

  /// \brief Changes how thick the rope visualization is. This is independent of the simulated rope thickness.
  void SetThickness(float fThickness);                // [ property ]
  float GetThickness() const { return m_fThickness; } // [ property ]

  /// \brief Sets how round the rope shall be.
  void SetDetail(ezUInt32 uiDetail);                // [ property ]
  ezUInt32 GetDetail() const { return m_uiDetail; } // [ property ]

  /// \brief If enabled, the rendered mesh will be slightly more detailed along the rope.
  void SetSubdivide(bool bSubdivide);                // [ property ]
  bool GetSubdivide() const { return m_bSubdivide; } // [ property ]

  /// \brief How often to repeat the U texture coordinate along the rope's length.
  void SetUScale(float fUScale);                            // [ property ]
  float GetUScale() const { return m_fUScale; }             // [ property ]

  void OnMsgSetColor(ezMsgSetColor& ref_msg);               // [ msg handler ]
  void OnMsgSetMeshMaterial(ezMsgSetMeshMaterial& ref_msg); // [ msg handler ]

private:
  void OnRopePoseUpdated(ezMsgRopePoseUpdated& msg);        // [ msg handler ]
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
