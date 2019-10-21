#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class ezTypeScriptAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptAssetProperties, ezReflectedClass);

public:
  ezTypeScriptAssetProperties();
  ~ezTypeScriptAssetProperties();


};
