#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Object/ObjectProxyAccessor.h>

class QToolButton;
class QAction;
struct ezPhantomRttiManagerEvent;

/// \brief Helper accessor to pretend all exposed parameters always have a value defined.
/// The exposed parameters are stored as just a sparse map. Only the elements that are overwritten from their defaults are actually stored in the component. Thus, requesting the value of an exposed parameter that has not been overwritten results in failure. To fix this, this class will automatically return the default value of an exposed parameter. This allows the tooling code to always show every exposed parameter's value independent on whether it was overwritten or remains at the default value.
class EZ_EDITORFRAMEWORK_DLL ezExposedParameterCommandAccessor : public ezObjectProxyAccessor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExposedParameterCommandAccessor, ezObjectProxyAccessor);

public:
  ezExposedParameterCommandAccessor(ezObjectAccessorBase* pSource, const ezAbstractProperty* pParameterProp, const ezAbstractProperty* pM_pParameterSourceProp);

  virtual ezStatus GetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index = ezVariant()) override;
  virtual ezStatus SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;
  virtual ezStatus RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezInt32& out_iCount) override;
  virtual ezStatus GetKeys(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_keys) override;
  virtual ezStatus GetValues(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_values) override;

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

/// \brief Accessor to pretend the exposed parameters map property is an object of the generated phantom type.
/// This fake type accessor is created by taking the property name and redirecting to the exposed parameter map's element under that name. As long as no code path is looking at the actual type of the object this works with any property widget. An ezQtTypeWidget constructed with the exposed parameter type and this accessor will produce a normal type widget that looks like the exposed parameter type but redirects all read / writes into the exposed parameter map property. Additionally, this class ensures the value stored in the map is converted to match the property type exactly.
class EZ_EDITORFRAMEWORK_DLL ezExposedParametersAsTypeCommandAccessor : public ezObjectProxyAccessor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExposedParametersAsTypeCommandAccessor, ezObjectProxyAccessor);

public:
  ezExposedParametersAsTypeCommandAccessor(ezExposedParameterCommandAccessor* pSource);
  ezExposedParameterCommandAccessor* GetSourceAccessor() const { return static_cast<ezExposedParameterCommandAccessor*>(m_pSource); }

  virtual ezStatus GetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index = ezVariant()) override;
  virtual ezStatus SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;
  virtual ezStatus InsertValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;
  virtual ezStatus RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus MoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex) override;
  virtual ezStatus GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezInt32& out_iCount) override;
  virtual ezStatus GetKeys(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_keys) override;
  virtual ezStatus GetValues(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_values) override;

protected:
  ezStatus GetSubValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value);
  ezStatus SetSubValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezDelegate<ezStatus(ezVariant& subValue)>& func);

  /// \brief Make sure that any property retrieved via this accessor matches the expected property type to make sure no invalid data is passed into one of the property widgets generated under the type widget.
  void PatchPropertyType(ezVariant& ref_value, const ezAbstractProperty* pProp);
};

/// \brief Custom widget for properties annotated with the ezExposedParametersAttribute attribute.
/// Technically exposed parameters are stored as an ezVariantDictionary but that leaves much to be desired for usability. This class uses ezExposedParameterCommandAccessor to always show all exposed parameters in the dictionary even if none were overwritten. Additionally, ezExposedParametersAsTypeCommandAccessor is used to project the exposed parameters into a phantom type widget to make editing exposed parameters indistinguishable from editing a normal type object. A button can be used to switch between the two representations.
class EZ_EDITORFRAMEWORK_DLL ezQtExposedParametersPropertyWidget : public ezQtPropertyStandardTypeContainerWidget
{
  Q_OBJECT

public:
  ezQtExposedParametersPropertyWidget();
  virtual ~ezQtExposedParametersPropertyWidget();
  virtual void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items) override;

protected:
  virtual void OnInit() override;
  virtual void UpdateElement(ezUInt32 index) override;
  virtual void UpdatePropertyMetaState() override;
  virtual void GetRequiredElements(ezDynamicArray<ezVariant>& out_keys) const override;

private:
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);
  void PhantomTypeRegistryEventHandler(const ezPhantomRttiManagerEvent& e);
  void FlushOrQueueChanges(bool bNeedsUpdate, bool bNeedsMetaDataUpdate);
  bool RemoveUnusedKeys(bool bTestOnly);
  bool FixKeyTypes(bool bTestOnly);
  void UpdateActionState();

private:
  static bool s_bRawMode;

private:
  ezUniquePtr<ezExposedParameterCommandAccessor> m_pProxy;
  ezUniquePtr<ezExposedParametersAsTypeCommandAccessor> m_pTypeProxy;
  ezObjectAccessorBase* m_pSourceObjectAccessor = nullptr;
  ezString m_sExposedParamProperty;
  mutable ezDynamicArray<ezExposedParameter> m_Parameters;
  bool m_bNeedsUpdate = false;
  bool m_bNeedsMetaDataUpdate = false;

  ezQtTypeWidget* m_pTypeWidget = nullptr;
  QVBoxLayout* m_pTypeViewLayout = nullptr;
  QToolButton* m_pFixMeButton = nullptr;
  QToolButton* m_pToggleRawModeButton = nullptr;
  QAction* m_pRemoveUnusedAction = nullptr;
  QAction* m_pFixTypesAction = nullptr;
};
