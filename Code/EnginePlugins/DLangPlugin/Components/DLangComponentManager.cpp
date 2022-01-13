#include <DLangPlugin/DLangPluginPCH.h>

#include <DLangPlugin/Compiler/DCompiler.h>
#include <DLangPlugin/Components/DLangComponent.h>
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/IO/OSFile.h>

ezDLangCompiler ezDLangComponentManager::s_Compiler;
ezInt32 ezDLangComponentManager::s_iDllBuild = 0;

ezDLangComponentManager::ezDLangComponentManager(ezWorld* pWorld)
  : SUPER(pWorld)
{
}

ezDLangComponentManager::~ezDLangComponentManager() = default;

void ezDLangComponentManager::Initialize()
{
  SUPER::Initialize();

  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezDLangComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);
}

void ezDLangComponentManager::Deinitialize()
{
  if (m_hPlugin != nullptr)
  {
    FreeLibrary(ezMinWindows::ToNative(m_hPlugin));
    m_hPlugin = nullptr;
  }

  SUPER::Deinitialize();
}

class CppClass
{
public:
  virtual int QueryMyValue(int i)
  {
    ezLog::Info("QueryMyValue: {}", i);
    return i + 1;
  }
};

extern "C"
{
  using ds_dll = int (*)(int, CppClass*);
}

void ezDLangComponentManager::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (s_Compiler.Initialize().Failed())
    return;

  if (m_hPlugin == nullptr)
  {
    ezStringBuilder sTargetBin = ezOSFile::GetApplicationDirectory();

    if (s_iDllBuild == 0)
      sTargetBin.AppendFormat("/DLangAsteroids.dll"); // TODO
    else
      sTargetBin.AppendFormat("/DLangAsteroids_{}.dll", s_iDllBuild); // TODO

    ++s_iDllBuild;

    s_Compiler.SetOutputBinary(sTargetBin);

    if (s_Compiler.CompileProjectLib().Failed())
    {
      ezLog::Error("Failed to compile D code plugin DLL.");
      return;
    }

    m_hPlugin = ezMinWindows::FromNative(LoadLibraryA(sTargetBin));
    if (m_hPlugin == nullptr)
    {
      ezLog::Error("Couldn't load D code plugin DLL.");
      return;
    }
  }

  //ds_dll f1 = (ds_dll)GetProcAddress(ezMinWindows::ToNative(m_hPlugin), "dll");
  //if (f1)
  //{
  //  CppClass c;

  //  int v = f1(2, &c);
  //  ezLog::Info("Value: {}", v);
  //}
}

void ezDLangComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  EZ_PROFILE_SCOPE("DLang Update");

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndSimulating())
    {
      it->Update();
    }
  }
}
