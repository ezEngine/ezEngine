#include <Mcp/McpPCH.h>

#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>
#include <Mcp/McpTranslation.h>
#include <Mcp/Tools/McpRttiTool.h>

#include <Foundation/Reflection/ReflectionUtils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpRttiTool, 1, ezRTTIDefaultAllocator<ezMcpRttiTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// The largest number of entries any of these tools will return. A project has well over a thousand
  /// reflected types, and an unfiltered query would otherwise fill the client's context with names it
  /// did not ask for. Every listing reports the total number of matches alongside, so the agent can
  /// tell 'this is all of them' from 'narrow your filter'.
  constexpr ezUInt32 s_uiMaxResults = 200;

  bool NameMatches(const ezRTTI* pType, ezStringView sFilter)
  {
    return sFilter.IsEmpty() || pType->GetTypeName().FindSubString_NoCase(sFilter) != nullptr;
  }
} // namespace

void ezMcpRttiTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "rtti_find_types";
    desc.m_sDescription = "Searches the reflected C++ types by name and returns only the matching type names. This is the entry "
                          "point into the reflection data: narrow down to a few names here, then use rtti_type_info or "
                          "rtti_type_properties on those. Results are capped, but the total number of matches is reported.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("name":{"type":"string","description":"Only return types whose name contains this text, case insensitive. Empty returns everything, which is a lot."},)"
                          R"("derivedFrom":{"type":"string","description":"Only return types deriving from this type, e.g. 'ezComponent'."},)"
                          R"("concreteOnly":{"type":"boolean","description":"If true, exclude abstract types and types that cannot be allocated. Default false."})"
                          R"(}})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "rtti_type_info";
    desc.m_sDescription = "Returns the details of one reflected type: its parent type, the plugin it comes from, its type flags, "
                          "its attributes, and its callable functions with full signatures. Where the type has them, also the "
                          "name the editor's UI shows for it and a link to its documentation, which is the only description of "
                          "what the type is actually for. Deliberately does NOT include the property list - use "
                          "rtti_type_properties for that.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("name":{"type":"string","description":"The exact type name, e.g. 'ezMeshComponent'."})"
                          R"(},"required":["name"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "rtti_type_properties";
    desc.m_sDescription = "Returns the reflected properties of one type, with their value type, category (member, array, set, "
                          "map, constant), flags and attributes. This is what tells you the exact property names and value "
                          "types to use when reading or writing an object's properties. Properties that have one also carry "
                          "the label the editor shows and a description of what the property does - the latter exists nowhere "
                          "else, so consult it before guessing from a name.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("name":{"type":"string","description":"The exact type name, e.g. 'ezMeshComponent'."},)"
                          R"("recursive":{"type":"boolean","description":"If true, also include the properties inherited from base types. Default false, which returns only the type's own properties."})"
                          R"(},"required":["name"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "rtti_derived_types";
    desc.m_sDescription = "Returns the names of all types deriving from the given type. Use this to answer questions like which "
                          "component types exist ('ezComponent') or which types can go into a specific property.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("name":{"type":"string","description":"The exact base type name, e.g. 'ezComponent'."},)"
                          R"("concreteOnly":{"type":"boolean","description":"If true, exclude abstract types and types that cannot be allocated. Default false."})"
                          R"(},"required":["name"]})";
  }
}

void ezMcpRttiTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "rtti_find_types")
    ExecuteFindTypes(arguments, out_result);
  else if (sToolName == "rtti_type_info")
    ExecuteTypeInfo(arguments, out_result);
  else if (sToolName == "rtti_type_properties")
    ExecuteTypeProperties(arguments, out_result);
  else if (sToolName == "rtti_derived_types")
    ExecuteDerivedTypes(arguments, out_result);
}

void ezMcpRttiTool::WriteTypeFlags(ezMcpJsonWriter& ref_writer, const ezRTTI* pType)
{
  const ezBitflags<ezTypeFlags>& flags = pType->GetTypeFlags();

  ref_writer.BeginArray("flags");

  if (flags.IsSet(ezTypeFlags::StandardType))
    ref_writer.WriteString("StandardType");
  if (flags.IsSet(ezTypeFlags::IsEnum))
    ref_writer.WriteString("IsEnum");
  if (flags.IsSet(ezTypeFlags::Bitflags))
    ref_writer.WriteString("Bitflags");
  if (flags.IsSet(ezTypeFlags::Class))
    ref_writer.WriteString("Class");
  if (flags.IsSet(ezTypeFlags::Abstract))
    ref_writer.WriteString("Abstract");
  if (flags.IsSet(ezTypeFlags::Phantom))
    ref_writer.WriteString("Phantom");
  if (flags.IsSet(ezTypeFlags::Minimal))
    ref_writer.WriteString("Minimal");

  ref_writer.EndArray();
}

