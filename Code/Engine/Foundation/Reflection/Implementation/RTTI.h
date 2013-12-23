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
  ezRTTI(const char* szName, const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezRTTIAllocator* pAllocator, ezArrayPtr<ezAbstractProperty*> pProperties, ezArrayPtr<ezAbstractMessageHandler*> pMessageHandlers);

  /// \brief Returns the name of this type.
  const char* GetTypeName() const { return m_szTypeName; }

  /// \brief Returns the type that is the base class of this type. May be NULL if this type has no base class.
  const ezRTTI* GetParentType() const { return m_pParentType; }

  /// \brief Returns true if this type is derived from the given type.
  bool IsDerivedFrom(const ezRTTI* pBaseType) const;

  /// \brief Returns true if this type is derived from the given type.
  template<typename BASE>
  bool IsDerivedFrom() const { return IsDerivedFrom(ezGetStaticRTTI<BASE>()); }

  /// \brief Returns the object through which instances of this type can be allocated.
  ezRTTIAllocator* GetAllocator() const { return m_pAllocator; }

  /// \brief Returns the array of properties that this type has.
  const ezArrayPtr<ezAbstractProperty*>& GetProperties() const { return m_Properties; }

  /// \brief Returns the array of message handlers that this type has.
  const ezArrayPtr<ezAbstractMessageHandler*>& GetMessageHandlers() const { return m_MessageHandlers; }

  /// \brief Returns the size (in bytes) of an instance of this type.
  ezUInt32 GetTypeSize() const { return m_uiTypeSize; }

  /// \brief Dispatches the given message to the proper message handler, if there is one available. Returns true if so, false if no message handler for this type exists.
  template<typename MSG>
  bool HandleMessageOfType(void* pInstance, MSG* pMsg, bool bSearchBaseTypes = true) const
  {
    return HandleMessageOfType(pInstance, MSG::MSG_ID, pMsg, bSearchBaseTypes);
  }

  /// \brief Dispatches the given message to the proper message handler, if there is one available. Returns true if so, false if no message handler for this type exists.
  bool HandleMessageOfType(void* pInstance, ezMessageId id, ezMessage* pMsg, bool bSearchBaseTypes = true) const;

  /// \brief Iterates over all ezRTTI instances and returns the one with the given name, or NULL of no such type exists.
  static ezRTTI* FindTypeByName(const char* szName);

  /// \brief Will iterate over all properties of this type and (optionally) the base types to search for a property with the given name.
  ezAbstractProperty* FindPropertyByName(const char* szName, bool bSearchBaseTypes = true) const;

  /// \brief Iterates over all message handlers of this type and (optionally) the base types to search for a message handler for the given message type.
  ezAbstractMessageHandler* FindMessageHandler(ezMessageId id, bool bSearchBaseTypes = true) const;

  /// \brief Returns the name of the plugin which this type is declared in.
  const char* GetPluginName() const { return m_szPluginName; }

private:
  const char* m_szPluginName;
  const char* m_szTypeName;
  const ezRTTI* m_pParentType;
  ezUInt32 m_uiTypeSize;
  ezRTTIAllocator* m_pAllocator;
  ezArrayPtr<ezAbstractProperty*> m_Properties;
  ezArrayPtr<ezAbstractMessageHandler*> m_MessageHandlers;

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Reflection);

  /// \brief Assigns the given plugin name to every ezRTTI instance that has no plugin assigned yet.
  static void AssignPlugin(const char* szPluginName);

  /// \brief Handles events by ezPlugin, to figure out which types were provided by which plugin
  static void PluginEventHandler(const ezPlugin::PluginEvent& EventData);
};


// ***********************************
// ***** Object Allocator Struct *****


/// \brief The interface for an allocator that creates instances of reflected types.
struct EZ_FOUNDATION_DLL ezRTTIAllocator
{
  /// \brief Returns whether the type that is represented by this allocator, can be dynamically allocated at runtime.
  virtual bool CanAllocate() const { return true; }

  /// \brief Allocates one instance.
  virtual void* Allocate() = 0;

  /// \brief Deallocates the given instance.
  virtual void Deallocate(void* pObject) = 0;
};

/// \brief Dummy Allocator for types that should not be allocatable through the reflection system.
struct EZ_FOUNDATION_DLL ezRTTINoAllocator : public ezRTTIAllocator
{
  /// \brief Returns false, because this type of allocator is used for classes that shall not be allocated dynamically.
  virtual bool CanAllocate() const EZ_OVERRIDE { return false; }

  /// \brief Will trigger an assert.
  virtual void* Allocate() EZ_OVERRIDE
  {
    EZ_REPORT_FAILURE("This function should never be called.");
    return NULL;
  }

  /// \brief Will trigger an assert.
  virtual void Deallocate(void* pObject) EZ_OVERRIDE
  {
    EZ_REPORT_FAILURE("This function should never be called.");
  }
};

/// \brief Default implementation of ezRTTIAllocator that allocates instances via default new/delete.
template<typename CLASS>
struct ezRTTIDefaultAllocator : public ezRTTIAllocator
{
  /// \brief Returns a new instance that was allocated on the default heap.
  virtual void* Allocate() EZ_OVERRIDE
  {
    return EZ_DEFAULT_NEW(CLASS);
  }

  /// \brief Deletes the given instance from the default heap.
  virtual void Deallocate(void* pObject) EZ_OVERRIDE
  {
    CLASS* pPointer = (CLASS*) pObject;
    EZ_DEFAULT_DELETE(pPointer);
  }
};




