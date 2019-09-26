#pragma once

#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

#include <Core/Scripting/DuktapeWrapper.h>
#include <Foundation/Types/Status.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

class ezWorld;

class ezTypeScriptBinding
{
  /// \name Basics
  ///@{

public:
  ezTypeScriptBinding();
  ~ezTypeScriptBinding();

  void Initialize(ezTypeScriptTranspiler& transpiler, ezWorld& world);
  ezStatus SetupBinding();

  ezDuktapeWrapper& GetDukTapeWrapper() { return m_Duk; }

private:
  ezDuktapeWrapper m_Duk;
  ezTypeScriptTranspiler* m_pTranspiler = nullptr;

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
  ezStatus Init_RequireModules();
  ezStatus Init_Log();
  ezStatus Init_GameObject();
  ezStatus Init_Component();


  ///@}
  /// \name ezWorld
  ///@{
private:
  void StoreWorld(ezWorld* pWorld);
  //void RetrieveWorld();

  ezWorld* m_pWorld = nullptr;
  ///@}
};
