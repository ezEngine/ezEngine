#include <CorePCH.h>

#include <Core/World/WorldModule.h>
#include <Core/World/WorldModuleConfig.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

ezResult ezWorldModuleConfig::Save()
{
  m_InterfaceImpls.Sort();

  ezStringBuilder sPath;
  sPath = ":project/WorldModules.ddl";

  ezFileWriter file;
  if (file.Open(sPath).Failed())
    return EZ_FAILURE;

  ezOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(ezOpenDdlWriter::TypeStringMode::Compliant);

  for (auto& interfaceImpl : m_InterfaceImpls)
  {
    writer.BeginObject("InterfaceImpl");

    ezOpenDdlUtils::StoreString(writer, interfaceImpl.m_sInterfaceName, "Interface");
    ezOpenDdlUtils::StoreString(writer, interfaceImpl.m_sImplementationName, "Implementation");

    writer.EndObject();
  }

  return EZ_SUCCESS;
}

void ezWorldModuleConfig::Load()
{
  EZ_LOG_BLOCK("ezWorldModuleConfig::Load()");

  m_InterfaceImpls.Clear();

  ezStringBuilder sPath;
  sPath = ":project/WorldModules.ddl";

  ezFileReader file;
  if (file.Open(sPath).Failed())
  {
    ezLog::Warning("Could not open world module interface impl config file '{0}'", sPath);
    return;
  }

  ezOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, ezLog::GetThreadLocalLogSystem()).Failed())
  {
    ezLog::Error("Failed to parse world module interface impl config file '{0}'", sPath);
    return;
  }

  const ezOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const ezOpenDdlReaderElement* pInterfaceImpl = pTree->GetFirstChild(); pInterfaceImpl != nullptr; pInterfaceImpl = pInterfaceImpl->GetSibling())
  {
    if (!pInterfaceImpl->IsCustomType("InterfaceImpl"))
      continue;

    const ezOpenDdlReaderElement* pInterface = pInterfaceImpl->FindChildOfType(ezOpenDdlPrimitiveType::String, "Interface");
    const ezOpenDdlReaderElement* pImplementation = pInterfaceImpl->FindChildOfType(ezOpenDdlPrimitiveType::String, "Implementation");

    // this prevents duplicates
    AddInterfaceImplementation(pInterface->GetPrimitivesString()[0], pImplementation->GetPrimitivesString()[0]);
  }
}

void ezWorldModuleConfig::Apply()
{
  EZ_LOG_BLOCK("ezWorldModuleConfig::Apply");

  for (const auto& interfaceImpl : m_InterfaceImpls)
  {
    ezWorldModuleFactory::GetInstance()->RegisterInterfaceImplementation(interfaceImpl.m_sInterfaceName, interfaceImpl.m_sImplementationName);
  }
}

void ezWorldModuleConfig::AddInterfaceImplementation(ezStringView sInterfaceName, ezStringView sImplementationName)
{
  for (auto& interfaceImpl : m_InterfaceImpls)
  {
    if (interfaceImpl.m_sInterfaceName == sInterfaceName)
    {
      interfaceImpl.m_sImplementationName = sImplementationName;
      return;
    }
  }

  m_InterfaceImpls.PushBack({sInterfaceName, sImplementationName});
}

void ezWorldModuleConfig::RemoveInterfaceImplementation(ezStringView sInterfaceName)
{
  for (ezUInt32 i = 0; i < m_InterfaceImpls.GetCount(); ++i)
  {
    if (m_InterfaceImpls[i].m_sInterfaceName == sInterfaceName)
    {
      m_InterfaceImpls.RemoveAtAndCopy(i);
      return;
    }
  }
}


EZ_STATICLINK_FILE(Core, Core_World_Implementation_WorldModuleConfig);

