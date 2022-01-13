#include "CppStructure.h"
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Utilities/ConversionUtils.h>

ezResult CppStructure::ParseXML(ezStreamReader& stream)
{
  m_sXmlContent.ReadAll(stream);

  xml_node<>* root_node;
  m_XmlDoc.parse<0>(const_cast<char*>(m_sXmlContent.GetData()));

  root_node = m_XmlDoc.first_node("CastXML");

  ezString id;
  ezStringBuilder xmlNodeType;

  for (auto node = root_node->first_node(); node != nullptr; node = node->next_sibling())
  {
    auto pIdAttr = node->first_attribute("id", 2);

    if (pIdAttr == nullptr)
      continue;

    id = pIdAttr->value();
    xmlNodeType = node->name();

    auto& element = m_IdToElement[id];

    if (xmlNodeType == "Namespace")
    {
      element.m_Type = CodeElementType::Namespace;
      element.m_uiIndex = ParseNamespace(node);
    }
    else if (xmlNodeType == "Struct")
    {
      element.m_Type = CodeElementType::Struct;
      element.m_uiIndex = ParseStructure(node, CE_Structure::Type::Struct);
    }
    else if (xmlNodeType == "Typedef")
    {
      element.m_Type = CodeElementType::Typedef;
      element.m_uiIndex = ParseTypedef(node);
    }
    else if (xmlNodeType == "Class")
    {
      element.m_Type = CodeElementType::Class;
      element.m_uiIndex = ParseStructure(node, CE_Structure::Type::Class);
    }
    else if (xmlNodeType == "Function")
    {
      element.m_Type = CodeElementType::Function;
      element.m_uiIndex = ParseFunction(node);
    }
    else if (xmlNodeType == "Variable")
    {
      element.m_Type = CodeElementType::Variable;
      element.m_uiIndex = ParseVariable(node);
    }
    else if (xmlNodeType == "OperatorFunction")
    {
      element.m_Type = CodeElementType::OperatorFunction;
      element.m_uiIndex = ParseOperatorFunction(node);
    }
    else if (xmlNodeType == "Unimplemented")
    {
      element.m_Type = CodeElementType::Unimplemented;
      element.m_uiIndex = ParseUnimplemented(node);
    }
    else if (xmlNodeType == "Union")
    {
      element.m_Type = CodeElementType::Union;
      element.m_uiIndex = ParseStructure(node, CE_Structure::Type::Union);
    }
    else if (xmlNodeType == "CvQualifiedType")
    {
      element.m_Type = CodeElementType::CvQualifiedType;
      element.m_uiIndex = ParseCvQualifiedType(node);
    }
    else if (xmlNodeType == "Enumeration")
    {
      element.m_Type = CodeElementType::Enumeration;
      element.m_uiIndex = ParseEnumeration(node);
    }
    else if (xmlNodeType == "Constructor")
    {
      element.m_Type = CodeElementType::Constructor;
      element.m_uiIndex = ParseConstructor(node);
    }
    else if (xmlNodeType == "PointerType")
    {
      element.m_Type = CodeElementType::PointerType;
      element.m_uiIndex = ParsePointerType(node);
    }
    else if (xmlNodeType == "FundamentalType")
    {
      element.m_Type = CodeElementType::FundamentalType;
      element.m_uiIndex = ParseFundamentalType(node);
    }
    else if (xmlNodeType == "OperatorMethod")
    {
      element.m_Type = CodeElementType::OperatorMethod;
      element.m_uiIndex = ParseOperatorMethod(node);
    }
    else if (xmlNodeType == "Destructor")
    {
      element.m_Type = CodeElementType::Destructor;
      element.m_uiIndex = ParseDestructor(node);
    }
    else if (xmlNodeType == "Field")
    {
      element.m_Type = CodeElementType::Field;
      element.m_uiIndex = ParseField(node);
    }
    else if (xmlNodeType == "ElaboratedType")
    {
      element.m_Type = CodeElementType::ElaboratedType;
      element.m_uiIndex = ParseElaboratedType(node);
    }
    else if (xmlNodeType == "ReferenceType")
    {
      element.m_Type = CodeElementType::ReferenceType;
      element.m_uiIndex = ParseReferenceType(node);
    }
    else if (xmlNodeType == "Method")
    {
      element.m_Type = CodeElementType::Method;
      element.m_uiIndex = ParseMethod(node);
    }
    else if (xmlNodeType == "ArrayType")
    {
      element.m_Type = CodeElementType::ArrayType;
      element.m_uiIndex = ParseArrayType(node);
    }
    else if (xmlNodeType == "Converter")
    {
      element.m_Type = CodeElementType::Converter;
      element.m_uiIndex = ParseConverter(node);
    }
    else if (xmlNodeType == "FunctionType")
    {
      element.m_Type = CodeElementType::FunctionType;
      element.m_uiIndex = ParseFunctionType(node);
    }
    else if (xmlNodeType == "Comment")
    {
      element.m_Type = CodeElementType::Comment;
      element.m_uiIndex = ParseComment(node);
    }
    else if (xmlNodeType == "File")
    {
      element.m_Type = CodeElementType::File;
      element.m_uiIndex = ParseFile(node);
    }
    else
    {
      ezLog::Error("Unknown type '{}'", xmlNodeType);
    }
  }

  return EZ_SUCCESS;
}

