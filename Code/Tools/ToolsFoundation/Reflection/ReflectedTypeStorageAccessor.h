#pragma once

#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageManager.h>

/// \brief An ezIReflectedTypeAccessor implementation that also stores the actual data that is defined in the passed ezReflectedType.
///
/// This class is used to store data on the tool side for classes that are not known to the tool but exist outside of it
/// like engine components. As this is basically a complex value map the used type can be hot-reloaded. For this, the
/// ezReflectedType just needs to be updated with its new definition in the ezReflectedTypeManager and all ezReflectedTypeStorageAccessor
/// will be automatically rearranged to match the new class layout.
class EZ_TOOLSFOUNDATION_DLL ezReflectedTypeStorageAccessor : public ezIReflectedTypeAccessor
{
  friend class ezReflectedTypeStorageManager;
public:
  ezReflectedTypeStorageAccessor(ezReflectedTypeHandle hReflectedType); // [tested]
  ~ezReflectedTypeStorageAccessor();
 
  virtual const ezVariant GetValue(const ezPropertyPath& path) const override; // [tested]
  virtual bool SetValue(const ezPropertyPath& path, const ezVariant& value) override; // [tested]

private:
  ezDynamicArray<ezVariant> m_Data;
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping* m_pMapping;
};

