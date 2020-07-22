#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>


class ezLUTAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLUTAssetProperties, ezReflectedClass);

public:
  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  const char* GetInputFile() const { return m_Input; }
  void SetInputFile(const char* szFile) { m_Input = szFile; }

  ezString GetAbsoluteInputFilePath() const;

private:
  ezString m_Input;
};
