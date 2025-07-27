#pragma once

#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class ezStatus;

/// \brief Helper class to modify an ezVariant as if it was a container.
/// GetValue and SetValue are valid for all variant types.
/// The remaining accessor functions require an VariantArray or VariantDictionary type.
class EZ_TOOLSFOUNDATION_DLL ezVariantStorageAccessor
{
public:
  ezVariantStorageAccessor(ezStringView sProperty, ezVariant& value);
  ezVariantStorageAccessor(ezStringView sProperty, const ezVariant& value);

  ezVariant GetValue(ezVariant index = ezVariant(), ezStatus* pRes = nullptr) const;
  ezStatus SetValue(const ezVariant& value, ezVariant index = ezVariant());

  ezInt32 GetCount() const;
  ezStatus GetKeys(ezDynamicArray<ezVariant>& out_keys) const;
  ezStatus InsertValue(const ezVariant& index, const ezVariant& value);
  ezStatus RemoveValue(const ezVariant& index);
  ezStatus MoveValue(const ezVariant& oldIndex, const ezVariant& newIndex);

private:
  ezStringView m_sProperty;
  ezVariant& m_Value;
};