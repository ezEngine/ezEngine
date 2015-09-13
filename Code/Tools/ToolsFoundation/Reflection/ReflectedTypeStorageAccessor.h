#pragma once

#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageManager.h>

/// \brief An ezIReflectedTypeAccessor implementation that also stores the actual data that is defined in the passed ezRTTI.
///
/// This class is used to store data on the tool side for classes that are not known to the tool but exist outside of it
/// like engine components. As this is basically a complex value map the used type can be hot-reloaded. For this, the
/// ezRTTI just needs to be updated with its new definition in the ezPhantomRttiManager and all ezReflectedTypeStorageAccessor
/// will be automatically rearranged to match the new class layout.
class EZ_TOOLSFOUNDATION_DLL ezReflectedTypeStorageAccessor : public ezIReflectedTypeAccessor
{
  friend class ezReflectedTypeStorageManager;
public:
  ezReflectedTypeStorageAccessor(const ezRTTI* hReflectedType, ezDocumentObjectBase* pOwner); // [tested]
  ~ezReflectedTypeStorageAccessor();
 
  virtual const ezVariant GetValue(const ezPropertyPath& path, ezVariant index = ezVariant()) const override; // [tested]
  virtual bool SetValue(const ezPropertyPath& path, const ezVariant& value, ezVariant index = ezVariant()) override; // [tested]

  virtual ezInt32 GetCount(const ezPropertyPath& path) const override;
  virtual bool GetKeys(const ezPropertyPath& path, ezHybridArray<ezVariant, 16>& out_keys) const override;

  virtual bool InsertValue(const ezPropertyPath& path, ezVariant index, const ezVariant& value) override;
  virtual bool RemoveValue(const ezPropertyPath& path, ezVariant index) override;
  virtual bool MoveValue(const ezPropertyPath& path, ezVariant oldIndex, ezVariant newIndex) override;

  virtual ezVariant GetPropertyChildIndex(const ezPropertyPath& path, const ezVariant& value) const;

private:
  ezDynamicArray<ezVariant> m_Data;
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping* m_pMapping;
};

