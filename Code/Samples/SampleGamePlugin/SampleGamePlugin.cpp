#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <SampleGamePlugin/SampleGamePluginDLL.h>

// BEGIN-DOCS-CODE-SNIPPET: plugin-setup
EZ_PLUGIN_ON_LOADED()
{
  // you could do something here, though this is rare
}

EZ_PLUGIN_ON_UNLOADED()
{
  // you could do something here, though this is rare
}
// END-DOCS-CODE-SNIPPET
