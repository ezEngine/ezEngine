#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Types/Bitflags.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>

namespace JPH
{
  class Shape;
  class BodyCreationSettings;
} // namespace JPH

class EZ_JOLTPLUGIN_DLL ezJoltActorComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezJoltActorComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltActorComponent

public:
  ezJoltActorComponent();
  ~ezJoltActorComponent();

  void SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;      // [ property ]

  ezUInt8 m_uiCollisionLayer = 0;     // [ property ]
  ezSurfaceResourceHandle m_hSurface; // [ property ]

  ezBitflags<ezOnJoltContact> m_OnContact; // [ property ]

  const ezJoltUserData* GetUserData() const;

  /// \brief Sets the object filter ID to use. This can only be set right after creation, before the component gets activated.
  void SetInitialObjectFilterID(ezUInt32 uiObjectFilterID);

  /// \brief The object filter ID can be used to ignore collisions specifically with this one object.
  ezUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; }

protected:
  void ExtractSubShapeGeometry(const ezGameObject* pObject, ezMsgExtractGeometry& msg) const;

  const ezJoltMaterial* GetJoltMaterial() const;

  static void GatherShapes(ezDynamicArray<ezJoltSubShape>& shapes, ezGameObject* pObject, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial);
  ezResult CreateShape(JPH::BodyCreationSettings* pSettings, float fDensity);

  virtual void CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial) {}

  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
  ezUInt32 m_uiJoltBodyID = ezInvalidIndex;
  ezUInt32 m_uiObjectFilterID = ezInvalidIndex;
};