void ezMcpRttiTool::WritePropertyFlags(ezMcpJsonWriter& ref_writer, ezStringView sName, ezBitflags<ezPropertyFlags> flags)
{
  // An empty flag array says nothing that its absence does not, and these repeat for every argument of
  // every function - which is exactly the token cost the split-tool design exists to avoid.
  if (flags.GetValue() == 0)
    return;

  ref_writer.BeginArray(sName);

  if (flags.IsSet(ezPropertyFlags::StandardType))
    ref_writer.WriteString("StandardType");
  if (flags.IsSet(ezPropertyFlags::IsEnum))
    ref_writer.WriteString("IsEnum");
  if (flags.IsSet(ezPropertyFlags::Bitflags))
    ref_writer.WriteString("Bitflags");
  if (flags.IsSet(ezPropertyFlags::Class))
    ref_writer.WriteString("Class");
  if (flags.IsSet(ezPropertyFlags::Const))
    ref_writer.WriteString("Const");
  if (flags.IsSet(ezPropertyFlags::Reference))
    ref_writer.WriteString("Reference");
  if (flags.IsSet(ezPropertyFlags::Pointer))
    ref_writer.WriteString("Pointer");
  if (flags.IsSet(ezPropertyFlags::PointerOwner))
    ref_writer.WriteString("PointerOwner");
  if (flags.IsSet(ezPropertyFlags::ReadOnly))
    ref_writer.WriteString("ReadOnly");
  if (flags.IsSet(ezPropertyFlags::Hidden))
    ref_writer.WriteString("Hidden");
  if (flags.IsSet(ezPropertyFlags::Phantom))
    ref_writer.WriteString("Phantom");
  if (flags.IsSet(ezPropertyFlags::VarOut))
    ref_writer.WriteString("VarOut");
  if (flags.IsSet(ezPropertyFlags::VarInOut))
    ref_writer.WriteString("VarInOut");

  ref_writer.EndArray();
}

void ezMcpRttiTool::WriteAttributes(ezMcpJsonWriter& ref_writer, ezArrayPtr<const ezPropertyAttribute* const> attributes)
{
  if (attributes.IsEmpty())
    return;

  ref_writer.BeginArray("attributes");

  for (const ezPropertyAttribute* pAttr : attributes)
  {
    if (pAttr == nullptr)
      continue;

    WriteAttribute(ref_writer, pAttr);
  }

  ref_writer.EndArray();
}

const ezPropertyAttribute* ezMcpRttiTool::GetAttributeFromVariant(const ezVariant& value)
{
  if (value.GetType() != ezVariant::Type::TypedPointer)
    return nullptr;

  const ezTypedPointer ptr = value.Get<ezTypedPointer>();

  if (ptr.m_pObject == nullptr || ptr.m_pType == nullptr)
    return nullptr;

  if (!ptr.m_pType->IsDerivedFrom<ezPropertyAttribute>())
    return nullptr;

  return static_cast<const ezPropertyAttribute*>(ptr.m_pObject);
}

void ezMcpRttiTool::WriteAttribute(ezMcpJsonWriter& ref_writer, const ezPropertyAttribute* pAttr)
{
  {
    const ezRTTI* pAttrType = pAttr->GetDynamicRTTI();

    ref_writer.BeginObject();
    ref_writer.AddVariableString("type", pAttrType->GetTypeName());

    // An attribute's own reflected members carry the interesting part - the clamp range of an
    // ezClampValueAttribute, the type filter of an ezAssetBrowserAttribute. They are read generically,
    // so attributes added later show up here without this tool knowing about them.
    for (const ezAbstractProperty* pProp : pAttrType->GetProperties())
    {
      if (pProp->GetCategory() == ezPropertyCategory::Member)
      {
        const ezVariant value = ezReflectionUtils::GetMemberPropertyValue(static_cast<const ezAbstractMemberProperty*>(pProp), pAttr);

        if (value.IsValid())
        {
          ref_writer.AddVariableVariant(pProp->GetPropertyName(), value);
        }
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Array)
      {
        // Attributes such as ezContainerAttribute keep their payload in an array. Skipping those would
        // silently drop the only interesting part of the attribute.
        const ezAbstractArrayProperty* pArray = static_cast<const ezAbstractArrayProperty*>(pProp);

        const ezUInt32 uiCount = pArray->GetCount(pAttr);

        if (uiCount == 0)
          continue;

        ref_writer.BeginArray(pProp->GetPropertyName());

        for (ezUInt32 uiIndex = 0; uiIndex < uiCount; ++uiIndex)
        {
          const ezVariant value = ezReflectionUtils::GetArrayPropertyValue(pArray, pAttr, uiIndex);

          // An attribute nested inside another one - ezFunctionArgumentAttributes holds the attributes
          // of a single function argument this way. Writing it as a plain variant would report only its
          // type and drop the members, which for an ezDefaultValueAttribute is the entire content.
          if (const ezPropertyAttribute* pNested = GetAttributeFromVariant(value))
          {
            WriteAttribute(ref_writer, pNested);
          }
          else
          {
            ref_writer.WriteVariant(value);
          }
        }

        ref_writer.EndArray();
      }
    }

    ref_writer.EndObject();
  }
}

