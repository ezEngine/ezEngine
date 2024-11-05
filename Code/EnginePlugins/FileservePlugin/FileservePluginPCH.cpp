#include <FileservePlugin/FileservePluginPCH.h>

EZ_STATICLINK_LIBRARY(FileServePlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(FileServePlugin_Client_FileserveClient);
  EZ_STATICLINK_REFERENCE(FileServePlugin_FileservePlugin);
}
