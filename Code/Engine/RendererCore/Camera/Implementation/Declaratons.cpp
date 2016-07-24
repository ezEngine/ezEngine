#include <RendererCore/PCH.h>
#include <RendererCore/Camera/Declarations.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezCameraComponentUsageHint, 1)
  EZ_ENUM_CONSTANT(ezCameraComponentUsageHint::None),
  EZ_ENUM_CONSTANT(ezCameraComponentUsageHint::MainView),
  EZ_ENUM_CONSTANT(ezCameraComponentUsageHint::EditorView),
  EZ_ENUM_CONSTANT(ezCameraComponentUsageHint::Thumbnail),
EZ_END_STATIC_REFLECTED_ENUM();


