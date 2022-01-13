#include "DLangGenerator.h"
#include "CppStructure.h"
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

DLangGenerator::DLangGenerator() = default;
DLangGenerator::~DLangGenerator() = default;

bool DLangGenerator::IsAllowedDefaultArg(ezString& arg)
{
  if (arg == "nullptr")
  {
    arg = "null";
    return true;
  }

  if (arg == "true")
    return true;
  if (arg == "false")
    return true;

  {
    double val;
    if (ezConversionUtils::StringToFloat(arg, val, nullptr).Succeeded())
    {
      if (arg.EndsWith_NoCase(".f"))
      {
        ezStringBuilder tmp;
        tmp = arg;
        tmp.ReplaceLast_NoCase(".f", ".0f");
        arg = tmp;
      }

      return true;
    }
  }

  {
    ezInt32 val;
    if (ezConversionUtils::StringToInt(arg, val, nullptr).Succeeded())
      return true;
  }

  return false;
}

ezResult DLangGenerator::GenerateStructure(const char* szClassName, TargetType targetType, bool bWithSurroundings)
{
  for (const auto& c : m_pStructure->m_CesStructure)
  {
    if (c.m_sName == szClassName)
    {
      BeginStructure(c, targetType, bWithSurroundings);
      EZ_SUCCEED_OR_RETURN(WriteMembers(c));
      EndStructure(c, bWithSurroundings);
      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

ezResult DLangGenerator::GenerateEnum(const char* szEnumName)
{
  for (const auto& c : m_pStructure->m_CesEnumeration)
  {
    if (c.m_sName == szEnumName)
    {
      EZ_SUCCEED_OR_RETURN(WriteEnum(c));
    }
  }

  return EZ_SUCCESS;
}

void DLangGenerator::BeginStructure(const CE_Structure& ce, TargetType targetType, bool bWithSurroundings)
{
  m_TargetType = targetType;
  m_lastVisibility = CE_Visibility::Public;

  if (bWithSurroundings)
  {
    ezStringBuilder tmp = ce.m_sName;
    tmp.ReplaceAll("<", "(");
    tmp.ReplaceAll(">", ")");

    ezStringBuilder line;

    if (m_iSubStructure == 0)
    {
      switch (ce.m_Type)
      {
        case CE_Structure::Type::Struct:
          line.Append("extern(C++, struct) ");
          break;
        case CE_Structure::Type::Class:
          line.Append("extern(C++, class) ");
          break;
        case CE_Structure::Type::Union:
          line.Append("extern(C++) ");
          break;

          EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }

    switch (targetType)
    {
      case TargetType::Value:
        line.Append("struct ");
        break;
      case TargetType::Reference:
        line.Append("class ");
        break;
      case TargetType::Union:
        line.Append("union ");
        break;

        EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    line.Append(tmp.GetView());

    WriteIndentedLine(line);
    WriteIndentedLine("{");
  }

  Indent();
  ++m_iSubStructure;
}

void DLangGenerator::EndStructure(const CE_Structure& ce, bool bWithSurroundings)
{
  --m_iSubStructure;

  Unindent();

  if (bWithSurroundings)
  {
    WriteIndentedLine("}");
  }

  m_TargetType = TargetType::None;
}

ezResult DLangGenerator::WriteMembers(const CE_Structure& ce)
{
  if (m_iSubStructure == 1 && m_TargetType == TargetType::InterfaceStruct)
  {
    // deactivate the default constructor
    WriteIndentedLine("@disable this();");
  }

  for (const ezString& member : ce.m_Members)
  {
    auto itElem = m_pStructure->m_IdToElement.Find(member);
    if (!itElem.IsValid())
      continue;

    switch (itElem.Value().m_Type)
    {
      case CodeElementType::Method:
        EZ_SUCCEED_OR_RETURN(WriteMethod(m_pStructure->m_CesMethod[itElem.Value().m_uiIndex]));
        break;

      case CodeElementType::Field:
        EZ_SUCCEED_OR_RETURN(WriteField(m_pStructure->m_CesField[itElem.Value().m_uiIndex]));
        break;

      case CodeElementType::Destructor:
        // destructors are not supported by D
        break;

      case CodeElementType::Class:
        WriteIndentedLine("// sub-class (not implemented)");
        break;

      case CodeElementType::Constructor:
        EZ_SUCCEED_OR_RETURN(WriteConstructor(m_pStructure->m_CesConstructor[itElem.Value().m_uiIndex]));
        break;

      case CodeElementType::Converter:
        WriteIndentedLine("// converter (not implemented)");
        break;

      case CodeElementType::Enumeration:
      {
        auto& e = m_pStructure->m_CesEnumeration[itElem.Value().m_uiIndex];
        WriteVisibility(e.m_Visibility);
        EZ_SUCCEED_OR_RETURN(WriteEnum(e));
        break;
      }

      case CodeElementType::Namespace:
        WriteIndentedLine("// namespace (not implemented)");
        break;

      case CodeElementType::OperatorMethod:
      {
        auto& e = m_pStructure->m_CesOperatorMethod[itElem.Value().m_uiIndex];
        WriteIndentedLine(ezFmt("// Operator: {}", e.m_sName));
        break;
      }

      case CodeElementType::Struct:
      {
        auto& e = m_pStructure->m_CesStructure[itElem.Value().m_uiIndex];
        BeginStructure(e, TargetType::Value, true);
        EZ_SUCCEED_OR_RETURN(WriteMembers(e));
        EndStructure(e, true);
        break;
      }

      case CodeElementType::Typedef:
      {
        auto& e = m_pStructure->m_CesTypedef[itElem.Value().m_uiIndex];
        EZ_SUCCEED_OR_RETURN(WriteTypedef(e));
        break;
      }

      case CodeElementType::Union:
      {
        auto& e = m_pStructure->m_CesStructure[itElem.Value().m_uiIndex];
        BeginStructure(e, TargetType::Union, true);
        EZ_SUCCEED_OR_RETURN(WriteMembers(e));
        EndStructure(e, true);
        break;
      }

      case CodeElementType::Variable:
        EZ_SUCCEED_OR_RETURN(WriteVariable(m_pStructure->m_CesVariable[itElem.Value().m_uiIndex]));
        break;

      case CodeElementType::Unimplemented:
        WriteIndentedLine("// CastXML: unimplemented");
        break;

        EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }

  return EZ_SUCCESS;
}

ezResult DLangGenerator::WriteMethod(const CE_Method& ce)
{
  if (ce.m_Visibility != CE_Visibility::Public)
    return EZ_SUCCESS; // not an error, just skip the method

  ezStringBuilder sType;
  if (BuildType(sType, ce.m_sReturnTypeID).Failed())
  {
    WriteIndentedLine(ezFmt("// method '{}' - unsupported return type", ce.m_sName));
    return EZ_SUCCESS; // not an error, just skip the method
  }

  WriteVisibility(ce.m_Visibility);

  ezStringBuilder sLine;

  if (ce.m_bStatic)
  {
    sLine.Append("static ");
  }
  else if (!ce.m_bVirtual)
  {
    if (m_TargetType == TargetType::Reference)
    {
      sLine.Append("final ");
    }
  }
  else if (ce.m_bOverride)
  {
    sLine.Append("override ");
  }

  sLine.Append(sType, " ", ce.m_sName, "(");

  for (const auto& arg : ce.m_Arguments)
  {
    sType.Clear();
    if (BuildType(sType, arg.m_sTypeID).Failed())
    {
      WriteIndentedLine(ezFmt("// method '{}' - unsupported argument '{}'", ce.m_sName, arg.m_sName));
      return EZ_SUCCESS; // not an error, just skip the method
    }

    sLine.Append(sType, " ", arg.m_sName);

    if (!arg.m_sDefault.IsEmpty())
    {
      if (IsAllowedDefaultArg(const_cast<ezString&>(arg.m_sDefault)))
      {
        sLine.AppendFormat(" = {}, ", arg.m_sDefault);
      }
      else
      {
        sLine.AppendFormat(" /* = {} */, ", arg.m_sDefault);
      }
    }
    else
    {
      sLine.Append(", ");
    }
  }

  sLine.TrimWordEnd(", ");
  sLine.Append(")");

  if (ce.m_bConst)
    sLine.Append(" const");

  sLine.Append(";");

  WriteIndentedLine(sLine);

  return EZ_SUCCESS;
}

ezResult DLangGenerator::WriteConstructor(const CE_Constructor& ce)
{
  if (ce.m_Visibility != CE_Visibility::Public)
    return EZ_SUCCESS; // not an error, just skip the method

  if (ce.m_Arguments.IsEmpty())
    return EZ_SUCCESS; // D doesn't support C++ default constructors

  if (ce.m_bArtifical)
    return EZ_SUCCESS; // D should just generate its own

  WriteVisibility(ce.m_Visibility);

  ezStringBuilder sLine;

  sLine.Append("this(");

  ezStringBuilder sType;
  for (const auto& arg : ce.m_Arguments)
  {
    sType.Clear();
    if (BuildType(sType, arg.m_sTypeID).Failed())
    {
      WriteIndentedLine(ezFmt("// constructor - unsupported argument '{}'", arg.m_sName));
      return EZ_SUCCESS; // not an error, just skip the constructor
    }

    sLine.Append(sType, " ", arg.m_sName);

    if (!arg.m_sDefault.IsEmpty())
    {
      if (IsAllowedDefaultArg(const_cast<ezString&>(arg.m_sDefault)))
      {
        sLine.AppendFormat(" = {}, ", arg.m_sDefault);
      }
      else
      {
        sLine.AppendFormat(" /* = {} */, ", arg.m_sDefault);
      }
    }
    else
    {
      sLine.Append(", ");
    }
  }

  sLine.TrimWordEnd(", ");
  sLine.Append(")");

  sLine.Append(";");

  WriteIndentedLine(sLine);
  return EZ_SUCCESS;
}

ezResult DLangGenerator::WriteField(const CE_Field& ce)
{
  if (m_TargetType != TargetType::Value)
    return EZ_SUCCESS;

  //if (ce.m_Visibility != CE_Visibility::Public)
  //  return EZ_SUCCESS;

  ezStringBuilder type;
  if (BuildType(type, ce.m_sTypeID).Failed())
  {
    WriteIndentedLine(ezFmt("// Field '{}' has unsupported type", ce.m_sName));
    return EZ_SUCCESS;
  }

  WriteVisibility(ce.m_Visibility);

  WriteIndentedLine(ezFmt("{} {};", type, ce.m_sName));

  return EZ_SUCCESS;
}

ezResult DLangGenerator::WriteEnum(const CE_Enumeration& ce)
{
  if (!ce.m_sTypeID.IsEmpty())
  {
    ezStringBuilder type;
    BuildType(type, ce.m_sTypeID).AssertSuccess();
    WriteIndentedLine(ezFmt("enum {} : {}", ce.m_sName, type));
  }
  else
  {
    WriteIndentedLine(ezFmt("enum {}", ce.m_sName));
  }

  WriteIndentedLine("{");
  Indent();

  for (const auto& val : ce.m_Values)
  {
    WriteIndentedLine(ezFmt("{} = {},", val.m_sName, val.m_sValue));
  }

  Unindent();
  WriteIndentedLine("}");

  return EZ_SUCCESS;
}

ezResult DLangGenerator::WriteVariable(const CE_Variable& ce)
{
  if (m_TargetType != TargetType::Value)
  {
    return EZ_SUCCESS;
  }

  if (!ce.m_bStatic)
  {
    EZ_REPORT_FAILURE("Non-static variable encountered");
    return EZ_FAILURE;
  }

  ezStringBuilder tmp;
  if (BuildType(tmp, ce.m_sTypeID).Failed())
  {
    WriteIndentedLine(ezFmt("// Variable '{}' has unsupported type", ce.m_sName));
    return EZ_SUCCESS;
  }

  tmp.Prepend("extern export static __gshared ");
  tmp.Append(" ", ce.m_sName, ";");

  WriteVisibility(ce.m_Visibility);
  WriteIndentedLine(tmp);

  return EZ_SUCCESS;
}

ezResult DLangGenerator::WriteTypedef(const CE_Typedef& ce)
{
  ezStringBuilder tmp;
  if (BuildType(tmp, ce.m_sTypeID).Failed())
  {
    WriteIndentedLine(ezFmt("// Typedef '{}' has unsupported type", ce.m_sName));
    return EZ_SUCCESS;
  }

  tmp.PrependFormat("alias {} = ", ce.m_sName);
  tmp.Append(";");
  WriteIndentedLine(tmp);
  return EZ_SUCCESS;
}

ezResult DLangGenerator::BuildType(ezStringBuilder& out, const ezString& sTypeID, bool* bIsValueType /*= nullptr*/) const
{
  auto it = m_pStructure->m_IdToElement.Find(sTypeID);
  if (!it.IsValid())
    return EZ_FAILURE;

  bool bValueTypeDummy = true;
  if (bIsValueType == nullptr)
    bIsValueType = &bValueTypeDummy;

  const CodeElementType type = it.Value().m_Type;
  switch (type)
  {
    case CodeElementType::FundamentalType:
    {
      const auto& ft = m_pStructure->m_CesFundamentalType[it.Value().m_uiIndex];

      if (ft.m_sName == "void")
        out.Append("void");
      else if (ft.m_sName == "bool")
        out.Append("bool");
      else if (ft.m_sName == "signed char")
        out.Append("byte");
      else if (ft.m_sName == "unsigned char")
        out.Append("ubyte");
      else if (ft.m_sName == "char")
        out.Append("char");
      else if (ft.m_sName == "wchar_t")
        out.Append("core.stdc.stddef.wchar_t");
      else if (ft.m_sName == "short" || ft.m_sName == "signed short" || ft.m_sName == "short int")
        out.Append("short");
      else if (ft.m_sName == "unsigned short" || ft.m_sName == "short unsigned int")
        out.Append("ushort");
      else if (ft.m_sName == "int" || ft.m_sName == "signed int" || ft.m_sName == "long int")
        out.Append("int");
      else if (ft.m_sName == "unsigned int" || ft.m_sName == "unsigned" || ft.m_sName == "long unsigned int")
        out.Append("uint");
      else if (ft.m_sName == "long long int")
        out.Append("long");
      else if (ft.m_sName == "long long unsigned int")
        out.Append("ulong");
      else if (ft.m_sName == "float")
        out.Append("float");
      else if (ft.m_sName == "double" || ft.m_sName == "long double")
        out.Append("double");
      else
        out.Append(ft.m_sName.GetView());
      return EZ_SUCCESS;
    }

    case CodeElementType::CvQualifiedType:
    {
      const auto& cvt = m_pStructure->m_CesCvQualifiedType[it.Value().m_uiIndex];

      if (BuildType(out, cvt.m_sTypeID, bIsValueType).Failed())
        return EZ_FAILURE;

      if (cvt.m_bConst)
      {
        out.Prepend("const(");
        out.Append(")");
      }
      else
      {
        // TODO: volatile ? (supported in D ??)
      }
      return EZ_SUCCESS;
    }

    case CodeElementType::PointerType:
    {
      const auto& pt = m_pStructure->m_CesPointerType[it.Value().m_uiIndex];

      if (BuildType(out, pt.m_sTypeID, bIsValueType).Failed())
        return EZ_FAILURE;

      if (*bIsValueType)
      {
        out.Append("*");
      }
      else
      {
        *bIsValueType = true;
      }

      return EZ_SUCCESS;
    }

    case CodeElementType::ReferenceType:
    {
      const auto& rt = m_pStructure->m_CesReferenceType[it.Value().m_uiIndex];

      if (BuildType(out, rt.m_sTypeID, bIsValueType).Failed())
        return EZ_FAILURE;

      out.Prepend("ref ");
      return EZ_SUCCESS;
    }

    case CodeElementType::Struct:
    case CodeElementType::Class:
    case CodeElementType::Union:
    {
      const auto& st = m_pStructure->m_CesStructure[it.Value().m_uiIndex];

      if (!IsTypeWhitelisted(st.m_sName, bIsValueType))
        return EZ_FAILURE;

      ezStringBuilder tmp = st.m_sName;
      tmp.ReplaceAll("<", "!(");
      tmp.ReplaceAll(">", ")");

      out.Append(tmp.GetView());
      return EZ_SUCCESS;
    }

    case CodeElementType::Typedef:
    {
      const auto& tt = m_pStructure->m_CesTypedef[it.Value().m_uiIndex];

      if (IsTypeWhitelisted(tt.m_sName, nullptr))
      {
        out.Append(tt.m_sName.GetView());
        return EZ_SUCCESS;
      }

      return BuildType(out, tt.m_sTypeID, bIsValueType);
    }

    case CodeElementType::Enumeration:
    {
      const auto& st = m_pStructure->m_CesEnumeration[it.Value().m_uiIndex];

      if (!IsTypeWhitelisted(st.m_sName, nullptr))
        return EZ_FAILURE;

      out.Append(st.m_sName.GetView());
      return EZ_SUCCESS;
    }

    case CodeElementType::ArrayType:
    {
      const auto& st = m_pStructure->m_CesArrayType[it.Value().m_uiIndex];

      if (BuildType(out, st.m_sTypeID, bIsValueType).Failed())
        return EZ_FAILURE;

      if (st.m_uiSize != ezInvalidIndex)
      {
        out.AppendFormat("[{}]", st.m_uiSize + 1);
      }
      else
      {
        EZ_ASSERT_NOT_IMPLEMENTED;
      }

      return EZ_SUCCESS;
    }

    case CodeElementType::FunctionType:
      return EZ_FAILURE;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return EZ_FAILURE;
}

void DLangGenerator::WriteVisibility(CE_Visibility vis)
{
  if (vis == m_lastVisibility)
    return;

  m_lastVisibility = vis;

  switch (vis)
  {
    case CE_Visibility::Public:
      WriteIndentedLine("public:", -1);
      break;
    case CE_Visibility::Protected:
      WriteIndentedLine("protected:", -1);
      break;
    case CE_Visibility::Private:
      WriteIndentedLine("private:", -1);
      break;
  }
}

void DLangGenerator::SetStructure(const CppStructure& structure)
{
  m_pStructure = &structure;
}

void DLangGenerator::ClearOutput()
{
  m_sOutput.Clear();
}

void DLangGenerator::WriteIndentation(ezInt32 iIndentOffset)
{
  for (ezInt32 i = 0; i < m_iIndentation + iIndentOffset; ++i)
  {
    m_sOutput.Append("  ");
  }
}

void DLangGenerator::EndLine()
{
  m_sOutput.Append("\n");
}

void DLangGenerator::WriteIndentedLine(const ezFormatString& fmt, ezInt32 iIndentOffset)
{
  WriteIndentation(iIndentOffset);
  m_sOutput.AppendFormat(fmt);
  EndLine();
}

void DLangGenerator::WhitelistType(const char* szTypeName, TargetType type)
{
  if (!m_WhitelistedTypes.Contains(szTypeName))
  {
    m_WhitelistedTypes[szTypeName] = type;
  }
}

bool DLangGenerator::IsTypeWhitelisted(const ezString& type, bool* bIsValueType) const
{
  if (m_WhitelistedTypes.Contains(type))
  {
    if (bIsValueType)
    {
      auto tt = m_WhitelistedTypes.Find(type).Value();
      *bIsValueType = (tt != TargetType::Reference);
    }

    return true;
  }

  return false;
}
