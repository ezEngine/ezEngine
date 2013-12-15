#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Foundation/Communication/Message.h>

// *****************************************
// ***** Runtime Type Information Data *****

struct ezRTTIAllocator;
class ezAbstractProperty;
class ezAbstractMessageHandler;

class ezRTTI : public ezEnumerable<ezRTTI>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezRTTI);

public:
  ezRTTI(const char* szName, const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezRTTIAllocator* pAllocator, ezArrayPtr<ezAbstractProperty*> pProperties, ezArrayPtr<ezAbstractMessageHandler*> pMessageHandlers);

  const char* GetTypeName() const { return m_szTypeName; }

  const ezRTTI* GetParentType() const { return m_pParentType; }

  bool IsDerivedFrom(const ezRTTI* pBaseType) const;

  template<typename BASE>
  bool IsDerivedFrom() const { return IsDerivedFrom(ezGetStaticRTTI<BASE>()); }

  ezRTTIAllocator* GetAllocator() const { return m_pAllocator; }

  ezUInt32 GetPropertyCount() const { return m_Properties.GetCount(); }

  const ezAbstractProperty* GetProperty(ezUInt32 uiIndex) const { return m_Properties[uiIndex]; }

  ezUInt32 GetMessageHandlerCount() const { return m_MessageHandlers.GetCount(); }

  const ezAbstractMessageHandler* GetMessageHandler(ezUInt32 uiIndex) const { return m_MessageHandlers[uiIndex]; }

  ezUInt32 GetTypeSize() const { return m_uiTypeSize; }

  template<typename MSG>
  bool HandleMessageOfType(void* pInstance, MSG* pMsg) const
  {
    return HandleMessageOfType(pInstance, MSG::MSG_ID, pMsg);
  }

  bool HandleMessageOfType(void* pInstance, ezMessageId id, ezMessage* pMsg) const;

private:
  const char* m_szTypeName;
  const ezRTTI* m_pParentType;
  ezUInt32 m_uiTypeSize;
  ezRTTIAllocator* m_pAllocator;
  ezArrayPtr<ezAbstractProperty*> m_Properties;
  ezArrayPtr<ezAbstractMessageHandler*> m_MessageHandlers;
};


// ***********************************
// ***** Object Allocator Struct *****


/// \brief The interface for an allocator that creates instances of reflected types.
struct ezRTTIAllocator
{
  /// \brief Returns whether the type that is represented by this allocator, can be dynamically allocated at runtime.
  /// Every deriving class must implement this and return true.
  virtual bool CanAllocate() = 0;

  /// \brief Allocates one instance.
  virtual void* Allocate() = 0;

  /// \brief Deallocates the given instance.
  virtual void Deallocate(void* pObject) = 0;
};

/// \brief Dummy Allocator for types that should not be allocatable through the reflection system.
struct ezRTTINoAllocator : public ezRTTIAllocator
{
  virtual bool CanAllocate() EZ_OVERRIDE { return false; }

  virtual void* Allocate() EZ_OVERRIDE
  {
    EZ_REPORT_FAILURE("This function should never be called.");
    return NULL;
  }

  virtual void Deallocate(void* pObject) EZ_OVERRIDE
  {
    EZ_REPORT_FAILURE("This function should never be called.");
  }
};

/// \brief Default implementation of ezRTTIAllocator that allocates instances via default new/delete.
template<typename CLASS>
struct ezRTTIDefaultAllocator : public ezRTTIAllocator
{
  virtual bool CanAllocate() { return true; }

  virtual void* Allocate() EZ_OVERRIDE
  {
    return EZ_DEFAULT_NEW(CLASS);
  }

  virtual void Deallocate(void* pObject) EZ_OVERRIDE
  {
    CLASS* pPointer = (CLASS*) pObject;
    EZ_DEFAULT_DELETE(pPointer);
  }
};




