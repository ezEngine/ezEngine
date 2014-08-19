#pragma once

#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

class ezReflectedClass;

/// \brief An ezIReflectedTypeAccessor implementation that works directly on a reflected type instance.
///
/// Wraps around an already existing reflected class instance to allow reading and writing
/// reflected properties via ezVariants. This is useful if you want to store objects inside a DOM
/// that provide more than just data. This class can also be used to set properties on an engine instance
/// of an editor reflected type.
class EZ_TOOLSFOUNDATION_DLL ezReflectedTypeDirectAccessor : public ezIReflectedTypeAccessor
{
public:
  /// \brief Use this ctor for static reflection.
  ezReflectedTypeDirectAccessor(void* pInstance, const ezRTTI* pRtti); // [tested]

  /// \brief Use this ctor for dynamic reflection.
  ezReflectedTypeDirectAccessor(ezReflectedClass* pInstance); // [tested]

  virtual const ezVariant GetValue(const ezPropertyPath& path) const override; // [tested]
  virtual bool SetValue(const ezPropertyPath& path, const ezVariant& value) override; // [tested]

private:
  const ezRTTI* m_pRtti;
  void* m_pInstance;
};

