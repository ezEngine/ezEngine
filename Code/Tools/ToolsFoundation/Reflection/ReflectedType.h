#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Containers/Set.h>

class ezReflectedType;
class ezReflectedTypeManager;
class ezReflectedTypeStorageManager;

/// \brief A path of property names to the POD data type that is to be set / get inside ezIReflectedTypeAccessor.
typedef ezHybridArray<const char*, 6> ezPropertyPath;


typedef ezGenericId<24, 8> ezReflectedTypeId;

/// \brief Handle for a ezReflectedType.
///
/// ezReflectedType can be changed at runtime so pointers shouldn't be stored for it.
class EZ_TOOLSFOUNDATION_DLL ezReflectedTypeHandle
{
public:
  typedef ezUInt32 StorageType;
  EZ_DECLARE_HANDLE_TYPE(ezReflectedTypeHandle, ezReflectedTypeId);
  friend class ezReflectedTypeManager;
  friend class ezReflectedTypeStorageManager;

public:
  const ezReflectedType* GetType() const; // [tested]
};

/// \brief Event message used by the ezReflectedTypeManager.
struct EZ_TOOLSFOUNDATION_DLL ezReflectedTypeChange
{
  ezReflectedTypeHandle m_hType;
  const ezReflectedType* pOldType;
  const ezReflectedType* pNewType;
};

EZ_DECLARE_FLAGS(ezUInt8, PropertyFlags, IsPOD, IsReadOnly);

/// \brief Stores the description of a reflected property in a serializable form, used by ezReflectedTypeDescriptor.
struct EZ_TOOLSFOUNDATION_DLL ezReflectedPropertyDescriptor
{
  ezReflectedPropertyDescriptor(const char* szName, const char* szType, ezBitflags<PropertyFlags> flags); // [tested]
  ezReflectedPropertyDescriptor(const char* szName, ezVariant::Type::Enum type, ezBitflags<PropertyFlags> flags); // [tested]

  ezString m_sName;
  ezString m_sType;
  ezEnum<ezVariant::Type, ezUInt8> m_Type;
  ezBitflags<PropertyFlags> m_Flags;
};

/// \brief Stores the description of a reflected type in a serializable form. Used by ezReflectedTypeManager to add new types.
struct EZ_TOOLSFOUNDATION_DLL ezReflectedTypeDescriptor
{
  ezString m_sTypeName;
  ezString m_sPluginName;
  ezString m_sParentTypeName;
  ezString m_sDefaultInitialization;
  ezDynamicArray<ezReflectedPropertyDescriptor> m_Properties;
};


/// \brief Describes a reflected property - what type it is and what name and access rights it has.
struct EZ_TOOLSFOUNDATION_DLL ezReflectedProperty
{
  ezReflectedProperty(const char* szName, ezVariant::Type::Enum type, ezBitflags<PropertyFlags> flags); // [tested]
  ezReflectedProperty(const char* szName, ezReflectedTypeHandle m_hType, ezBitflags<PropertyFlags> flags); // [tested]

  ezHashedString m_sPropertyName;          ///< Name of the property, must be unique inside an ezReflectedType.
  ezReflectedTypeHandle m_hTypeHandle;     ///< invalid for any type that is directly storable in a ezVariant.
  ezEnum<ezVariant::Type, ezUInt8> m_Type; ///< POD type storable in an ezVariant. If m_pType is set, this value is set to 'Invalid'.
  ezBitflags<PropertyFlags> m_Flags;       ///< Property flags (IsPOD, IsReadOnly).
};


/// \brief Describes the properties and functions of a reflected type.
///
/// The content of this class describes what data needs to be stored to describe a class instance of the represented type.
class EZ_TOOLSFOUNDATION_DLL ezReflectedType
{
  friend class ezReflectedTypeManager;
public:
  ezHashedString GetPluginName() const { return m_sPluginName; } // [tested]
  ezHashedString GetTypeName() const { return m_sTypeName; } // [tested]
  ezReflectedTypeHandle GetParentTypeHandle() const { return m_hParentType; } // [tested]
  ezReflectedTypeHandle GetTypeHandle() const { return m_hType; } // [tested]

  const ezUInt32 GetPropertyCount() const { return m_Properties.GetCount(); } // [tested]
  const ezReflectedProperty* GetPropertyByIndex(ezUInt32 uiIndex) const; // [tested]
  const ezReflectedProperty* GetPropertyByName(const char* szPropertyName) const; // [tested]

  void GetDependencies(ezSet<ezReflectedTypeHandle>& out_dependencies, bool bTransitive = false) const;
  const ezString& GetDefaultInitialization() const { return m_sDefaultInitialization; }

private:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezReflectedType);
  ezReflectedType(const char* szTypeName, const char* szPluginName, ezReflectedTypeHandle hParentType);
  void RegisterProperties();

private:
  ezHashedString m_sTypeName;
  ezHashedString m_sPluginName;
  ezReflectedTypeHandle m_hParentType;
  ezReflectedTypeHandle m_hType;
  ezString m_sDefaultInitialization;

  ezDynamicArray<ezReflectedProperty> m_Properties;
  ezHashTable<const char*, ezUInt32> m_NameToIndex;
  ezSet<ezReflectedTypeHandle> m_Dependencies;
};

