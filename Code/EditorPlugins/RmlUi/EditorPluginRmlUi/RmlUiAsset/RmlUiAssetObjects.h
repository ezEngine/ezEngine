#pragma once

#include <RmlUiPlugin/Resources/RmlUiResource.h>

class ezRmlUiAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiAssetProperties, ezReflectedClass);

public:
  ezRmlUiAssetProperties();
  ~ezRmlUiAssetProperties();

  ezString m_sRmlFile;
  ezEnum<ezRmlUiScaleMode> m_ScaleMode;
  ezVec2U32 m_ReferenceResolution;
};
