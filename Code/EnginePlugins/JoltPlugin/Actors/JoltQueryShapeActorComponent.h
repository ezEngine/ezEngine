#pragma once

#include <JoltPlugin/Actors/JoltActorComponent.h>

//////////////////////////////////////////////////////////////////////////

class EZ_JOLTPLUGIN_DLL ezJoltQueryShapeActorComponentManager : public ezComponentManager<class ezJoltQueryShapeActorComponent, ezBlockStorageType::FreeList>
{
public:
  ezJoltQueryShapeActorComponentManager(ezWorld* pWorld);
  ~ezJoltQueryShapeActorComponentManager();

private:
  friend class ezJoltWorldModule;
  friend class ezJoltQueryShapeActorComponent;

  void UpdateMovingQueryShapes();

  ezDynamicArray<ezJoltQueryShapeActorComponent*> m_MovingQueryShapes;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A physics actor that can be moved procedurally (like a kinematic actor) but that doesn't affect rigid bodies.
///
/// It passes right through dynamic actors. However, you can detect it via raycasts or shape casts.
/// This is useful to represent detail shapes (like the collision shapes of animated meshes) that should be pickable,
/// but that shouldn't interact with the world otherwise.
/// They are more lightweight at runtime than full kinematic dynamic actors.
class EZ_JOLTPLUGIN_DLL ezJoltQueryShapeActorComponent : public ezJoltActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltQueryShapeActorComponent, ezJoltActorComponent, ezJoltQueryShapeActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltQueryShapeActorComponent
public:
  ezJoltQueryShapeActorComponent();
  ~ezJoltQueryShapeActorComponent();

  void SetSurfaceFile(ezStringView sFile); // [ property ]
  ezStringView GetSurfaceFile() const;     // [ property ]

  ezSurfaceResourceHandle m_hSurface;      // [ property ]

protected:
  const ezJoltMaterial* GetJoltMaterial() const;
};
