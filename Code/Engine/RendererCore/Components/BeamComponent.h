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

/// \brief Renders a thick line from its own location to the position of another game object.
///
/// This is meant for simple effects, like laser beams. The geometry is very low resolution and won't look good close up.
/// When possible, use a highly emissive material without any pattern, where the bloom will hide the simple geometry.
///
/// For doing dynamic laser beams, you can combine it with the ezRaycastComponent, which will move the target component.
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

  /// \brief Sets the GUID of the target object to which to draw the beam.
  void SetTargetObject(const char* szReference); // [ property ]

  /// \brief How wide to make the beam geometry
  void SetWidth(float fWidth); // [ property ]
  float GetWidth() const;      // [ property ]

  /// \brief How many world units the texture coordinates should take up, for using a repeatable texture for the beam.
  void SetUVUnitsPerWorldUnit(float fUVUnitsPerWorldUnit); // [ property ]
  float GetUVUnitsPerWorldUnit() const;                    // [ property ]

  ezMaterialResourceHandle GetMaterial() const;

  /// \brief The object to which to draw the beam.
  ezGameObjectHandle m_hTargetObject; // [ property ]

  /// \brief Optional color to tint the beam.
  ezColor m_Color = ezColor::White; // [ property ]

protected:
  void Update();

  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  float m_fWidth = 0.1f;               // [ property ]
  float m_fUVUnitsPerWorldUnit = 1.0f; // [ property ]

  /// \brief Which material asset to use for rendering the beam geometry.
  ezMaterialResourceHandle m_hMaterial; // [ property ]

  const float m_fDistanceUpdateEpsilon = 0.02f;

  ezMeshResourceHandle m_hMesh;

  ezVec3 m_vLastOwnerPosition = ezVec3::MakeZero();
  ezVec3 m_vLastTargetPosition = ezVec3::MakeZero();

  void CreateMeshes();
  void BuildMeshResourceFromGeometry(ezGeometry& Geometry, ezMeshResourceDescriptor& MeshDesc) const;
  void ReinitMeshes();
  void Cleanup();

  const char* DummyGetter() const { return nullptr; }
};
