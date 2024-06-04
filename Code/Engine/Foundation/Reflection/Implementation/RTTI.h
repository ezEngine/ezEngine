#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

// *****************************************
// ***** Runtime Type Information Data *****

struct ezRTTIAllocator;
class ezAbstractProperty;
class ezAbstractFunctionProperty;
class ezAbstractMessageHandler;
struct ezMessageSenderInfo;
class ezPropertyAttribute;
class ezMessage;
using ezMessageId = ezUInt16;

/// \brief This class holds information about reflected types. Each instance represents one type that is known to the reflection
/// system.
///
/// Instances of this class are typically created through the macros from the StaticRTTI.h header.
/// Each instance represents one type. This class holds information about derivation hierarchies and exposed properties. You can thus find
/// out whether a type is derived from some base class and what properties of which types are available. Properties can then be read and
/// modified on instances of this type.
class EZ_FOUNDATION_DLL ezRTTI
{
public:
  /// \brief The constructor requires all the information about the type that this object represents.
  ezRTTI(ezStringView sName, const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt8 uiVariantType,
    ezBitflags<ezTypeFlags> flags, ezRTTIAllocator* pAllocator, ezArrayPtr<const ezAbstractProperty*> properties, ezArrayPtr<const ezAbstractFunctionProperty*> functions,
    ezArrayPtr<const ezPropertyAttribute*> attributes, ezArrayPtr<ezAbstractMessageHandler*> messageHandlers,
    ezArrayPtr<ezMessageSenderInfo> messageSenders, const ezRTTI* (*fnVerifyParent)());


  ~ezRTTI();

  /// \brief Can be called in debug builds to check that all reflected objects are correctly set up.
  void VerifyCorrectness() const;

  /// \brief Calls VerifyCorrectness() on all ezRTTI objects.
  static void VerifyCorrectnessForAllTypes();

  /// \brief Returns the name of this type.
  EZ_ALWAYS_INLINE ezStringView GetTypeName() const { return m_sTypeName; } // [tested]

  /// \brief Returns the hash of the name of this type.
  EZ_ALWAYS_INLINE ezUInt64 GetTypeNameHash() const { return m_uiTypeNameHash; } // [tested]

  /// \brief Returns the type that is the base class of this type. May be nullptr if this type has no base class.
  EZ_ALWAYS_INLINE const ezRTTI* GetParentType() const { return m_pParentType; } // [tested]

  /// \brief Returns the corresponding variant type for this type or Invalid if there is none.
  EZ_ALWAYS_INLINE ezVariantType::Enum GetVariantType() const { return static_cast<ezVariantType::Enum>(m_uiVariantType); }

  /// \brief Returns true if this type is derived from the given type (or of the same type).
  EZ_ALWAYS_INLINE bool IsDerivedFrom(const ezRTTI* pBaseType) const // [tested]
  {
    const ezUInt32 thisGeneration = m_ParentHierarchy.GetCount();
    const ezUInt32 baseGeneration = pBaseType->m_ParentHierarchy.GetCount();
    EZ_ASSERT_DEBUG(thisGeneration > 0 && baseGeneration > 0, "SetupParentHierarchy() has not been called");
    return thisGeneration >= baseGeneration && m_ParentHierarchy.GetData()[thisGeneration - baseGeneration] == pBaseType;
  }

  /// \brief Returns true if this type is derived from or identical to the given type.
  template <typename BASE>
  EZ_ALWAYS_INLINE bool IsDerivedFrom() const // [tested]
  {
    return IsDerivedFrom(ezGetStaticRTTI<BASE>());
  }

  /// \brief Returns the object through which instances of this type can be allocated.
  EZ_ALWAYS_INLINE ezRTTIAllocator* GetAllocator() const { return m_pAllocator; } // [tested]

  /// \brief Returns the array of properties that this type has. Does NOT include properties from base classes.
  EZ_ALWAYS_INLINE ezArrayPtr<const ezAbstractProperty* const> GetProperties() const { return m_Properties; } // [tested]

  EZ_ALWAYS_INLINE ezArrayPtr<const ezAbstractFunctionProperty* const> GetFunctions() const { return m_Functions; }

  EZ_ALWAYS_INLINE ezArrayPtr<const ezPropertyAttribute* const> GetAttributes() const { return m_Attributes; }

  /// \brief Returns the first attribute that derives from the given type, or nullptr if nothing is found.
  template <typename Type>
  const Type* GetAttributeByType() const;

  /// \brief Returns the list of properties that this type has, including derived properties from all base classes.
  void GetAllProperties(ezDynamicArray<const ezAbstractProperty*>& out_properties) const; // [tested]

  /// \brief Returns the size (in bytes) of an instance of this type.
  EZ_ALWAYS_INLINE ezUInt32 GetTypeSize() const { return m_uiTypeSize; } // [tested]

  /// \brief Returns the version number of this type.
  EZ_ALWAYS_INLINE ezUInt32 GetTypeVersion() const { return m_uiTypeVersion; }

