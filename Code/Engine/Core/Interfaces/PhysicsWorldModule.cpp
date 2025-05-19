#include <Core/CorePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysicsWorldModuleInterface, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezPhysicsShapeType, 1)
  EZ_BITFLAGS_CONSTANT(ezPhysicsShapeType::Static),
  EZ_BITFLAGS_CONSTANT(ezPhysicsShapeType::Dynamic),
  EZ_BITFLAGS_CONSTANT(ezPhysicsShapeType::Query),
  EZ_BITFLAGS_CONSTANT(ezPhysicsShapeType::Trigger),
  EZ_BITFLAGS_CONSTANT(ezPhysicsShapeType::Character),
  EZ_BITFLAGS_CONSTANT(ezPhysicsShapeType::Ragdoll),
  EZ_BITFLAGS_CONSTANT(ezPhysicsShapeType::Rope),
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgPhysicsAddImpulse);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgPhysicsAddImpulse, 1, ezRTTIDefaultAllocator<ezMsgPhysicsAddImpulse>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    EZ_MEMBER_PROPERTY("Impulse", m_vImpulse),
    EZ_MEMBER_PROPERTY("ObjectFilterID", m_uiObjectFilterID),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgPhysicsJointBroke);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgPhysicsJointBroke, 1, ezRTTIDefaultAllocator<ezMsgPhysicsJointBroke>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("JointObject", m_hJointObject)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgObjectGrabbed);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgObjectGrabbed, 1, ezRTTIDefaultAllocator<ezMsgObjectGrabbed>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("GrabbedBy", m_hGrabbedBy),
    EZ_MEMBER_PROPERTY("GotGrabbed", m_bGotGrabbed),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgReleaseObjectGrab);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgReleaseObjectGrab, 1, ezRTTIDefaultAllocator<ezMsgReleaseObjectGrab>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("GrabbedObjectToRelease", m_hGrabbedObjectToRelease),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgBuildStaticMesh);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgBuildStaticMesh, 1, ezRTTIDefaultAllocator<ezMsgBuildStaticMesh>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezExcludeFromScript()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


EZ_STATICLINK_FILE(Core, Core_Interfaces_PhysicsWorldModule);
