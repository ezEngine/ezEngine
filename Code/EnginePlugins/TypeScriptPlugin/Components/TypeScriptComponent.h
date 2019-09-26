#pragma once

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

#include <Core/Scripting/DuktapeWrapper.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

class ezTypeScriptBinding;

class EZ_TYPESCRIPTPLUGIN_DLL ezTypeScriptComponentManager : public ezComponentManager<class ezTypeScriptComponent, ezBlockStorageType::FreeList>
{
  typedef ezComponentManager<class ezTypeScriptComponent, ezBlockStorageType::FreeList> SUPER;

public:
  ezTypeScriptComponentManager(ezWorld* pWorld);
  ~ezTypeScriptComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  ezTypeScriptBinding m_TsBinding;
  ezTypeScriptTranspiler m_Transpiler;

private:
  void Update(const ezWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class EZ_TYPESCRIPTPLUGIN_DLL ezTypeScriptComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezTypeScriptComponent, ezComponent, ezTypeScriptComponentManager);

public:
  ezTypeScriptComponent();
  ~ezTypeScriptComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface
  //
protected:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezTypeScriptComponent Interface
  //
public:
protected:
  void Update(ezTypeScriptBinding& script);
};
