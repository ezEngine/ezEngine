#pragma once

#include <RendererCore/Lights/ReflectionProbeComponentBase.h>

class EZ_RENDERERCORE_DLL ezSphereReflectionProbeComponentManager final : public ezComponentManager<class ezSphereReflectionProbeComponent, ezBlockStorageType::Compact>
{
public:
  ezSphereReflectionProbeComponentManager(ezWorld* pWorld);
};

//////////////////////////////////////////////////////////////////////////
// ezSphereReflectionProbeComponent

/// \brief Sphere reflection probe component.
///
/// The generated reflection cube map is is projected to infinity. So parallax correction takes place.
class EZ_RENDERERCORE_DLL ezSphereReflectionProbeComponent : public ezReflectionProbeComponentBase
{
  EZ_DECLARE_COMPONENT_TYPE(ezSphereReflectionProbeComponent, ezReflectionProbeComponentBase, ezSphereReflectionProbeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezSphereReflectionProbeComponent

public:
  ezSphereReflectionProbeComponent();
  ~ezSphereReflectionProbeComponent();

  void SetRadius(float fRadius);                                   // [ property ]
  float GetRadius() const;                                         // [ property ]

  void SetFalloff(float fFalloff);                                 // [ property ]
  float GetFalloff() const { return m_fFalloff; }                  // [ property ]

  void SetSphereProjection(bool bSphereProjection);                // [ property ]
  bool GetSphereProjection() const { return m_bSphereProjection; } // [ property ]

protected:
  //////////////////////////////////////////////////////////////////////////
  // Editor
  void OnObjectCreated(const ezAbstractObjectNode& node);

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void OnTransformChanged(ezMsgTransformChanged& msg);
  float m_fRadius = 5.0f;
  float m_fFalloff = 0.1f;
  bool m_bSphereProjection = true;
};
