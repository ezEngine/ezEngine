#include <PCH.h>
#include <SampleApp_RTS/Components/SteeringBehaviorComponent.h>
#include <SampleApp_RTS/Level.h>

EZ_IMPLEMENT_COMPONENT_TYPE(SteeringBehaviorComponent, SteeringBehaviorComponentManager);

ezVec3 SteeringBehaviorComponent::g_vSteeringDirections[g_iSteeringDirections];

SteeringBehaviorComponent::SteeringBehaviorComponent()
{
  for (ezInt32 i = 0; i < g_iSteeringDirections; ++i)
  {
    m_fDirectionDesire[i] = 0.0f;
    m_fDirectionWhisker[i] = 5.0f;

    ezAngle a = ezAngle::Degree(360.0f / g_iSteeringDirections * i);

    ezMat3 mRot;
    mRot.SetRotationMatrixY(a);

    g_vSteeringDirections[i] = mRot * ezVec3(1, 0, 0);
  }
}

