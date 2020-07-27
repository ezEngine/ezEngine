#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/World.h>

struct EZ_GAMEENGINE_DLL ezGrabbableItemGrabPoint
{
  ezVec3 m_vLocalPosition;
  ezQuat m_qLocalRotation;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezGrabbableItemGrabPoint);

//////////////////////////////////////////////////////////////////////////

using ezGrabbableItemComponentManager = ezComponentManager<class ezGrabbableItemComponent, ezBlockStorageType::Compact>;

class EZ_GAMEENGINE_DLL ezGrabbableItemComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezGrabbableItemComponent, ezComponent, ezGrabbableItemComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezGrabbableItemComponent

public:
  ezGrabbableItemComponent();
  ~ezGrabbableItemComponent();

  ezUInt32 GrabPoints_GetCount() const;                                       // [ property ]
  ezGrabbableItemGrabPoint GrabPoints_GetValue(ezUInt32 uiIndex) const;       // [ property ]
  void GrabPoints_SetValue(ezUInt32 uiIndex, ezGrabbableItemGrabPoint value); // [ property ]
  void GrabPoints_Insert(ezUInt32 uiIndex, ezGrabbableItemGrabPoint value);   // [ property ]
  void GrabPoints_Remove(ezUInt32 uiIndex);                                   // [ property ]

  ezDynamicArray<ezGrabbableItemGrabPoint> m_GrabPoints;
};
