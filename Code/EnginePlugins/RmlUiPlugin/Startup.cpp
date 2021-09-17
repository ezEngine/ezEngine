#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

static ezRmlUiResourceLoader s_RmlUiResourceLoader;

EZ_BEGIN_PLUGIN(ezRmlUiPlugin)
EZ_END_PLUGIN;

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
    ezResourceManager::SetResourceTypeLoader<ezRmlUiResource>(&s_RmlUiResourceLoader);

    ezResourceManager::RegisterResourceForAssetType("RmlUi", ezGetStaticRTTI<ezRmlUiResource>());

    {
      ezRmlUiResourceDescriptor desc;
      ezRmlUiResourceHandle hResource = ezResourceManager::CreateResource<ezRmlUiResource>("RmlUiMissing", std::move(desc), "Fallback for missing rml ui resource");
      ezResourceManager::SetResourceTypeMissingFallback<ezRmlUiResource>(hResource);
    }

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

    ezResourceManager::SetResourceTypeLoader<ezRmlUiResource>(nullptr);

    ezRmlUiResource::CleanupDynamicPluginReferences();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on
