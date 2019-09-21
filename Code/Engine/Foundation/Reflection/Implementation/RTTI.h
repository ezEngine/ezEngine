#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Utilities/EnumerableClass.h>


// *****************************************
// ***** Runtime Type Information Data *****

struct ezRTTIAllocator;
class ezAbstractProperty;
class ezAbstractFunctionProperty;
class ezAbstractMessageHandler;
struct ezMessageSenderInfo;
class ezPropertyAttribute;
class ezMessage;
typedef ezUInt16 ezMessageId;

/// \brief This enumerable class holds information about reflected types. Each instance represents one type that is known to the reflection
/// system.
///
/// Instances of this class are typically created through the macros from the StaticRTTI.h header.
/// Each instance represents one type. This class holds information about derivation hierarchies and exposed properties. You can thus find
/// out whether a type is derived from some base class and what properties of which types are available. Properties can then be read and
/// modified on instances of this type.
class EZ_FOUNDATION_DLL ezRTTI : public ezEnumerable<ezRTTI>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezRTTI);

public:
  /// \brief The constructor requires all the information about the type that this object represents.
  ezRTTI(const char* szName, const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt32 uiVariantType,
    ezBitflags<ezTypeFlags> flags, ezRTTIAllocator* pAllocator, ezArrayPtr<ezAbstractProperty*> properties,
    ezArrayPtr<ezAbstractProperty*> functions, ezArrayPtr<ezPropertyAttribute*> attributes,
    ezArrayPtr<ezAbstractMessageHandler*> messageHandlers, ezArrayPtr<ezMessageSenderInfo> messageSenders,
    const ezRTTI* (*fnVerifyParent)());


  ~ezRTTI();

  /// \brief Can be called in debug builds to check that all reflected objects are correctly set up.
  void VerifyCorrectness() const;

  /// \brief Calls VerifyCorrectness() on all ezRTTI objects.
  static void VerifyCorrectnessForAllTypes();

  /// \brief Returns the name of this type.
  EZ_ALWAYS_INLINE const char* GetTypeName() const { return m_szTypeName; } // [tested]

  /// \brief Returns the hash of the name of this type.
  EZ_ALWAYS_INLINE ezUInt32 GetTypeNameHash() const { return m_uiTypeNameHash; } // [tested]

  /// \brief Returns the type that is the base class of this type. May be nullptr if this type has no base class.
  EZ_ALWAYS_INLINE const ezRTTI* GetParentType() const { return m_pParentType; } // [tested]

  /// \brief Returns the corresponding variant type for this type or Invalid if there is none.
  EZ_ALWAYS_INLINE ezVariant::Type::Enum GetVariantType() const { return static_cast<ezVariant::Type::Enum>(m_uiVariantType); }

  /// \brief Returns true if this type is derived from the given type.
  bool IsDerivedFrom(const ezRTTI* pBaseType) const; // [tested]

  /// \brief Returns true if this type is derived from or identical to the given type.
  template <typename BASE>
  EZ_ALWAYS_INLINE bool IsDerivedFrom() const // [tested]
  {
    return IsDerivedFrom(ezGetStaticRTTI<BASE>());
  }

  /// \brief Returns the object through which instances of this type can be allocated.
  EZ_ALWAYS_INLINE ezRTTIAllocator* GetAllocator() const { return m_pAllocator; } // [tested]

  /// \brief Returns the array of properties that this type has. Does NOT include properties from base classes.
  EZ_ALWAYS_INLINE const ezArrayPtr<ezAbstractProperty*>& GetProperties() const { return m_Properties; } // [tested]

  EZ_ALWAYS_INLINE const ezArrayPtr<ezAbstractFunctionProperty*>& GetFunctions() const { return m_Functions; }

  EZ_ALWAYS_INLINE const ezArrayPtr<ezPropertyAttribute*>& GetAttributes() const { return m_Attributes; }

  /// \brief Returns the first attribute that derives from the given type, or nullptr if nothing is found.
  template <typename Type>
  const Type* GetAttributeByType() const;

  /// \brief Returns the list of properties that this type has, including derived properties from all base classes.
  void GetAllProperties(ezHybridArray<ezAbstractProperty*, 32>& out_Properties) const; // [tested]

  /// \brief Returns the size (in bytes) of an instance of this type.
  EZ_ALWAYS_INLINE ezUInt32 GetTypeSize() const { return m_uiTypeSize; } // [tested]

  /// \brief Returns the version number of this type.
  EZ_ALWAYS_INLINE ezUInt32 GetTypeVersion() const { return m_uiTypeVersion; }

  /// \brief Returns the type flags.
  EZ_ALWAYS_INLINE const ezBitflags<ezTypeFlags>& GetTypeFlags() const { return m_TypeFlags; } // [tested]

  /// \brief Searches all ezRTTI instances for the one with the given name, or nullptr if no such type exists.
  static ezRTTI* FindTypeByName(const char* szName); // [tested]

  /// \brief Searches all ezRTTI instances for the one with the given hashed name, or nullptr if no such type exists.
  static ezRTTI* FindTypeByNameHash(ezUInt32 uiNameHash); // [tested]

  /// \brief Will iterate over all properties of this type and (optionally) the base types to search for a property with the given name.
  ezAbstractProperty* FindPropertyByName(const char* szName, bool bSearchBaseTypes = true) const; // [tested]

  /// \brief Returns the name of the plugin which this type is declared in.
  EZ_ALWAYS_INLINE const char* GetPluginName() const { return m_szPluginName; } // [tested]

  /// \brief Returns the array of message handlers that this type has.
  EZ_ALWAYS_INLINE const ezArrayPtr<ezAbstractMessageHandler*>& GetMessageHandlers() const { return m_MessageHandlers; }

  /// \brief Dispatches the given message to the proper message handler, if there is one available. Returns true if so, false if no message
  /// handler for this type exists.
  bool DispatchMessage(void* pInstance, ezMessage& msg) const;

  /// \brief Dispatches the given message to the proper message handler, if there is one available. Returns true if so, false if no message
  /// handler for this type exists.
  bool DispatchMessage(const void* pInstance, ezMessage& msg) const;

  /// \brief Returns whether this type can handle the given message type.
  template <typename MessageType>
  EZ_ALWAYS_INLINE bool CanHandleMessage() const
  {
    return CanHandleMessage(MessageType::GetTypeMsgId());
  }

  /// \brief Returns whether this type can handle the message type with the given id.
  inline bool CanHandleMessage(ezMessageId id) const
  {
    EZ_ASSERT_DEBUG(m_bGatheredDynamicMessageHandlers, "Message handler table should have been gathered at this point.\n"
                                                       "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                       "you may have forgotten to instantiate an ezPlugin object inside your plugin DLL.");

    const ezUInt32 uiIndex = id - m_uiMsgIdOffset;
    return uiIndex < m_DynamicMessageHandlers.GetCount() && m_DynamicMessageHandlers[uiIndex] != nullptr;
  }

  EZ_ALWAYS_INLINE const ezArrayPtr<ezMessageSenderInfo>& GetMessageSender() const { return m_MessageSenders; }

  /// \brief Fire this if a type was dynamically added or changed outside of plugin registration.
  static ezEvent<const ezRTTI*> s_TypeUpdatedEvent;

protected:
  const char* m_szPluginName;
  const char* m_szTypeName;
  ezArrayPtr<ezAbstractProperty*> m_Properties;
  ezArrayPtr<ezAbstractFunctionProperty*> m_Functions;
  ezArrayPtr<ezPropertyAttribute*> m_Attributes;
  void UpdateType(const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt32 uiVariantType,
    ezBitflags<ezTypeFlags> flags);
  void RegisterType(ezRTTI* pType);
  void UnregisterType(ezRTTI* pType);

  void GatherDynamicMessageHandlers();
  /// \brief Returns a hash table that accelerates ezRTTI::FindTypeByName.
  ///   The hash table type cannot be put in the header due to circular includes.
  ///   Function is used by RegisterType / UnregisterType to add / remove type from table.
  static void* GetTypeHashTable();

  const ezRTTI* m_pParentType;
  ezRTTIAllocator* m_pAllocator;

  ezUInt32 m_uiVariantType;
  ezUInt32 m_uiTypeSize;
  ezUInt32 m_uiTypeVersion = 0;
  ezUInt32 m_uiTypeNameHash = 0;
  ezBitflags<ezTypeFlags> m_TypeFlags;
  ezUInt32 m_uiMsgIdOffset;

  bool m_bGatheredDynamicMessageHandlers;
  const ezRTTI* (*m_fnVerifyParent)();

  ezArrayPtr<ezAbstractMessageHandler*> m_MessageHandlers;
  ezDynamicArray<ezAbstractMessageHandler*, ezStaticAllocatorWrapper>
    m_DynamicMessageHandlers; // do not track this data, it won't be deallocated before shutdown

  ezArrayPtr<ezMessageSenderInfo> m_MessageSenders;

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Reflection);

  /// \brief Assigns the given plugin name to every ezRTTI instance that has no plugin assigned yet.
  static void AssignPlugin(const char* szPluginName);

  static void SanityCheckType(ezRTTI* pType);

  /// \brief Handles events by ezPlugin, to figure out which types were provided by which plugin
  static void PluginEventHandler(const ezPlugin::PluginEvent& EventData);
};