void ezMcpRttiTool::WriteProperty(ezMcpJsonWriter& ref_writer, const ezRTTI* pOwnerType, const ezAbstractProperty* pProp)
{
  ref_writer.BeginObject();

  ref_writer.AddVariableString("name", pProp->GetPropertyName());

  // The label the property grid shows, which is not always the property name, and the text behind its
  // tooltip - the only description of what the property does that exists anywhere in the data.
  ezMcpTranslation::AddOptionalString(ref_writer, "displayName", ezMcpTranslation::GetPropertyDisplayName(pOwnerType, pProp->GetPropertyName()));
  ezMcpTranslation::AddOptionalString(ref_writer, "description", ezMcpTranslation::GetPropertyTooltip(pOwnerType, pProp->GetPropertyName()));

  const ezRTTI* pPropType = pProp->GetSpecificType();
  ref_writer.AddVariableString("type", pPropType != nullptr ? pPropType->GetTypeName() : ezStringView());

  switch (pProp->GetCategory())
  {
    case ezPropertyCategory::Constant:
      ref_writer.AddVariableString("category", "constant");
      break;
    case ezPropertyCategory::Member:
      ref_writer.AddVariableString("category", "member");
      break;
    case ezPropertyCategory::Function:
      ref_writer.AddVariableString("category", "function");
      break;
    case ezPropertyCategory::Array:
      ref_writer.AddVariableString("category", "array");
      break;
    case ezPropertyCategory::Set:
      ref_writer.AddVariableString("category", "set");
      break;
    case ezPropertyCategory::Map:
      ref_writer.AddVariableString("category", "map");
      break;
    default:
      ref_writer.AddVariableString("category", "unknown");
      break;
  }

  const ezBitflags<ezPropertyFlags>& flags = pProp->GetFlags();

  WritePropertyFlags(ref_writer, "flags", flags);

  // For enum and bitflags properties the valid values are the constants of the value type, and without
  // them the agent has no way to know what may be assigned.
  if (flags.IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags) && pPropType != nullptr)
  {
    ref_writer.BeginArray("enumValues");

    for (const ezAbstractProperty* pConstant : pPropType->GetProperties())
    {
      if (pConstant->GetCategory() != ezPropertyCategory::Constant)
        continue;

      ref_writer.BeginObject();
      ref_writer.AddVariableString("name", pConstant->GetPropertyName());

      // An enum constant is registered under its fully qualified name ('ezDecalMode::BaseColorORM'),
      // which is also the translation key, so no owner type is needed here.
      ezMcpTranslation::AddOptionalString(ref_writer, "displayName", ezMcpTranslation::GetDisplayName(pConstant->GetPropertyName()));

      ref_writer.AddVariableVariant("value", static_cast<const ezAbstractConstantProperty*>(pConstant)->GetConstant());
      ref_writer.EndObject();
    }

    ref_writer.EndArray();
  }

  WriteAttributes(ref_writer, pProp->GetAttributes());

  ref_writer.EndObject();
}

