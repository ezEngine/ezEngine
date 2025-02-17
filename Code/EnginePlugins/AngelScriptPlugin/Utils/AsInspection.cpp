#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>

void ezAngelScriptUtils::RetrieveAsInfos(asIScriptEngine* pEngine, ezAsInfos& out_infos)
{
  ezStringBuilder tmp;

  // Enumerations
  {
    for (ezUInt32 idx = 0; idx < pEngine->GetEnumCount(); ++idx)
    {
      const asITypeInfo* pType = pEngine->GetEnumByIndex(idx);

      out_infos.m_Types.Insert(pType->GetName());
      out_infos.m_Namespaces.Insert(pType->GetNamespace());

      for (ezUInt32 valIdx = 0; valIdx < pType->GetEnumValueCount(); ++valIdx)
      {
        int value;
        const char* szString = pType->GetEnumValueByIndex(valIdx, &value);

        out_infos.m_EnumValues.Insert(szString);

        tmp.Set(pType->GetName(), "::", szString);
        out_infos.m_AllDeclarations.Insert(tmp);
      }
    }
  }

  // Object Types
  {
    for (ezUInt32 idx = 0; idx < pEngine->GetObjectTypeCount(); ++idx)
    {
      const asITypeInfo* pType = pEngine->GetObjectTypeByIndex(idx);
      const ezRTTI* pRtti = ezAngelScriptUtils::MapToRTTI(pType->GetTypeId(), pEngine);

      out_infos.m_Types.Insert(pType->GetName());
      out_infos.m_Namespaces.Insert(pType->GetNamespace());

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
            out_infos.m_Properties.Insert(tmp);
          }
        }
        else
        {
          out_infos.m_Methods.Insert(pFunc->GetName());
        }

        out_infos.m_AllDeclarations.Insert(GetNiceFunctionDeclaration(pFunc, true, true));
      }
    }
  }

  // Global Functions
  {
    for (ezUInt32 funcIdx = 0; funcIdx < pEngine->GetGlobalFunctionCount(); ++funcIdx)
    {
      const asIScriptFunction* pFunc = pEngine->GetGlobalFunctionByIndex(funcIdx);

      out_infos.m_GlobalFunctions.Insert(pFunc->GetName());
      out_infos.m_Namespaces.Insert(pFunc->GetNamespace());
      out_infos.m_AllDeclarations.Insert(GetNiceFunctionDeclaration(pFunc, true, true));
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

      out_infos.m_Namespaces.Insert(szNamespace);
      out_infos.m_Properties.Insert(szName);

      if (ezStringUtils::IsNullOrEmpty(szNamespace))
      {
        tmp.Set(pEngine->GetTypeDeclaration(typeId, true), " ", szName);
      }
      else
      {
        tmp.Set(pEngine->GetTypeDeclaration(typeId, true), " ", szNamespace, "::", szName);
      }

      out_infos.m_AllDeclarations.Insert(tmp);
    }
  }

  // Callbacks
  {
    for (ezUInt32 idx = 0; idx < pEngine->GetFuncdefCount(); ++idx)
    {
      const asITypeInfo* pFunc = pEngine->GetFuncdefByIndex(idx);

      if (pFunc->GetParentType() != nullptr)
        continue;

      out_infos.m_Namespaces.Insert(pFunc->GetNamespace());
      out_infos.m_Types.Insert(pFunc->GetName());

      tmp = ezAngelScriptUtils::GetNiceFunctionDeclaration(pFunc->GetFuncdefSignature());

      tmp.Prepend("funcdef ");
      out_infos.m_AllDeclarations.Insert(tmp);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

static void InsertInOrder(ezDynamicArray<ezString>& ref_typeOrder, const ezRTTI* pRtti)
{
  if (pRtti == nullptr)
    return;

  if (ref_typeOrder.Contains(pRtti->GetTypeName()))
    return;

  InsertInOrder(ref_typeOrder, pRtti->GetParentType());
  ref_typeOrder.PushBack(pRtti->GetTypeName());
}

static ezStringView DealWithNamespace(ezStringBuilder& inout_sNamespace, const char* szNewNamespace, ezStringBuilder& out_sContent)
{
  if (inout_sNamespace != szNewNamespace)
  {
    if (!inout_sNamespace.IsEmpty())
    {
      out_sContent.Append("}\n\n");
    }

    inout_sNamespace = szNewNamespace;

    if (!inout_sNamespace.IsEmpty())
    {
      out_sContent.Append("namespace ", inout_sNamespace, "\n{\n");
    }
  }

  return inout_sNamespace.IsEmpty() ? "" : "  ";
}

void ezAngelScriptUtils::GenerateAsPredefinedFile(asIScriptEngine* pEngine, ezStringBuilder& out_sContent)
{
  out_sContent.Clear();
  out_sContent.Reserve(1024 * 32);

  ezStringView sIndent = "  ";
  ezStringBuilder tmp;
  ezStringBuilder sNamespace;

  // first all the typedefs
  {
    out_sContent.Append("// *** TYPEDEFS *** \n\n");

    for (ezUInt32 idx = 0; idx < pEngine->GetTypedefCount(); ++idx)
    {
      const asITypeInfo* pType = pEngine->GetTypedefByIndex(idx);

      out_sContent.Append("typedef ", ezAngelScriptUtils::GetAsTypeName(pEngine, pType->GetTypedefTypeId()), " ", pType->GetName(), ";\n");
    }
  }

  // then all the enums
  {
    out_sContent.Append("\n// *** ENUMS *** \n\n");

    for (ezUInt32 idx = 0; idx < pEngine->GetEnumCount(); ++idx)
    {
      const asITypeInfo* pType = pEngine->GetEnumByIndex(idx);

      sIndent = DealWithNamespace(sNamespace, pType->GetNamespace(), out_sContent);

      out_sContent.Append("enum ", pType->GetName(), "\n{\n");

      for (ezUInt32 valIdx = 0; valIdx < pType->GetEnumValueCount(); ++valIdx)
      {
        int value;
        const char* szString = pType->GetEnumValueByIndex(valIdx, &value);

        out_sContent.AppendFormat("{}  {} = {},\n", sIndent, szString, value);
      }

      out_sContent.Append("}\n\n");
    }

    if (!sNamespace.IsEmpty())
    {
      out_sContent.Append("}\n\n");
      sNamespace.Clear();
    }
  }

  {
    out_sContent.Append("\n// *** CALLBACKS *** \n\n");

    for (ezUInt32 idx = 0; idx < pEngine->GetFuncdefCount(); ++idx)
    {
      const asITypeInfo* pFunc = pEngine->GetFuncdefByIndex(idx);

      if (pFunc->GetParentType() != nullptr)
        continue;

      sIndent = DealWithNamespace(sNamespace, pFunc->GetNamespace(), out_sContent);

      out_sContent.Append(sIndent, "funcdef ", ezAngelScriptUtils::GetNiceFunctionDeclaration(pFunc->GetFuncdefSignature()), ";\n");
    }

    if (!sNamespace.IsEmpty())
    {
      out_sContent.Append("}\n\n");
      sNamespace.Clear();
    }
  }

  // now all the object types
  {
    out_sContent.Append("\n// *** TYPES *** \n\n");

    // first find all types and sort them by inheritance, so that base classes are defined first
    ezDynamicArray<ezString> typeOrder;

    for (ezUInt32 typeIdx = 0; typeIdx < pEngine->GetObjectTypeCount(); ++typeIdx)
    {
      const asITypeInfo* pType = pEngine->GetObjectTypeByIndex(typeIdx);
      const ezRTTI* pRtti = ezAngelScriptUtils::MapToRTTI(pType->GetTypeId(), pEngine);

      if (pRtti != nullptr)
      {
        InsertInOrder(typeOrder, pRtti);
      }
      else
      {
        typeOrder.PushBack(pType->GetName());
      }
    }

    ezStringBuilder sTemplateName;

    for (const ezString& sType : typeOrder)
    {
      const asITypeInfo* pType = pEngine->GetTypeInfoByName(sType);
      if (pType == nullptr)
        continue;

      // mark it as a string type
      if (ezStringUtils::FindSubString(pType->GetName(), "String") != nullptr)
      {
        out_sContent.Append("[BuiltinString]\n");
      }

      out_sContent.Append("class ", pType->GetName());

      if (pType->GetFlags() & asEObjTypeFlags::asOBJ_TEMPLATE)
      {
        out_sContent.Append("<T>");
        sTemplateName.Set(pType->GetName(), "<T>");
      }

      // append base class if available
      if (const ezRTTI* pRtti = ezAngelScriptUtils::MapToRTTI(pType->GetTypeId(), pEngine))
      {
        if (pRtti->GetParentType() && pRtti->GetParentType() != ezGetStaticRTTI<ezReflectedClass>())
        {
          if (pEngine->GetTypeInfoByName(pRtti->GetParentType()->GetTypeName().GetStartPointer()))
          {
            out_sContent.Append(" : ", pRtti->GetParentType()->GetTypeName());
          }
        }
      }

      out_sContent.Append("\n{\n");

      // child funcdefs
      for (ezUInt32 idx = 0; idx < pType->GetChildFuncdefCount(); ++idx)
      {
        auto pFuncDefType = pType->GetChildFuncdef(idx);

        tmp = ezAngelScriptUtils::GetNiceFunctionDeclaration(pFuncDefType->GetFuncdefSignature());
        tmp.ReplaceAll("T[]", sTemplateName);
        out_sContent.Append(sIndent, "  funcdef ", tmp, ";\n");
      }

      // append all the properties (members)
      for (ezUInt32 idx = 0; idx < pType->GetPropertyCount(); ++idx)
      {
        const char* szName;
        int typeId;

        bool isPrivate = false, isProtected = false, isReference = false;
        pType->GetProperty(idx, &szName, &typeId, &isPrivate, &isProtected, nullptr, &isReference);

        if (isPrivate || isProtected)
          continue;

        ezStringBuilder sDecl = pType->GetPropertyDeclaration(idx);
        out_sContent.Append(sIndent, "  ", sDecl, ";\n");
      }

      if (out_sContent.EndsWith(";\n"))
        out_sContent.Append("\n");

      // now all the constructors (we don't need the destructors)
      for (ezUInt32 methodIdx = 0; methodIdx < pType->GetBehaviourCount(); ++methodIdx)
      {
        asEBehaviours behavior;
        const asIScriptFunction* pFunc = pType->GetBehaviourByIndex(methodIdx, &behavior);

        if (pFunc->IsPrivate())
          continue;

        if (behavior != asEBehaviours::asBEHAVE_CONSTRUCT)
          continue;

        tmp = ezAngelScriptUtils::GetNiceFunctionDeclaration(pFunc);
        out_sContent.Append(sIndent, "  ", tmp, ";\n");
      }

      if (out_sContent.EndsWith(";\n"))
        out_sContent.Append("\n");

      // now write out all the methods
      for (ezUInt32 methodIdx = 0; methodIdx < pType->GetMethodCount(); ++methodIdx)
      {
        const asIScriptFunction* pFunc = pType->GetMethodByIndex(methodIdx, false);

        if (pFunc->IsPrivate())
          continue;

        const intptr_t flags = reinterpret_cast<const intptr_t>(pFunc->GetUserData(ezAsUserData::FuncFlags));

        // ignore the methods that are flagged as duplicates because they are inherited from a base class
        if ((flags & 0x01) != 0)
          continue;

        if (pFunc->IsProperty())
        {
          // if this is a property, we don't write it out as a method, but as a member

          tmp = pFunc->GetName();

          // only continue for getters, where we can inspect the return value
          if (tmp.TrimWordStart("get_"))
          {
            if (const char* szRetTypeName = ezAngelScriptUtils::GetAsTypeName(pEngine, pFunc->GetReturnTypeId()))
            {
              tmp.Prepend(szRetTypeName, " ");
            }
            else
            {
              tmp.Prepend("// ERROR: unknown-type ");
            }

            out_sContent.Append(sIndent, "  ", tmp, ";\n");
          }
        }
        else
        {
          if (ezStringUtils::IsEqual(pFunc->GetName(), "opImplCast"))
          {
            // not needed, we have the base class
            continue;
          }

          tmp = ezAngelScriptUtils::GetNiceFunctionDeclaration(pFunc);
          tmp.ReplaceAll("T[]", sTemplateName);
          out_sContent.Append(sIndent, "  ", tmp, ";\n");
        }
      }

      out_sContent.Append("}\n\n");
    }
  }

  // we need  to also declare this, because it isn't registered as a global type
  {
    out_sContent.Append("\n// *** EXTRA *** \n\n");

    const char* szClassCode = R"(
// Base class for objects that one shall be able to instantiate through the ezScriptComponent.
class ezAngelScriptClass : ezIAngelScriptClass
{
    ezScriptComponent@ GetOwnerComponent();
    ezGameObject@ GetOwner();
    ezWorld@ GetWorld();
    void SetUpdateInterval(ezTime interval);

    // Functions to override:
    // void OnActivated();
    // void OnDeactivated();
    // void OnSimulationStarted();
    // void Update();
}
    )";

    out_sContent.Append(szClassCode);
  }

  // now all the global functions in namespaces
  {
    out_sContent.Append("\n// *** GLOBAL FUNCTIONS *** \n\n");

    for (ezUInt32 funcIdx = 0; funcIdx < pEngine->GetGlobalFunctionCount(); ++funcIdx)
    {
      const asIScriptFunction* pFunc = pEngine->GetGlobalFunctionByIndex(funcIdx);

      sIndent = DealWithNamespace(sNamespace, pFunc->GetNamespace(), out_sContent);

      out_sContent.Append(sIndent, ezAngelScriptUtils::GetNiceFunctionDeclaration(pFunc), ";\n");
    }

    if (!sNamespace.IsEmpty())
    {
      out_sContent.Append("}\n\n");
      sNamespace.Clear();
    }
  }

  // now all the global properties/variables in namespaces
  {
    out_sContent.Append("\n// *** GLOBAL PROPERTIES *** \n\n");

    ezStringBuilder sNamespace;

    for (ezUInt32 idx = 0; idx < pEngine->GetGlobalPropertyCount(); ++idx)
    {
      const char* szName;
      const char* szNewNamespace;
      int typeId;
      bool isConst;
      pEngine->GetGlobalPropertyByIndex(idx, &szName, &szNewNamespace, &typeId, &isConst);

      sIndent = DealWithNamespace(sNamespace, szNewNamespace, out_sContent);

      tmp = pEngine->GetTypeDeclaration(typeId, false);

      out_sContent.Append(sIndent, tmp, " ", szName, ";\n");
    }

    if (!sNamespace.IsEmpty())
    {
      out_sContent.Append("}\n\n");
      sNamespace.Clear();
    }
  }
}