ezUInt32 CppStructure::ParseNamespace(xml_node<>* node)
{
  auto& ce = m_CesNamespace.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  if (auto pAttrName = node->first_attribute("name", 4))
  {
    ce.m_sName = pAttrName->value();
  }

  ParseContext(node, ce.m_sContextID);

  // TODO: parse / split all member IDs ?

  return m_CesNamespace.GetCount() - 1;
}

ezUInt32 CppStructure::ParseTypedef(xml_node<>* node)
{
  auto& ce = m_CesTypedef.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  auto pAttrName = node->first_attribute("name", 4);
  ce.m_sName = pAttrName->value();

  auto pAttrType = node->first_attribute("type", 4);
  ce.m_sTypeID = pAttrType->value();

  ParseContext(node, ce.m_sContextID);

  return m_CesTypedef.GetCount() - 1;
}

ezUInt32 CppStructure::ParseStructure(xml_node<>* node, CE_Structure::Type type)
{
  auto& ce = m_CesStructure.ExpandAndGetRef();
  ce.m_Type = type;
  //ce.m_xmlNode = node;

  if (auto pAttrName = node->first_attribute("name", 4))
  {
    ce.m_sName = pAttrName->value();
  }

  if (auto pMembersAttr = node->first_attribute("members", 7))
  {
    ezHybridArray<ezStringView, 32> members;

    ezStringBuilder mem = pMembersAttr->value();
    mem.Split(false, members, " ");

    ce.m_Members.SetCount(members.GetCount());
    for (ezUInt32 i = 0; i < members.GetCount(); ++i)
    {
      ce.m_Members[i] = members[i];
    }
  }

  ParseContext(node, ce.m_sContextID);

  return m_CesStructure.GetCount() - 1;
}

ezUInt32 CppStructure::ParseFunction(xml_node<>* node)
{
  auto& ce = m_CesFunction.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  auto pAttrName = node->first_attribute("name", 4);
  auto pAttrRetType = node->first_attribute("returns", 7);

  ce.m_sName = pAttrName->value();
  ce.m_sReturnTypeID = pAttrRetType->value();

  ParseContext(node, ce.m_sContextID);
  ParseArguments(node->first_node(), ce.m_Arguments);

  return m_CesFunction.GetCount() - 1;
}

