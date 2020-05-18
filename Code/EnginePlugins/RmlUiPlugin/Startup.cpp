#include <RmlUiPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RmlUi, RmlUiPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {    
  }

  ON_CORESYSTEMS_SHUTDOWN
  {    
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    if (ezRmlUi::GetSingleton() == nullptr)
    {
      EZ_DEFAULT_NEW(ezRmlUi);
    }
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (ezRmlUi* pRmlUi = ezRmlUi::GetSingleton())
    {
      EZ_DEFAULT_DELETE(pRmlUi);
    }
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezPlugin g_Plugin(false);
