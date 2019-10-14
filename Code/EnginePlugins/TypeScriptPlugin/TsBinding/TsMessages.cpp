#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

void ezTypeScriptBinding::GenerateMessagesFile(const char* szFile)
{
  ezStringBuilder sFileContent;

  sFileContent =
    R"(
import __Message = require("./Message")
export import Message = __Message.Message;

)";

  GenerateAllMessagesCode(sFileContent);

  ezFileWriter file;
  if (file.Open(szFile).Failed())
  {
    ezLog::Error("Failed to open file '{}'", szFile);
    return;
  }

  file.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount());
}

static void CreateMessageTypeList(ezSet<const ezRTTI*>& found, ezDynamicArray<const ezRTTI*>& sorted, const ezRTTI* pRtti)
{
  if (found.Contains(pRtti))
    return;

  if (!pRtti->IsDerivedFrom<ezMessage>())
    return;

  if (pRtti == ezGetStaticRTTI<ezMessage>())
    return;

  found.Insert(pRtti);
  CreateMessageTypeList(found, sorted, pRtti->GetParentType());

  sorted.PushBack(pRtti);
}

void ezTypeScriptBinding::GenerateAllMessagesCode(ezStringBuilder& out_Code)
{
  ezSet<const ezRTTI*> found;
  ezDynamicArray<const ezRTTI*> sorted;
  sorted.Reserve(100);

  for (auto pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    CreateMessageTypeList(found, sorted, pRtti);
  }

  for (auto pRtti : sorted)
  {
    GenerateMessageCode(out_Code, pRtti);
  }
}

void ezTypeScriptBinding::GenerateMessageCode(ezStringBuilder& out_Code, const ezRTTI* pRtti)
{
  ezStringBuilder sType, sParentType;
  GetTsName(pRtti, sType);

  GetTsName(pRtti->GetParentType(), sParentType);

  out_Code.AppendFormat("export class {0} extends {1}\n", sType, sParentType);
  out_Code.Append("{\n");
  out_Code.AppendFormat("  constructor() { super(); this.TypeNameHash = {}; }\n", pRtti->GetTypeNameHash());
  GenerateMessagePropertiesCode(out_Code, pRtti);
  out_Code.Append("}\n\n");
}

void ezTypeScriptBinding::GenerateMessagePropertiesCode(ezStringBuilder& out_Code, const ezRTTI* pRtti)
{
  ezStringBuilder sProp;
  ezStringBuilder sDefault;

  for (ezAbstractProperty* pProp : pRtti->GetProperties())
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member)
      continue;

    ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pProp);

    const char* szTypeName = TsType(pMember->GetSpecificType());
    if (szTypeName == nullptr)
      continue;

    const ezVariant def = ezReflectionUtils::GetDefaultValue(pMember);

    if (def.CanConvertTo<ezString>())
    {
      sDefault = def.ConvertTo<ezString>();
    }

    if (!sDefault.IsEmpty())
    {
      sProp.Format("  {0}: {1} = {2};\n", pMember->GetPropertyName(), szTypeName, def);
    }
    else
    {
      sProp.Format("  {0}: {1};\n", pMember->GetPropertyName(), szTypeName);
    }

    out_Code.Append(sProp.GetView());
  }
}

void ezTypeScriptBinding::InjectMessageImportExport(const char* szFile, const char* szMessageFile)
{
  ezSet<const ezRTTI*> found;
  ezDynamicArray<const ezRTTI*> sorted;
  sorted.Reserve(100);

  for (auto pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    CreateMessageTypeList(found, sorted, pRtti);
  }

  ezStringBuilder sImportExport, sTypeName;

  sImportExport.Format(R"(

// AUTO-GENERATED
import __AllMessages = require("{}")
)",
    szMessageFile);

  for (const ezRTTI* pRtti : sorted)
  {
    GetTsName(pRtti, sTypeName);
    sImportExport.AppendFormat("export import {0}  = __AllMessages.{0};\n",
      sTypeName);
  }


  ezStringBuilder sFinal;

  {
    ezFileReader fileIn;
    fileIn.Open(szFile);

    ezStringBuilder sSrc;
    sSrc.ReadAll(fileIn);

    //if (const char* szAutoGen = sSrc.FindSubString("// AUTO-GENERATED"))
    //{
    //  sFinal.SetSubString_FromTo(sSrc.GetData(), szAutoGen);
    //  sFinal.Trim(" \t\n\r");
    //}
    //else
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
