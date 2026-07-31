#include <EditorPluginMcp/EditorPluginMcpPCH.h>

#include <EditorPluginMcp/McpDocument.h>
#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>
#include <EditorPluginMcp/McpTools/ObjectTool.h>

#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpObjectTool, 1, ezRTTIDefaultAllocator<ezMcpObjectTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// How many objects one object_tree call may report before it says it truncated.
  constexpr ezUInt32 c_uiMaxTreeObjects = 200;

  ezStringView ObjectToolCategoryToString(ezPropertyCategory::Enum category)
  {
    // Deliberately the same words rtti_type_properties reports, so an agent that looked a type up
    // does not have to map one vocabulary onto another.
    switch (category)
    {
      case ezPropertyCategory::Constant: return "constant";
      case ezPropertyCategory::Member: return "member";
      case ezPropertyCategory::Function: return "function";
      case ezPropertyCategory::Array: return "array";
      case ezPropertyCategory::Set: return "set";
      case ezPropertyCategory::Map: return "map";
      default: return "unknown";
    }
  }

  /// Reads the components of a vector, quaternion or colour out of whatever shape the client sent.
  ///
  /// Accepts the object form this tool reports values in ({"x":1,"y":2,"z":3}, {"r":..,"g":..}) and a
  /// plain array in component order. Returns false when the value is not one of those, which is not an
  /// error - the caller then tries the ordinary conversion.
  /// \param fDefaultAlpha Used when a colour is given without its alpha. It is the opaque value in the
  ///        target's own units, which is 1 for ezColor but 255 for the byte based ezColorGammaUB -
  ///        getting that wrong makes a colour written without an alpha come out invisible.
  bool ObjectToolReadComponents(const ezVariant& input, ezUInt32 uiCount, bool bColor, double fDefaultAlpha, double* out_pComponents)
  {
    if (const ezVariantArray* pArray = input.IsA<ezVariantArray>() ? &input.Get<ezVariantArray>() : nullptr)
    {
      if (pArray->GetCount() != uiCount)
        return false;

      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        if (!(*pArray)[i].CanConvertTo<double>())
          return false;

        out_pComponents[i] = (*pArray)[i].ConvertTo<double>();
      }

      return true;
    }

    if (!input.IsA<ezVariantDictionary>())
      return false;

    const ezVariantDictionary& dict = input.Get<ezVariantDictionary>();

    // 'a' is optional so a colour can be given without its alpha, which is how a human writes one and
    // how most sources report one.
    const char* szVectorNames[] = {"x", "y", "z", "w"};
    const char* szColorNames[] = {"r", "g", "b", "a"};
    const char* const* szNames = bColor ? szColorNames : szVectorNames;

    for (ezUInt32 i = 0; i < uiCount; ++i)
    {
      const ezVariant* pValue = nullptr;

      if (!dict.TryGetValue(szNames[i], pValue) || !pValue->CanConvertTo<double>())
      {
        if (bColor && i == 3)
        {
          out_pComponents[i] = fDefaultAlpha;
          continue;
        }

        return false;
      }

      out_pComponents[i] = pValue->ConvertTo<double>();
    }

    return true;
  }

  /// Builds the vector, quaternion or colour a property wants, if that is what the target type is.
  bool ObjectToolCoerceComposite(const ezVariant& input, ezVariant::Type::Enum targetType, ezVariant& out_value)
  {
    // Anything already of the right type went through the ordinary path before this is reached.
    if (!input.IsA<ezVariantArray>() && !input.IsA<ezVariantDictionary>())
      return false;

    double c[4] = {0.0, 0.0, 0.0, 0.0};

    switch (targetType)
    {
      case ezVariant::Type::Vector2:
        if (!ObjectToolReadComponents(input, 2, false, 0.0, c))
          return false;
        out_value = ezVec2(static_cast<float>(c[0]), static_cast<float>(c[1]));
        return true;

      case ezVariant::Type::Vector3:
        if (!ObjectToolReadComponents(input, 3, false, 0.0, c))
          return false;
        out_value = ezVec3(static_cast<float>(c[0]), static_cast<float>(c[1]), static_cast<float>(c[2]));
        return true;

      case ezVariant::Type::Vector4:
        if (!ObjectToolReadComponents(input, 4, false, 0.0, c))
          return false;
        out_value = ezVec4(static_cast<float>(c[0]), static_cast<float>(c[1]), static_cast<float>(c[2]), static_cast<float>(c[3]));
        return true;

      case ezVariant::Type::Vector2I:
        if (!ObjectToolReadComponents(input, 2, false, 0.0, c))
          return false;
        out_value = ezVec2I32(static_cast<ezInt32>(c[0]), static_cast<ezInt32>(c[1]));
        return true;

      case ezVariant::Type::Vector3I:
        if (!ObjectToolReadComponents(input, 3, false, 0.0, c))
          return false;
        out_value = ezVec3I32(static_cast<ezInt32>(c[0]), static_cast<ezInt32>(c[1]), static_cast<ezInt32>(c[2]));
        return true;

      case ezVariant::Type::Vector4I:
        if (!ObjectToolReadComponents(input, 4, false, 0.0, c))
          return false;
        out_value = ezVec4I32(static_cast<ezInt32>(c[0]), static_cast<ezInt32>(c[1]), static_cast<ezInt32>(c[2]), static_cast<ezInt32>(c[3]));
        return true;

      case ezVariant::Type::Vector2U:
        if (!ObjectToolReadComponents(input, 2, false, 0.0, c))
          return false;
        out_value = ezVec2U32(static_cast<ezUInt32>(c[0]), static_cast<ezUInt32>(c[1]));
        return true;

      case ezVariant::Type::Vector3U:
        if (!ObjectToolReadComponents(input, 3, false, 0.0, c))
          return false;
        out_value = ezVec3U32(static_cast<ezUInt32>(c[0]), static_cast<ezUInt32>(c[1]), static_cast<ezUInt32>(c[2]));
        return true;

      case ezVariant::Type::Vector4U:
        if (!ObjectToolReadComponents(input, 4, false, 0.0, c))
          return false;
        out_value = ezVec4U32(static_cast<ezUInt32>(c[0]), static_cast<ezUInt32>(c[1]), static_cast<ezUInt32>(c[2]), static_cast<ezUInt32>(c[3]));
        return true;

      case ezVariant::Type::Quaternion:
      {
        if (!ObjectToolReadComponents(input, 4, false, 0.0, c))
          return false;

        ezQuat q;
        q.x = static_cast<float>(c[0]);
        q.y = static_cast<float>(c[1]);
        q.z = static_cast<float>(c[2]);
        q.w = static_cast<float>(c[3]);

        // A rotation assembled from four numbers the caller may have rounded is not necessarily a unit
        // quaternion, and an unnormalized one scales everything below the object.
        q.Normalize();

        out_value = q;
        return true;
      }

      case ezVariant::Type::Color:
        if (!ObjectToolReadComponents(input, 4, true, 1.0, c))
          return false;
        out_value = ezColor(static_cast<float>(c[0]), static_cast<float>(c[1]), static_cast<float>(c[2]), static_cast<float>(c[3]));
        return true;

      case ezVariant::Type::ColorGamma:
      {
        if (!ObjectToolReadComponents(input, 4, true, 255.0, c))
          return false;

        // Reported as the 0-255 bytes it is stored as, so that is what comes back in.
        out_value = ezColorGammaUB(static_cast<ezUInt8>(c[0]), static_cast<ezUInt8>(c[1]), static_cast<ezUInt8>(c[2]), static_cast<ezUInt8>(c[3]));
        return true;
      }

      default:
        return false;
    }
  }

  /// Turns a value from the tool arguments into a variant the accessor will accept for this property.
  ///
  /// The JSON parser gives every number as a double and an AI client sends numbers as strings often
  /// enough that a straight type check would reject correct intent. Enums and bitflags additionally
  /// arrive as their names, which is the only form an agent can read out of rtti_type_properties.
  ezResult ObjectToolCoerceValue(const ezVariant& input, const ezAbstractProperty* pProp, ezVariant& out_value, ezStringBuilder& out_sError)
  {
    const ezRTTI* pPropType = pProp->GetSpecificType();
    const ezBitflags<ezPropertyFlags> flags = pProp->GetFlags();

    if (flags.IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags) && pPropType != nullptr)
    {
      // A name, possibly fully qualified. Numbers are still accepted, because a round trip through
      // this tool reports the name but other sources report the value.
      if (input.IsA<ezString>() || input.IsA<ezStringView>())
      {
        const ezString sValue = input.ConvertTo<ezString>();

        ezInt64 iValue = 0;
        if (ezReflectionUtils::StringToEnumeration(pPropType, sValue, iValue))
        {
          out_value = iValue;
          return EZ_SUCCESS;
        }

        out_sError.SetFormat("'{}' is not a valid value for '{}'. 'rtti_type_properties' lists the valid names under 'enumValues'.",
          sValue, pProp->GetPropertyName());
        return EZ_FAILURE;
      }

      if (input.CanConvertTo<ezInt64>())
      {
        out_value = input.ConvertTo<ezInt64>();
        return EZ_SUCCESS;
      }

      out_sError.SetFormat("'{}' expects an enum name or a number.", pProp->GetPropertyName());
      return EZ_FAILURE;
    }

    // For a plain member the property's own type says what is wanted. Converting rather than
    // requiring an exact match is what lets 42 arrive as a double and still set an ezInt32.
    if (pPropType != nullptr && flags.IsSet(ezPropertyFlags::StandardType))
    {
      const ezVariant::Type::Enum targetType = pPropType->GetVariantType();

      // A vector or colour arrives as the object it was reported as ({"x":1,"y":2,"z":3}) or as an
      // array, and ezVariant converts neither. Without this, reading a position and writing it back -
      // the most ordinary thing to do with a transform - fails on the value this tool itself produced.
      if (ObjectToolCoerceComposite(input, targetType, out_value))
        return EZ_SUCCESS;

      // An ezStringView property must still be written as an ezString: a variant that goes into the
      // command history has to own its value, and ezSetObjectPropertyCommand asserts on one that does
      // not. Asset reference properties are declared this way - ezPrefabReferenceComponent::Prefab
      // among them - so this is the ordinary case, not an exotic one.
      if (targetType == ezVariant::Type::StringView)
      {
        if (!input.CanConvertTo<ezString>())
        {
          out_sError.SetFormat("The value given for '{}' cannot be converted to a string.", pProp->GetPropertyName());
          return EZ_FAILURE;
        }

        out_value = input.ConvertTo<ezString>();
        return EZ_SUCCESS;
      }

      if (targetType != ezVariant::Type::Invalid && input.GetType() != targetType)
      {
        if (!input.CanConvertTo(targetType))
        {
          out_sError.SetFormat("The value given for '{}' cannot be converted to {}.", pProp->GetPropertyName(), pPropType->GetTypeName());
          return EZ_FAILURE;
        }

        ezResult conversion = EZ_FAILURE;
        out_value = input.ConvertTo(targetType, &conversion);

        if (conversion.Failed())
        {
          out_sError.SetFormat("The value given for '{}' could not be converted to {}.", pProp->GetPropertyName(), pPropType->GetTypeName());
          return EZ_FAILURE;
        }

        return EZ_SUCCESS;
      }
    }

    out_value = input;
    return EZ_SUCCESS;
  }

  /// Reads the 'value' argument, which unlike the others may legitimately be of any type.
  const ezVariant* ObjectToolGetRawValue(const ezVariantDictionary& arguments, ezStringView sKey)
  {
    const ezVariant* pValue = nullptr;
    if (arguments.TryGetValue(sKey, pValue))
      return pValue;

    return nullptr;
  }

  /// Checks that an index actually addresses an existing element.
  ///
  /// The accessors hand the index straight to reflection without validating it, where an out of range
  /// array index asserts - which kills the editor rather than failing the call. So every index that
  /// reaches a container has to be checked here first. 'bAllowEnd' is for insertion, where appending
  /// one past the last element is the normal case.
  ezResult ObjectToolValidateIndex(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp,
    const ezVariant& index, bool bAllowEnd, ezStringBuilder& out_sError)
  {
    const ezPropertyCategory::Enum category = pProp->GetCategory();

    if (category == ezPropertyCategory::Map)
    {
      // A map key that does not exist is a lookup miss rather than an out of bounds access, but the
      // same reasoning applies: report it here instead of finding out deeper down.
      if (bAllowEnd)
        return EZ_SUCCESS;

      ezDynamicArray<ezVariant> keys;
      if (pAccessor->GetKeysByName(pObject, pProp->GetPropertyName(), keys).Failed())
      {
        out_sError.SetFormat("Could not read the keys of '{}', so a key cannot be used with it safely.", pProp->GetPropertyName());
        return EZ_FAILURE;
      }

      const ezString sKey = index.ConvertTo<ezString>();

      for (const ezVariant& key : keys)
      {
        if (key.ConvertTo<ezString>() == sKey)
          return EZ_SUCCESS;
      }

      out_sError.SetFormat("'{}' has no key '{}'. 'object_properties' lists the keys it has.", pProp->GetPropertyName(), sKey);
      return EZ_FAILURE;
    }

    if (category != ezPropertyCategory::Array && category != ezPropertyCategory::Set)
      return EZ_SUCCESS;

    ezInt32 iCount = 0;
    if (pAccessor->GetCountByName(pObject, pProp->GetPropertyName(), iCount).Failed())
    {
      // Refusing rather than passing the index through: the reflection layer asserts on an out of
      // range index, and an assert is a dead editor. Without a count there is no way to know the
      // index is safe, so the only correct answer is not to try.
      out_sError.SetFormat("Could not determine how many elements '{}' has, so an index cannot be used with it safely.",
        pProp->GetPropertyName());
      return EZ_FAILURE;
    }

    const ezInt64 iIndex = index.ConvertTo<ezInt64>();
    const ezInt64 iLimit = bAllowEnd ? iCount : iCount - 1;

    if (iIndex < 0 || iIndex > iLimit)
    {
      if (iCount == 0)
      {
        out_sError.SetFormat("'{}' is empty, so there is no element {}.", pProp->GetPropertyName(), iIndex);
      }
      else
      {
        out_sError.SetFormat("Index {} is out of range for '{}', which has {} element(s), so the valid range is 0 to {}.",
          iIndex, pProp->GetPropertyName(), iCount, iLimit);
      }

      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  /// Turns the 'index' argument into what the accessor expects: an ezUInt32 for arrays and sets, a
  /// string key for maps. Returns an invalid variant when no index was given.
  ezVariant ObjectToolGetIndex(const ezVariantDictionary& arguments, ezPropertyCategory::Enum category)
  {
    const ezVariant* pIndex = ObjectToolGetRawValue(arguments, "index");

    if (pIndex == nullptr || !pIndex->IsValid())
      return ezVariant();

    if (category == ezPropertyCategory::Map)
      return ezVariant(pIndex->ConvertTo<ezString>());

    if (pIndex->CanConvertTo<ezUInt32>())
      return ezVariant(pIndex->ConvertTo<ezUInt32>());

    return ezVariant();
  }
} // namespace

void ezMcpObjectTool::WritePropertyValue(ezMcpJsonWriter& ref_writer, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject,
  const ezAbstractProperty* pProp, bool bIncludeValues)
{
  ref_writer.BeginObject();
  ref_writer.AddVariableString("name", pProp->GetPropertyName());

  const ezRTTI* pPropType = pProp->GetSpecificType();
  ref_writer.AddVariableString("type", pPropType != nullptr ? pPropType->GetTypeName() : ezStringView());

  const ezPropertyCategory::Enum category = pProp->GetCategory();
  ref_writer.AddVariableString("category", ObjectToolCategoryToString(category));

  const ezBitflags<ezPropertyFlags> flags = pProp->GetFlags();

  // Only the flags that change what a caller may do with the property. The full set is available from
  // rtti_type_properties and would be repeated noise on every value.
  if (flags.IsSet(ezPropertyFlags::ReadOnly))
    ref_writer.AddVariableBool("readOnly", true);

  if (flags.IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
    ref_writer.AddVariableBool("isEnum", true);

  if (!bIncludeValues)
  {
    ref_writer.EndObject();
    return;
  }

  switch (category)
  {
    case ezPropertyCategory::Member:
    {
      // A member whose value is an embedded object reports that object's guid, not its contents: the
      // tree is walked with follow up calls, so one response stays bounded regardless of nesting.
      if (flags.IsAnySet(ezPropertyFlags::Class | ezPropertyFlags::Pointer) && !flags.IsSet(ezPropertyFlags::StandardType))
      {
        const ezDocumentObject* pChild = pAccessor->GetChildObjectByName(pObject, pProp->GetPropertyName(), ezVariant());

        if (pChild != nullptr)
        {
          ezStringBuilder sChildGuid;
          ezConversionUtils::ToString(pChild->GetGuid(), sChildGuid);
          ref_writer.AddVariableString("objectGuid", sChildGuid);
          ref_writer.AddVariableString("objectType", pChild->GetType() != nullptr ? pChild->GetType()->GetTypeName() : ezStringView());
        }
        else
        {
          // Distinguishes 'the pointer is null' from 'this property has no value', which for an
          // optional sub object is the interesting difference.
          ref_writer.AddVariableBool("null", true);
        }

        break;
      }

      ezVariant value;
      const ezStatus res = pAccessor->GetValueByName(pObject, pProp->GetPropertyName(), value);

      if (res.Failed())
      {
        ref_writer.AddVariableString("error", res.GetMessageString());
        break;
      }

      // An enum's number means nothing to a reader, and the name is what has to be passed back in.
      if (flags.IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags) && pPropType != nullptr && value.CanConvertTo<ezInt64>())
      {
        ezStringBuilder sName;
        if (ezReflectionUtils::EnumerationToString(pPropType, value.ConvertTo<ezInt64>(), sName, ezReflectionUtils::EnumConversionMode::ValueNameOnly))
        {
          ref_writer.AddVariableString("value", sName);
          break;
        }
      }

      ref_writer.BeginVariable("value");
      ref_writer.WriteVariant(value);
      ref_writer.EndVariable();
      break;
    }

    case ezPropertyCategory::Array:
    case ezPropertyCategory::Set:
    {
      ezInt32 iCount = 0;
      if (pAccessor->GetCountByName(pObject, pProp->GetPropertyName(), iCount).Succeeded())
        ref_writer.AddVariableInt32("count", iCount);

      break;
    }

    case ezPropertyCategory::Map:
    {
      ezDynamicArray<ezVariant> keys;
      if (pAccessor->GetKeysByName(pObject, pProp->GetPropertyName(), keys).Succeeded())
      {
        ref_writer.AddVariableInt32("count", keys.GetCount());

        // Keys, unlike elements, are what a caller needs in order to ask for anything at all.
        ref_writer.BeginArray("keys");
        for (const ezVariant& key : keys)
        {
          ref_writer.WriteString(key.ConvertTo<ezString>());
        }
        ref_writer.EndArray();
      }
      break;
    }

    default:
      break;
  }

  ref_writer.EndObject();
}

void ezMcpObjectTool::WriteTreeNode(ezMcpJsonWriter& ref_writer, const ezDocumentObject* pObject, ezUInt32 uiRemainingDepth, ezUInt32& ref_uiBudget)
{
  ref_writer.BeginObject();

  ezStringBuilder sGuid;
  ezConversionUtils::ToString(pObject->GetGuid(), sGuid);
  ref_writer.AddVariableString("guid", sGuid);

  const ezString sName = ezMcpDocument::GetObjectName(pObject);
  if (!sName.IsEmpty())
    ref_writer.AddVariableString("name", sName);

  const ezRTTI* pType = pObject->GetType();
  ref_writer.AddVariableString("type", pType != nullptr ? pType->GetTypeName() : ezStringView());

  // Which property of the parent this object sits in. A game object's children and its components are
  // both children in this hierarchy and are told apart by nothing else. Written only when it is not
  // 'Children', which is the overwhelming majority and would otherwise repeat on every single node.
  const ezStringView sParentProperty = pObject->GetParentProperty();
  if (!sParentProperty.IsEmpty() && sParentProperty != "Children")
    ref_writer.AddVariableString("parentProperty", sParentProperty);

  const ezHybridArray<ezDocumentObject*, 8>& children = pObject->GetChildren();
  ref_writer.AddVariableUInt32("numChildren", children.GetCount());

  if (uiRemainingDepth > 0 && !children.IsEmpty())
  {
    ref_writer.BeginArray("children");

    for (const ezDocumentObject* pChild : children)
    {
      if (ref_uiBudget == 0)
        break;

      --ref_uiBudget;
      WriteTreeNode(ref_writer, pChild, uiRemainingDepth - 1, ref_uiBudget);
    }

    ref_writer.EndArray();
  }

  ref_writer.EndObject();
}

void ezMcpObjectTool::ExecuteTree(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sDocument = ezMcpJson::GetString(arguments, "document");

  ezDocument* pDocument = ezMcpDocument::Find(sDocument);

  if (pDocument == nullptr)
  {
    ezMcpDocument::SetNotOpenError(out_result, sDocument);
    return;
  }

  const ezDocumentObjectManager* pManager = pDocument->GetObjectManager();
  const ezStringView sObject = ezMcpJson::GetString(arguments, "object");

  // Unlike the other tools here, an empty 'object' does not mean the single top level object: a scene
  // has many, and listing them is the reason this tool exists. The root is not reported itself - it is
  // an implementation detail with no type a caller could do anything with.
  const ezDocumentObject* pRoot = pManager->GetRootObject();

  if (!sObject.IsEmpty())
  {
    ezStringBuilder sError;
    pRoot = ezMcpDocument::ResolveObject(pDocument, sObject, sError);

    if (pRoot == nullptr)
    {
      out_result.SetError(sError);
      return;
    }
  }

  if (pRoot == nullptr)
  {
    out_result.SetError("The document has no object hierarchy.");
    return;
  }

  // Clamped rather than rejected: a depth of 500 is a clumsy way of saying 'all of it', and the object
  // cap bounds the response either way.
  const ezInt64 iDepth = ezMcpJson::GetInt(arguments, "depth", 2);
  const ezUInt32 uiDepth = static_cast<ezUInt32>(ezMath::Clamp<ezInt64>(iDepth, 0, 20));

  ezUInt32 uiBudget = c_uiMaxTreeObjects;

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableString("document", pDocument->GetDocumentPath());
  writer.AddVariableUInt32("depth", uiDepth);

  if (sObject.IsEmpty())
  {
    // Walking the root's children rather than the root itself, so the same shape comes back whether the
    // document has one top level object or a hundred.
    writer.BeginArray("objects");

    for (const ezDocumentObject* pChild : pRoot->GetChildren())
    {
      if (uiBudget == 0)
        break;

      --uiBudget;
      WriteTreeNode(writer, pChild, uiDepth, uiBudget);
    }

    writer.EndArray();
  }
  else
  {
    writer.BeginArray("objects");
    --uiBudget;
    WriteTreeNode(writer, pRoot, uiDepth, uiBudget);
    writer.EndArray();
  }

  writer.AddVariableUInt32("returned", c_uiMaxTreeObjects - uiBudget);

  // A hierarchy cut off by the cap and one cut off by 'depth' look the same from the outside, so both
  // are reported: 'numChildren' on a node without 'children' says where to continue either way.
  writer.AddVariableBool("truncated", uiBudget == 0);

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpObjectTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "object_tree";
    desc.m_sDescription =
      "Lists the objects inside an open document as a hierarchy: guid, name, type and child count per object. This is where the "
      "object guids that 'object_properties' and 'object_modify' take come from, and the only way to find anything in a scene, "
      "whose objects have no other identifier. Returns the document's top level objects with no 'object' argument, or the subtree "
      "below the given one. "
      "Components appear as children of their game object, marked by 'parentProperty':'Components'. That field is omitted for an "
      "ordinary child object, so its absence means a child in the scene hierarchy. The response is capped at 200 objects; an "
      "object whose children were not included still reports how many it has, so descend with another call using its guid rather "
      "than asking for a large 'depth' at once. "
      "The document has to be open - use 'document_open' with 'focus' false to open it without a window.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open document. 'document_list' reports both."},)"
                          R"("object":{"type":"string","description":"Guid of the object to start at. Omit to list the document's top level objects."},)"
                          R"("depth":{"type":"number","description":"How many levels of children to include. 0 lists only the objects themselves with their child counts. Defaults to 2."})"
                          R"(},"required":["document"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "object_properties";
    desc.m_sDescription =
      "Reads the properties of one object inside an open document, with their current values. With no 'object' argument this is the "
      "document's top level object, which for most asset types is the single object holding all of the asset's settings - so reading "
      "an asset's configuration is one call with just 'document'. "
      "Only direct properties are returned: a property whose value is another object reports that object's guid under 'objectGuid', "
      "and arrays, sets and maps report a count (and, for maps, their keys) rather than their elements. Pass 'property' with an "
      "'index' to read one element. The document has to be open - use 'document_open' with 'focus' false to open it invisibly.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open document. 'document_list' reports both."},)"
                          R"("object":{"type":"string","description":"Guid of the object to read, as reported by a previous call. Omit for the document's top level object."},)"
                          R"("property":{"type":"string","description":"Only read this one property. Combine with 'index' to read a single container element."},)"
                          R"("index":{"description":"Element to read from an array or set (a number) or from a map (the key). Only valid together with 'property'."},)"
                          R"("includeValues":{"type":"boolean","description":"If false, only report the property names and types, not their values. Defaults to true."})"
                          R"(},"required":["document"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "object_modify";
    desc.m_sDescription =
      "Changes a property of an object inside an open document. Each call is one undoable step, exactly as if a user had made the "
      "change in the editor, and shows up in the undo menu labelled as coming from MCP. "
      "The document is NOT saved - call 'document_save' when the change is meant to be permanent, which is also what makes it "
      "visible to the asset system. Undo it with 'object_undo'. "
      "Operations: 'set' writes a value (for an array or map element, pass 'index'); 'insert' adds a value to a container; 'remove' "
      "deletes a container element; 'move' reorders one; 'addObject' creates an object of a given type inside a property and "
      "'removeObject' deletes one by index. Enum properties take the value name, as reported by 'rtti_type_properties'. "
      "Three operations act on the object named by 'object' rather than on a property, which is what to use with a guid from "
      "'object_tree': 'deleteObject' deletes it and everything below it, 'moveObject' moves it under 'newParent', and "
      "'duplicateObject' copies it, with its children and components, into 'newParent' (its own parent by default). "
      "This is also how a scene is built: 'addObject' on a game object with property 'Children' and type 'ezGameObject' creates a "
      "child object, and with property 'Components' and a component type name ('rtti_derived_types' of 'ezComponent' lists them) "
      "adds a component to it. New guids come back as 'addedObject'/'addedObjects'; set their properties with further calls. "
      "Vectors and colours are written the way they are reported ({\"x\":1,\"y\":2,\"z\":3} or [1,2,3]).";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open document."},)"
                          R"("object":{"type":"string","description":"Guid of the object to modify. Omit for the document's top level object."},)"
                          R"("property":{"type":"string","description":"Name of the property to change. See 'object_properties'."},)"
                          R"("operation":{"type":"string","enum":["set","insert","remove","move","addObject","removeObject","deleteObject","moveObject","duplicateObject"],"description":"What to do. Defaults to 'set'. 'deleteObject', 'moveObject' and 'duplicateObject' act on 'object' itself and need no 'property'."},)"
                          R"("newParent":{"type":"string","description":"Guid of the object to move or duplicate into, for 'moveObject' and 'duplicateObject'. Defaults to the object's current parent, which for 'moveObject' means reordering it among its siblings."},)"
                          R"("value":{"description":"The new value, for 'set' and 'insert'. Enums take the value name as a string."},)"
                          R"("index":{"description":"Which element: a number for arrays and sets, the key for maps. Required for 'remove' and for 'set' on a container."},)"
                          R"("newIndex":{"description":"Where to move the element to, for 'move'."},)"
                          R"("type":{"type":"string","description":"The type to instantiate, for 'addObject'. Must be compatible with the property."})"
                          R"(},"required":["document"]})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "object_undo";
    desc.m_sDescription =
      "Undoes or redoes changes in an open document, and reports what is on its undo stack. This is the same stack the user's undo "
      "menu shows, so it also undoes changes the user made. The stack lives on the open document and is lost when the document is "
      "closed. Call with no 'action' to only look.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("document":{"type":"string","description":"Guid or path of an open document."},)"
                          R"("action":{"type":"string","enum":["undo","redo"],"description":"Omit to only report the current stack."},)"
                          R"("count":{"type":"number","description":"How many steps to undo or redo. Defaults to 1."})"
                          R"(},"required":["document"]})";
  }
}

void ezMcpObjectTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "object_tree")
    ExecuteTree(arguments, out_result);
  else if (sToolName == "object_properties")
    ExecuteReadProperties(arguments, out_result);
  else if (sToolName == "object_modify")
    ExecuteModify(arguments, out_result);
  else if (sToolName == "object_undo")
    ExecuteUndo(arguments, out_result);
}

void ezMcpObjectTool::ExecuteReadProperties(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sDocument = ezMcpJson::GetString(arguments, "document");

  ezDocument* pDocument = ezMcpDocument::Find(sDocument);

  if (pDocument == nullptr)
  {
    ezMcpDocument::SetNotOpenError(out_result, sDocument);
    return;
  }

  ezStringBuilder sError;
  const ezDocumentObject* pObject = ezMcpDocument::ResolveObject(pDocument, ezMcpJson::GetString(arguments, "object"), sError);

  if (pObject == nullptr)
  {
    out_result.SetError(sError);
    return;
  }

  ezObjectAccessorBase* pAccessor = pDocument->GetObjectAccessor();

  if (pAccessor == nullptr)
  {
    out_result.SetError("The document has no object accessor, so its properties cannot be read.");
    return;
  }

  const ezRTTI* pType = pObject->GetType();

  if (pType == nullptr)
  {
    out_result.SetError("The object has no type information, so its properties cannot be read.");
    return;
  }

  const bool bIncludeValues = ezMcpJson::GetBool(arguments, "includeValues", true);
  const ezStringView sSingleProperty = ezMcpJson::GetString(arguments, "property");

  // Everything that can fail is resolved before the writer exists. ~ezStandardJSONWriter asserts if
  // the stream was not closed properly, so returning early from between BeginObject() and EndObject()
  // kills the editor rather than failing the call - see Status.md.
  const ezAbstractProperty* pSingleProp = nullptr;
  ezVariant singleIndex;

  if (!sSingleProperty.IsEmpty())
  {
    pSingleProp = pAccessor->FindPropertyByName(pObject, sSingleProperty);

    if (pSingleProp == nullptr)
    {
      ezStringBuilder s;
      s.SetFormat("'{}' has no property '{}'. Call without 'property' to list the ones it has.", pType->GetTypeName(), sSingleProperty);
      out_result.SetError(s);
      return;
    }

    singleIndex = ObjectToolGetIndex(arguments, pSingleProp->GetCategory());

    if (singleIndex.IsValid())
    {
      // An out of range index asserts inside the reflection layer, which would also take the editor
      // down, so it is rejected before anything reads through it.
      ezStringBuilder sIndexError;
      if (ObjectToolValidateIndex(pAccessor, pObject, pSingleProp, singleIndex, false, sIndexError).Failed())
      {
        out_result.SetError(sIndexError);
        return;
      }
    }
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  ezStringBuilder sObjectGuid;
  ezConversionUtils::ToString(pObject->GetGuid(), sObjectGuid);
  writer.AddVariableString("object", sObjectGuid);
  writer.AddVariableString("objectType", pType->GetTypeName());

  if (pSingleProp != nullptr)
  {
    const ezAbstractProperty* pProp = pSingleProp;
    const ezVariant index = singleIndex;

    if (index.IsValid())
    {
      // One container element, which is the follow up call the container listing invites.
      ezVariant value;
      const ezStatus res = pAccessor->GetValueByName(pObject, sSingleProperty, value, index);

      if (res.Failed())
      {
        // Reported as a field rather than through SetError(), because the writer is already open.
        // EndAll() closes whatever is still open so the partial result is still valid JSON.
        writer.AddVariableString("property", sSingleProperty);
        writer.AddVariableString("error", res.GetMessageString());
        writer.EndAll();

        out_result.m_sText = writer.GetResult();
        out_result.m_bIsError = true;
        return;
      }

      writer.AddVariableString("property", sSingleProperty);
      writer.BeginVariable("index");
      writer.WriteVariant(index);
      writer.EndVariable();

      // An element that is an object reports its guid, matching how member properties are reported.
      if (value.IsA<ezUuid>())
      {
        const ezDocumentObject* pChild = pDocument->GetObjectManager()->GetObject(value.Get<ezUuid>());

        ezStringBuilder sChildGuid;
        ezConversionUtils::ToString(value.Get<ezUuid>(), sChildGuid);
        writer.AddVariableString("objectGuid", sChildGuid);

        if (pChild != nullptr && pChild->GetType() != nullptr)
          writer.AddVariableString("objectType", pChild->GetType()->GetTypeName());
      }
      else
      {
        writer.BeginVariable("value");
        writer.WriteVariant(value);
        writer.EndVariable();
      }

      writer.EndObject();
      out_result.m_sText = writer.GetResult();
      return;
    }

    writer.BeginArray("properties");
    WritePropertyValue(writer, pAccessor, pObject, pProp, bIncludeValues);
    writer.EndArray();
    writer.EndObject();

    out_result.m_sText = writer.GetResult();
    return;
  }

  writer.BeginArray("properties");

  ezUInt32 uiCount = 0;

  // GetAllProperties() includes inherited ones, which is right here even though this tool only reads
  // *direct* properties: 'direct' means 'not recursing into sub objects', not 'declared on this exact
  // type'. An asset's settings routinely come partly from a base class, and a caller asking what it
  // can change on this object does not care which class declared what.
  ezDynamicArray<const ezAbstractProperty*> allProperties;
  pType->GetAllProperties(allProperties);

  for (const ezAbstractProperty* pProp : allProperties)
  {
    if (pProp == nullptr)
      continue;

    // Functions are not values and constants belong to the type rather than the object.
    if (pProp->GetCategory() == ezPropertyCategory::Function || pProp->GetCategory() == ezPropertyCategory::Constant)
      continue;

    ++uiCount;
    WritePropertyValue(writer, pAccessor, pObject, pProp, bIncludeValues);
  }

  writer.EndArray();
  writer.AddVariableUInt32("count", uiCount);
  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpObjectTool::ExecuteModify(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sDocument = ezMcpJson::GetString(arguments, "document");

  ezDocument* pDocument = ezMcpDocument::Find(sDocument);

  if (pDocument == nullptr)
  {
    ezMcpDocument::SetNotOpenError(out_result, sDocument);
    return;
  }

  if (pDocument->IsReadOnly())
  {
    out_result.SetError("The document is read only and cannot be modified.");
    return;
  }

  ezStringBuilder sError;
  const ezDocumentObject* pObject = ezMcpDocument::ResolveObject(pDocument, ezMcpJson::GetString(arguments, "object"), sError);

  if (pObject == nullptr)
  {
    out_result.SetError(sError);
    return;
  }

  ezObjectAccessorBase* pAccessor = pDocument->GetObjectAccessor();

  if (pAccessor == nullptr)
  {
    out_result.SetError("The document has no object accessor, so it cannot be modified.");
    return;
  }

  const ezStringView sProperty = ezMcpJson::GetString(arguments, "property");

  // These address the object itself rather than one of its properties, because that is the form an
  // agent has: object_tree reports guids, not the index a child happens to sit at inside its parent's
  // 'Children' or 'Components'. Handled before the property lookup below, which for 'moveObject' would
  // look the property up on the wrong object - it belongs to the new parent.
  const ezStringView sEarlyOperation = ezMcpJson::GetString(arguments, "operation");

  if (sEarlyOperation == "deleteObject")
  {
    ExecuteDeleteObject(pDocument, pObject, pAccessor, out_result);
    return;
  }

  if (sEarlyOperation == "moveObject")
  {
    ExecuteMoveObject(arguments, pDocument, pObject, pAccessor, out_result);
    return;
  }

  if (sEarlyOperation == "duplicateObject")
  {
    ExecuteDuplicateObject(arguments, pDocument, pObject, out_result);
    return;
  }

  if (sProperty.IsEmpty())
  {
    out_result.SetError("No 'property' argument given. 'object_properties' lists the properties of an object.");
    return;
  }

  const ezAbstractProperty* pProp = pAccessor->FindPropertyByName(pObject, sProperty);

  if (pProp == nullptr)
  {
    ezStringBuilder s;
    s.SetFormat("'{}' has no property '{}'. 'object_properties' lists the ones it has.",
      pObject->GetType() != nullptr ? pObject->GetType()->GetTypeName() : ezStringView("<unknown>"), sProperty);
    out_result.SetError(s);
    return;
  }

  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    ezStringBuilder s;
    s.SetFormat("'{}' is read only.", sProperty);
    out_result.SetError(s);
    return;
  }

  const ezStringView sOperation = ezMcpJson::GetString(arguments, "operation", "set");
  const ezPropertyCategory::Enum category = pProp->GetCategory();
  const ezVariant index = ObjectToolGetIndex(arguments, category);

  // The label the user sees in the undo menu. Naming MCP makes it obvious which changes were an
  // agent's rather than their own.
  ezStringBuilder sTransactionName;
  sTransactionName.SetFormat("MCP: {} '{}'", sOperation, sProperty);

  ezStatus res(EZ_SUCCESS);
  ezStringBuilder sAddedObjectGuid;

  pAccessor->StartTransaction(sTransactionName);

  if (sOperation == "set" || sOperation == "insert")
  {
    const ezVariant* pRawValue = ObjectToolGetRawValue(arguments, "value");

    if (pRawValue == nullptr || !pRawValue->IsValid())
    {
      pAccessor->CancelTransaction();
      ezStringBuilder s;
      s.SetFormat("'{}' needs a 'value' argument.", sOperation);
      out_result.SetError(s);
      return;
    }

    ezVariant value;
    ezStringBuilder sCoerceError;

    if (ObjectToolCoerceValue(*pRawValue, pProp, value, sCoerceError).Failed())
    {
      pAccessor->CancelTransaction();
      out_result.SetError(sCoerceError);
      return;
    }

    if (sOperation == "set")
    {
      // A container without an index would silently do nothing useful, so say what is missing.
      if (category != ezPropertyCategory::Member && !index.IsValid())
      {
        pAccessor->CancelTransaction();
        ezStringBuilder s;
        s.SetFormat("'{}' is a {}, so 'set' needs an 'index' saying which element to write. Use 'insert' to add a new one.",
          sProperty, ObjectToolCategoryToString(category));
        out_result.SetError(s);
        return;
      }

      ezStringBuilder sIndexError;
      if (index.IsValid() && ObjectToolValidateIndex(pAccessor, pObject, pProp, index, false, sIndexError).Failed())
      {
        pAccessor->CancelTransaction();
        out_result.SetError(sIndexError);
        return;
      }

      res = pAccessor->SetValueByName(pObject, sProperty, value, index);
    }
    else
    {
      if (category == ezPropertyCategory::Member)
      {
        pAccessor->CancelTransaction();
        ezStringBuilder s;
        s.SetFormat("'{}' is a single value, not a container, so it cannot be inserted into. Use 'set'.", sProperty);
        out_result.SetError(s);
        return;
      }

      ezVariant insertIndex = index;

      if (!insertIndex.IsValid())
      {
        if (category == ezPropertyCategory::Map)
        {
          pAccessor->CancelTransaction();
          ezStringBuilder s;
          s.SetFormat("'{}' is a map, so 'insert' needs an 'index' giving the key to store the value under.", sProperty);
          out_result.SetError(s);
          return;
        }

        // Appending is what 'insert without an index' means for a caller, but the accessor takes an
        // invalid index to mean 'no position' and fails with a message about a key called '<Invalid>'.
        // Turning it into 'one past the last element' is what the caller intended.
        ezInt32 iCount = 0;
        pAccessor->GetCountByName(pObject, sProperty, iCount).IgnoreResult();
        insertIndex = static_cast<ezUInt32>(ezMath::Max(iCount, 0));
      }
      else
      {
        ezStringBuilder sIndexError;
        if (ObjectToolValidateIndex(pAccessor, pObject, pProp, insertIndex, true, sIndexError).Failed())
        {
          pAccessor->CancelTransaction();
          out_result.SetError(sIndexError);
          return;
        }
      }

      res = pAccessor->InsertValueByName(pObject, sProperty, value, insertIndex);
    }
  }
  else if (sOperation == "remove")
  {
    if (!index.IsValid())
    {
      pAccessor->CancelTransaction();
      out_result.SetError("'remove' needs an 'index' saying which element to remove.");
      return;
    }

    ezStringBuilder sIndexError;
    if (ObjectToolValidateIndex(pAccessor, pObject, pProp, index, false, sIndexError).Failed())
    {
      pAccessor->CancelTransaction();
      out_result.SetError(sIndexError);
      return;
    }

    res = pAccessor->RemoveValueByName(pObject, sProperty, index);
  }
  else if (sOperation == "move")
  {
    const ezVariant newIndex = [&]()
    {
      const ezVariant* pNewIndex = ObjectToolGetRawValue(arguments, "newIndex");

      if (pNewIndex == nullptr || !pNewIndex->IsValid())
        return ezVariant();

      if (category == ezPropertyCategory::Map)
        return ezVariant(pNewIndex->ConvertTo<ezString>());

      return pNewIndex->CanConvertTo<ezUInt32>() ? ezVariant(pNewIndex->ConvertTo<ezUInt32>()) : ezVariant();
    }();

    if (!index.IsValid() || !newIndex.IsValid())
    {
      pAccessor->CancelTransaction();
      out_result.SetError("'move' needs both 'index' and 'newIndex'.");
      return;
    }

    ezStringBuilder sIndexError;

    // The destination may be one past the end, which is how an element is moved to the back.
    if (ObjectToolValidateIndex(pAccessor, pObject, pProp, index, false, sIndexError).Failed() ||
        ObjectToolValidateIndex(pAccessor, pObject, pProp, newIndex, true, sIndexError).Failed())
    {
      pAccessor->CancelTransaction();
      out_result.SetError(sIndexError);
      return;
    }

    res = pAccessor->MoveValueByName(pObject, sProperty, index, newIndex);
  }
  else if (sOperation == "addObject")
  {
    const ezStringView sType = ezMcpJson::GetString(arguments, "type");

    if (sType.IsEmpty())
    {
      pAccessor->CancelTransaction();
      out_result.SetError("'addObject' needs a 'type' argument naming the type to create.");
      return;
    }

    const ezRTTI* pType = ezRTTI::FindTypeByName(sType);

    if (pType == nullptr)
    {
      pAccessor->CancelTransaction();
      ezStringBuilder s;
      s.SetFormat("'{}' is not a known type. 'rtti_derived_types' lists the types that derive from a given base.", sType);
      out_result.SetError(s);
      return;
    }

    // Instantiating a type the property cannot hold would either fail deep inside the accessor or
    // produce an object that cannot be serialised, so it is checked here.
    if (pProp->GetSpecificType() != nullptr && !pType->IsDerivedFrom(pProp->GetSpecificType()))
    {
      pAccessor->CancelTransaction();
      ezStringBuilder s;
      s.SetFormat("'{}' does not derive from '{}', which is what '{}' holds.", sType, pProp->GetSpecificType()->GetTypeName(), sProperty);
      out_result.SetError(s);
      return;
    }

    ezVariant addIndex = index;

    // Adding into a container without saying where means appending, same as 'insert'.
    if (!addIndex.IsValid() && (category == ezPropertyCategory::Array || category == ezPropertyCategory::Set))
    {
      ezInt32 iCount = 0;
      pAccessor->GetCountByName(pObject, sProperty, iCount).IgnoreResult();
      addIndex = static_cast<ezUInt32>(ezMath::Max(iCount, 0));
    }
    else if (addIndex.IsValid())
    {
      ezStringBuilder sIndexError;
      if (ObjectToolValidateIndex(pAccessor, pObject, pProp, addIndex, true, sIndexError).Failed())
      {
        pAccessor->CancelTransaction();
        out_result.SetError(sIndexError);
        return;
      }
    }

    // The object manager knows the rules that reflection does not: a scene refuses a component type
    // that may only exist once on an object, or one that does not belong on this kind of object at all.
    // Asking first turns a failed transaction into a message that says which rule was broken.
    const ezStatus canAdd = pDocument->GetObjectManager()->CanAdd(pType, pObject, sProperty, addIndex);

    if (canAdd.Failed())
    {
      pAccessor->CancelTransaction();
      ezStringBuilder s;
      s.SetFormat("'{}' cannot be added to '{}': {}", sType, sProperty, canAdd.GetMessageString());
      out_result.SetError(s);
      return;
    }

    ezUuid newObjectGuid;
    res = pAccessor->AddObjectByName(pObject, sProperty, addIndex, pType, newObjectGuid);

    if (res.Succeeded())
      ezConversionUtils::ToString(newObjectGuid, sAddedObjectGuid);
  }
  else if (sOperation == "removeObject")
  {
    // GetChildObjectByName() reads through GetValueByName(), so an unchecked index asserts here too.
    ezStringBuilder sIndexError;
    if (index.IsValid() && ObjectToolValidateIndex(pAccessor, pObject, pProp, index, false, sIndexError).Failed())
    {
      pAccessor->CancelTransaction();
      out_result.SetError(sIndexError);
      return;
    }

    const ezDocumentObject* pChild = pAccessor->GetChildObjectByName(pObject, sProperty, index);

    if (pChild == nullptr)
    {
      pAccessor->CancelTransaction();
      ezStringBuilder s;
      s.SetFormat("'{}' holds no object{} to remove.", sProperty, index.IsValid() ? " at that index" : "");
      out_result.SetError(s);
      return;
    }

    res = pAccessor->RemoveObject(pChild);
  }
  else
  {
    pAccessor->CancelTransaction();
    ezStringBuilder s;
    s.SetFormat("'{}' is not a valid operation. Use one of: set, insert, remove, move, addObject, removeObject.", sOperation);
    out_result.SetError(s);
    return;
  }

  if (res.Failed())
  {
    // Cancelling rather than finishing leaves no half applied step on the undo stack.
    pAccessor->CancelTransaction();

    ezStringBuilder s;
    s.SetFormat("Could not {} '{}': {}", sOperation, sProperty, res.GetMessageString());
    out_result.SetError(s);
    return;
  }

  pAccessor->FinishTransaction();

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableString("property", sProperty);
  writer.AddVariableString("operation", sOperation);

  if (!sAddedObjectGuid.IsEmpty())
    writer.AddVariableString("addedObject", sAddedObjectGuid);

  writer.AddVariableBool("modified", pDocument->IsModified());

  // Saying this on every successful change is repetitive, but an unsaved change is invisible to the
  // asset system, and an agent that assumes otherwise transforms stale data.
  writer.AddVariableString("note", "The document is not saved yet. Call 'document_save' to write it to disk.");

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpObjectTool::ExecuteDeleteObject(ezDocument* pDocument, const ezDocumentObject* pObject, ezObjectAccessorBase* pAccessor, ezMcpToolResult& out_result)
{
  const ezDocumentObjectManager* pManager = pDocument->GetObjectManager();

  // No guard against deleting the root here: ezMcpDocument::ResolveObject() refuses to hand it out, which
  // is where that case is caught for every tool at once.
  //
  // Asked before the transaction is opened: the manager refuses things the accessor would otherwise
  // fail on deep inside, and its message says why - a component the scene requires, for instance.
  const ezStatus canRemove = pManager->CanRemove(pObject);

  if (canRemove.Failed())
  {
    ezStringBuilder s;
    s.SetFormat("This object cannot be deleted: {}", canRemove.GetMessageString());
    out_result.SetError(s);
    return;
  }

  // Recorded before the object is gone, so the result can say what was deleted rather than only that
  // something was.
  const ezStringBuilder sName = ezMcpDocument::GetObjectName(pObject);
  const ezStringView sType = pObject->GetType() != nullptr ? pObject->GetType()->GetTypeName() : ezStringView();
  const ezUInt32 uiChildren = pObject->GetChildren().GetCount();

  ezStringBuilder sGuid;
  ezConversionUtils::ToString(pObject->GetGuid(), sGuid);

  pAccessor->StartTransaction("MCP: delete object");

  const ezStatus res = pAccessor->RemoveObject(pObject);

  if (res.Failed())
  {
    pAccessor->CancelTransaction();

    ezStringBuilder s;
    s.SetFormat("Could not delete the object: {}", res.GetMessageString());
    out_result.SetError(s);
    return;
  }

  pAccessor->FinishTransaction();

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableString("deletedObject", sGuid);

  if (!sName.IsEmpty())
    writer.AddVariableString("name", sName);

  writer.AddVariableString("type", sType);

  // A game object takes its children and components with it, which is the part that is easy to
  // underestimate from a guid alone.
  if (uiChildren > 0)
    writer.AddVariableUInt32("deletedChildren", uiChildren);

  writer.AddVariableBool("modified", pDocument->IsModified());
  writer.AddVariableString("note", "The document is not saved yet. Call 'document_save' to write it to disk, or 'object_undo' to take "
                                   "this back.");

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpObjectTool::ExecuteMoveObject(const ezVariantDictionary& arguments, ezDocument* pDocument, const ezDocumentObject* pObject,
  ezObjectAccessorBase* pAccessor, ezMcpToolResult& out_result)
{
  const ezDocumentObjectManager* pManager = pDocument->GetObjectManager();
  const ezStringView sNewParent = ezMcpJson::GetString(arguments, "newParent");

  // No 'newParent' means reordering within the current one, which is the other thing a move is for.
  const ezDocumentObject* pNewParent = pObject->GetParent();

  if (!sNewParent.IsEmpty())
  {
    ezStringBuilder sParentError;
    pNewParent = ezMcpDocument::ResolveObject(pDocument, sNewParent, sParentError);

    if (pNewParent == nullptr)
    {
      out_result.SetError(sParentError);
      return;
    }
  }

  if (pNewParent == nullptr)
  {
    out_result.SetError("The object has no parent to move it within, and no 'newParent' was given.");
    return;
  }

  // Keeping the object's own parent property is what makes moving a game object between parents a
  // single argument: a child stays under 'Children', a component under 'Components'.
  ezStringView sProperty = ezMcpJson::GetString(arguments, "property");

  if (sProperty.IsEmpty())
    sProperty = pObject->GetParentProperty();

  if (sProperty.IsEmpty())
  {
    out_result.SetError("Could not tell which property of the new parent to move the object into. Pass 'property', e.g. 'Children'.");
    return;
  }

  ezVariant index = ObjectToolGetIndex(arguments, ezPropertyCategory::Array);

  if (!index.IsValid())
  {
    // Append. The count is read from the new parent, so moving to the end of a different parent works
    // without the caller knowing how many children it has.
    ezInt32 iCount = 0;
    pAccessor->GetCountByName(pNewParent, sProperty, iCount).IgnoreResult();
    index = static_cast<ezUInt32>(ezMath::Max(iCount, 0));
  }

  // CanMove() is what rejects moving an object onto itself or into its own child, which would otherwise
  // detach that whole branch from the document. It also covers what CanAdd() and CanRemove() check.
  const ezStatus canMove = pManager->CanMove(pObject, pNewParent, sProperty, index);

  if (canMove.Failed())
  {
    ezStringBuilder s;
    s.SetFormat("This object cannot be moved there: {}", canMove.GetMessageString());
    out_result.SetError(s);
    return;
  }

  pAccessor->StartTransaction("MCP: move object");

  const ezStatus res = pAccessor->MoveObjectByName(pObject, pNewParent, sProperty, index);

  if (res.Failed())
  {
    pAccessor->CancelTransaction();

    ezStringBuilder s;
    s.SetFormat("Could not move the object: {}", res.GetMessageString());
    out_result.SetError(s);
    return;
  }

  pAccessor->FinishTransaction();

  ezStringBuilder sGuid, sParentGuid;
  ezConversionUtils::ToString(pObject->GetGuid(), sGuid);
  ezConversionUtils::ToString(pNewParent->GetGuid(), sParentGuid);

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableString("movedObject", sGuid);
  writer.AddVariableString("newParent", sParentGuid);
  writer.AddVariableString("property", sProperty);
  writer.AddVariableVariant("index", index);

  // A game object keeps its local transform, so it lands wherever the new parent's transform puts it -
  // not where it appeared to be before. Worth saying, because the agent cannot see the viewport.
  writer.AddVariableString("note", "The object keeps its local position and rotation, so its position in the world follows the new "
                                   "parent's transform. The document is not saved yet.");

  writer.AddVariableBool("modified", pDocument->IsModified());

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpObjectTool::ExecuteDuplicateObject(const ezVariantDictionary& arguments, ezDocument* pDocument, const ezDocumentObject* pObject,
  ezMcpToolResult& out_result)
{
  const ezStringView sNewParent = ezMcpJson::GetString(arguments, "newParent");

  const ezDocumentObject* pParent = pObject->GetParent();

  if (!sNewParent.IsEmpty())
  {
    ezStringBuilder sParentError;
    pParent = ezMcpDocument::ResolveObject(pDocument, sNewParent, sParentError);

    if (pParent == nullptr)
    {
      out_result.SetError(sParentError);
      return;
    }
  }

  if (pParent == nullptr)
  {
    // Only reachable for an object that sits in no parent at all. Checked because everything below reads
    // through it, and a null dereference here would take the editor down instead of failing the call.
    out_result.SetError("The object has no parent to duplicate it into, and no 'newParent' was given.");
    return;
  }

  ezSelectionManager* pSelection = pDocument->GetSelectionManager();

  if (pSelection == nullptr)
  {
    out_result.SetError("This document type has no selection, so nothing can be copied out of it.");
    return;
  }

  // CopySelectedObjects() works off the selection - that is the only entry point the documents
  // implement, there is no 'copy this object' call. Restored right after, so a failed copy does not
  // leave the selection moved. Note the paste below selects what it created, exactly as the editor's
  // own paste does, so the selection does not survive a successful duplicate - that is reported rather
  // than undone, because seeing the new object selected is useful to the user watching.
  const ezDeque<const ezDocumentObject*> previousSelection = pSelection->GetSelection();

  ezAbstractObjectGraph graph;
  ezStringBuilder sMimeType;

  pSelection->SetSelection(pObject);
  const bool bCopied = pDocument->CopySelectedObjects(graph, sMimeType);
  pSelection->SetSelection(previousSelection);

  if (!bCopied)
  {
    out_result.SetError("This document type does not support copying objects, so they cannot be duplicated either. Create the object "
                        "with 'addObject' and set its properties instead.");
    return;
  }

  // The paste command takes the graph as text, the same way the editor's own copy and paste do.
  ezContiguousMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);
  ezAbstractGraphDdlSerializer::Write(memoryWriter, &graph, nullptr, false);
  memoryWriter.WriteBytes("\0", 1).IgnoreResult();

  // Which children the parent had before, so the new object can be identified afterwards: the paste
  // command keeps the objects it created to itself.
  ezHybridArray<ezUuid, 16> before;
  for (const ezDocumentObject* pChild : pParent->GetChildren())
  {
    before.PushBack(pChild->GetGuid());
  }

  ezPasteObjectsCommand cmd;
  cmd.m_Parent = pParent->GetGuid();
  cmd.m_sMimeType = sMimeType;
  cmd.m_sGraphTextFormat = reinterpret_cast<const char*>(streamStorage.GetData());

  // The picked position is where the user's mouse last pointed in a viewport, which is meaningless for
  // a tool call and would drop the copy somewhere unrelated.
  cmd.m_bAllowPickedPosition = false;

  ezCommandHistory* pHistory = pDocument->GetCommandHistory();

  pHistory->StartTransaction("MCP: duplicate object");

  const ezStatus res = pHistory->AddCommand(cmd);

  if (res.Failed())
  {
    pHistory->CancelTransaction();

    ezStringBuilder s;
    s.SetFormat("Could not duplicate the object: {}", res.GetMessageString());
    out_result.SetError(s);
    return;
  }

  pHistory->FinishTransaction();

  ezMcpJsonWriter writer;
  writer.BeginObject();

  ezStringBuilder sGuid;
  ezConversionUtils::ToString(pObject->GetGuid(), sGuid);
  writer.AddVariableString("sourceObject", sGuid);

  ezConversionUtils::ToString(pParent->GetGuid(), sGuid);
  writer.AddVariableString("parent", sGuid);

  writer.BeginArray("addedObjects");
  for (const ezDocumentObject* pChild : pParent->GetChildren())
  {
    if (before.Contains(pChild->GetGuid()))
      continue;

    ezStringBuilder sNewGuid;
    ezConversionUtils::ToString(pChild->GetGuid(), sNewGuid);
    writer.WriteString(sNewGuid);
  }
  writer.EndArray();

  writer.AddVariableBool("modified", pDocument->IsModified());
  writer.AddVariableString("note", "The copy sits at the same position as the original, so it is hidden inside it until it is moved "
                                   "('moveObject', or set its LocalPosition). It is now the document's selection, as it would be "
                                   "after pasting in the editor. The document is not saved yet.");

  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpObjectTool::ExecuteUndo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sDocument = ezMcpJson::GetString(arguments, "document");

  ezDocument* pDocument = ezMcpDocument::Find(sDocument);

  if (pDocument == nullptr)
  {
    ezMcpDocument::SetNotOpenError(out_result, sDocument);
    return;
  }

  ezCommandHistory* pHistory = pDocument->GetCommandHistory();

  if (pHistory == nullptr)
  {
    out_result.SetError("The document has no command history.");
    return;
  }

  const ezStringView sAction = ezMcpJson::GetString(arguments, "action");
  const ezUInt32 uiCount = static_cast<ezUInt32>(ezMath::Max<ezInt64>(ezMcpJson::GetInt(arguments, "count", 1), 1));

  ezUInt32 uiDone = 0;
  ezStringBuilder sFailure;

  if (sAction == "undo")
  {
    for (ezUInt32 i = 0; i < uiCount && pHistory->CanUndo(); ++i)
    {
      if (pHistory->Undo().Failed())
      {
        sFailure = "Undo failed.";
        break;
      }

      ++uiDone;
    }

    // Distinguishes 'nothing to undo' from 'undo did not work', which call for different responses.
    if (uiDone == 0 && sFailure.IsEmpty())
      sFailure = "There is nothing to undo in this document.";
  }
  else if (sAction == "redo")
  {
    for (ezUInt32 i = 0; i < uiCount && pHistory->CanRedo(); ++i)
    {
      if (pHistory->Redo().Failed())
      {
        sFailure = "Redo failed.";
        break;
      }

      ++uiDone;
    }

    if (uiDone == 0 && sFailure.IsEmpty())
      sFailure = "There is nothing to redo in this document.";
  }
  else if (!sAction.IsEmpty())
  {
    ezStringBuilder s;
    s.SetFormat("'{}' is not a valid action. Use 'undo' or 'redo', or omit it to only report the stack.", sAction);
    out_result.SetError(s);
    return;
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();

  if (!sAction.IsEmpty())
  {
    writer.AddVariableString("action", sAction);
    writer.AddVariableUInt32("stepsPerformed", uiDone);
  }

  writer.AddVariableBool("canUndo", pHistory->CanUndo());
  writer.AddVariableBool("canRedo", pHistory->CanRedo());
  writer.AddVariableBool("modified", pDocument->IsModified());

  if (!sFailure.IsEmpty())
    writer.AddVariableString("error", sFailure);

  writer.EndObject();

  out_result.m_sText = writer.GetResult();

  // Asking to undo when there is nothing to undo is a failed call, not a silent no-op.
  out_result.m_bIsError = !sFailure.IsEmpty();
}