void ezMcpRttiTool::WriteFunction(ezMcpJsonWriter& ref_writer, const ezAbstractFunctionProperty* pFunc)
{
  ref_writer.BeginObject();

  ref_writer.AddVariableString("name", pFunc->GetPropertyName());

  switch (pFunc->GetFunctionType())
  {
    case ezFunctionType::Member:
      ref_writer.AddVariableString("functionType", "member");
      break;
    case ezFunctionType::StaticMember:
      ref_writer.AddVariableString("functionType", "static");
      break;
    case ezFunctionType::Constructor:
      ref_writer.AddVariableString("functionType", "constructor");
      break;
    default:
      ref_writer.AddVariableString("functionType", "unknown");
      break;
  }

  // A null return type means void, which is not a reflected type of its own.
  const ezRTTI* pReturnType = pFunc->GetReturnType();
  ref_writer.AddVariableString("returnType", pReturnType != nullptr ? pReturnType->GetTypeName() : "void"_ezsv);

  if (pReturnType != nullptr)
  {
    WritePropertyFlags(ref_writer, "returnFlags", pFunc->GetReturnFlags());
  }

  // Functions exposed to scripting carry the argument names, which the reflection data itself does not
  // hold - without them the arguments would only be identifiable by position.
  const ezScriptableFunctionAttribute* pScriptable = pFunc->GetAttributeByType<ezScriptableFunctionAttribute>();

  if (pScriptable != nullptr)
  {
    ref_writer.AddVariableBool("scriptable", true);
  }

  const ezUInt32 uiArgCount = pFunc->GetArgumentCount();

  if (uiArgCount == 0)
  {
    WriteAttributes(ref_writer, pFunc->GetAttributes());
    ref_writer.EndObject();
    return;
  }

  ref_writer.BeginArray("arguments");
  for (ezUInt32 uiArg = 0; uiArg < uiArgCount; ++uiArg)
  {
    ref_writer.BeginObject();

    if (pScriptable != nullptr && uiArg < pScriptable->GetArgumentCount())
    {
      ref_writer.AddVariableString("name", pScriptable->GetArgumentName(uiArg));
    }

    const ezRTTI* pArgType = pFunc->GetArgumentType(uiArg);
    ref_writer.AddVariableString("type", pArgType != nullptr ? pArgType->GetTypeName() : ezStringView());

    // The flags say whether the argument is an out or inout parameter, which decides whether a caller
    // has to pass a value in at all.
    WritePropertyFlags(ref_writer, "flags", pFunc->GetArgumentFlags(uiArg));

    ref_writer.EndObject();
  }
  ref_writer.EndArray();

  WriteAttributes(ref_writer, pFunc->GetAttributes());

  ref_writer.EndObject();
}