ezUInt32 CppStructure::ParseVariable(xml_node<>* node)
{
  auto& ce = m_CesVariable.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  auto pAttrName = node->first_attribute("name", 4);
  ce.m_sName = pAttrName->value();

  auto pAttrType = node->first_attribute("type", 4);
  ce.m_sTypeID = pAttrType->value();

  if (auto pAttr = node->first_attribute("static", 6))
    ce.m_bStatic = true;

  ParseContext(node, ce.m_sContextID);
  ParseVisibility(node, ce.m_Visibility);

  return m_CesVariable.GetCount() - 1;
}

ezUInt32 CppStructure::ParseOperatorFunction(xml_node<>* node)
{
  auto& ce = m_CesOperatorFunction.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  auto pAttrName = node->first_attribute("name", 4);
  ce.m_sName = pAttrName->value();

  auto pAttrRetType = node->first_attribute("returns", 7);
  ce.m_sReturnTypeID = pAttrRetType->value();

  ParseContext(node, ce.m_sContextID);
  ParseArguments(node->first_node(), ce.m_Arguments);

  return m_CesOperatorFunction.GetCount() - 1;
}

ezUInt32 CppStructure::ParseUnimplemented(xml_node<>* node)
{
  // no interesting data in here
  return ezMath::MaxValue<ezUInt32>();
}

ezUInt32 CppStructure::ParseCvQualifiedType(xml_node<>* node)
{
  auto& ce = m_CesCvQualifiedType.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  auto pAttrType = node->first_attribute("type", 4);
  ce.m_sTypeID = pAttrType->value();

  if (auto pAttrConst = node->first_attribute("const", 5))
    ce.m_bConst = true;
  if (auto pAttrVolatile = node->first_attribute("volatile", 8))
    ce.m_bVolatile = true;

  return m_CesCvQualifiedType.GetCount() - 1;
}

ezUInt32 CppStructure::ParseEnumeration(xml_node<>* node)
{
  auto& ce = m_CesEnumeration.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  if (auto pAttrName = node->first_attribute("name", 4))
  {
    ce.m_sName = pAttrName->value();
  }

  if (auto pAttrType = node->first_attribute("type", 4))
  {
    ce.m_sTypeID = pAttrType->value();
  }

  ParseContext(node, ce.m_sContextID);
  ParseVisibility(node, ce.m_Visibility);
  ParseEnumValues(node->first_node(), ce.m_Values);

  return m_CesEnumeration.GetCount() - 1;
}

ezUInt32 CppStructure::ParseFundamentalType(xml_node<>* node)
{
  auto& ce = m_CesFundamentalType.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  auto pAttrName = node->first_attribute("name", 4);
  ce.m_sName = pAttrName->value();

  return m_CesFundamentalType.GetCount() - 1;
}

ezUInt32 CppStructure::ParsePointerType(xml_node<>* node)
{
  auto& ce = m_CesPointerType.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  auto pAttrType = node->first_attribute("type", 4);
  ce.m_sTypeID = pAttrType->value();

  return m_CesPointerType.GetCount() - 1;
}

ezUInt32 CppStructure::ParseConstructor(xml_node<>* node)
{
  auto& ce = m_CesConstructor.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  ParseContext(node, ce.m_sContextID);
  ParseVisibility(node, ce.m_Visibility);

  if (auto pAttrConst = node->first_attribute("artificial", 10))
    ce.m_bArtifical = true;

  ParseArguments(node->first_node(), ce.m_Arguments);

  return m_CesConstructor.GetCount() - 1;
}