  /// \brief Returns the type flags.
  EZ_ALWAYS_INLINE const ezBitflags<ezTypeFlags>& GetTypeFlags() const { return m_TypeFlags; } // [tested]

  /// \brief Searches all ezRTTI instances for the one with the given name, or nullptr if no such type exists.
  static const ezRTTI* FindTypeByName(ezStringView sName); // [tested]

  /// \brief Searches all ezRTTI instances for the one with the given hashed name, or nullptr if no such type exists.
  static const ezRTTI* FindTypeByNameHash(ezUInt64 uiNameHash); // [tested]
  static const ezRTTI* FindTypeByNameHash32(ezUInt32 uiNameHash);

  using PredicateFunc = ezDelegate<bool(const ezRTTI*), 48>;
  /// \brief Searches all ezRTTI instances for one where the given predicate function returns true
  static const ezRTTI* FindTypeIf(PredicateFunc func);

  /// \brief Will iterate over all properties of this type and (optionally) the base types to search for a property with the given name.
  const ezAbstractProperty* FindPropertyByName(ezStringView sName, bool bSearchBaseTypes = true) const; // [tested]

  /// \brief Returns the name of the plugin which this type is declared in.
  EZ_ALWAYS_INLINE ezStringView GetPluginName() const { return m_sPluginName; } // [tested]

  /// \brief Returns the array of message handlers that this type has.
  EZ_ALWAYS_INLINE const ezArrayPtr<ezAbstractMessageHandler*>& GetMessageHandlers() const { return m_MessageHandlers; }

  /// \brief Dispatches the given message to the proper message handler, if there is one available. Returns true if so, false if no message
  /// handler for this type exists.
  bool DispatchMessage(void* pInstance, ezMessage& ref_msg) const;

  /// \brief Dispatches the given message to the proper message handler, if there is one available. Returns true if so, false if no message
  /// handler for this type exists.
  bool DispatchMessage(const void* pInstance, ezMessage& ref_msg) const;

  /// \brief Returns whether this type can handle the given message type.
  template <typename MessageType>
  EZ_ALWAYS_INLINE bool CanHandleMessage() const
  {
    return CanHandleMessage(MessageType::GetTypeMsgId());
  }

  /// \brief Returns whether this type can handle the message type with the given id.
  inline bool CanHandleMessage(ezMessageId id) const
  {
    EZ_ASSERT_DEBUG(m_uiMsgIdOffset != ezSmallInvalidIndex, "Message handler table should have been gathered at this point.\n"
                                                            "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                            "you may have forgotten to instantiate an ezPlugin object inside your plugin DLL.");

    const ezUInt32 uiIndex = id - m_uiMsgIdOffset;
    return uiIndex < m_DynamicMessageHandlers.GetCount() && m_DynamicMessageHandlers.GetData()[uiIndex] != nullptr;
  }

  EZ_ALWAYS_INLINE const ezArrayPtr<ezMessageSenderInfo>& GetMessageSender() const { return m_MessageSenders; }

  struct ForEachOptions
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      None = 0,
      ExcludeNonAllocatable = EZ_BIT(0), ///< Excludes all types that cannot be allocated through ezRTTI. They may still be creatable through regular C++, though.
      ExcludeAbstract = EZ_BIT(1),       ///< Excludes all types that are marked as 'abstract'. They may not be abstract in the C++ sense, though.
      ExcludeNotConcrete = ExcludeNonAllocatable | ExcludeAbstract,

      Default = None
    };

    struct Bits
    {
      ezUInt8 ExcludeNonAllocatable : 1;
      ezUInt8 ExcludeAbstract : 1;
    };
  };

  using VisitorFunc = ezDelegate<void(const ezRTTI*), 48>;
  static void ForEachType(VisitorFunc func, ezBitflags<ForEachOptions> options = ForEachOptions::Default); // [tested]

  static void ForEachDerivedType(const ezRTTI* pBaseType, VisitorFunc func, ezBitflags<ForEachOptions> options = ForEachOptions::Default);

  template <typename T>
  static EZ_ALWAYS_INLINE void ForEachDerivedType(VisitorFunc func, ezBitflags<ForEachOptions> options = ForEachOptions::Default)
  {
    ForEachDerivedType(ezGetStaticRTTI<T>(), func, options);
  }

