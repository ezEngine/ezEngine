#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Messages/EventMessage.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

using ezBeamComponentManager = ezComponentManagerSimple<class ezBeamComponent, ezComponentUpdateType::Always>;

struct ezMsgExtractRenderData;
class ezGeometry;
class ezMeshResourceDescriptor;

/// \brief A beam component
class EZ_RENDERERCORE_DLL ezBeamComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezBeamComponent, ezRenderComponent, ezBeamComponentManager);

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

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // ezBeamComponent

public:
  ezBeamComponent();
  ~ezBeamComponent();

  void SetTargetObject(const char* szReference); // [ property ]

  void SetWidth(float fWidth); // [ property ]
  float GetWidth() const;      // [ property ]

  void SetUVUnitsPerWorldUnit(float fUVUnitsPerWorldUnit); // [ property ]
  float GetUVUnitsPerWorldUnit() const;                    // [ property ]

  void SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;      // [ property ]

  ezMaterialResourceHandle GetMaterial() const;

  ezGameObjectHandle m_hTargetObject; // [ property ]

  ezColor m_Color; // [ property ]

protected:
  void Update();

  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  float m_fWidth = 0.1f;               // [ property ]
  float m_fUVUnitsPerWorldUnit = 1.0f; // [ property ]

  ezMaterialResourceHandle m_hMaterial; // [ property ]

  const float m_fDistanceUpdateEpsilon = 0.02f;

  // State
  ezMeshResourceHandle m_hMesh;

  ezVec3 m_vLastOwnerPosition = ezVec3::MakeZero();
  ezVec3 m_vLastTargetPosition = ezVec3::MakeZero();

  void CreateMeshes();
  void BuildMeshResourceFromGeometry(ezGeometry& Geometry, ezMeshResourceDescriptor& MeshDesc) const;
  void ReinitMeshes();
  void Cleanup();

  const char* DummyGetter() const { return nullptr; }
};