// ***********************************
// ***** Object Allocator Struct *****


/// \brief The interface for an allocator that creates instances of reflected types.
struct EZ_FOUNDATION_DLL ezRTTIAllocator
{
  /// \brief Returns whether the type that is represented by this allocator, can be dynamically allocated at runtime.
  virtual bool CanAllocate() const { return true; } // [tested]

  /// \brief Allocates one instance.
  template <typename T>
  ezInternal::NewInstance<T> Allocate(ezAllocatorBase* pAllocator = nullptr)
  {
    return AllocateInternal(pAllocator).Cast<T>();
  }

  /// \brief Clones the given instance.
  template <typename T>
  ezInternal::NewInstance<T> Clone(const void* pObject, ezAllocatorBase* pAllocator = nullptr)
  {
    return CloneInternal(pObject, pAllocator).Cast<T>();
  }

  /// \brief Deallocates the given instance.
  virtual void Deallocate(void* pObject, ezAllocatorBase* pAllocator = nullptr) = 0; // [tested]

private:
  virtual ezInternal::NewInstance<void> AllocateInternal(ezAllocatorBase* pAllocator) = 0;
  virtual ezInternal::NewInstance<void> CloneInternal(const void* pObject, ezAllocatorBase* pAllocator)
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
  virtual ezInternal::NewInstance<void> AllocateInternal(ezAllocatorBase* pAllocator) override // [tested]
  {
    EZ_REPORT_FAILURE("This function should never be called.");
    return ezInternal::NewInstance<void>(nullptr, pAllocator);
  }

