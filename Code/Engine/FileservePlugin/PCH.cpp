#include <PCH.h>

EZ_STATICLINK_LIBRARY(FileservePlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(FileservePlugin_Client_FileserveClient);
  EZ_STATICLINK_REFERENCE(FileservePlugin_Client_FileserveDataDir);
  EZ_STATICLINK_REFERENCE(FileservePlugin_Fileserver_ClientContext);
  EZ_STATICLINK_REFERENCE(FileservePlugin_Fileserver_Fileserver);
  EZ_STATICLINK_REFERENCE(FileservePlugin_Main);
  EZ_STATICLINK_REFERENCE(FileservePlugin_Network_NetworkInterface);
  EZ_STATICLINK_REFERENCE(FileservePlugin_Network_NetworkMessage);
}

