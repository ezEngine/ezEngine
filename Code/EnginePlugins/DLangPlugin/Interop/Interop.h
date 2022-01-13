#pragma once

#include <DLangPlugin/DLangPluginDLL.h>

class ezGameObject;

using ComponentCreationFunc = ezDLangBaseComponent* (*)(ezGameObject* pOwner, ezComponent* pOwnerComponent);
using ComponentDestructionFunc = void (*)(ezDLangBaseComponent*);

namespace ezDLangInterop
{
  EZ_DLANGPLUGIN_DLL void RegisterComponentType(const char* szName, ComponentCreationFunc creator, ComponentDestructionFunc destructor);
  EZ_DLANGPLUGIN_DLL ezDLangBaseComponent* CreateComponentType(const char* szName, ezGameObject* pOwner, ezComponent* pOwnerComponent);
  EZ_DLANGPLUGIN_DLL void DestroyComponentType(ezDLangBaseComponent* pComp);
} // namespace ezDLangInterop
