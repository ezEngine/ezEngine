#include <CorePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysicsWorldModuleInterface, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgPhysicsAddImpulse);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgPhysicsAddImpulse, 1, ezRTTIDefaultAllocator<ezMsgPhysicsAddImpulse>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    EZ_MEMBER_PROPERTY("Impulse", m_vImpulse),
    EZ_MEMBER_PROPERTY("ShapeID", m_uiShapeId),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgPhysicsAddForce);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgPhysicsAddForce, 1, ezRTTIDefaultAllocator<ezMsgPhysicsAddForce>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    EZ_MEMBER_PROPERTY("Force", m_vForce),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgPhysicsJointBroke);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgPhysicsJointBroke, 1, ezRTTIDefaultAllocator<ezMsgPhysicsJointBroke>)
//{
  //EZ_BEGIN_PROPERTIES
  //{
  //  EZ_MEMBER_PROPERTY("JointObject", m_hJointObject)
  //}
  //EZ_END_PROPERTIES;
//}
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgBuildStaticMesh);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgBuildStaticMesh, 1, ezRTTIDefaultAllocator<ezMsgBuildStaticMesh>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


EZ_STATICLINK_FILE(Core, Core_Interfaces_PhysicsWorldModule);
