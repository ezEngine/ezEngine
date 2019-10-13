#pragma once

#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

#include <Core/Scripting/DuktapeContext.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HashTable.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

class ezWorld;

enum ezTypeScriptBindingIndexProperty
{
  ComponentHandle,
};

class ezTypeScriptBinding
{
  /// \name Basics
  ///@{

public:
  ezTypeScriptBinding();
  ~ezTypeScriptBinding();

  ezResult Initialize(ezTypeScriptTranspiler& transpiler, ezWorld& world);
  ezResult LoadComponent(const char* szComponent);

  ezDuktapeContext& GetDukTapeWrapper() { return m_Duk; }
  duk_context* GetDukContext() { return m_Duk.GetContext(); }

private:
  static void GetTsName(const ezRTTI* pRtti, ezStringBuilder& out_sName);

  ezDuktapeContext m_Duk;
  ezTypeScriptTranspiler* m_pTranspiler = nullptr;
  bool m_bInitialized = false;
  ezMap<ezString, bool> m_LoadedComponents;

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

private:

  static void GenerateMessagesFile(const char* szFile);
  static void GenerateAllMessagesCode(ezStringBuilder& out_Code);
  static void GenerateMessageCode(ezStringBuilder& out_Code, const ezRTTI* pRtti);
  static void GenerateMessagePropertiesCode(ezStringBuilder& out_Code, const ezRTTI* pRtti);


  ///@}
  /// \name Modules
  ///@{
public:
  void SetModuleSearchPath(const char* szPath);

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
  ezResult Init_FunctionBinding();
  ezResult Init_PropertyBinding();


  ///@}
  /// \name ezWorld
  ///@{
public:
  static ezWorld* RetrieveWorld(duk_context* pDuk);

private:
  void StoreWorld(ezWorld* pWorld);

  static ezHashTable<duk_context*, ezWorld*> s_DukToWorld;

  ///@}
  /// \name ezGameObject
  ///@{
public:
  static ezGameObjectHandle RetrieveGameObjectHandle(duk_context* pDuk, ezInt32 iObjIdx = 0 /* use 0, if the game object is passed in as the 'this' object (first parameter) */);
  static ezGameObject* ExpectGameObject(duk_context* pDuk, ezInt32 iObjIdx = 0 /* use 0, if the game object is passed in as the 'this' object (first parameter) */);
  static void DukPutGameObject(duk_context* pDuk, const ezGameObjectHandle& hObject);
  static void DukPutGameObject(duk_context* pDuk, const ezGameObject* pObject);

  ///@}
  /// \name Components
  ///@{
public:
  static ezResult CreateTsComponent(duk_context* pDuk, const char* szTypeName, const ezComponentHandle& hCppComponent, const char* szDebugString = "");
  static void DukPutComponentObject(duk_context* pDuk, const ezComponentHandle& hComponent);
  static void DukPutComponentObject(duk_context* pDuk, ezComponent* pComponent);
  void DeleteTsComponent(const ezComponentHandle& hCppComponent);
  static ezComponentHandle RetrieveComponentHandle(duk_context* pDuk, ezInt32 iObjIdx = 0 /* use 0, if the component is passed in as the 'this' object (first parameter) */);

  template <typename ComponentType>
  static ComponentType* ExpectComponent(duk_context* pDuk, ezInt32 iObjIdx = 0 /* use 0, if the game object is passed in as the 'this' object (first parameter) */);

  ///@}
  /// \name Math
  ///@{

  static ezVec3 GetVec3(duk_context* pDuk, ezInt32 iObjIdx);
  static ezQuat GetQuat(duk_context* pDuk, ezInt32 iObjIdx);
  static void PushVec3(duk_context* pDuk, const ezVec3& value);
  static void PushQuat(duk_context* pDuk, const ezQuat& value);

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
