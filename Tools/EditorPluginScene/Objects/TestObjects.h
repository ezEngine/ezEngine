#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <Foundation/Math/Transform.h>

class ezSceneObjectEditorProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneObjectEditorProperties);

public:
  ezSceneObjectEditorProperties();

  void SetName(const char* szName) { m_sName.Assign(szName); }
  const char* GetName() const { return m_sName.GetString().GetData(); }

  ezVec3 m_vPivot;

protected:
  ezHashedString m_sName;
};

