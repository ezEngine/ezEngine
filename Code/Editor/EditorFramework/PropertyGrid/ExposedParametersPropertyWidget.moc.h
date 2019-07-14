#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Object/ObjectProxyAccessor.h>

class QToolButton;
class QAction;

class EZ_EDITORFRAMEWORK_DLL ezExposedParameterCommandAccessor : public ezObjectProxyAccessor
{
public:
  ezExposedParameterCommandAccessor(
    ezObjectAccessorBase* pSource, const ezAbstractProperty* pParameterProp, const ezAbstractProperty* m_pParameterSourceProp);

  virtual ezStatus GetValue(
    const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index = ezVariant()) override;
  virtual ezStatus SetValue(
    const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;
  virtual ezStatus RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezInt32& out_iCount) override;
  virtual ezStatus GetKeys(
    const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezHybridArray<ezVariant, 16>& out_keys) override;
  virtual ezStatus GetValues(
    const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezHybridArray<ezVariant, 16>& out_values) override;

public:
  const ezExposedParameters* GetExposedParams(const ezDocumentObject* pObject);
  const ezExposedParameter* GetExposedParam(const ezDocumentObject* pObject, const char* szParamName);
  const ezRTTI* GetExposedParamsType(const ezDocumentObject* pObject);
  const ezRTTI* GetCommonExposedParamsType(const ezHybridArray<ezPropertySelection, 8>& items);
  bool IsExposedProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp);

public:
  const ezAbstractProperty* m_pParameterProp = nullptr;
  const ezAbstractProperty* m_pParameterSourceProp = nullptr;
};

class EZ_EDITORFRAMEWORK_DLL ezQtExposedParameterPropertyWidget : public ezQtVariantPropertyWidget
{
  Q_OBJECT;

protected:
  virtual void InternalSetValue(const ezVariant& value);
};

class EZ_EDITORFRAMEWORK_DLL ezQtExposedParametersPropertyWidget : public ezQtPropertyStandardTypeContainerWidget
{
  Q_OBJECT

public:
  ezQtExposedParametersPropertyWidget();
  virtual ~ezQtExposedParametersPropertyWidget();
  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;

protected:
  virtual void OnInit() override;
  virtual ezQtPropertyWidget* CreateWidget(ezUInt32 index) override;
  virtual void UpdateElement(ezUInt32 index) override;

private:
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);
  void FlushQueuedChanges();
  bool RemoveUnusedKeys(bool bTestOnly);
  bool FixKeyTypes(bool bTestOnly);
  void UpdateActionState();

private:
  ezUniquePtr<ezExposedParameterCommandAccessor> m_Proxy;
  ezObjectAccessorBase* m_pSourceObjectAccessor = nullptr;
  ezString m_sExposedParamProperty;
  mutable ezDynamicArray<ezExposedParameter> m_Parameters;
  bool m_bNeedsUpdate = false;
  bool m_bNeedsMetaDataUpdate = false;

  QToolButton* m_pFixMeButton = nullptr;
  QAction* m_pRemoveUnusedAction = nullptr;
  QAction* m_pFixTypesAction = nullptr;
};
