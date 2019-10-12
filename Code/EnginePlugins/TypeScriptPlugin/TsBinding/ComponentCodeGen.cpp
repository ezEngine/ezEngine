#include <TypeScriptPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

ezResult ezTypeScriptBinding::SetupProjectCode()
{
  ezStringBuilder sAbsSrcFolder;
  EZ_SUCCEED_OR_RETURN(ezFileSystem::ResolvePath(":plugins/TypeScript", &sAbsSrcFolder, nullptr));

  ezStringBuilder sAbsDstFolder;
  EZ_SUCCEED_OR_RETURN(ezFileSystem::ResolvePath(":project/TypeScript", &sAbsDstFolder, nullptr));

  EZ_SUCCEED_OR_RETURN(ezOSFile::CopyFolder(sAbsSrcFolder, sAbsDstFolder));

  GenerateComponentsFile(":project/TypeScript/ez/AllComponents.ts");
  InjectComponentImportExport(":project/TypeScript/ez.ts", "./ez/AllComponents");

  return EZ_SUCCESS;
}

static void GetTsName(const ezRTTI* pRtti, ezStringBuilder& out_sName)
{
  out_sName = pRtti->GetTypeName();
  out_sName.TrimWordStart("ez");
}

void ezTypeScriptBinding::GenerateComponentCode(ezStringBuilder& out_Code, const ezRTTI* pRtti)
{
  ezStringBuilder sComponentType, sParentType;
  GetTsName(pRtti, sComponentType);

  GetTsName(pRtti->GetParentType(), sParentType);

  out_Code.AppendFormat("export class {0} extends {1}\n", sComponentType, sParentType);
  out_Code.Append("{\n");
  out_Code.AppendFormat("  public static GetTypeNameHash(): number { return {}; }\n", pRtti->GetTypeNameHash());
  GenerateExposedFunctionsCode(out_Code, pRtti);
  GeneratePropertiesCode(out_Code, pRtti);
  out_Code.Append("}\n\n");
}

static void CreateComponentTypeList(ezSet<const ezRTTI*>& found, ezDynamicArray<const ezRTTI*>& sorted, const ezRTTI* pRtti)
{
  if (found.Contains(pRtti))
    return;

  if (!pRtti->IsDerivedFrom<ezComponent>())
    return;

  if (pRtti == ezGetStaticRTTI<ezComponent>())
    return;

  found.Insert(pRtti);
  CreateComponentTypeList(found, sorted, pRtti->GetParentType());

  sorted.PushBack(pRtti);
}

void ezTypeScriptBinding::GenerateAllComponentsCode(ezStringBuilder& out_Code)
{
  ezSet<const ezRTTI*> found;
  ezDynamicArray<const ezRTTI*> sorted;
  sorted.Reserve(100);

  for (auto pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    CreateComponentTypeList(found, sorted, pRtti);
  }

  for (auto pRtti : sorted)
  {
    GenerateComponentCode(out_Code, pRtti);
  }
}

void ezTypeScriptBinding::GenerateComponentsFile(const char* szFile)
{
  ezStringBuilder sFileContent;

  sFileContent =
    R"(
import __GameObject = require("./GameObject")
export import GameObject = __GameObject.GameObject;

import __Component = require("./Component")
export import Component = __Component.Component;

)";

  GenerateAllComponentsCode(sFileContent);

  ezFileWriter file;
  if (file.Open(szFile).Failed())
  {
    ezLog::Error("Failed to open file '{}'", szFile);
    return;
  }

  file.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount());
}

void ezTypeScriptBinding::InjectComponentImportExport(const char* szFile, const char* szComponentFile)
{
  ezSet<const ezRTTI*> found;
  ezDynamicArray<const ezRTTI*> sorted;
  sorted.Reserve(100);

  for (auto pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    CreateComponentTypeList(found, sorted, pRtti);
  }

  ezStringBuilder sImportExport, sTypeName;

  sImportExport.Format(R"(

// AUTO-GENERATED
import __AllComponents = require("{}")
)",
    szComponentFile);

  for (const ezRTTI* pRtti : sorted)
  {
    GetTsName(pRtti, sTypeName);
    sImportExport.AppendFormat("export import {0}  = __AllComponents.{0};\n",
      sTypeName);
  }


  ezStringBuilder sFinal;

  {
    ezFileReader fileIn;
    fileIn.Open(szFile);

    ezStringBuilder sSrc;
    sSrc.ReadAll(fileIn);

    if (const char* szAutoGen = sSrc.FindSubString("// AUTO-GENERATED"))
    {
      sFinal.SetSubString_FromTo(sSrc.GetData(), szAutoGen);
      sFinal.Trim(" \t\n\r");
    }
    else
    {
      sFinal = sSrc;
      sFinal.Append("\n\n");
    }

    sFinal.Append(sImportExport.GetView());
    sFinal.Append("\n");
  }

  {
    ezFileWriter fileOut;
    fileOut.Open(szFile);
    fileOut.WriteBytes(sFinal.GetData(), sFinal.GetElementCount());
  }
}
