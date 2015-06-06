#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Containers/Set.h>

class ezRTTI;
class ezPhantomRttiManager;
class ezReflectedTypeStorageManager;

/// \brief A path of property names to the data type that is to be set / get inside ezIReflectedTypeAccessor.
class EZ_TOOLSFOUNDATION_DLL ezPropertyPath : public ezHybridArray<ezString, 6>
{
public:
  ezPropertyPath();
  ezPropertyPath(const char* szPath);

  ezStringBuilder GetPathString() const;
};

/// \brief Event message used by the ezPhantomRttiManager.
struct EZ_TOOLSFOUNDATION_DLL ezPhantomTypeChange
{
  const ezRTTI* m_pChangedType;
};

/// \brief Stores the description of a reflected property in a serializable form, used by ezReflectedTypeDescriptor.
struct EZ_TOOLSFOUNDATION_DLL ezReflectedPropertyDescriptor
{
  ezReflectedPropertyDescriptor() {}
  ezReflectedPropertyDescriptor(ezPropertyCategory::Enum category, const char* szName, const char* szType, ezVariant::Type::Enum type, ezBitflags<ezPropertyFlags> flags); // [tested]
  ezReflectedPropertyDescriptor(const char* szName, ezVariant::Type::Enum type, const ezVariant& constantValue); // [tested]

  ezEnum<ezPropertyCategory> m_Category;
  ezString m_sName;

  ezString m_sType;
  //ezEnum<ezVariantType> m_Type;

  ezBitflags<ezPropertyFlags> m_Flags;

  ezVariant m_ConstantValue;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_TOOLSFOUNDATION_DLL, ezReflectedPropertyDescriptor);

/// \brief Stores the description of a reflected type in a serializable form. Used by ezPhantomRttiManager to add new types.
struct EZ_TOOLSFOUNDATION_DLL ezReflectedTypeDescriptor
{
  ezString m_sTypeName;
  ezString m_sPluginName;
  ezString m_sParentTypeName;
  ezString m_sDefaultInitialization;
  ezBitflags<ezTypeFlags> m_Flags;
  ezDynamicArray<ezReflectedPropertyDescriptor> m_Properties;
  ezUInt32 m_uiTypeSize;
  ezUInt32 m_uiTypeVersion;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_TOOLSFOUNDATION_DLL, ezReflectedTypeDescriptor);



