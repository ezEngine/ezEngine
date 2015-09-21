#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>


// *****************************************
// ***** Runtime Type Information Data *****

struct ezRTTIAllocator;
class ezAbstractProperty;
class ezAbstractMessageHandler;



/// \brief This enumerable class holds information about reflected types. Each instance represents one type that is known to the reflection system.
///
/// Instances of this class are typically created through the macros from the StaticRTTI.h header.
/// Each instance represents one type. This class holds information about derivation hierarchies and exposed properties. You can thus find out
/// whether a type is derived from some base class and what properties of which types are available. Properties can then be read and modified on
/// instances of this type.
class EZ_FOUNDATION_DLL ezRTTI : public ezEnumerable<ezRTTI>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezRTTI);

public:
  /// \brief The constructor requires all the information about the type that this object represents.
  ezRTTI(const char* szName, const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt32 uiVariantType, ezBitflags<ezTypeFlags> flags,
    ezRTTIAllocator* pAllocator, ezArrayPtr<ezAbstractProperty*> properties, ezArrayPtr<ezAbstractMessageHandler*> messageHandlers, const ezRTTI*(*fnVerifyParent)());

  ~ezRTTI();

  /// \brief Returns the name of this type.
  EZ_FORCE_INLINE const char* GetTypeName() const { return m_szTypeName; } // [tested]

  /// \brief Returns the type that is the base class of this type. May be nullptr if this type has no base class.
  EZ_FORCE_INLINE const ezRTTI* GetParentType() const { return m_pParentType; } // [tested]

  /// \brief Returns the corresponding variant type for this type or Invalid if there is none.
  EZ_FORCE_INLINE ezVariant::Type::Enum GetVariantType() const { return static_cast<ezVariant::Type::Enum>(m_uiVariantType); }

  /// \brief Returns true if this type is derived from the given type.
  bool IsDerivedFrom(const ezRTTI* pBaseType) const; // [tested]

  /// \brief Returns true if this type is derived from the given type.
  template<typename BASE>
  EZ_FORCE_INLINE bool IsDerivedFrom() const { return IsDerivedFrom(ezGetStaticRTTI<BASE>()); } // [tested]

  /// \brief Returns the object through which instances of this type can be allocated.
  EZ_FORCE_INLINE ezRTTIAllocator* GetAllocator() const { return m_pAllocator; } // [tested]

  /// \brief Returns the array of properties that this type has. Does NOT include properties from base classes.
  EZ_FORCE_INLINE const ezArrayPtr<ezAbstractProperty*>& GetProperties() const { return m_Properties; } // [tested]

  /// \brief Returns the list of properties that this type has, including derived properties from all base classes.
  void GetAllProperties(ezHybridArray<ezAbstractProperty*, 32>& out_Properties) const; // [tested]

  /// \brief Returns the size (in bytes) of an instance of this type.
  EZ_FORCE_INLINE ezUInt32 GetTypeSize() const { return m_uiTypeSize; } // [tested]

  /// \brief Returns the version number of this type.
  EZ_FORCE_INLINE ezUInt32 GetTypeVersion() const { return m_uiTypeVersion; }

  /// \brief Returns the type flags.
  EZ_FORCE_INLINE const ezBitflags<ezTypeFlags>& GetTypeFlags() const { return m_TypeFlags; } // [tested]

  /// \brief Iterates over all ezRTTI instances and returns the one with the given name, or nullptr if no such type exists.
  static ezRTTI* FindTypeByName(const char* szName); // [tested]

  /// \brief Will iterate over all properties of this type and (optionally) the base types to search for a property with the given name.
  ezAbstractProperty* FindPropertyByName(const char* szName, bool bSearchBaseTypes = true) const; // [tested]

  /// \brief Returns the name of the plugin which this type is declared in.
  EZ_FORCE_INLINE const char* GetPluginName() const { return m_szPluginName; } // [tested]

  /// \brief Returns the array of message handlers that this type has.
  EZ_FORCE_INLINE const ezArrayPtr<ezAbstractMessageHandler*>& GetMessageHandlers() const { return m_MessageHandlers; }

  /// \brief Dispatches the given message to the proper message handler, if there is one available. Returns true if so, false if no message handler for this type exists.
  bool DispatchMessage(void* pInstance, ezMessage& msg) const;

  /// \brief Dispatches the given message to the proper message handler, if there is one available. Returns true if so, false if no message handler for this type exists.
  bool DispatchMessage(const void* pInstance, ezMessage& msg) const;

  /// \brief Returns whether this type can handle the given message type.
  template <typename MessageType>
  EZ_FORCE_INLINE bool CanHandleMessage() const
  {
    return CanHandleMessage(MessageType::GetMsgId());
  }

  /// \brief Returns whether this type can handle the message type with the given id.
  EZ_FORCE_INLINE bool CanHandleMessage(ezMessageId id) const
  {
    const ezUInt32 uiIndex = id - m_uiMsgIdOffset;
    return uiIndex < m_DynamicMessageHandlers.GetCount() && m_DynamicMessageHandlers[uiIndex] != nullptr;
  }

protected:
  const char* m_szPluginName;
  const char* m_szTypeName;
  ezArrayPtr<ezAbstractProperty*> m_Properties;

  void UpdateType(const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt32 uiVariantType, ezBitflags<ezTypeFlags> flags);

private:
  const ezRTTI* m_pParentType;
  ezUInt32 m_uiVariantType;
  ezUInt32 m_uiTypeSize;
  ezUInt32 m_uiMsgIdOffset;
  ezUInt32 m_uiTypeVersion;
  ezBitflags<ezTypeFlags> m_TypeFlags;
  ezRTTIAllocator* m_pAllocator;

  ezArrayPtr<ezAbstractMessageHandler*> m_MessageHandlers;
  ezDynamicArray<ezAbstractMessageHandler*> m_DynamicMessageHandlers;
  
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
  virtual void* Allocate() = 0; // [tested]

  /// \brief Deallocates the given instance.
  virtual void Deallocate(void* pObject) = 0; // [tested]
};

/// \brief Dummy Allocator for types that should not be allocatable through the reflection system.
struct EZ_FOUNDATION_DLL ezRTTINoAllocator : public ezRTTIAllocator
{
  /// \brief Returns false, because this type of allocator is used for classes that shall not be allocated dynamically.
  virtual bool CanAllocate() const override { return false; } // [tested]

  /// \brief Will trigger an assert.
  virtual void* Allocate() override // [tested]
  {
    EZ_REPORT_FAILURE("This function should never be called.");
    return nullptr;
  }

  /// \brief Will trigger an assert.
  virtual void Deallocate(void* pObject) override // [tested]
  {
    EZ_REPORT_FAILURE("This function should never be called.");
  }
};

/// \brief Default implementation of ezRTTIAllocator that allocates instances via default new/delete.
template<typename CLASS>
struct ezRTTIDefaultAllocator : public ezRTTIAllocator
{
  /// \brief Returns a new instance that was allocated on the default heap.
  virtual void* Allocate() override // [tested]
  {
    return EZ_DEFAULT_NEW(CLASS);
  }

  /// \brief Deletes the given instance from the default heap.
  virtual void Deallocate(void* pObject) override // [tested]
  {
    CLASS* pPointer = static_cast<CLASS*>(pObject);
    EZ_DEFAULT_DELETE(pPointer);
  }
};