ezUInt32 CppStructure::ParseOperatorMethod(xml_node<>* node)
{
  auto& ce = m_CesOperatorMethod.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  auto pAttrName = node->first_attribute("name", 4);
  ce.m_sName = pAttrName->value();

  auto pAttrRetType = node->first_attribute("returns", 7);
  ce.m_sReturnTypeID = pAttrRetType->value();

  ParseContext(node, ce.m_sContextID);
  ParseVisibility(node, ce.m_Visibility);

  if (auto pAttr = node->first_attribute("const", 5))
    ce.m_bConst = true;
  if (auto pAttr = node->first_attribute("static", 6))
    ce.m_bStatic = true;
  if (auto pAttr = node->first_attribute("virtual", 7))
    ce.m_bVirtual = true;
  if (auto pAttr = node->first_attribute("overrides", 9))
    ce.m_bOverride = true;
  if (auto pAttr = node->first_attribute("artificial", 10))
    ce.m_bArtificial = true;

  ParseArguments(node->first_node(), ce.m_Arguments);

  return m_CesOperatorMethod.GetCount() - 1;
}

ezUInt32 CppStructure::ParseDestructor(xml_node<>* node)
{
  auto& ce = m_CesDestructor.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  auto pAttrName = node->first_attribute("name", 4);
  ce.m_sName = pAttrName->value();

  ParseContext(node, ce.m_sContextID);
  ParseVisibility(node, ce.m_Visibility);

  if (auto pAttrConst = node->first_attribute("artificial", 10))
    ce.m_bArtifical = true;

  return m_CesDestructor.GetCount() - 1;
}

ezUInt32 CppStructure::ParseField(xml_node<>* node)
{
  auto& ce = m_CesField.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  auto pAttrName = node->first_attribute("name", 4);
  ce.m_sName = pAttrName->value();

  auto pAttrType = node->first_attribute("type", 4);
  ce.m_sTypeID = pAttrType->value();

  ParseContext(node, ce.m_sContextID);
  ParseVisibility(node, ce.m_Visibility);

  return m_CesField.GetCount() - 1;
}

ezUInt32 CppStructure::ParseElaboratedType(xml_node<>* node)
{
  auto& ce = m_CesElaboratedType.ExpandAndGetRef();

  auto pAttrType = node->first_attribute("type", 4);
  ce.m_sTypeID = pAttrType->value();

  return m_CesElaboratedType.GetCount() - 1;
}

ezUInt32 CppStructure::ParseReferenceType(xml_node<>* node)
{
  auto& ce = m_CesReferenceType.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  auto pAttrType = node->first_attribute("type", 4);
  ce.m_sTypeID = pAttrType->value();

  return m_CesReferenceType.GetCount() - 1;
}

ezUInt32 CppStructure::ParseMethod(xml_node<>* node)
{
  auto& ce = m_CesMethod.ExpandAndGetRef();
  //ce.m_xmlNode = node;

  auto pAttrName = node->first_attribute("name", 4);
  ce.m_sName = pAttrName->value();

  auto pAttrRetType = node->first_attribute("returns", 7);
  ce.m_sReturnTypeID = pAttrRetType->value();

  ParseContext(node, ce.m_sContextID);
  ParseVisibility(node, ce.m_Visibility);

  if (auto pAttr = node->first_attribute("const", 5))
    ce.m_bConst = true;
  if (auto pAttr = node->first_attribute("static", 6))
    ce.m_bStatic = true;
  if (auto pAttr = node->first_attribute("virtual", 7))
    ce.m_bVirtual = true;
  if (auto pAttr = node->first_attribute("overrides", 9))
    ce.m_bOverride = true;

  ParseArguments(node->first_node(), ce.m_Arguments);

  return m_CesMethod.GetCount() - 1;
}

ezUInt32 CppStructure::ParseArrayType(xml_node<>* node)
{
  auto& ce = m_CesArrayType.ExpandAndGetRef();

  auto pAttrType = node->first_attribute("type", 4);
  ce.m_sTypeID = pAttrType->value();

  auto pAttrSize = node->first_attribute("max", 3);
  ezConversionUtils::StringToUInt(pAttrSize->value(), ce.m_uiSize).IgnoreResult(); // this can fail, ie. the string can be empty

  return m_CesArrayType.GetCount() - 1;
}

