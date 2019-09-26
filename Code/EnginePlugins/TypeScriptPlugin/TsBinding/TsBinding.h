#pragma once

#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

#include <Core/Scripting/DuktapeWrapper.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

class ezWorld;

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
  /// \name Modules
  ///@{
public:
  void SetModuleSearchPath(const char* szPath);

private:
  static int DukSearchModule(duk_context* pContext);

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
private:
  void StoreWorld(ezWorld* pWorld);
  //void RetrieveWorld();

  ezWorld* m_pWorld = nullptr;
  ///@}
};
