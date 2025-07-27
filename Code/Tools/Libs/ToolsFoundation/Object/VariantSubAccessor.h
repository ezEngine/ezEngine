#pragma once

#include <ToolsFoundation/Object/ObjectProxyAccessor.h>

class ezDocumentObject;

/// \brief Accessor for a sub-tree on an ezVariant property.
/// The tools foundation code uses an ezDocumentObject, one of its ezAbstractProperty and an optional ezVariant index to reference to properties. Any deeper hierarchies must be built from additional objects. This principle prevents the GUI to reference anything inside an ezVariant that stores an VariantArray or VariantDictionary as ezVariant is a pure value type and cannot store additional objects on the tool side. To work around this, this class creates a view one level deeper into an ezVariant. This is done by calling SetSubItems which for each object in the map moves the view into the sub-tree referenced by the given value of the map.
class EZ_TOOLSFOUNDATION_DLL ezVariantSubAccessor : public ezObjectProxyAccessor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVariantSubAccessor, ezObjectProxyAccessor);

public:
  /// \brief Constructor
  /// \param pSource The original accessor that is going to be proxied. By chaining this class an ezVariant can be explored deeper and deeper.
  /// \param pProp The ezVariant property that is going to be proxied. Only this property is allowed to be accessed by the accessor functions.
  ezVariantSubAccessor(ezObjectAccessorBase* pSource, const ezAbstractProperty* pProp);
  /// \brief Sets the sub-tree indices for the selected objects.
  /// \param subItemMap Object to index map. Note that as this is in the ToolsFoundation it cannot use the ezPropertySelection class.
  void SetSubItems(const ezMap<const ezDocumentObject*, ezVariant>& subItemMap);
  /// \brief Returns the property this accessor wraps.
  const ezAbstractProperty* GetRootProperty() const { return m_pProp; }
  /// \brief How many level deep the view is inside the property.
  ezInt32 GetDepth() const;
  /// Builds a path up the hierarchy of wrapped ezVariantSubAccessor objects to determine the path to the current sub-tree of the ezVariant.
  /// \param pObject The object for which the path should be computed
  /// \param out_path An array of indices that has to be followed from the root of the ezVariant to each the current sub-tree view.
  /// \return Returns EZ_FAILURE if pObject is not known.
  ezResult GetPath(const ezDocumentObject* pObject, ezDynamicArray<ezVariant>& out_path) const;

  virtual ezStatus GetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index = ezVariant()) override;
  virtual ezStatus SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;
  virtual ezStatus InsertValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;
  virtual ezStatus RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus MoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex) override;
  virtual ezStatus GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezInt32& out_iCount) override;
  virtual ezStatus GetKeys(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_keys) override;
  virtual ezStatus GetValues(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_values) override;

private:
  ezStatus GetSubValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value);
  ezStatus SetSubValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezDelegate<ezStatus(ezVariant& subValue)>& func);

private:
  const ezAbstractProperty* m_pProp = nullptr;
  ezMap<const ezDocumentObject*, ezVariant> m_SubItemMap;
};