ezUInt32 CppStructure::ParseConverter(xml_node<>* node)
{
  return ezMath::MaxValue<ezUInt32>();
}

ezUInt32 CppStructure::ParseFunctionType(xml_node<>* node)
{
  return ezMath::MaxValue<ezUInt32>();
}

ezUInt32 CppStructure::ParseComment(xml_node<>* node)
{
  auto& ce = m_CesComment.ExpandAndGetRef();
  //ce.m_xmlNode = node;
  return m_CesComment.GetCount() - 1;
}

ezUInt32 CppStructure::ParseFile(xml_node<>* node)
{
  auto& ce = m_CesFile.ExpandAndGetRef();

  auto pAttr = node->first_attribute("name", 4);

  ezStringBuilder tmp = pAttr->value();
  tmp.MakeCleanPath();

  ce.m_sPath = tmp;

  return m_CesFile.GetCount() - 1;
}

void CppStructure::ParseArguments(xml_node<>* arg, ezDynamicArray<CE_Argument>& arguments)
{
  for (; arg != nullptr; arg = arg->next_sibling())
  {
    auto& a = arguments.ExpandAndGetRef();

    if (ezStringUtils::IsEqual(arg->name(), "Ellipsis"))
    {
      a.m_sName = "...";
      a.m_sTypeID = "...";
    }
    else
    {
      if (auto pAttrName = arg->first_attribute("name", 4))
      {
        a.m_sName = pAttrName->value();
      }

      auto pAttrType = arg->first_attribute("type", 4);
      a.m_sTypeID = pAttrType->value();

      if (auto pAttrDef = arg->first_attribute("default", 7))
      {
        a.m_sDefault = pAttrDef->value();
      }
    }
  }
}

void CppStructure::ParseEnumValues(xml_node<>* arg, ezDynamicArray<CE_EnumValue>& values)
{
  for (; arg != nullptr; arg = arg->next_sibling())
  {
    auto& a = values.ExpandAndGetRef();

    if (auto pAttrName = arg->first_attribute("name", 4))
    {
      a.m_sName = pAttrName->value();
    }
    if (auto pAttrInit = arg->first_attribute("init", 4))
    {
      a.m_sValue = pAttrInit->value();
    }
  }
}

void CppStructure::ParseVisibility(xml_node<>* node, CE_Visibility& out)
{
  if (auto pAttrVis = node->first_attribute("access", 6))
  {
    if (ezStringUtils::IsEqual(pAttrVis->value(), "public"))
    {
      out = CE_Visibility::Public;
    }
    else if (ezStringUtils::IsEqual(pAttrVis->value(), "protected"))
    {
      out = CE_Visibility::Protected;
    }
    else if (ezStringUtils::IsEqual(pAttrVis->value(), "private"))
    {
      out = CE_Visibility::Private;
    }
    else
    {
      EZ_REPORT_FAILURE("Unknown access qualifier.");
    }
  }
}

void CppStructure::ParseContext(xml_node<>* node, ezString& out)
{
  if (auto pAttr = node->first_attribute("context", 7))
  {
    out = pAttr->value();
  }
}

