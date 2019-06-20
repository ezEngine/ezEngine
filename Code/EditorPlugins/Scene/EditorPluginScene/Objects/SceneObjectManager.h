#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <SharedPluginScene/Common/Messages.h>

class ezDocument;

class ezSceneDocumentSettings : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocumentSettings, ezReflectedClass);

  ezDynamicArray<ezExposedSceneProperty> m_ExposedProperties;
};

class ezSceneDocumentRoot : public ezDocumentRoot
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocumentRoot, ezDocumentRoot);

  ezSceneDocumentSettings* m_pSettings;
};

class ezSceneObjectManager : public ezDocumentObjectManager
{
public:
  ezSceneObjectManager();
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override;

private:
  virtual ezStatus InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, const char* szParentProperty, const ezVariant& index) const override;
  virtual ezStatus InternalCanSelect(const ezDocumentObject* pObject) const override;
  virtual ezStatus InternalCanMove(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const char* szParentProperty, const ezVariant& index) const override;

};


