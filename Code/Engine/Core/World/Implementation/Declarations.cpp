#include <PCH.h>

#include <Core/World/Declarations.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezObjectMode, 1)
  EZ_ENUM_CONSTANTS(ezObjectMode::Automatic, ezObjectMode::ForceDynamic)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezOnComponentFinishedAction, 1)
  EZ_ENUM_CONSTANTS(ezOnComponentFinishedAction::None, ezOnComponentFinishedAction::DeleteComponent, ezOnComponentFinishedAction::DeleteGameObject)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezOnComponentFinishedAction2, 1)
  EZ_ENUM_CONSTANTS(ezOnComponentFinishedAction2::None, ezOnComponentFinishedAction2::DeleteComponent, ezOnComponentFinishedAction2::DeleteGameObject, ezOnComponentFinishedAction2::Restart)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

EZ_STATICLINK_FILE(Core, Core_World_Implementation_Declarations);

