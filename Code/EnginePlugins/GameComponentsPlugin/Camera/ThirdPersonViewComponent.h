#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>
#include <RendererCore/Components/CameraComponent.h>

class EZ_GAMECOMPONENTS_DLL ezThirdPersonViewComponentManager : public ezComponentManager<class ezThirdPersonViewComponent, ezBlockStorageType::Compact>
{
public:
  ezThirdPersonViewComponentManager(ezWorld* pWorld);
  ~ezThirdPersonViewComponentManager();

  virtual void Initialize() override;

private:
  void Update(const ezWorldModule::UpdateContext& context);

  friend class ezThirdPersonViewComponent;
};

/// \brief The third-person View component is used to place an object, typically a camera, relative to another object with clear line of sight.
///
/// The component will make the owner object look at the target point and place it at a certain distance.
/// When there are physical obstacles between the camera and the target, it moves the owner object closer.
class EZ_GAMECOMPONENTS_DLL ezThirdPersonViewComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezThirdPersonViewComponent, ezComponent, ezThirdPersonViewComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezThirdPersonViewComponent

public:
  ezThirdPersonViewComponent();
  ~ezThirdPersonViewComponent();

  /// \brief Changes the object that the view should focus on.
  void SetTargetObject(const char* szTargetObject);
  const char* GetTargetObject() const;

  /// \brief Makes the camera rotate up or down by the given angle within the defined boundaries.
  void RotateUp(ezAngle angle); // [ scriptable ]

protected:
  void Update();
  void OnSimulationStarted() override;

private:
  // properties
  ezString m_sTargetObject;                               // [ property ]
  ezVec3 m_vTargetOffsetHigh = ezVec3::MakeZero();        // [ property ]
  ezVec3 m_vTargetOffsetLow = ezVec3::MakeZero();         // [ property ]
  float m_fMinDistance = 0.25f;                           // [ property ]
  float m_fMaxDistance = 3.0f;                            // [ property ]
  float m_fMaxDistanceUp = 3.0f;                          // [ property ]
  float m_fMaxDistanceDown = 1.0f;                        // [ property ]
  ezAngle m_MinUpRotation = ezAngle::MakeFromDegree(-70); // [ property ]
  ezAngle m_MaxUpRotation = ezAngle::MakeFromDegree(+80); // [ property ]
  ezUInt8 m_uiCollisionLayer = 0;                         // [ property ]
  float m_fSweepWidth = 0.2f;                             // [ property ]
  float m_fZoomInSpeed = 10.0f;                           // [ property ]
  float m_fZoomOutSpeed = 1.0f;                           // [ property ]

  // runtime state
  ezAngle m_RotateUp;
  ezAngle m_CurUpRotation;
  ezGameObjectHandle m_hTargetObject;
  float m_fCurDistance = 1.0f;
};
