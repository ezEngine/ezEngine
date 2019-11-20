#include <TypeScriptPluginPCH.h>

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

void ezTypeScriptBinding::GenerateEnumsFile()
{
  const char* szFile = ":project/TypeScript/ez/Enums.ts";

  ezStringBuilder sFileContent;
  ezStringBuilder sType, sName;

  for (const ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (pRtti->GetProperties().IsEmpty())
      continue;

    if (pRtti->IsDerivedFrom<ezEnumBase>() || pRtti->IsDerivedFrom<ezBitflagsBase>())
    {
      sName = pRtti->GetTypeName();
      sName.TrimWordStart("ez");

      sType.Format("export enum {0} { ", sName);

      for (auto pProp : pRtti->GetProperties().GetSubArray(1))
      {
        if (pProp->GetCategory() == ezPropertyCategory::Constant)
        {
          const ezVariant value = static_cast<const ezAbstractConstantProperty*>(pProp)->GetConstant();
          const ezInt64 iValue = value.ConvertTo<ezInt64>();

          sType.AppendFormat(" {0} = {1},", ezStringUtils::FindLastSubString(pProp->GetPropertyName(), "::") + 2, iValue);
        }
      }

      sType.Shrink(0, 1);
      sType.Append(" }\n");

      sFileContent.Append(sType.GetView());
    }
  }

  ezFileWriter file;
  if (file.Open(szFile).Failed())
  {
    ezLog::Error("Failed to open file '{}'", szFile);
    return;
  }

  file.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount());
}
