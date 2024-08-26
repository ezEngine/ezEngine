#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/World.h>

struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;

struct EZ_GAMEENGINE_DLL ezGrabbableItemGrabPoint
{
  ezVec3 m_vLocalPosition;
  ezQuat m_qLocalRotation;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezGrabbableItemGrabPoint);

//////////////////////////////////////////////////////////////////////////

using ezGrabbableItemComponentManager = ezComponentManager<class ezGrabbableItemComponent, ezBlockStorageType::Compact>;

/// \brief Used to define 'grab points' on an object where a player can pick up and hold the item
///
/// The grabbable item component is typically added to objects with a dynamic physics actor to mark it as an item that can be
/// picked up, and to define the anchor points at which the object can be held.
/// Of course a game can utilize this information without a physical actor and physically holding objects as well.
///
/// Each grab point defines how the object would be oriented when held.
///
/// The component only holds data, it doesn't add any custom behavior. It is the responsibility of other components to use this
/// data in a sensible way.
class EZ_GAMEENGINE_DLL ezGrabbableItemComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezGrabbableItemComponent, ezComponent, ezGrabbableItemComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezGrabbableItemComponent

public:
  ezGrabbableItemComponent();
  ~ezGrabbableItemComponent();

  void SetDebugShowPoints(bool bShow);                   // [ property ]
  bool GetDebugShowPoints() const;                       // [ property ]

  ezDynamicArray<ezGrabbableItemGrabPoint> m_GrabPoints; // [ property ]

  static void DebugDrawGrabPoint(const ezWorld& world, const ezTransform& globalGrabPointTransform);

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;
};
