#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

// clang-format off
EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezCVar);

// The CVars need to be saved and loaded whenever plugins are loaded and unloaded.
// Therefore we register as early as possible (Base Startup) at the plugin system,
// to be informed about plugin changes.
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, CVars)

  // for saving and loading we need the filesystem, so make sure we are initialized after
  // and shutdown before the filesystem is
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "FileSystem"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezPlugin::Events().AddEventHandler(ezCVar::PluginEventHandler);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    // save the CVars every time the core is shut down
    // at this point the filesystem might already be uninitialized by the user (data dirs)
    // in that case the variables cannot be saved, but it will fail silently
    // if it succeeds, the most recent state will be serialized though
    ezCVar::SaveCVars();

    ezPlugin::Events().RemoveEventHandler(ezCVar::PluginEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    // save the CVars every time the engine is shut down
    // at this point the filesystem should usually still be configured properly
    ezCVar::SaveCVars();
  }

  // The user is responsible to call 'ezCVar::SetStorageFolder' to define where the CVars are
  // actually stored. That call will automatically load all CVar states.

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on


ezString ezCVar::s_sStorageFolder;
ezEvent<const ezCVarEvent&> ezCVar::s_AllCVarEvents;

void ezCVar::AssignSubSystemPlugin(ezStringView sPluginName)
{
  ezCVar* pCVar = ezCVar::GetFirstInstance();

  while (pCVar)
  {
    if (pCVar->m_sPluginName.IsEmpty())
      pCVar->m_sPluginName = sPluginName;

    pCVar = pCVar->GetNextInstance();
  }
}

void ezCVar::PluginEventHandler(const ezPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case ezPluginEvent::BeforeLoading:
    {
      // before a new plugin is loaded, make sure all currently available CVars
      // are assigned to the proper plugin
      // all not-yet assigned cvars cannot be in any plugin, so assign them to the 'static' plugin
      AssignSubSystemPlugin("Static");
    }
    break;

    case ezPluginEvent::AfterLoadingBeforeInit:
    {
      // after we loaded a new plugin, but before it is initialized,
      // find all new CVars and assign them to that new plugin
      AssignSubSystemPlugin(EventData.m_sPluginBinary);

      // now load the state of all CVars
      LoadCVars();
    }
    break;

    case ezPluginEvent::BeforeUnloading:
    {
      SaveCVars();
    }
    break;

    default:
      break;
  }
}

ezCVar::ezCVar(ezStringView sName, ezBitflags<ezCVarFlags> Flags, ezStringView sDescription)
  : m_sName(sName)
  , m_sDescription(sDescription)
  , m_Flags(Flags)
{
  EZ_ASSERT_DEV(!m_sDescription.IsEmpty(), "Please add a useful description for CVar '{}'.", sName);
}

ezCVar* ezCVar::FindCVarByName(ezStringView sName)
{
  ezCVar* pCVar = ezCVar::GetFirstInstance();

  while (pCVar)
  {
    if (pCVar->GetName().IsEqual_NoCase(sName))
      return pCVar;

    pCVar = pCVar->GetNextInstance();
  }

  return nullptr;
}

void ezCVar::SetStorageFolder(ezStringView sFolder)
{
  s_sStorageFolder = sFolder;
}

ezCommandLineOptionBool opt_NoFileCVars("cvar", "-no-file-cvars", "Disables loading CVar values from the user-specific, persisted configuration file.", false);

void ezCVar::SaveCVarsToFile(ezStringView sPath, bool bIgnoreSaveFlag)
{
  ezHybridArray<ezCVar*, 128> allCVars;

  for (ezCVar* pCVar = ezCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (bIgnoreSaveFlag || pCVar->GetFlags().IsAnySet(ezCVarFlags::Save))
    {
      allCVars.PushBack(pCVar);
    }
  }

  SaveCVarsToFileInternal(sPath, allCVars);
}

