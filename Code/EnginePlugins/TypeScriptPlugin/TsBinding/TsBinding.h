#pragma once

#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Scripting/DuktapeContext.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HashTable.h>
#include <TypeScriptPlugin/Resources/ScriptCompendiumResource.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

class ezWorld;
class ezTypeScriptComponent;

using ezJavaScriptResourceHandle = ezTypedResourceHandle<class ezJavaScriptResource>;

enum ezTypeScriptBindingIndexProperty
{
  ComponentHandle,
  GameObjectHandle
};

class EZ_TYPESCRIPTPLUGIN_DLL ezTypeScriptBinding
{
public:
  struct TsMessageHandler
  {
    const ezRTTI* m_pMessageType = nullptr;
    ezString m_sHandlerFunc;
  };

  struct TsComponentInfo
  {
    ezHybridArray<TsMessageHandler, 4> m_MessageHandlers;
  };

  using TsComponentTypeInfo = ezMap<ezString, TsComponentInfo>::ConstIterator;

  /// \name Basics
  ///@{

public:
  ezTypeScriptBinding();
  ~ezTypeScriptBinding();

  ezResult Initialize(ezWorld& world);
  ezResult LoadComponent(const ezJavaScriptResourceHandle& hResource, TsComponentTypeInfo& out_TypeInfo);
  const TsComponentInfo* GetComponentTypeInfo(const char* szComponentType) const;

  void RegisterMessageHandlersForComponentType(const char* szComponent);

  EZ_ALWAYS_INLINE ezDuktapeContext& GetDukTapeContext() { return m_Duk; }
  EZ_ALWAYS_INLINE duk_context* GetDukContext() { return m_Duk.GetContext(); }

private:
  static void GetTsName(const ezRTTI* pRtti, ezStringBuilder& out_sName);

  ezDuktapeContext m_Duk;
  bool m_bInitialized = false;
  ezMap<ezString, bool> m_LoadedComponents;
  ezScriptCompendiumResourceHandle m_hScriptCompendium;

  ///@}
  /// \name Typescript Code Generation
  ///@{
public:
  static ezResult SetupProjectCode();

private:
  static void GenerateComponentCode(ezStringBuilder& out_Code, const ezRTTI* pRtti);
  static void GenerateExposedFunctionsCode(ezStringBuilder& out_Code, const ezRTTI* pRtti);
  static void GeneratePropertiesCode(ezStringBuilder& out_Code, const ezRTTI* pRtti);
  static void GenerateAllComponentsCode(ezStringBuilder& out_Code);
  static void GenerateComponentsFile(const char* szFile);
  static void InjectComponentImportExport(const char* szFile, const char* szComponentFile);
  static void InjectMessageImportExport(const char* szFile, const char* szComponentFile);

  ///@}
  /// \name Function Binding
  ///@{

public:
  struct FunctionBinding
  {
    ezAbstractFunctionProperty* m_pFunc = nullptr;
  };

  static const FunctionBinding* FindFunctionBinding(ezUInt32 uiFunctionHash);

private:
  static ezUInt32 ComputeFunctionBindingHash(const ezRTTI* pType, ezAbstractFunctionProperty* pFunc);
  static void SetupRttiFunctionBindings();
  static const char* TsType(const ezRTTI* pRtti);
  static int __CPP_Binding_RegisterMessageHandler(duk_context* pDuk);

  static ezHashTable<ezUInt32, FunctionBinding> s_BoundFunctions;


  ///@}
  /// \name Property Binding
  ///@{

public:
  struct PropertyBinding
  {
    ezAbstractMemberProperty* m_pMember = nullptr;
  };

  static const PropertyBinding* FindPropertyBinding(ezUInt32 uiHash);

private:
  static ezUInt32 ComputePropertyBindingHash(const ezRTTI* pType, ezAbstractMemberProperty* pMember);
  static void SetupRttiPropertyBindings();

  static ezHashTable<ezUInt32, PropertyBinding> s_BoundProperties;

  ///@}
  /// \name Message Binding
  ///@{

public:
  static ezUniquePtr<ezMessage> MessageFromParameter(duk_context* pDuk, ezInt32 iObjIdx);
  static void DukPutMessage(duk_context* pDuk, const ezMessage& msg);

