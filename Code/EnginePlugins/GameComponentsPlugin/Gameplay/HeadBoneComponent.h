#pragma once

#include <GameComponentsPlugin/GameComponentsDLL.h>

typedef ezComponentManagerSimple<class ezHeadBoneComponent, ezComponentUpdateType::WhenSimulating> ezHeadBoneComponentManager;

class EZ_GAMECOMPONENTS_DLL ezHeadBoneComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezHeadBoneComponent, ezComponent, ezHeadBoneComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezHeadBoneComponent

public:
  ezHeadBoneComponent();
  ~ezHeadBoneComponent();


  void SetVerticalRotation(float fRadians);    // [ scriptable ]
  void ChangeVerticalRotation(float fRadians); // [ scriptable ]

  ezAngle m_NewVerticalRotation;                       // [ property ]
  ezAngle m_MaxVerticalRotation = ezAngle::Degree(80); // [ property ]

protected:
  void Update();

  ezAngle m_CurVerticalRotation;
};
