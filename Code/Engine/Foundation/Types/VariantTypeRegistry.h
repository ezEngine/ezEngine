#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Utilities/EnumerableClass.h>

class ezStreamWriter;
class ezStreamReader;
class ezVariantTypeInfo;

/// \brief Variant type registry allows for custom variant type infos to be accessed.
///
/// Custom variant types are defined via the EZ_DECLARE_CUSTOM_VARIANT_TYPE and EZ_DEFINE_CUSTOM_VARIANT_TYPE macros.
/// \sa EZ_DECLARE_CUSTOM_VARIANT_TYPE, EZ_DEFINE_CUSTOM_VARIANT_TYPE
class EZ_FOUNDATION_DLL ezVariantTypeRegistry
{
  EZ_DECLARE_SINGLETON(ezVariantTypeRegistry);

public:
  /// \brief Find the variant type info for the given ezRTTI type.
  /// \return ezVariantTypeInfo if one exits for the given type, otherwise nullptr.
  const ezVariantTypeInfo* FindVariantTypeInfo(const ezRTTI* pType) const;
  ~ezVariantTypeRegistry();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, VariantTypeRegistry);
  ezVariantTypeRegistry();

  void PluginEventHandler(const ezPluginEvent& EventData);
  void UpdateTypes();

  ezHashTable<const ezRTTI*, const ezVariantTypeInfo*> m_TypeInfos;
};

/// \brief Defines functions to allow the full feature set of ezVariant to be used.
/// \sa EZ_DEFINE_CUSTOM_VARIANT_TYPE, ezVariantTypeRegistry
class EZ_FOUNDATION_DLL ezVariantTypeInfo : public ezEnumerable<ezVariantTypeInfo>
{
public:
  ezVariantTypeInfo();
  virtual const ezRTTI* GetType() const = 0;
  virtual ezUInt32 Hash(const void* pObject) const = 0;
  virtual bool Equal(const void* pObjectA, const void* pObjectB) const = 0;
  virtual void Serialize(ezStreamWriter& ref_writer, const void* pObject) const = 0;
  virtual void Deserialize(ezStreamReader& ref_reader, void* pObject) const = 0;

  EZ_DECLARE_ENUMERABLE_CLASS(ezVariantTypeInfo);
};

/// \brief Helper template used by EZ_DEFINE_CUSTOM_VARIANT_TYPE.
/// \sa EZ_DEFINE_CUSTOM_VARIANT_TYPE
template <typename T>
class ezVariantTypeInfoT : public ezVariantTypeInfo
{
  const ezRTTI* GetType() const override
  {
    return ezGetStaticRTTI<T>();
  }
  ezUInt32 Hash(const void* pObject) const override
  {
    return ezHashHelper<T>::Hash(*static_cast<const T*>(pObject));
  }
  bool Equal(const void* pObjectA, const void* pObjectB) const override
  {
    return ezHashHelper<T>::Equal(*static_cast<const T*>(pObjectA), *static_cast<const T*>(pObjectB));
  }
  void Serialize(ezStreamWriter& writer, const void* pObject) const override
  {
    writer << *static_cast<const T*>(pObject);
  }
  void Deserialize(ezStreamReader& reader, void* pObject) const override
  {
    reader >> *static_cast<T*>(pObject);
  }
};

/// \brief Defines a custom variant type, allowing it to be serialized and compared. The type needs to be declared first before using this macro.
///
/// The given type must implement ezHashHelper and ezStreamWriter / ezStreamReader operators.
/// Macros should be placed in any cpp. Note that once a custom type is defined, it is considered a value type and will be passed by value. It must be linked into every editor and engine dll to allow serialization. Thus it should only be used for common types in base libraries.
/// Limitations: Currently only member variables are supported on custom types, no arrays, set, maps etc. For best performance, any custom type smaller than 16 bytes should be POD so it can be inlined into the ezVariant.
/// \sa EZ_DECLARE_CUSTOM_VARIANT_TYPE, ezVariantTypeRegistry, ezVariant
#define EZ_DEFINE_CUSTOM_VARIANT_TYPE(TYPE)                                                                                                                                       \
  static_assert(ezVariantTypeDeduction<TYPE>::value == ezVariantType::TypedObject, "EZ_DECLARE_CUSTOM_VARIANT_TYPE needs to be added to the header defining TYPE"); \
  ezVariantTypeInfoT<TYPE> g_ezVariantTypeInfoT_##TYPE;
