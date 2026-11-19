#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

using ezBeamComponentManager = ezComponentManagerSimple<class ezBeamComponent, ezComponentUpdateType::Always>;

struct ezMsgExtractRenderData;
struct ezMsgSetColor;
struct ezMsgSetCustomData;

/// Renders a thick line from its own location to the position of another game object.
///
/// This is meant for simple effects, like laser beams or the trail behind a fast projectile. The geometry is a cross of two
/// double-sided quads and won't look good close up. When possible, use a highly emissive material without any pattern, where
/// the bloom will hide the simple geometry.
///
/// All beams share one unit-size mesh, which is stretched and rotated at render time. Thus moving the owner or the target costs
/// nothing. The texture coordinates are normalized: U goes from 0 at the owner to 1 at the target, V from 0 to 1 across the
/// beam. A material that needs a repeating texture can get the beam length through the custom data (see SetCustomData()).
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

  /// Sets the GUID of the target object to which to draw the beam.
  void SetTargetObject(const char* szReference); // [ property ]

  /// How wide to make the beam geometry. Scaled by the owner's scale.
  void SetWidth(float fWidth); // [ property ]
  float GetWidth() const;      // [ property ]

  /// With a target object: if not zero, the beam is at most this long (the sign is ignored). It starts at the owner and points
  /// toward the target. Zero means no limit.
  ///
  /// Without a target object: the beam goes along the owner's X axis and is exactly this long. A negative value makes it go
  /// along -X, for example for a trail behind a projectile that flies along +X. Zero means no beam.
  void SetMaxLength(float fMaxLength);                         // [ property ]
  float GetMaxLength() const;                                  // [ property ]

  void SetMaterial(const ezMaterialResourceHandle& hMaterial); // [ property ]
  const ezMaterialResourceHandle& GetMaterial() const;         // [ property ]

  /// Optional color to tint the beam. Passed to the shader as the instance color.
  void SetColor(const ezColor& color);                // [ property ]
  const ezColor& GetColor() const { return m_Color; } // [ property ]

  /// Arbitrary data that is passed to the shader as the instance custom data (GetInstanceData().CustomData).
  ///
  /// The beam doesn't use it itself. For a repeating texture, pass the beam length (in texture repetitions) and multiply U by it.
  void SetCustomData(const ezVec4& vData);                      // [ property ]
  const ezVec4& GetCustomData() const { return m_vCustomData; } // [ property ]

  /// The object to which to draw the beam.
  ezGameObjectHandle m_hTargetObject; // [ property ]

protected:
  void Update();

  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void OnMsgSetColor(ezMsgSetColor& ref_msg);           // [ msg handler ]
  void OnMsgSetCustomData(ezMsgSetCustomData& ref_msg); // [ msg handler ]

  /// Computes the beam end point in world space, with the max length applied. Returns false, if there is nothing to draw.
  bool ComputeEndPosition(ezVec3& out_vEndPosition) const;

  float m_fWidth = 0.1f;     // [ property ]
  float m_fMaxLength = 0.0f; // [ property ]

  /// Which material asset to use for rendering the beam geometry.
  ezMaterialResourceHandle m_hMaterial; // [ property ]

  ezColor m_Color = ezColor::White;     // [ property ]
  ezVec4 m_vCustomData = ezVec4(0, 1, 0, 1); // [ property ]

  /// The end point in the owner's local space, from the last bounds update.
  ezVec3 m_vLastLocalEndPosition = ezVec3::MakeZero();

  /// The unit beam mesh, shared by all beam components.
  ezMeshResourceHandle m_hMesh;

  const char* DummyGetter() const { return nullptr; }

  mutable ezInstanceDataOffset m_InstanceDataOffset;
};
