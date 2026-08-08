#include <Mcp/McpPCH.h>

#include <Mcp/McpJsonWriter.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Uuid.h>

ezMcpJsonWriter::ezMcpJsonWriter()
  : m_Writer(&m_Storage)
{
  SetOutputStream(&m_Writer);

  // every character here is a token that the client pays for, and nothing reads this by eye
  SetWhitespaceMode(ezJSONWriter::WhitespaceMode::None);
  SetArrayMode(ezJSONWriter::ArrayMode::InOneLine);
}

ezMcpJsonWriter::~ezMcpJsonWriter() = default;

ezStringView ezMcpJsonWriter::GetResult()
{
  const ezArrayPtr<const ezUInt8> bytes = m_Storage.GetContiguousMemoryRange(0);

  m_sResult = ezStringView(reinterpret_cast<const char*>(bytes.GetPtr()), bytes.GetCount());

  return m_sResult.GetView();
}

void ezMcpJsonWriter::WriteVariant(const ezVariant& value)
{
  switch (value.GetType())
  {
    case ezVariant::Type::TypedPointer:
    {
      // Written as a description of the target rather than as its contents: the pointer may be null,
      // may point at a type with cycles, and the model can look the type up with the rtti tools anyway.
      const ezTypedPointer ptr = value.Get<ezTypedPointer>();

      const ezRTTI* pType = ptr.m_pType;

      // The variant carries the *declared* pointer type, which for a reflected container is the base
      // class - an array of ezPropertyAttribute* reports 'ezPropertyAttribute' for every element,
      // losing which attribute it actually is. When the target derives from ezReflectedClass it can
      // say so itself, and that is the name worth reporting.
      if (ptr.m_pObject != nullptr && pType != nullptr && pType->IsDerivedFrom<ezReflectedClass>())
      {
        pType = static_cast<const ezReflectedClass*>(ptr.m_pObject)->GetDynamicRTTI();
      }

      BeginObject();
      AddVariableString("$type", pType != nullptr ? pType->GetTypeName() : ezStringView());

      if (ptr.m_pObject == nullptr)
      {
        AddVariableBool("$null", true);
      }

      EndObject();
      return;
    }

    case ezVariant::Type::TypedObject:
    {
      // Only the type is written. Serialising the members would need the property system and could
      // recurse without bound, which is not what a JSON writer should be doing.
      const ezRTTI* pType = value.GetReflectedType();

      BeginObject();
      AddVariableString("$type", pType != nullptr ? pType->GetTypeName() : ezStringView());
      EndObject();
      return;
    }

    default:
      break;
  }

  // The base class ends in EZ_REPORT_FAILURE for a type its switch does not cover, and an assert here
  // is a dead editor in the middle of answering a tool call. Everything the enum currently defines is handled by
  // one of the two writers, so this only catches a type added later.
  const ezVariant::Type::Enum type = value.GetType();

  const bool bBaseHandlesIt = (type > ezVariant::Type::Invalid && type < ezVariant::Type::LastStandardType) ||
                              type == ezVariant::Type::VariantArray || type == ezVariant::Type::VariantDictionary ||
                              type == ezVariant::Type::Invalid;

  if (bBaseHandlesIt)
  {
    ezStandardJSONWriter::WriteVariant(value);
    return;
  }

  BeginObject();
  AddVariableString("$type", value.GetReflectedType() != nullptr ? value.GetReflectedType()->GetTypeName() : ezStringView());
  AddVariableUInt32("$variantType", static_cast<ezUInt32>(type));
  AddVariableString("$note", "This value's type has no JSON representation, so only its type is reported.");
  EndObject();
}
