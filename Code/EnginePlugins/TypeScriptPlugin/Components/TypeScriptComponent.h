#pragma once

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Scripting/DuktapeContext.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

class ezTypeScriptBinding;

using ezJavaScriptResourceHandle = ezTypedResourceHandle<class ezJavaScriptResource>;

class EZ_TYPESCRIPTPLUGIN_DLL ezTypeScriptComponentManager : public ezComponentManager<class ezTypeScriptComponent, ezBlockStorageType::FreeList>
{
  using SUPER = ezComponentManager<class ezTypeScriptComponent, ezBlockStorageType::FreeList>;

public:
  ezTypeScriptComponentManager(ezWorld* pWorld);
  ~ezTypeScriptComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  ezTypeScriptBinding& GetTsBinding() const { return m_TsBinding; }

  static ezTypeScriptTranspiler s_Transpiler;

private:
  void Update(const ezWorldModule::UpdateContext& context);

  mutable ezTypeScriptBinding m_TsBinding;
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
  virtual void Deinitialize() override;

  virtual bool OnUnhandledMessage(ezMessage& msg) override;
  virtual bool OnUnhandledMessage(ezMessage& msg) const override;

  bool HandleUnhandledMessage(ezMessage& msg);

  //////////////////////////////////////////////////////////////////////////
  // ezTypeScriptComponent Interface
  //
protected:
  void Update(ezTypeScriptBinding& script);

  ezTypeScriptBinding::TsComponentTypeInfo m_ComponentTypeInfo;

  void SetJavaScriptResourceFile(const char* szFile); // [ property ]
  const char* GetJavaScriptResourceFile() const;      // [ property ]

  void SetJavaScriptResource(const ezJavaScriptResourceHandle& hResource); // [ property ]
  const ezJavaScriptResourceHandle& GetJavaScriptResource() const;         // [ property ]

private:
  ezJavaScriptResourceHandle m_hJsResource;
};
