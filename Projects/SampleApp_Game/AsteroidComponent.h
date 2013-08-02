#pragma once

#include "Main.h"
#include <Core/World/World.h>

class AsteroidComponent;
typedef ezComponentManagerSimple<AsteroidComponent> AsteroidComponentManager;

class AsteroidComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(AsteroidComponent, AsteroidComponentManager);

public:
  AsteroidComponent();

  void Update();

  float m_fRotationSpeed;
  ezInt32 m_iShapeRandomSeed;
  float m_fRadius;
};


