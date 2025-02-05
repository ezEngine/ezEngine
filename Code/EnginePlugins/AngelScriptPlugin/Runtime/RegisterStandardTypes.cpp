#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AngelScriptEngineSingleton.h>
#include <Core/World/Declarations.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Strings/HashedString.h>

void ezAngelScriptEngineSingleton::RegisterStandardTypes()
{
  EZ_LOG_BLOCK("AS::RegisterStandardTypes");

  AS_CHECK(m_pEngine->RegisterTypedef("ezInt8", "int8"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezInt16", "int16"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezInt32", "int32"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezInt64", "int64"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezUInt8", "uint8"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezUInt16", "uint16"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezUInt32", "uint32"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezUInt64", "uint64"));

  AS_CHECK(m_pEngine->RegisterObjectType("ezRTTI", 0, asOBJ_REF | asOBJ_NOCOUNT));

  // TODO AngelScript: ezResult ?

  RegisterPodValueType<ezVec2>();
  RegisterPodValueType<ezVec3>();
  RegisterPodValueType<ezVec4>();
  RegisterPodValueType<ezAngle>();
  RegisterPodValueType<ezQuat>();
  RegisterPodValueType<ezMat3>();
  RegisterPodValueType<ezMat4>();
  RegisterPodValueType<ezTransform>();
  RegisterPodValueType<ezTime>();
  RegisterPodValueType<ezColor>();
  RegisterPodValueType<ezColorGammaUB>();
  RegisterPodValueType<ezStringView>();
  RegisterPodValueType<ezGameObjectHandle>();
  RegisterPodValueType<ezComponentHandle>();
  RegisterPodValueType<ezTempHashedString>();
  RegisterPodValueType<ezHashedString>();

  RegisterNonPodValueType<ezString>();
  RegisterNonPodValueType<ezStringBuilder>();

  RegisterRefType<ezGameObject>();
  RegisterRefType<ezComponent>();
  RegisterRefType<ezWorld>();
  RegisterRefType<ezMessage>();
  RegisterRefType<ezClock>();

  {
    AS_CHECK(m_pEngine->RegisterObjectType("ezRandom", 0, asOBJ_REF | asOBJ_NOCOUNT));
    AddForbiddenType("ezRandom");
  }

  Register_RTTI();
  Register_Vec2();
  Register_Vec3();
  Register_Vec4();
  Register_Angle();
  Register_Quat();
  Register_Transform();
  Register_GameObject();
  Register_Time();
  Register_Mat3();
  Register_Mat4();
  Register_World();
  Register_Clock();
  Register_StringView();
  Register_String();
  Register_StringBuilder();
  Register_HashedString();
  Register_TempHashedString();
  Register_Color();
  Register_ColorGammaUB();
  Register_Random();
  Register_Math();

  // TODO AngelScript: register these standard types
  // ezBoundingBox
  // ezBoundingSphere
  // ezPlane
}