void ezCVar::SaveCVars()
{
  if (s_sStorageFolder.IsEmpty())
    return;

  // this command line disables loading and saving CVars to and from files
  if (opt_NoFileCVars.GetOptionValue(ezCommandLineOption::LogMode::FirstTimeIfSpecified))
    return;

  // first gather all the cvars by plugin
  ezMap<ezString, ezHybridArray<ezCVar*, 128>> PluginCVars;

  {
    ezCVar* pCVar = ezCVar::GetFirstInstance();
    while (pCVar)
    {
      // only store cvars that should be saved
      if (pCVar->GetFlags().IsAnySet(ezCVarFlags::Save))
      {
        if (!pCVar->m_sPluginName.IsEmpty())
          PluginCVars[pCVar->m_sPluginName].PushBack(pCVar);
        else
          PluginCVars["Static"].PushBack(pCVar);
      }

      pCVar = pCVar->GetNextInstance();
    }
  }

  ezMap<ezString, ezHybridArray<ezCVar*, 128>>::Iterator it = PluginCVars.GetIterator();

  ezStringBuilder sTemp;

  // now save all cvars in their plugin specific file
  while (it.IsValid())
  {
    // create the plugin specific file
    sTemp.SetFormat("{0}/CVars_{1}.cfg", s_sStorageFolder, it.Key());

    SaveCVarsToFileInternal(sTemp, it.Value());

    // continue with the next plugin
    ++it;
  }
}

void ezCVar::SaveCVarsToFileInternal(ezStringView path, const ezDynamicArray<ezCVar*>& vars)
{
  ezStringBuilder sTemp;
  ezFileWriter File;
  if (File.Open(path.GetData(sTemp)) == EZ_SUCCESS)
  {
    // write one line for each cvar, to save its current value
    for (ezUInt32 var = 0; var < vars.GetCount(); ++var)
    {
      ezCVar* pCVar = vars[var];

      switch (pCVar->GetType())
      {
        case ezCVarType::Int:
        {
          ezCVarInt* pInt = (ezCVarInt*)pCVar;
          sTemp.SetFormat("{0} = {1}\n", pCVar->GetName(), pInt->GetValue(ezCVarValue::DelayedSync));
        }
        break;
        case ezCVarType::Bool:
        {
          ezCVarBool* pBool = (ezCVarBool*)pCVar;
          sTemp.SetFormat("{0} = {1}\n", pCVar->GetName(), pBool->GetValue(ezCVarValue::DelayedSync) ? "true" : "false");
        }
        break;
        case ezCVarType::Float:
        {
          ezCVarFloat* pFloat = (ezCVarFloat*)pCVar;
          sTemp.SetFormat("{0} = {1}\n", pCVar->GetName(), pFloat->GetValue(ezCVarValue::DelayedSync));
        }
        break;
        case ezCVarType::String:
        {
          ezCVarString* pString = (ezCVarString*)pCVar;
          sTemp.SetFormat("{0} = \"{1}\"\n", pCVar->GetName(), pString->GetValue(ezCVarValue::DelayedSync));
        }
        break;
        default:
          EZ_REPORT_FAILURE("Unknown CVar Type: {0}", pCVar->GetType());
          break;
      }

      // add the one line for that cvar to the config file
      File.WriteBytes(sTemp.GetData(), sTemp.GetElementCount()).IgnoreResult();
    }
  }
}

void ezCVar::LoadCVars(bool bOnlyNewOnes /*= true*/, bool bSetAsCurrentValue /*= true*/)
{
  LoadCVarsFromCommandLine(bOnlyNewOnes, bSetAsCurrentValue);
  LoadCVarsFromFile(bOnlyNewOnes, bSetAsCurrentValue);
}

static ezResult ParseLine(const ezString& sLine, ezStringBuilder& out_sVarName, ezStringBuilder& out_sVarValue)
{
  const char* szSign = sLine.FindSubString("=");

  if (szSign == nullptr)
    return EZ_FAILURE;

  {
    ezStringView sSubString(sLine.GetData(), szSign);

    // remove all trailing spaces
    while (sSubString.EndsWith(" "))
      sSubString.Shrink(0, 1);

    out_sVarName = sSubString;
  }

  {
    ezStringView sSubString(szSign + 1);

    // remove all spaces
    while (sSubString.StartsWith(" "))
      sSubString.Shrink(1, 0);

    // remove all trailing spaces
    while (sSubString.EndsWith(" "))
      sSubString.Shrink(0, 1);


    // remove " and start and end

    if (sSubString.StartsWith("\""))
      sSubString.Shrink(1, 0);

    if (sSubString.EndsWith("\""))
      sSubString.Shrink(0, 1);

    out_sVarValue = sSubString;
  }

  return EZ_SUCCESS;
}

