#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>

class ezWorld;

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
  ezSmallArray<ezAbstractFunctionProperty*, NumInplaceFunctions> m_FunctionRawPtrs;
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

class EZ_CORE_DLL ezScriptInstance
{
public:
  ezScriptInstance(ezReflectedClass& inout_owner, ezWorld* pWorld);
  virtual ~ezScriptInstance() = default;

  ezReflectedClass& GetOwner() { return m_Owner; }
  ezWorld* GetWorld() { return m_pWorld; }

  virtual void ApplyParameters(const ezArrayMap<ezHashedString, ezVariant>& parameters) = 0;

private:
  ezReflectedClass& m_Owner;
  ezWorld* m_pWorld = nullptr;
};
