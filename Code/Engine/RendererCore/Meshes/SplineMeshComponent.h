#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Meshes/SplineMeshUtils.h>

struct ezMsgSplineChanged;

using ezSplineMeshComponentManager = ezComponentManager<class ezSplineMeshComponent, ezBlockStorageType::Compact>;

/// \brief TODO: Documentation
class EZ_RENDERERCORE_DLL ezSplineMeshComponent : public ezMeshComponentBase
{
  EZ_DECLARE_COMPONENT_TYPE(ezSplineMeshComponent, ezMeshComponentBase, ezSplineMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezSplineMeshComponent

public:
  ezSplineMeshComponent();
  ~ezSplineMeshComponent();

  void SetStartPart(const ezSplineMeshPart& part);                                                       // [ property ]
  const ezSplineMeshPart& GetStartPart() const { return m_Desc.m_StartPart; }                            // [ property ]

  void SetMiddleParts(ezArrayPtr<const ezSplineMeshPart> middleParts);                                   // [ property ]
  ezArrayPtr<const ezSplineMeshPart> GetMiddleParts() const { return m_Desc.m_MiddleParts; }             // [ property ]

  void SetEndPart(const ezSplineMeshPart& part);                                                         // [ property ]
  const ezSplineMeshPart& GetEndPart() const { return m_Desc.m_EndPart; }                                // [ property ]

  void SetDistributionMode(ezEnum<ezSplineMeshDistributionMode> mode);                                   // [ property ]
  ezEnum<ezSplineMeshDistributionMode> GetDistributionMode() const { return m_Desc.m_DistributionMode; } // [ property ]

  void SetSeed(ezInt32 iSeed);                                                                           // [ property ]
  ezInt32 GetSeed() const { return m_Desc.m_iSeed; }                                                     // [ property ]

  void SetOffsetY(float fOffsetY);                                                                       // [ property ]
  float GetOffsetY() const { return m_fOffsetY; }                                                        // [ property ]

  void SetOffsetZ(float fOffsetZ);                                                                       // [ property ]
  float GetOffsetZ() const { return m_fOffsetZ; }                                                        // [ property ]

private:
  ezUInt32 MiddleParts_GetCount() const { return m_Desc.m_MiddleParts.GetCount(); }
  const ezSplineMeshPart& MiddleParts_GetValue(ezUInt32 uiIndex) const { return m_Desc.m_MiddleParts[uiIndex]; }
  void MiddleParts_SetValue(ezUInt32 uiIndex, const ezSplineMeshPart& value);
  void MiddleParts_Insert(ezUInt32 uiIndex, const ezSplineMeshPart& value);
  void MiddleParts_Remove(ezUInt32 uiIndex);

  void OnMsgSplineChanged(ezMsgSplineChanged& ref_msg);           // [ msg handler ]
  void OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg) const; // [ msg handler ]

  ezResult GenerateSplineMesh(ezMeshResourceDescriptor& out_splineMeshDesc) const;
  void UpdateSplineMesh();

  const ezSplineComponent* GetSplineComponent() const;

  ezSplineMeshDescriptor m_Desc;

  float m_fOffsetY = 0.0f;
  float m_fOffsetZ = 0.0f;
};
