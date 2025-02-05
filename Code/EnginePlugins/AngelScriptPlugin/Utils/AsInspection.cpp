#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>

void ezAngelScriptUtils::RetrieveAsInfos(asIScriptEngine* pEngine, ezAsInfos& out_Infos)
{
  ezStringBuilder tmp;

  // Enumerations
  {
    for (ezUInt32 idx = 0; idx < pEngine->GetEnumCount(); ++idx)
    {
      const asITypeInfo* pType = pEngine->GetEnumByIndex(idx);

      out_Infos.m_Types.Insert(pType->GetName());
      out_Infos.m_Namespaces.Insert(pType->GetNamespace());

      for (ezUInt32 valIdx = 0; valIdx < pType->GetEnumValueCount(); ++valIdx)
      {
        int value;
        const char* szString = pType->GetEnumValueByIndex(valIdx, &value);

        out_Infos.m_EnumValues.Insert(szString);

        tmp.Set(pType->GetName(), "::", szString);
        out_Infos.m_AllDeclarations.Insert(tmp);
      }
    }
  }

  // Object Types
  {
    for (ezUInt32 idx = 0; idx < pEngine->GetObjectTypeCount(); ++idx)
    {
      const asITypeInfo* pType = pEngine->GetObjectTypeByIndex(idx);
      const ezRTTI* pRtti = ezAngelScriptUtils::MapToRTTI(pType->GetTypeId(), pEngine);

      out_Infos.m_Types.Insert(pType->GetName());
      out_Infos.m_Namespaces.Insert(pType->GetNamespace());

      for (ezUInt32 methodIdx = 0; methodIdx < pType->GetMethodCount(); ++methodIdx)
      {
        const asIScriptFunction* pFunc = pType->GetMethodByIndex(methodIdx, false);

        if (pFunc->IsPrivate())
          continue;

        const intptr_t flags = reinterpret_cast<const intptr_t>(pFunc->GetUserData(ezAsUserData::FuncFlags));

        if ((flags & 0x01) != 0) // duplicate function to fake inheritance
          continue;

        if (pFunc->IsProperty())
        {
          tmp = pFunc->GetName();
          tmp.TrimWordStart("set_");

          if (tmp.TrimWordStart("get_"))
          {
            out_Infos.m_Properties.Insert(tmp);
          }
        }
        else
        {
          out_Infos.m_Methods.Insert(pFunc->GetName());
        }

        out_Infos.m_AllDeclarations.Insert(GetNiceFunctionDeclaration(pFunc, true, true));
      }
    }
  }

  // Global Functions
  {
    for (ezUInt32 funcIdx = 0; funcIdx < pEngine->GetGlobalFunctionCount(); ++funcIdx)
    {
      const asIScriptFunction* pFunc = pEngine->GetGlobalFunctionByIndex(funcIdx);

      out_Infos.m_GlobalFunctions.Insert(pFunc->GetName());
      out_Infos.m_Namespaces.Insert(pFunc->GetNamespace());
      out_Infos.m_AllDeclarations.Insert(GetNiceFunctionDeclaration(pFunc, true, true));
    }
  }

  // Global Properties
  {
    ezStringBuilder sNamespace;

    for (ezUInt32 idx = 0; idx < pEngine->GetGlobalPropertyCount(); ++idx)
    {
      const char* szName;
      const char* szNamespace;
      int typeId;
      bool isConst;
      pEngine->GetGlobalPropertyByIndex(idx, &szName, &szNamespace, &typeId, &isConst);

      out_Infos.m_Namespaces.Insert(szNamespace);
      out_Infos.m_Properties.Insert(szName);

      if (ezStringUtils::IsNullOrEmpty(szNamespace))
      {
        tmp.Set(pEngine->GetTypeDeclaration(typeId, true), " ", szName);
      }
      else
      {
        tmp.Set(pEngine->GetTypeDeclaration(typeId, true), " ", szNamespace, "::", szName);
      }

      out_Infos.m_AllDeclarations.Insert(tmp);
    }
  }
}
