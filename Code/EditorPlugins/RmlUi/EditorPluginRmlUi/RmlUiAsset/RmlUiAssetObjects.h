#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class ezRmlUiAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiAssetProperties, ezReflectedClass);

public:
  ezRmlUiAssetProperties();
  ~ezRmlUiAssetProperties();

  ezString m_sRmlFile;
};
