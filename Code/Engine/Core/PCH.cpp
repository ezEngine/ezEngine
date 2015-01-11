#include <Core/PCH.h>
#include <Foundation/PCH.h>

EZ_STATICLINK_LIBRARY(Core)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(Core_Application_Implementation_Application);
  EZ_STATICLINK_REFERENCE(Core_Application_Implementation_MainLoop);
  EZ_STATICLINK_REFERENCE(Core_Input_DeviceTypes_DeviceTypes);
  EZ_STATICLINK_REFERENCE(Core_Input_Implementation_Action);
  EZ_STATICLINK_REFERENCE(Core_Input_Implementation_InputDevice);
  EZ_STATICLINK_REFERENCE(Core_Input_Implementation_InputManager);
  EZ_STATICLINK_REFERENCE(Core_Input_Implementation_ScancodeTable);
  EZ_STATICLINK_REFERENCE(Core_Input_Implementation_Startup);
  EZ_STATICLINK_REFERENCE(Core_Input_Implementation_VirtualThumbStick);
  EZ_STATICLINK_REFERENCE(Core_ResourceManager_Implementation_Resource);
  EZ_STATICLINK_REFERENCE(Core_ResourceManager_Implementation_ResourceManager);
  EZ_STATICLINK_REFERENCE(Core_ResourceManager_Implementation_ResourceTypeLoader);
  EZ_STATICLINK_REFERENCE(Core_World_Implementation_Component);
  EZ_STATICLINK_REFERENCE(Core_World_Implementation_ComponentManager);
  EZ_STATICLINK_REFERENCE(Core_World_Implementation_GameObject);
  EZ_STATICLINK_REFERENCE(Core_World_Implementation_World);
  EZ_STATICLINK_REFERENCE(Core_World_Implementation_WorldData);
}

