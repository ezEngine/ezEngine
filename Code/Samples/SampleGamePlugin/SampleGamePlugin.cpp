#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <SampleGamePlugin/SampleGamePluginDLL.h>

// BEGIN-DOCS-CODE-SNIPPET: plugin-setup
static void OnPluginLoaded(bool)
{
  // you could do something here, though this is rare
}

static void OnPluginUnloaded(bool)
{
  // you could do something here, though this is rare
}

ezPlugin s_Plugin(false, &OnPluginLoaded, &OnPluginUnloaded);
// END-DOCS-CODE-SNIPPET