  bool DeliverMessage(const TsComponentTypeInfo& typeInfo, ezTypeScriptComponent* pComponent, ezMessage& msg);

private:
  static void GenerateMessagesFile(const char* szFile);
  static void GenerateAllMessagesCode(ezStringBuilder& out_Code);
  static void GenerateMessageCode(ezStringBuilder& out_Code, const ezRTTI* pRtti);
  static void GenerateMessagePropertiesCode(ezStringBuilder& out_Code, const ezRTTI* pRtti);

  ezString m_sCurrentTsMsgHandlerRegistrator;
  ezMap<ezString, TsComponentInfo> m_TsComponentTypes;


  ///@}
  /// \name Modules
  ///@{
private:
  static int DukSearchModule(duk_context* pDuk);

  ///@}
  /// \name Initialization
  ///@{
private:
  ezResult Init_RequireModules();
  ezResult Init_Log();
  ezResult Init_GameObject();
  ezResult Init_Component();
  ezResult Init_World();
  ezResult Init_FunctionBinding();
  ezResult Init_PropertyBinding();


  ///@}
  /// \name ezWorld
  ///@{
public:
  static ezWorld* RetrieveWorld(duk_context* pDuk);
  static ezTypeScriptBinding* RetrieveBinding(duk_context* pDuk);

private:
  void StoreWorld(ezWorld* pWorld);

  static ezHashTable<duk_context*, ezWorld*> s_DukToWorld;

  ///@}
  /// \name ezGameObject
  ///@{
public:
  static ezGameObjectHandle RetrieveGameObjectHandle(duk_context* pDuk, ezInt32 iObjIdx = 0 /* use 0, if the game object is passed in as the 'this' object (first parameter) */);
  static ezGameObject* ExpectGameObject(duk_context* pDuk, ezInt32 iObjIdx = 0 /* use 0, if the game object is passed in as the 'this' object (first parameter) */);
  bool DukPutGameObject(const ezGameObjectHandle& hObject);
  void DukPutGameObject(const ezGameObject* pObject);

  ///@}
  /// \name Components
  ///@{
public:
  void DukPutComponentObject(const ezComponentHandle& hComponent);
  void DukPutComponentObject(ezComponent* pComponent);
  void DeleteTsComponent(const ezComponentHandle& hCppComponent);
  static ezComponentHandle RetrieveComponentHandle(duk_context* pDuk, ezInt32 iObjIdx = 0 /* use 0, if the component is passed in as the 'this' object (first parameter) */);

  template <typename ComponentType>
  static ComponentType* ExpectComponent(duk_context* pDuk, ezInt32 iObjIdx = 0 /* use 0, if the game object is passed in as the 'this' object (first parameter) */);

  ///@}
  /// \name Math
  ///@{

  static ezVec3 GetVec3(duk_context* pDuk, ezInt32 iObjIdx);
  static ezVec3 GetVec3Property(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx);
  static ezQuat GetQuat(duk_context* pDuk, ezInt32 iObjIdx);
  static ezQuat GetQuatProperty(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx);
  static ezColor GetColor(duk_context* pDuk, ezInt32 iObjIdx);
  static ezColor GetColorProperty(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx);
  static void PushVec3(duk_context* pDuk, const ezVec3& value);
  static void PushQuat(duk_context* pDuk, const ezQuat& value);
  static void PushColor(duk_context* pDuk, const ezColor& value);

  ///@}
  /// \name C++ Object Registration
  ///@{
public:
  bool RegisterGameObject(ezGameObjectHandle handle, ezUInt32& out_uiStashIdx);
  ezResult RegisterComponent(const char* szTypeNamem, ezComponentHandle handle, ezUInt32& out_uiStashIdx);


private:
  void StoreReferenceInStash(ezUInt32 uiStashIdx);
  bool DukPushStashObject(ezUInt32 uiStashIdx);

  // TODO: clean up stash every once in a while

  ezUInt32 m_uiNextStashObjIdx = 1024;
  ezMap<ezGameObjectHandle, ezUInt32> m_GameObjectToStashIdx;
  ezMap<ezComponentHandle, ezUInt32> m_ComponentToStashIdx;

  ///@}
};

template <typename ComponentType>
ComponentType* ezTypeScriptBinding::ExpectComponent(duk_context* pDuk, ezInt32 iObjIdx /*= 0 */)
{
  ezComponentHandle hOwnHandle = ezTypeScriptBinding::RetrieveComponentHandle(pDuk, iObjIdx);

  ComponentType* pComponent = nullptr;
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(pDuk);
  EZ_VERIFY(pWorld->TryGetComponent(hOwnHandle, pComponent), "Invalid component parameter");

  return pComponent;
}
