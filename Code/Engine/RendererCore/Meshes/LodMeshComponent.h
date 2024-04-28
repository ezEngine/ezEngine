#pragma once

#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

using ezLodMeshComponentManager = ezComponentManager<class ezLodMeshComponent, ezBlockStorageType::Compact>;

struct ezLodMeshLod
{
  const char* GetMeshFile() const;
  void SetMeshFile(const char* szFile);

  ezMeshResourceHandle m_hMesh;
  float m_fThreshold;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezLodMeshLod);

/// \brief Renders one of several level-of-detail meshes depending on the distance to the camera.
///
/// This component is very similar to the ezLodComponent, please read it's description for details.
/// The difference is, that this component doesn't switch child object on and off, but rather only selects between different render-meshes.
/// As such there is less performance impact for switching between meshes and also the memory overhead for storing LOD information is smaller.
/// If it is only desired to switch between meshes, it is also more convenient to work with just a single component.
///
/// The component does not allow to place the LOD meshes differently, they all need to have the same origin.
/// Compared with the regular ezMeshComponent there is also no way to override the used materials, since each LOD mesh may use different materials.
class EZ_RENDERERCORE_DLL ezLodMeshComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezLodMeshComponent, ezRenderComponent, ezLodMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezLodMeshComponent

public:
  ezLodMeshComponent();
  ~ezLodMeshComponent();

  /// \brief An additional tint color passed to the renderer to modify the mesh.
  void SetColor(const ezColor& color); // [ property ]
  const ezColor& GetColor() const;     // [ property ]

  /// \brief The sorting depth offset allows to tweak the order in which this mesh is rendered relative to other meshes.
  ///
  /// This is mainly useful for transparent objects to render them before or after other meshes.
  void SetSortingDepthOffset(float fOffset); // [ property ]
  float GetSortingDepthOffset() const;       // [ property ]

  /// \brief Enables text output to show the current coverage value and selected LOD.
  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  /// \brief Disabling the LOD range overlap functionality can make it easier to determine the desired coverage thresholds.
  void SetOverlapRanges(bool bOverlap);                 // [ property ]
  bool GetOverlapRanges() const;                        // [ property ]

  void OnMsgSetColor(ezMsgSetColor& ref_msg);           // [ msg handler ]
  void OnMsgSetCustomData(ezMsgSetCustomData& ref_msg); // [ msg handler ]

protected:
  virtual ezMeshRenderData* CreateRenderData() const;

  void UpdateSelectedLod(const ezView& view) const;
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  mutable ezInt32 m_iCurLod = 0;
  ezDynamicArray<ezLodMeshLod> m_Meshes;
  ezColor m_Color = ezColor::White;
  ezVec4 m_vCustomData = ezVec4::MakeZero();
  float m_fSortingDepthOffset = 0.0f;
  ezVec3 m_vBoundsOffset = ezVec3::MakeZero();
  float m_fBoundsRadius = 1.0f;
};