ezResult CppStructure::SynthesizeTemplate(const char* szTargetType, const char* szTemplateType1, const char* szTemplateType2)
{
  EZ_LOG_BLOCK("SynthesizeTemplate", szTargetType);

  CE_Structure* pC1 = nullptr;
  CE_Structure* pC2 = nullptr;

  for (auto& ce : m_CesStructure)
  {
    if (ce.m_sName == szTemplateType1)
    {
      pC1 = &ce;
    }
    if (ce.m_sName == szTemplateType2)
    {
      pC2 = &ce;
    }
  }

  if (pC1 == nullptr || pC2 == nullptr)
  {
    ezLog::Error("Couldn't find both templates '{}' and '{}'", szTemplateType1, szTemplateType2);
    return EZ_FAILURE;
  }

  PreprocessStructureForSynthesis(*pC1);
  PreprocessStructureForSynthesis(*pC2);

  if (pC1->m_Members.GetCount() != pC2->m_Members.GetCount())
  {
    ezLog::Error("Member count differs: {} / {}", pC1->m_Members.GetCount(), pC2->m_Members.GetCount());
    return EZ_FAILURE;
  }

  // register the fundamental type 'template type'
  {
    auto& ftid = m_IdToElement["_tt"];

    if (ftid.m_uiIndex == 0)
    {
      ftid.m_Type = CodeElementType::FundamentalType;
      ftid.m_uiIndex = m_CesFundamentalType.GetCount();

      auto& ft = m_CesFundamentalType.ExpandAndGetRef();
      ft.m_sName = "TYPE";
    }
  }

  switch (pC1->m_Type)
  {
    case CE_Structure::Type::Struct:
      m_IdToElement[szTargetType].m_Type = CodeElementType::Struct;
      break;
    case CE_Structure::Type::Class:
      m_IdToElement[szTargetType].m_Type = CodeElementType::Class;
      break;
    case CE_Structure::Type::Union:
      m_IdToElement[szTargetType].m_Type = CodeElementType::Union;
      break;
  }

  m_IdToElement[szTargetType].m_uiIndex = m_CesStructure.GetCount();

  CE_Structure& synth = m_CesStructure.ExpandAndGetRef();
  synth.m_Type = pC1->m_Type;
  synth.m_sContextID = pC1->m_sContextID;
  synth.m_sName = szTargetType;

  for (ezUInt32 m = 0; m < pC1->m_Members.GetCount(); ++m)
  {
    EZ_SUCCEED_OR_RETURN(SynthesizeMember(synth, pC1->m_Members[m], pC2->m_Members[m]));
  }

  return EZ_SUCCESS;
}


ezString CppStructure::RegisterSynthElement(CodeElementType type, ezUInt32 index)
{
  ezStringBuilder tmp;
  tmp.Format("_tt_{}", m_uiTtCounter++);

  m_IdToElement[tmp].m_Type = type;
  m_IdToElement[tmp].m_uiIndex = index;

  return tmp;
}

void CppStructure::PreprocessStructureForSynthesis(CE_Structure& s) const
{
  for (ezUInt32 i = 0; i < s.m_Members.GetCount(); ++i)
  {
    auto it = m_IdToElement.Find(s.m_Members[i]);

    // remove all destructors
    if (it.Value().m_Type == CodeElementType::Destructor)
    {
      s.m_Members.RemoveAtAndCopy(i);
    }
  }
}

