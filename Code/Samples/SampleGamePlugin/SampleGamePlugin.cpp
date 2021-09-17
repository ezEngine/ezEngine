#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <SampleGamePlugin/SampleGamePluginDLL.h>

// clang-format off

// BEGIN-DOCS-CODE-SNIPPET: plugin-setup
EZ_BEGIN_PLUGIN(ezSampleGamePlugin)

  ON_PLUGIN_LOADED
  {
    // you could do something here, though this is rare
  }

  ON_PLUGIN_UNLOADED
  {
    // you could do something here, though this is rare
  }

EZ_END_PLUGIN;
// END-DOCS-CODE-SNIPPET

// clang-format on