void ezCVar::LoadCVarsFromFile(bool bOnlyNewOnes, bool bSetAsCurrentValue, ezDynamicArray<ezCVar*>* pOutCVars)
{
  if (s_sStorageFolder.IsEmpty())
    return;

  // this command line disables loading and saving CVars to and from files
  if (opt_NoFileCVars.GetOptionValue(ezCommandLineOption::LogMode::FirstTimeIfSpecified))
    return;

  ezMap<ezString, ezHybridArray<ezCVar*, 128>> PluginCVars;

  // first gather all the cvars by plugin
  {
    for (ezCVar* pCVar = ezCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
    {
      // only load cvars that should be saved
      if (pCVar->GetFlags().IsAnySet(ezCVarFlags::Save))
      {
        if (!bOnlyNewOnes || pCVar->m_bHasNeverBeenLoaded)
        {
          if (!pCVar->m_sPluginName.IsEmpty())
            PluginCVars[pCVar->m_sPluginName].PushBack(pCVar);
          else
            PluginCVars["Static"].PushBack(pCVar);
        }
      }

      // it doesn't matter whether the CVar could be loaded from file, either it works the first time, or it stays at its current value
      pCVar->m_bHasNeverBeenLoaded = false;
    }
  }

  {
    ezMap<ezString, ezHybridArray<ezCVar*, 128>>::Iterator it = PluginCVars.GetIterator();

    ezStringBuilder sTemp;

    while (it.IsValid())
    {
      // create the plugin specific file
      sTemp.SetFormat("{0}/CVars_{1}.cfg", s_sStorageFolder, it.Key());

      LoadCVarsFromFileInternal(sTemp.GetView(), it.Value(), bSetAsCurrentValue, pOutCVars);

      // continue with the next plugin
      ++it;
    }
  }
}

void ezCVar::LoadCVarsFromFile(ezStringView sPath, bool bOnlyNewOnes, bool bSetAsCurrentValue, bool bIgnoreSaveFlag, ezDynamicArray<ezCVar*>* pOutCVars)
{
  ezHybridArray<ezCVar*, 128> allCVars;

  for (ezCVar* pCVar = ezCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (bIgnoreSaveFlag || pCVar->GetFlags().IsAnySet(ezCVarFlags::Save))
    {
      if (!bOnlyNewOnes || pCVar->m_bHasNeverBeenLoaded)
      {
        allCVars.PushBack(pCVar);
      }
    }

    // it doesn't matter whether the CVar could be loaded from file, either it works the first time, or it stays at its current value
    pCVar->m_bHasNeverBeenLoaded = false;
  }

  LoadCVarsFromFileInternal(sPath, allCVars, bSetAsCurrentValue, pOutCVars);
}

void ezCVar::LoadCVarsFromFileInternal(ezStringView path, const ezDynamicArray<ezCVar*>& vars, bool bSetAsCurrentValue, ezDynamicArray<ezCVar*>* pOutCVars)
{
  ezFileReader File;
  ezStringBuilder sTemp;

  if (File.Open(path.GetData(sTemp)) == EZ_SUCCESS)
  {
    ezStringBuilder sContent;
    sContent.ReadAll(File);

    ezDynamicArray<ezString> Lines;
    sContent.ReplaceAll("\r", ""); // remove carriage return

    // splits the string at occurrence of '\n' and adds each line to the 'Lines' container
    sContent.Split(true, Lines, "\n");

    ezStringBuilder sVarName;
    ezStringBuilder sVarValue;

    for (const ezString& sLine : Lines)
    {
      if (ParseLine(sLine, sVarName, sVarValue) == EZ_FAILURE)
        continue;

      // now find a variable with the same name
      for (ezUInt32 var = 0; var < vars.GetCount(); ++var)
      {
        ezCVar* pCVar = vars[var];

        if (!sVarName.IsEqual(pCVar->GetName()))
          continue;

        // found the cvar, now convert the text into the proper value *sigh*
        switch (pCVar->GetType())
        {
          case ezCVarType::Int:
          {
            ezInt32 Value = 0;
            if (ezConversionUtils::StringToInt(sVarValue, Value).Succeeded())
            {
              ezCVarInt* pTyped = (ezCVarInt*)pCVar;
              pTyped->m_Values[ezCVarValue::Stored] = Value;
              *pTyped = Value;
            }
          }
          break;
          case ezCVarType::Bool:
          {
            bool Value = sVarValue.IsEqual_NoCase("true");

            ezCVarBool* pTyped = (ezCVarBool*)pCVar;
            pTyped->m_Values[ezCVarValue::Stored] = Value;
            *pTyped = Value;
          }
          break;
          case ezCVarType::Float:
          {
            double Value = 0.0;
            if (ezConversionUtils::StringToFloat(sVarValue, Value).Succeeded())
            {
              ezCVarFloat* pTyped = (ezCVarFloat*)pCVar;
              pTyped->m_Values[ezCVarValue::Stored] = static_cast<float>(Value);
              *pTyped = static_cast<float>(Value);
            }
          }
          break;
          case ezCVarType::String:
          {
            const char* Value = sVarValue.GetData();

            ezCVarString* pTyped = (ezCVarString*)pCVar;
            pTyped->m_Values[ezCVarValue::Stored] = Value;
            *pTyped = Value;
          }
          break;
          default:
            EZ_REPORT_FAILURE("Unknown CVar Type: {0}", pCVar->GetType());
            break;
        }

        if (pOutCVars)
        {
          pOutCVars->PushBack(pCVar);
        }

        if (bSetAsCurrentValue)
          pCVar->SetToDelayedSyncValue();
      }
    }
  }
}

ezCommandLineOptionDoc opt_CVar("cvar", "-CVarName", "<value>", "Forces a CVar to the given value.\n\
Overrides persisted settings.\n\
Examples:\n\
-MyIntVar 42\n\
-MyStringVar \"Hello\"\n\
",
  nullptr);

void ezCVar::LoadCVarsFromCommandLine(bool bOnlyNewOnes /*= true*/, bool bSetAsCurrentValue /*= true*/, ezDynamicArray<ezCVar*>* pOutCVars /*= nullptr*/)
{
  ezStringBuilder sTemp;

  for (ezCVar* pCVar = ezCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (bOnlyNewOnes && !pCVar->m_bHasNeverBeenLoaded)
      continue;

    sTemp.Set("-", pCVar->GetName());

    if (ezCommandLineUtils::GetGlobalInstance()->GetOptionIndex(sTemp) != -1)
    {
      if (pOutCVars)
      {
        pOutCVars->PushBack(pCVar);
      }

      // has been specified on the command line -> mark it as 'has been loaded'
      pCVar->m_bHasNeverBeenLoaded = false;

      switch (pCVar->GetType())
      {
        case ezCVarType::Int:
        {
          ezCVarInt* pTyped = (ezCVarInt*)pCVar;
          ezInt32 Value = pTyped->m_Values[ezCVarValue::Stored];
          Value = ezCommandLineUtils::GetGlobalInstance()->GetIntOption(sTemp, Value);

          pTyped->m_Values[ezCVarValue::Stored] = Value;
          *pTyped = Value;
        }
        break;
        case ezCVarType::Bool:
        {
          ezCVarBool* pTyped = (ezCVarBool*)pCVar;
          bool Value = pTyped->m_Values[ezCVarValue::Stored];
          Value = ezCommandLineUtils::GetGlobalInstance()->GetBoolOption(sTemp, Value);

          pTyped->m_Values[ezCVarValue::Stored] = Value;
          *pTyped = Value;
        }
        break;
        case ezCVarType::Float:
        {
          ezCVarFloat* pTyped = (ezCVarFloat*)pCVar;
          double Value = pTyped->m_Values[ezCVarValue::Stored];
          Value = ezCommandLineUtils::GetGlobalInstance()->GetFloatOption(sTemp, Value);

          pTyped->m_Values[ezCVarValue::Stored] = static_cast<float>(Value);
          *pTyped = static_cast<float>(Value);
        }
        break;
        case ezCVarType::String:
        {
          ezCVarString* pTyped = (ezCVarString*)pCVar;
          ezString Value = ezCommandLineUtils::GetGlobalInstance()->GetStringOption(sTemp, 0, pTyped->m_Values[ezCVarValue::Stored]);

          pTyped->m_Values[ezCVarValue::Stored] = Value;
          *pTyped = Value;
        }
        break;
        default:
          EZ_REPORT_FAILURE("Unknown CVar Type: {0}", pCVar->GetType());
          break;
      }

      if (bSetAsCurrentValue)
        pCVar->SetToDelayedSyncValue();
    }
  }
}

void ezCVar::ListOfCVarsChanged(ezStringView sSetPluginNameTo)
{
  AssignSubSystemPlugin(sSetPluginNameTo);

  LoadCVars();

  ezCVarEvent e(nullptr);
  e.m_EventType = ezCVarEvent::Type::ListOfVarsChanged;

  s_AllCVarEvents.Broadcast(e);
}


EZ_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_CVar);
