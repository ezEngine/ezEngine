#include <CorePCH.h>

#include <Core/Interfaces/WindWorldModule.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezWindWorldModuleInterface, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezWindStrength, 1)
  EZ_ENUM_CONSTANTS(ezWindStrength::Calm, ezWindStrength::LightBreeze, ezWindStrength::GentleBreeze, ezWindStrength::ModerateBreeze, ezWindStrength::StrongBreeze, ezWindStrength::Storm)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

float ezWindStrength::GetInMetersPerSecond(Enum strength)
{
  // inspired by the Beaufort scale
  // https://en.wikipedia.org/wiki/Beaufort_scale

  switch (strength)
  {
    case Calm:
      return 0.5f;

    case LightBreeze:
      return 2.0f;

    case GentleBreeze:
      return 5.0f;

    case ModerateBreeze:
      return 9.0f;

    case StrongBreeze:
      return 14.0f;

    case Storm:
      return 20.0f;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

ezWindWorldModuleInterface::ezWindWorldModuleInterface(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
}

EZ_STATICLINK_FILE(Core, Core_Interfaces_WindWorldModule);
