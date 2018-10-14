#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class ezKrautTreeAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeAssetProperties, ezReflectedClass);

public:
  ezKrautTreeAssetProperties();

  ezString m_sKrautFile;
  float m_fUniformScaling;
};