ezResult CppStructure::SynthesizeMember(CE_Structure& synth, const ezString& m1, const ezString& m2)
{
  auto& mt1 = m_IdToElement[m1];
  auto& mt2 = m_IdToElement[m2];

  switch (mt1.m_Type)
  {
    case CodeElementType::Destructor:
    case CodeElementType::Converter:
      // all not supported
      return EZ_SUCCESS;
  }

  if (mt1.m_Type != mt2.m_Type)
  {
    ezLog::Error("Member CodeElementTypes differ: {} vs {}", (int)mt1.m_Type, (int)mt2.m_Type);
    return EZ_FAILURE;
  }

  switch (mt1.m_Type)
  {
    case CodeElementType::Enumeration:
    case CodeElementType::Constructor:
      break;

    case CodeElementType::OperatorMethod:
    {
      const auto& f1 = m_CesOperatorMethod[mt1.m_uiIndex];
      const auto& f2 = m_CesOperatorMethod[mt2.m_uiIndex];

      if (SynthesizeMethod(m_CesOperatorMethod.ExpandAndGetRef(), f1, f2).Failed())
      {
        m_CesOperatorMethod.PopBack();
        return EZ_FAILURE;
      }

      const ezString synthId = RegisterSynthElement(mt1.m_Type, m_CesOperatorMethod.GetCount() - 1);
      synth.m_Members.PushBack(synthId);

      break;
    }

    case CodeElementType::Method:
    {
      const auto& f1 = m_CesMethod[mt1.m_uiIndex];
      const auto& f2 = m_CesMethod[mt2.m_uiIndex];

      if (SynthesizeMethod(m_CesMethod.ExpandAndGetRef(), f1, f2).Failed())
      {
        m_CesMethod.PopBack();
        return EZ_FAILURE;
      }

      const ezString synthId = RegisterSynthElement(mt1.m_Type, m_CesMethod.GetCount() - 1);
      synth.m_Members.PushBack(synthId);

      break;
    }

    case CodeElementType::Field:
    {
      const auto& f1 = m_CesField[mt1.m_uiIndex];
      const auto& f2 = m_CesField[mt2.m_uiIndex];

      if (f1.m_sName != f2.m_sName)
        return EZ_FAILURE;

      ezString newType;
      EZ_SUCCEED_OR_RETURN(SynthesizeType(newType, f1.m_sTypeID, f2.m_sTypeID));

      auto& synthField = m_CesField.ExpandAndGetRef();
      synthField.m_sContextID = f1.m_sContextID;
      synthField.m_sName = f1.m_sName;
      synthField.m_sTypeID = newType;
      synthField.m_Visibility = f1.m_Visibility;

      const ezString synthId = RegisterSynthElement(CodeElementType::Field, m_CesField.GetCount() - 1);

      synth.m_Members.PushBack(synthId);

      break;
    }

    break;
    default:
      break;
  }

  return EZ_SUCCESS;
}

ezResult CppStructure::SynthesizeMethod(CE_Method& out, const CE_Method& f1, const CE_Method& f2)
{
  if (f1.m_sName != f2.m_sName)
    return EZ_FAILURE;
  if (f1.m_Arguments.GetCount() != f2.m_Arguments.GetCount())
    return EZ_FAILURE;

  ezString newType;
  EZ_SUCCEED_OR_RETURN(SynthesizeType(newType, f1.m_sReturnTypeID, f2.m_sReturnTypeID));

  out.m_sContextID = f1.m_sContextID;
  out.m_sName = f1.m_sName;
  out.m_sReturnTypeID = newType;
  out.m_Visibility = f1.m_Visibility;
  out.m_bConst = f1.m_bConst;
  out.m_bOverride = f1.m_bOverride;
  out.m_bStatic = f1.m_bStatic;
  out.m_bVirtual = f1.m_bVirtual;
  out.m_bArtificial = f1.m_bArtificial;
  out.m_Arguments.SetCount(f1.m_Arguments.GetCount());

  for (ezUInt32 arg = 0; arg < f1.m_Arguments.GetCount(); ++arg)
  {
    const auto& a1 = f1.m_Arguments[arg];
    const auto& a2 = f2.m_Arguments[arg];

    if (a1.m_sName != a2.m_sName)
      return EZ_FAILURE;

    EZ_SUCCEED_OR_RETURN(SynthesizeType(newType, a1.m_sTypeID, a2.m_sTypeID));

    out.m_Arguments[arg].m_sName = a1.m_sName;
    out.m_Arguments[arg].m_sTypeID = newType;
    out.m_Arguments[arg].m_sDefault = a1.m_sDefault;
  }

  return EZ_SUCCESS;
}