protected:
  ezStringView m_sPluginName;
  ezStringView m_sTypeName;
  ezArrayPtr<const ezAbstractProperty* const> m_Properties;
  ezArrayPtr<const ezAbstractFunctionProperty* const> m_Functions;
  ezArrayPtr<const ezPropertyAttribute* const> m_Attributes;
  void UpdateType(const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt8 uiVariantType, ezBitflags<ezTypeFlags> flags);
  void RegisterType();
  void UnregisterType();

  void GatherDynamicMessageHandlers();
  void SetupParentHierarchy();

  const ezRTTI* m_pParentType = nullptr;
  ezRTTIAllocator* m_pAllocator = nullptr;

  ezUInt32 m_uiTypeSize = 0;
  ezUInt32 m_uiTypeVersion = 0;
  ezUInt64 m_uiTypeNameHash = 0;
  ezUInt32 m_uiTypeIndex = 0;
  ezBitflags<ezTypeFlags> m_TypeFlags;
  ezUInt8 m_uiVariantType = 0;
  ezUInt16 m_uiMsgIdOffset = ezSmallInvalidIndex;

  const ezRTTI* (*m_VerifyParent)();

  ezArrayPtr<ezAbstractMessageHandler*> m_MessageHandlers;
  ezSmallArray<ezAbstractMessageHandler*, 1, ezStaticsAllocatorWrapper> m_DynamicMessageHandlers; // do not track this data, it won't be deallocated before shutdown

  ezArrayPtr<ezMessageSenderInfo> m_MessageSenders;
  ezSmallArray<const ezRTTI*, 7, ezStaticsAllocatorWrapper> m_ParentHierarchy;

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Reflection);

  /// \brief Assigns the given plugin name to every ezRTTI instance that has no plugin assigned yet.
  static void AssignPlugin(ezStringView sPluginName);

  static void SanityCheckType(ezRTTI* pType);

  /// \brief Handles events by ezPlugin, to figure out which types were provided by which plugin
  static void PluginEventHandler(const ezPluginEvent& EventData);
};

EZ_DECLARE_FLAGS_OPERATORS(ezRTTI::ForEachOptions);


// ***********************************
// ***** Object Allocator Struct *****


/// \brief The interface for an allocator that creates instances of reflected types.
struct EZ_FOUNDATION_DLL ezRTTIAllocator
{
  virtual ~ezRTTIAllocator();

  /// \brief Returns whether the type that is represented by this allocator, can be dynamically allocated at runtime.
  virtual bool CanAllocate() const { return true; } // [tested]

  /// \brief Allocates one instance.
  template <typename T>
  ezInternal::NewInstance<T> Allocate(ezAllocator* pAllocator = nullptr)
  {
    return AllocateInternal(pAllocator).Cast<T>();
  }

  /// \brief Clones the given instance.
  template <typename T>
  ezInternal::NewInstance<T> Clone(const void* pObject, ezAllocator* pAllocator = nullptr)
  {
    return CloneInternal(pObject, pAllocator).Cast<T>();
  }

  /// \brief Deallocates the given instance.
  virtual void Deallocate(void* pObject, ezAllocator* pAllocator = nullptr) = 0; // [tested]

private:
  virtual ezInternal::NewInstance<void> AllocateInternal(ezAllocator* pAllocator) = 0;
  virtual ezInternal::NewInstance<void> CloneInternal(const void* pObject, ezAllocator* pAllocator)
  {
    EZ_REPORT_FAILURE("Cloning is not supported by this allocator.");
    return ezInternal::NewInstance<void>(nullptr, pAllocator);
  }
};

/// \brief Dummy Allocator for types that should not be allocatable through the reflection system.
struct EZ_FOUNDATION_DLL ezRTTINoAllocator : public ezRTTIAllocator
{
  /// \brief Returns false, because this type of allocator is used for classes that shall not be allocated dynamically.
  virtual bool CanAllocate() const override { return false; } // [tested]

  /// \brief Will trigger an assert.
  virtual ezInternal::NewInstance<void> AllocateInternal(ezAllocator* pAllocator) override // [tested]
  {
    EZ_REPORT_FAILURE("This function should never be called.");
    return ezInternal::NewInstance<void>(nullptr, pAllocator);
  }

  /// \brief Will trigger an assert.
  virtual void Deallocate(void* pObject, ezAllocator* pAllocator) override // [tested]
  {
    EZ_REPORT_FAILURE("This function should never be called.");
  }
};

/// \brief Default implementation of ezRTTIAllocator that allocates instances via the given allocator.
template <typename CLASS, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
struct ezRTTIDefaultAllocator : public ezRTTIAllocator
{
  /// \brief Returns a new instance that was allocated with the given allocator.
  virtual ezInternal::NewInstance<void> AllocateInternal(ezAllocator* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    return EZ_NEW(pAllocator, CLASS);
  }

  /// \brief Clones the given instance with the given allocator.
  virtual ezInternal::NewInstance<void> CloneInternal(const void* pObject, ezAllocator* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    return CloneImpl(pObject, pAllocator, ezTraitInt<std::is_copy_constructible<CLASS>::value>());
  }

  /// \brief Deletes the given instance with the given allocator.
  virtual void Deallocate(void* pObject, ezAllocator* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    CLASS* pPointer = static_cast<CLASS*>(pObject);
    EZ_DELETE(pAllocator, pPointer);
  }

private:
  ezInternal::NewInstance<void> CloneImpl(const void* pObject, ezAllocator* pAllocator, ezTraitInt<0>)
  {
    EZ_REPORT_FAILURE("Clone failed since the type is not copy constructible");
    return ezInternal::NewInstance<void>(nullptr, pAllocator);
  }

  ezInternal::NewInstance<void> CloneImpl(const void* pObject, ezAllocator* pAllocator, ezTraitInt<1>)
  {
    return EZ_NEW(pAllocator, CLASS, *static_cast<const CLASS*>(pObject));
  }
};
