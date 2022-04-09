#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>

class ezWorld;
using ezScriptClassResourceHandle = ezTypedResourceHandle<class ezScriptClassResource>;

class EZ_CORE_DLL ezScriptInstance
{
public:
  virtual ~ezScriptInstance() {}
  virtual void ApplyParameters(const ezArrayMap<ezHashedString, ezVariant>& parameters) = 0;
};

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

  ezScriptRTTI(const char* szName, const ezRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers);
  ~ezScriptRTTI();

  const ezAbstractFunctionProperty* GetFunctionByIndex(ezUInt32 uiIndex) const;

private:
  ezString m_sTypeNameStorage;
  FunctionList m_FunctionStorage;
  MessageHandlerList m_MessageHandlerStorage;
  ezSmallArray<ezAbstractFunctionProperty*, NumInplaceFunctions> m_FunctionRawPtrs;
  ezSmallArray<ezAbstractMessageHandler*, NumInplaceFunctions> m_MessageHandlerRawPtrs;
};

class EZ_CORE_DLL ezScriptClassResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScriptClassResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezScriptClassResource);

public:
  ezScriptClassResource();
  ~ezScriptClassResource();

  const ezSharedPtr<ezScriptRTTI>& GetType() const { return m_pType; }

  virtual ezUniquePtr<ezScriptInstance> Instantiate(ezReflectedClass& owner, ezWorld* pWorld) const = 0;

protected:
  void CreateScriptType(const char* szName, const ezRTTI* pBaseType, ezScriptRTTI::FunctionList&& functions, ezScriptRTTI::MessageHandlerList&& messageHandlers);
  void DeleteScriptType();

  ezSharedPtr<ezScriptRTTI> m_pType;
};
