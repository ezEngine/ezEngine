#pragma once

#include <RendererCore/Lights/ReflectionProbeComponentBase.h>

class EZ_RENDERERCORE_DLL ezBoxReflectionProbeComponentManager final : public ezComponentManager<class ezBoxReflectionProbeComponent, ezBlockStorageType::Compact>
{
public:
  ezBoxReflectionProbeComponentManager(ezWorld* pWorld);
};

/// \brief Box reflection probe component.
///
/// The generated reflection cube map is projected on a box defined by this component's extents. The influence volume can be smaller than the projection which is defined by a scale and shift parameter. Each side of the influence volume has a separate falloff parameter to smoothly blend the probe into others.
class EZ_RENDERERCORE_DLL ezBoxReflectionProbeComponent : public ezReflectionProbeComponentBase
{
  EZ_DECLARE_COMPONENT_TYPE(ezBoxReflectionProbeComponent, ezReflectionProbeComponentBase, ezBoxReflectionProbeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezBoxReflectionProbeComponent

public:
  ezBoxReflectionProbeComponent();
  ~ezBoxReflectionProbeComponent();

  const ezVec3& GetExtents() const;                                       // [ property ]
  void SetExtents(const ezVec3& vExtents);                                // [ property ]

  const ezVec3& GetInfluenceScale() const;                                // [ property ]
  void SetInfluenceScale(const ezVec3& vInfluenceScale);                  // [ property ]
  const ezVec3& GetInfluenceShift() const;                                // [ property ]
  void SetInfluenceShift(const ezVec3& vInfluenceShift);                  // [ property ]

  void SetPositiveFalloff(const ezVec3& vFalloff);                        // [ property ]
  const ezVec3& GetPositiveFalloff() const { return m_vPositiveFalloff; } // [ property ]
  void SetNegativeFalloff(const ezVec3& vFalloff);                        // [ property ]
  const ezVec3& GetNegativeFalloff() const { return m_vNegativeFalloff; } // [ property ]

  void SetBoxProjection(bool bBoxProjection);                             // [ property ]
  bool GetBoxProjection() const { return m_bBoxProjection; }              // [ property ]

protected:
  //////////////////////////////////////////////////////////////////////////
  // Editor
  void OnObjectCreated(const ezAbstractObjectNode& node);

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void OnTransformChanged(ezMsgTransformChanged& msg);

protected:
  ezVec3 m_vExtents = ezVec3(5.0f);
  ezVec3 m_vInfluenceScale = ezVec3(1.0f);
  ezVec3 m_vInfluenceShift = ezVec3(0.0f);
  ezVec3 m_vPositiveFalloff = ezVec3(0.1f, 0.1f, 0.0f);
  ezVec3 m_vNegativeFalloff = ezVec3(0.1f, 0.1f, 0.0f);
  bool m_bBoxProjection = true;
};

/// \brief A special visualizer attribute for box reflection probes
class EZ_RENDERERCORE_DLL ezBoxReflectionProbeVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBoxReflectionProbeVisualizerAttribute, ezVisualizerAttribute);

public:
  ezBoxReflectionProbeVisualizerAttribute();

  ezBoxReflectionProbeVisualizerAttribute(const char* szExtentsProperty, const char* szInfluenceScaleProperty, const char* szInfluenceShiftProperty);

  const ezUntrackedString& GetExtentsProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetInfluenceScaleProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetInfluenceShiftProperty() const { return m_sProperty3; }
};
