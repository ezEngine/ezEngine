#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>

class ezWorld;

/// \brief Runtime type information for script classes, extending ezRTTI with script-specific functionality.
///
/// Manages type metadata for script classes including function properties and message handlers.
/// Supports reference counting and provides efficient storage for small numbers of functions
/// and message handlers through inplace storage optimization.
class EZ_CORE_DLL ezScriptRTTI : public ezRTTI, public ezRefCountingImpl
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezScriptRTTI);

public:
  enum
  {
    NumInplaceFunctions = 7
  };

  using FunctionList = ezSmallArray<ezUniquePtr<ezAbstractFunctionProperty>, NumInplaceFunctions>;
  using MessageHandlerList = ezSmallArray<ezUniquePtr<ezAbstractMessageHandler>, NumInplaceFunctions>;

  ezScriptRTTI(ezStringView sName, const ezRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers);
  ~ezScriptRTTI();

  const ezAbstractFunctionProperty* GetFunctionByIndex(ezUInt32 uiIndex) const;

private:
  ezString m_sTypeNameStorage;
  FunctionList m_FunctionStorage;
  MessageHandlerList m_MessageHandlerStorage;
  ezSmallArray<const ezAbstractFunctionProperty*, NumInplaceFunctions> m_FunctionRawPtrs;
  ezSmallArray<ezAbstractMessageHandler*, NumInplaceFunctions> m_MessageHandlerRawPtrs;
};

class EZ_CORE_DLL ezScriptFunctionProperty : public ezAbstractFunctionProperty
{
public:
  ezScriptFunctionProperty(ezStringView sName);
  ~ezScriptFunctionProperty();

private:
  ezHashedString m_sPropertyNameStorage;
};

struct ezScriptMessageDesc
{
  const ezRTTI* m_pType = nullptr;
  ezArrayPtr<const ezAbstractProperty* const> m_Properties;
};

class EZ_CORE_DLL ezScriptMessageHandler : public ezAbstractMessageHandler
{
public:
  ezScriptMessageHandler(const ezScriptMessageDesc& desc);
  ~ezScriptMessageHandler();

  void FillMessagePropertyValues(const ezMessage& msg, ezDynamicArray<ezVariant>& out_propertyValues);

private:
  ezArrayPtr<const ezAbstractProperty* const> m_Properties;
};

class EZ_CORE_DLL ezScriptInstance
{
public:
  ezScriptInstance(ezReflectedClass& inout_owner, ezWorld* pWorld);
  virtual ~ezScriptInstance() = default;

  ezReflectedClass& GetOwner() { return m_Owner; }
  ezWorld* GetWorld() { return m_pWorld; }

  virtual void SetInstanceVariables(const ezArrayMap<ezHashedString, ezVariant>& parameters);
  virtual void SetInstanceVariable(const ezHashedString& sName, const ezVariant& value) = 0;
  virtual ezVariant GetInstanceVariable(const ezHashedString& sName) = 0;

private:
  ezReflectedClass& m_Owner;
  ezWorld* m_pWorld = nullptr;
};

struct EZ_CORE_DLL ezScriptAllocator
{
  static ezAllocator* GetAllocator();
};

/// \brief creates a new instance of type using the script allocator
#define EZ_SCRIPT_NEW(type, ...) EZ_NEW(ezScriptAllocator::GetAllocator(), type, __VA_ARGS__)
