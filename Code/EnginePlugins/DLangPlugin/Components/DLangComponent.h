#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <DLangPlugin/DLangPluginDLL.h>
#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>

class ezDLangCompiler;
class ezDLangBaseComponent;

class EZ_DLANGPLUGIN_DLL ezDLangComponentManager : public ezComponentManager<class ezDLangComponent, ezBlockStorageType::FreeList>
{
  using SUPER = ezComponentManager<class ezDLangComponent, ezBlockStorageType::FreeList>;

public:
  ezDLangComponentManager(ezWorld* pWorld);
  ~ezDLangComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

private:
  void Update(const ezWorldModule::UpdateContext& context);

  ezMinWindows::HMODULE m_hPlugin = nullptr;
  static ezDLangCompiler s_Compiler;
  static ezInt32 s_iDllBuild;
};

//////////////////////////////////////////////////////////////////////////

class EZ_DLANGPLUGIN_DLL ezDLangComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDLangComponent, ezComponent, ezDLangComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

protected:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezDLangComponent

public:
  ezDLangComponent();
  ~ezDLangComponent();

  void SetDLangComponentGuid(const ezUuid& hResource);
  const ezUuid& GetDLangComponentGuid() const;

  void SetDLangComponentFile(const char* szFile); // [ property ]
  const char* GetDLangComponentFile() const;      // [ property ]

  void Update();

private:
  ezUuid m_DLangComponentGuid;
  ezDLangBaseComponent* m_pDLangComponent = nullptr;

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters

public:
  const ezRangeView<const char*, ezUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const ezVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, ezVariant& out_value) const;

private:
  ezArrayMap<ezHashedString, ezVariant> m_Parameters;
};