  /// \brief Will trigger an assert.
  virtual void Deallocate(void* pObject, ezAllocatorBase* pAllocator) override // [tested]
  {
    EZ_REPORT_FAILURE("This function should never be called.");
  }
};

/// \brief Default implementation of ezRTTIAllocator that allocates instances via the given allocator.
template <typename CLASS, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
struct ezRTTIDefaultAllocator : public ezRTTIAllocator
{
  /// \brief Returns a new instance that was allocated with the given allocator.
  virtual ezInternal::NewInstance<void> AllocateInternal(ezAllocatorBase* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    return EZ_NEW(pAllocator, CLASS);
  }

  /// \brief Clones the given instance with the given allocator.
  virtual ezInternal::NewInstance<void> CloneInternal(const void* pObject, ezAllocatorBase* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    return CloneImpl(pObject, pAllocator, ezTraitInt<std::is_copy_constructible<CLASS>::value>());
  }

  /// \brief Deletes the given instance with the given allocator.
  virtual void Deallocate(void* pObject, ezAllocatorBase* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    CLASS* pPointer = static_cast<CLASS*>(pObject);
    EZ_DELETE(pAllocator, pPointer);
  }

private:
  ezInternal::NewInstance<void> CloneImpl(const void* pObject, ezAllocatorBase* pAllocator, ezTraitInt<0>)
  {
    EZ_REPORT_FAILURE("Clone failed since the type is not copy constructible");
    return ezInternal::NewInstance<void>(nullptr, pAllocator);
  }

  ezInternal::NewInstance<void> CloneImpl(const void* pObject, ezAllocatorBase* pAllocator, ezTraitInt<1>)
  {
    return EZ_NEW(pAllocator, CLASS, *static_cast<const CLASS*>(pObject));
  }
};
