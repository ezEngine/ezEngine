#include <FoundationPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Utilities/CommandLineUtils.h>
#include <Utilities/ConversionUtils.h>

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
    ezPlugin::s_PluginEvents.AddEventHandler(ezCVar::PluginEventHandler);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    // save the CVars every time the core is shut down
    // at this point the filesystem might already be uninitialized by the user (data dirs)
    // in that case the variables cannot be saved, but it will fail silently
    // if it succeeds, the most recent state will be serialized though
    ezCVar::SaveCVars();

    ezPlugin::s_PluginEvents.RemoveEventHandler(ezCVar::PluginEventHandler);
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


ezString ezCVar::s_StorageFolder;
ezEvent<const ezCVar::CVarEvent&> ezCVar::s_AllCVarEvents;

void ezCVar::AssignSubSystemPlugin(const char* szPluginName)
{
  ezCVar* pCVar = ezCVar::GetFirstInstance();

  while (pCVar)
  {
    if (pCVar->m_szPluginName == nullptr)
      pCVar->m_szPluginName = szPluginName;

    pCVar = pCVar->GetNextInstance();
  }
}

void ezCVar::PluginEventHandler(const ezPlugin::PluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case ezPlugin::PluginEvent::BeforeLoading:
    {
      // before a new plugin is loaded, make sure all currently available CVars
      // are assigned to the proper plugin
      // all not-yet assigned cvars cannot be in any plugin, so assign them to the 'static' plugin
      AssignSubSystemPlugin("Static");
    }
    break;

    case ezPlugin::PluginEvent::AfterLoadingBeforeInit:
    {
      // after we loaded a new plugin, but before it is initialized,
      // find all new CVars and assign them to that new plugin
      if (EventData.m_pPluginObject)
        AssignSubSystemPlugin(EventData.m_pPluginObject->GetPluginName());

      // now load the state of all CVars
      LoadCVars();
    }
    break;

    case ezPlugin::PluginEvent::BeforeUnloading:
    {
      SaveCVars();
    }
    break;

    default:
      break;
  }
}

ezCVar::ezCVar(const char* szName, ezBitflags<ezCVarFlags> Flags, const char* szDescription)
{
  m_szPluginName = nullptr;     // will be filled out when plugins are loaded
  m_bHasNeverBeenLoaded = true; // next time 'LoadCVars' is called, its state will be changed

  m_szName = szName;
  m_Flags = Flags;
  m_szDescription = szDescription;

  // 'RequiresRestart' only works together with 'Save'
  if (m_Flags.IsAnySet(ezCVarFlags::RequiresRestart))
    m_Flags.Add(ezCVarFlags::Save);
}

ezCVar* ezCVar::FindCVarByName(const char* szName)
{
  ezCVar* pCVar = ezCVar::GetFirstInstance();

  while (pCVar)
  {
    if (ezStringUtils::IsEqual(pCVar->GetName(), szName))
      return pCVar;

    pCVar = pCVar->GetNextInstance();
  }

  return nullptr;
}

void ezCVar::SetStorageFolder(const char* szFolder)
{
  s_StorageFolder = szFolder;
}

void ezCVar::SaveCVars()
{
  if (s_StorageFolder.IsEmpty())
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
        if (pCVar->m_szPluginName != nullptr)
          PluginCVars[pCVar->m_szPluginName].PushBack(pCVar);
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
    sTemp.Format("{0}/CVars_{1}.cfg", s_StorageFolder, it.Key());

    ezFileWriter File;
    if (File.Open(sTemp.GetData()) == EZ_SUCCESS)
    {
      // write one line for each cvar, to save its current value
      for (ezUInt32 var = 0; var < it.Value().GetCount(); ++var)
      {
        ezCVar* pCVar = it.Value()[var];

        switch (pCVar->GetType())
        {
          case ezCVarType::Int:
          {
            ezCVarInt* pInt = (ezCVarInt*)pCVar;
            sTemp.Format("{0} = {1}\n", pCVar->GetName(), pInt->GetValue(ezCVarValue::Restart));
          }
          break;
          case ezCVarType::Bool:
          {
            ezCVarBool* pBool = (ezCVarBool*)pCVar;
            sTemp.Format("{0} = {1}\n", pCVar->GetName(), pBool->GetValue(ezCVarValue::Restart) ? "true" : "false");
          }
          break;
          case ezCVarType::Float:
          {
            ezCVarFloat* pFloat = (ezCVarFloat*)pCVar;
            sTemp.Format("{0} = {1}\n", pCVar->GetName(), pFloat->GetValue(ezCVarValue::Restart));
          }
          break;
          case ezCVarType::String:
          {
            ezCVarString* pString = (ezCVarString*)pCVar;
            sTemp.Format("{0} = \"{1}\"\n", pCVar->GetName(), pString->GetValue(ezCVarValue::Restart));
          }
          break;
          default:
            EZ_REPORT_FAILURE("Unknown CVar Type: {0}", pCVar->GetType());
            break;
        }

        // add the one line for that cvar to the config file
        File.WriteBytes(sTemp.GetData(), sTemp.GetElementCount());
      }
    }

    // continue with the next plugin
    ++it;
  }
}

