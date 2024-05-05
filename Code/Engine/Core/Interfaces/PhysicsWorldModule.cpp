#include <Core/CorePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Scripting/ScriptAttributes.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysicsWorldModuleInterface, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezPhysicsCastResult, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Position", m_vPosition),
    EZ_MEMBER_PROPERTY("Normal", m_vNormal),
    EZ_MEMBER_PROPERTY("Distance", m_fDistance),
    EZ_MEMBER_PROPERTY("ShapeObject", m_hShapeObject),
    EZ_MEMBER_PROPERTY("ActorObject", m_hActorObject),
    EZ_MEMBER_PROPERTY("ObjectFilterID", m_uiObjectFilterID),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezPhysicsShapeType, 1)
  EZ_BITFLAGS_CONSTANT(ezPhysicsShapeType::Static),
  EZ_BITFLAGS_CONSTANT(ezPhysicsShapeType::Dynamic),
  EZ_BITFLAGS_CONSTANT(ezPhysicsShapeType::Query),
  EZ_BITFLAGS_CONSTANT(ezPhysicsShapeType::Trigger),
  EZ_BITFLAGS_CONSTANT(ezPhysicsShapeType::Character),
  EZ_BITFLAGS_CONSTANT(ezPhysicsShapeType::Ragdoll),
  EZ_BITFLAGS_CONSTANT(ezPhysicsShapeType::Rope),
EZ_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptExtensionClass_Physics, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Raycast, In, "World", In, "Start", In, "Dir", In, "Distance", In, "CollisionLayer", In, "ShapeTypes", In, "IgnoreShapeId")->AddAttributes(
      new ezFunctionArgumentAttributes(4, new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
      new ezFunctionArgumentAttributes(5, new ezDefaultValueAttribute((ezInt32)(ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic).GetValue())),
      new ezFunctionArgumentAttributes(6, new ezDefaultValueAttribute(-1))),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezScriptExtensionAttribute("Physics"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezPhysicsCastResult* ezScriptExtensionClass_Physics::Raycast(const ezWorld* pWorld, const ezVec3& vStart, const ezVec3& vDir, float fDistance, ezUInt32 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes, ezUInt32 uiIgnoreObjectFilterID)
{
  EZ_IGNORE_UNUSED(pWorld);
  EZ_IGNORE_UNUSED(vStart);
  EZ_IGNORE_UNUSED(vDir);
  EZ_IGNORE_UNUSED(fDistance);
  EZ_IGNORE_UNUSED(uiCollisionLayer);
  EZ_IGNORE_UNUSED(shapeTypes);
  EZ_IGNORE_UNUSED(uiIgnoreObjectFilterID);

  EZ_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
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