void ezMcpRttiTool::ExecuteFindTypes(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sName = ezMcpJson::GetString(arguments, "name");
  const ezStringView sDerivedFrom = ezMcpJson::GetString(arguments, "derivedFrom");
  const bool bConcreteOnly = ezMcpJson::GetBool(arguments, "concreteOnly", false);

  const ezRTTI* pBaseType = nullptr;

  if (!sDerivedFrom.IsEmpty())
  {
    pBaseType = ezRTTI::FindTypeByName(sDerivedFrom);

    if (pBaseType == nullptr)
    {
      out_result.SetError(ezStringBuilder("Unknown type '", sDerivedFrom, "' passed as 'derivedFrom'. Use rtti_find_types without it to search for the correct name."));
      return;
    }
  }

  const ezBitflags<ezRTTI::ForEachOptions> options = bConcreteOnly ? ezRTTI::ForEachOptions::ExcludeNotConcrete : ezRTTI::ForEachOptions::Default;

  ezUInt32 uiTotalMatches = 0;
  ezHybridArray<ezStringView, 32> names;

  auto visitor = [&](const ezRTTI* pType)
  {
    if (!NameMatches(pType, sName))
      return;

    ++uiTotalMatches;

    if (names.GetCount() < s_uiMaxResults)
    {
      names.PushBack(pType->GetTypeName());
    }
  };

  if (pBaseType != nullptr)
  {
    ezRTTI::ForEachDerivedType(pBaseType, visitor, options);
  }
  else
  {
    ezRTTI::ForEachType(visitor, options);
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableUInt32("totalMatches", uiTotalMatches);
  writer.AddVariableUInt32("returned", names.GetCount());
  writer.AddVariableBool("truncated", uiTotalMatches > names.GetCount());

  writer.BeginArray("types");
  for (ezStringView sTypeName : names)
  {
    writer.WriteString(sTypeName);
  }
  writer.EndArray();

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpRttiTool::ExecuteTypeInfo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sName = ezMcpJson::GetString(arguments, "name");

  if (sName.IsEmpty())
  {
    out_result.SetError("Argument 'name' is required.");
    return;
  }

  const ezRTTI* pType = ezRTTI::FindTypeByName(sName);

  if (pType == nullptr)
  {
    out_result.SetError(ezStringBuilder("Unknown type '", sName, "'. Type names are case sensitive - use rtti_find_types to search for the correct name."));
    return;
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableString("name", pType->GetTypeName());

  // The two things reflection cannot express: what the user sees this type called in the editor UI, and
  // a link to prose about what it is for. Both are keyed on the type name, the same way asset types are,
  // so a component type carries them just as an asset type does.
  ezMcpTranslation::AddOptionalString(writer, "displayName", ezMcpTranslation::GetDisplayName(pType->GetTypeName()));
  ezMcpTranslation::AddOptionalString(writer, "helpUrl", ezMcpTranslation::GetHelpURL(pType->GetTypeName()));

  const ezRTTI* pParent = pType->GetParentType();
  writer.AddVariableString("parentType", pParent != nullptr ? pParent->GetTypeName() : ezStringView());

  writer.AddVariableString("plugin", pType->GetPluginName());
  writer.AddVariableUInt32("typeVersion", pType->GetTypeVersion());

  // Whether ezRTTI can construct one. False for abstract types, and for types that are only registered
  // for their property information.
  const ezRTTIAllocator* pAllocator = pType->GetAllocator();
  writer.AddVariableBool("canAllocate", pAllocator != nullptr && pAllocator->CanAllocate());

  WriteTypeFlags(writer, pType);

  // Counts rather than the lists themselves - this tool stays cheap on purpose, and the count is what
  // tells the agent whether calling rtti_type_properties is worth it.
  writer.AddVariableUInt32("numOwnProperties", pType->GetProperties().GetCount());

  ezDynamicArray<const ezAbstractProperty*> allProps;
  pType->GetAllProperties(allProps);
  writer.AddVariableUInt32("numAllProperties", allProps.GetCount());

  writer.BeginArray("functions");
  for (const ezAbstractFunctionProperty* pFunc : pType->GetFunctions())
  {
    WriteFunction(writer, pFunc);
  }
  writer.EndArray();

  WriteAttributes(writer, pType->GetAttributes());

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpRttiTool::ExecuteTypeProperties(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sName = ezMcpJson::GetString(arguments, "name");
  const bool bRecursive = ezMcpJson::GetBool(arguments, "recursive", false);

  if (sName.IsEmpty())
  {
    out_result.SetError("Argument 'name' is required.");
    return;
  }

  const ezRTTI* pType = ezRTTI::FindTypeByName(sName);

  if (pType == nullptr)
  {
    out_result.SetError(ezStringBuilder("Unknown type '", sName, "'. Type names are case sensitive - use rtti_find_types to search for the correct name."));
    return;
  }

  ezDynamicArray<const ezAbstractProperty*> properties;

  if (bRecursive)
  {
    // walks the base types as well, which is exactly what the argument asks for
    pType->GetAllProperties(properties);
  }
  else
  {
    properties = pType->GetProperties();
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableString("name", pType->GetTypeName());
  writer.AddVariableBool("recursive", bRecursive);

  writer.BeginArray("properties");
  for (const ezAbstractProperty* pProp : properties)
  {
    WriteProperty(writer, pType, pProp);
  }
  writer.EndArray();

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpRttiTool::ExecuteDerivedTypes(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sName = ezMcpJson::GetString(arguments, "name");
  const bool bConcreteOnly = ezMcpJson::GetBool(arguments, "concreteOnly", false);

  if (sName.IsEmpty())
  {
    out_result.SetError("Argument 'name' is required.");
    return;
  }

  const ezRTTI* pType = ezRTTI::FindTypeByName(sName);

  if (pType == nullptr)
  {
    out_result.SetError(ezStringBuilder("Unknown type '", sName, "'. Type names are case sensitive - use rtti_find_types to search for the correct name."));
    return;
  }

  const ezBitflags<ezRTTI::ForEachOptions> options = bConcreteOnly ? ezRTTI::ForEachOptions::ExcludeNotConcrete : ezRTTI::ForEachOptions::Default;

  ezUInt32 uiTotalMatches = 0;
  ezHybridArray<ezStringView, 32> names;

  ezRTTI::ForEachDerivedType(pType, [&](const ezRTTI* pDerived)
    {
      // ForEachDerivedType includes the base type itself, which is not what 'derived types' means here
      if (pDerived == pType)
        return;

      ++uiTotalMatches;

      if (names.GetCount() < s_uiMaxResults)
      {
        names.PushBack(pDerived->GetTypeName());
      } }, options);

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableString("baseType", pType->GetTypeName());
  writer.AddVariableUInt32("totalMatches", uiTotalMatches);
  writer.AddVariableUInt32("returned", names.GetCount());
  writer.AddVariableBool("truncated", uiTotalMatches > names.GetCount());

  writer.BeginArray("types");
  for (ezStringView sTypeName : names)
  {
    writer.WriteString(sTypeName);
  }
  writer.EndArray();

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}
