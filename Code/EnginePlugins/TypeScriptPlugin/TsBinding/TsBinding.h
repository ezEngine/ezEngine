#pragma once

#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

#include <Core/Scripting/DuktapeWrapper.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
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

  ezDuktapeWrapper& GetDukTapeWrapper() { return m_Duk; }

private:
  ezDuktapeWrapper m_Duk;
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
  static void GenerateAllComponentsCode(ezStringBuilder& out_Code);
  static void GenerateComponentsFile(const char* szFile);
  static void InjectComponentImportExport(const char* szFile, const char* szComponentFile);

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
  ezResult CreateTsComponent(const char* szTypeName, const ezComponentHandle& hCppComponent, const char* szDebugString = "");
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
  static ezQuat GetQuat(duk_context* pDuk, ezInt32 iObjIdx);

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