void ezCVar::LoadCVars(bool bOnlyNewOnes /*= true*/, bool bSetAsCurrentValue /*= true*/)
{
  LoadCVarsFromCommandLine(bOnlyNewOnes, bSetAsCurrentValue);
  LoadCVarsFromFile(bOnlyNewOnes, bSetAsCurrentValue);
}

static ezResult ReadLine(ezStreamReader& Stream, ezStringBuilder& sLine)
{
  sLine.Clear();

  char c[2];
  c[0] = '\0';
  c[1] = '\0';

  // read the first character
  if (Stream.ReadBytes(c, 1) == 0)
    return EZ_FAILURE;

  // skip all white-spaces at the beginning
  // also skip all empty lines
  // this shit is why I wanted to use Lua...
  while ((c[0] == '\n' || c[0] == '\r' || c[0] == ' ' || c[0] == '\t') && (Stream.ReadBytes(c, 1) > 0))
  {
  }

  // we found something that is not empty, so now read till the end of the line
  while (c[0] != '\0' && c[0] != '\n')
  {
    // skip all tabs and carriage returns
    if (c[0] != '\r' && c[0] != '\t')
    {
      sLine.Append(c);
    }

    // stop if we reached the end of the file
    if (Stream.ReadBytes(c, 1) == 0)
      break;
  }

  if (sLine.IsEmpty())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

static ezResult ParseLine(const ezStringBuilder& sLine, ezStringBuilder& VarName, ezStringBuilder& VarValue)
{
  // more parsing shit

  const char* szSign = sLine.FindSubString("=");

  if (szSign == nullptr)
    return EZ_FAILURE;

  {
    ezStringView sSubString(sLine.GetData(), szSign);

    // remove all trailing spaces
    while (sSubString.EndsWith(" "))
      sSubString.Shrink(0, 1);

    VarName = sSubString;
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

    VarValue = sSubString;
  }

  return EZ_SUCCESS;
}

void ezCVar::LoadCVarsFromFile(bool bOnlyNewOnes, bool bSetAsCurrentValue)
{
  if (s_StorageFolder.IsEmpty())
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
          if (pCVar->m_szPluginName != nullptr)
            PluginCVars[pCVar->m_szPluginName].PushBack(pCVar);
          else
            PluginCVars["Static"].PushBack(pCVar);
        }
      }

      // it doesn't matter whether the CVar could be loaded from file, either it works the first time, or it stays at its current value
      pCVar->m_bHasNeverBeenLoaded = false;
    }
  }

  // now load all cvars from their plugin specific file
  {
    ezMap<ezString, ezHybridArray<ezCVar*, 128>>::Iterator it = PluginCVars.GetIterator();

    ezStringBuilder sTemp;

    while (it.IsValid())
    {
      // create the plugin specific file
      sTemp.Format("{0}/CVars_{1}.cfg", s_StorageFolder, it.Key());

      ezFileReader File;
      if (File.Open(sTemp.GetData()) == EZ_SUCCESS)
      {
        ezStringBuilder sLine, sVarName, sVarValue;
        while (ReadLine(File, sLine) == EZ_SUCCESS)
        {
          if (ParseLine(sLine, sVarName, sVarValue) == EZ_FAILURE)
            continue;

          // now find a variable with the same name
          for (ezUInt32 var = 0; var < it.Value().GetCount(); ++var)
          {
            ezCVar* pCVar = it.Value()[var];

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

            if (bSetAsCurrentValue)
              pCVar->SetToRestartValue();
          }
        }
      }

      // continue with the next plugin
      ++it;
    }
  }
}

void ezCVar::LoadCVarsFromCommandLine(bool bOnlyNewOnes /*= true*/, bool bSetAsCurrentValue /*= true*/)
{
  ezStringBuilder sTemp;

  for (ezCVar* pCVar = ezCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (bOnlyNewOnes && !pCVar->m_bHasNeverBeenLoaded)
      continue;

    sTemp.Set("-", pCVar->GetName());

    if (ezCommandLineUtils::GetGlobalInstance()->GetOptionIndex(sTemp) != -1)
    {
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
        pCVar->SetToRestartValue();
    }
  }
}

EZ_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_CVar);
