#pragma once

#include <SharedPluginScene/Common/Messages.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezDocument;
class ezScene2Document;

class ezSceneDocumentSettingsBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocumentSettingsBase, ezReflectedClass);
};

class ezPrefabDocumentSettings : public ezSceneDocumentSettingsBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPrefabDocumentSettings, ezSceneDocumentSettingsBase);

public:
  ezDynamicArray<ezExposedSceneProperty> m_ExposedProperties;
};

class ezLayerDocumentSettings : public ezSceneDocumentSettingsBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLayerDocumentSettings, ezSceneDocumentSettingsBase);
};

class ezSceneDocumentRoot : public ezDocumentRoot
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocumentRoot, ezDocumentRoot);

public:
  ezSceneDocumentSettingsBase* m_pSettings;
};

class ezSceneObjectManager : public ezDocumentObjectManager
{
public:
  ezSceneObjectManager();
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override;

private:
  virtual ezStatus InternalCanAdd(
    const ezRTTI* pRtti, const ezDocumentObject* pParent, const char* szParentProperty, const ezVariant& index) const override;
  virtual ezStatus InternalCanSelect(const ezDocumentObject* pObject) const override;
  virtual ezStatus InternalCanMove(
    const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const char* szParentProperty, const ezVariant& index) const override;
};
