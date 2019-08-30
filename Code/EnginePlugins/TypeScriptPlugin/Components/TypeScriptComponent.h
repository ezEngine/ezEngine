#pragma once

#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

#include <Core/Scripting/DuktapeWrapper.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

class EZ_TYPESCRIPTPLUGIN_DLL ezTypeScriptComponentManager : public ezComponentManager<class ezTypeScriptComponent, ezBlockStorageType::FreeList>
{
  typedef ezComponentManager<class ezTypeScriptComponent, ezBlockStorageType::FreeList> SUPER;

public:
  ezTypeScriptComponentManager(ezWorld* pWorld);
  ~ezTypeScriptComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  void Update(const ezWorldModule::UpdateContext& context);

  ezDuktapeWrapper m_Script;
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

  //////////////////////////////////////////////////////////////////////////
  // ezTypeScriptComponent Interface
  //
public:
protected:
  void Update(ezDuktapeWrapper& script);

  ezResult TranspileFile(ezDuktapeWrapper& script, const char* szFile, ezStringBuilder& result) const;

  bool m_bScriptLoaded = false;
};