ezResult CppStructure::SynthesizeType(ezString& type, const ezString& m1, const ezString& m2)
{
  if (m1 == m2)
  {
    type = m2;
    return EZ_SUCCESS;
  }

  const auto& id1 = m_IdToElement[m1];
  const auto& id2 = m_IdToElement[m2];

  if (id1.m_Type != id2.m_Type)
    return EZ_FAILURE;

  if (id1.m_Type == CodeElementType::CvQualifiedType)
  {
    const auto& cv1 = m_CesCvQualifiedType[id1.m_uiIndex];
    const auto& cv2 = m_CesCvQualifiedType[id2.m_uiIndex];

    EZ_SUCCEED_OR_RETURN(SynthesizeType(type, cv1.m_sTypeID, cv2.m_sTypeID));

    // TODO: cache types

    auto& synthCV = m_CesCvQualifiedType.ExpandAndGetRef();
    synthCV.m_bConst = cv1.m_bConst;
    synthCV.m_bVolatile = cv1.m_bVolatile;
    synthCV.m_sTypeID = type;

    type = RegisterSynthElement(CodeElementType::CvQualifiedType, m_CesCvQualifiedType.GetCount() - 1);

    return EZ_SUCCESS;
  }

  if (id1.m_Type == CodeElementType::ReferenceType)
  {
    const auto& ref1 = m_CesReferenceType[id1.m_uiIndex];
    const auto& ref2 = m_CesReferenceType[id2.m_uiIndex];

    EZ_SUCCEED_OR_RETURN(SynthesizeType(type, ref1.m_sTypeID, ref2.m_sTypeID));

    // TODO: cache types

    auto& synthRef = m_CesReferenceType.ExpandAndGetRef();
    synthRef.m_sTypeID = type;

    type = RegisterSynthElement(CodeElementType::ReferenceType, m_CesReferenceType.GetCount() - 1);

    return EZ_SUCCESS;
  }

  if (id1.m_Type == CodeElementType::PointerType)
  {
    const auto& ptr1 = m_CesPointerType[id1.m_uiIndex];
    const auto& ptr2 = m_CesPointerType[id2.m_uiIndex];

    EZ_SUCCEED_OR_RETURN(SynthesizeType(type, ptr1.m_sTypeID, ptr2.m_sTypeID));

    // TODO: cache types

    auto& synthPtr = m_CesPointerType.ExpandAndGetRef();
    synthPtr.m_sTypeID = type;

    type = RegisterSynthElement(CodeElementType::PointerType, m_CesPointerType.GetCount() - 1);

    return EZ_SUCCESS;
  }

  if (id1.m_Type == CodeElementType::ArrayType)
  {
    const auto& ptr1 = m_CesArrayType[id1.m_uiIndex];
    const auto& ptr2 = m_CesArrayType[id2.m_uiIndex];

    if (ptr1.m_uiSize == ptr2.m_uiSize)
    {
      EZ_SUCCEED_OR_RETURN(SynthesizeType(type, ptr1.m_sTypeID, ptr2.m_sTypeID));

      // TODO: cache types

      auto& synthPtr = m_CesArrayType.ExpandAndGetRef();
      synthPtr.m_sTypeID = type;
      synthPtr.m_uiSize = ptr1.m_uiSize;

      type = RegisterSynthElement(CodeElementType::ArrayType, m_CesArrayType.GetCount() - 1);

      return EZ_SUCCESS;
    }
  }

  if (id1.m_Type == CodeElementType::Class || id1.m_Type == CodeElementType::Struct || id1.m_Type == CodeElementType::Union)
  {
    const auto& cs1 = m_CesStructure[id1.m_uiIndex];
    const auto& cs2 = m_CesStructure[id2.m_uiIndex];

    ezStringBuilder n1 = cs1.m_sName;
    ezStringBuilder n2 = cs2.m_sName;

    if (auto bracket = n1.FindSubString("<"))
    {
      n1.SetSubString_FromTo(n1.GetData(), bracket);
    }

    if (auto bracket = n2.FindSubString("<"))
    {
      n2.SetSubString_FromTo(n2.GetData(), bracket);
    }

    if (n1 == n2)
    {
      n1.Format("{}<TYPE>", n2);
      type = n1;
      return EZ_SUCCESS;
    }
  }

  type = "_tt";
  return EZ_SUCCESS;
}
