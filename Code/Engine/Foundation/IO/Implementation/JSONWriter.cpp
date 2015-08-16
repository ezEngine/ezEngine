#include <Foundation/PCH.h>
#include <Foundation/IO/JSONWriter.h>

void ezJSONWriter::AddVariableBool(const char* szName, bool value)
{
  BeginVariable(szName);
  WriteBool(value);
  EndVariable();
}

void ezJSONWriter::AddVariableInt32(const char* szName, ezInt32 value)
{
  BeginVariable(szName);
  WriteInt32(value);
  EndVariable();
}

void ezJSONWriter::AddVariableUInt32(const char* szName, ezUInt32 value)
{
  BeginVariable(szName);
  WriteUInt32(value);
  EndVariable();
}

void ezJSONWriter::AddVariableInt64(const char* szName, ezInt64 value)
{
  BeginVariable(szName);
  WriteInt64(value);
  EndVariable();
}

void ezJSONWriter::AddVariableUInt64(const char* szName, ezUInt64 value)
{
  BeginVariable(szName);
  WriteUInt64(value);
  EndVariable();
}

void ezJSONWriter::AddVariableFloat(const char* szName, float value)
{
  BeginVariable(szName);
  WriteFloat(value);
  EndVariable();
}

void ezJSONWriter::AddVariableDouble(const char* szName, double value)
{
  BeginVariable(szName);
  WriteDouble(value);
  EndVariable();
}

void ezJSONWriter::AddVariableString(const char* szName, const char* value)
{
  BeginVariable(szName);
  WriteString(value);
  EndVariable();
}

void ezJSONWriter::AddVariableNULL(const char* szName)
{
  BeginVariable(szName);
  WriteNULL();
  EndVariable();
}

void ezJSONWriter::AddVariableTime(const char* szName, ezTime value)
{
  BeginVariable(szName);
  WriteTime(value);
  EndVariable();
}

void ezJSONWriter::AddVariableUuid(const char* szName, ezUuid value)
{
  BeginVariable(szName);
  WriteUuid(value);
  EndVariable();
}

void ezJSONWriter::AddVariableColor(const char* szName, const ezColor& value)
{
  BeginVariable(szName);
  WriteColor(value);
  EndVariable();
}

void ezJSONWriter::AddVariableVec2(const char* szName, const ezVec2& value)
{
  BeginVariable(szName);
  WriteVec2(value);
  EndVariable();
}

void ezJSONWriter::AddVariableVec3(const char* szName, const ezVec3& value)
{
  BeginVariable(szName);
  WriteVec3(value);
  EndVariable();
}

void ezJSONWriter::AddVariableVec4(const char* szName, const ezVec4& value)
{
  BeginVariable(szName);
  WriteVec4(value);
  EndVariable();
}

void ezJSONWriter::AddVariableQuat(const char* szName, const ezQuat& value)
{
  BeginVariable(szName);
  WriteQuat(value);
  EndVariable();
}

void ezJSONWriter::AddVariableMat3(const char* szName, const ezMat3& value)
{
  BeginVariable(szName);
  WriteMat3(value);
  EndVariable();
}

void ezJSONWriter::AddVariableMat4(const char* szName, const ezMat4& value)
{
  BeginVariable(szName);
  WriteMat4(value);
  EndVariable();
}

void ezJSONWriter::AddVariableVariant(const char* szName, const ezVariant& value)
{
  BeginVariable(szName);
  WriteVariant(value);
  EndVariable();
}

void ezJSONWriter::WriteColor(const ezColor& value)
{
  EZ_REPORT_FAILURE("The complex data type ezColor is not supported by this JSON writer.");
}

void ezJSONWriter::WriteVec2(const ezVec2& value)
{
  EZ_REPORT_FAILURE("The complex data type ezVec2 is not supported by this JSON writer.");
}

void ezJSONWriter::WriteVec3(const ezVec3& value)
{
  EZ_REPORT_FAILURE("The complex data type ezVec3 is not supported by this JSON writer.");
}

void ezJSONWriter::WriteVec4(const ezVec4& value)
{
  EZ_REPORT_FAILURE("The complex data type ezVec4 is not supported by this JSON writer.");
}

void ezJSONWriter::WriteQuat(const ezQuat& value)
{
  EZ_REPORT_FAILURE("The complex data type ezQuat is not supported by this JSON writer.");
}

void ezJSONWriter::WriteMat3(const ezMat3& value)
{
  EZ_REPORT_FAILURE("The complex data type ezMat3 is not supported by this JSON writer.");
}

void ezJSONWriter::WriteMat4(const ezMat4& value)
{
  EZ_REPORT_FAILURE("The complex data type ezMat4 is not supported by this JSON writer.");
}

void ezJSONWriter::WriteVariant(const ezVariant& value)
{
  switch (value.GetType())
  {
  case ezVariant::Type::Invalid:
    //EZ_REPORT_FAILURE("Variant of Type 'Invalid' cannot be written as JSON.");
    WriteNULL();
    return;
  case ezVariant::Type::Bool:
    WriteBool(value.Get<bool>());
    return;
  case ezVariant::Type::Int8:
    WriteInt32(value.Get<ezInt8>());
    return;
  case ezVariant::Type::UInt8:
    WriteUInt32(value.Get<ezUInt8>());
    return;
  case ezVariant::Type::Int16:
    WriteInt32(value.Get<ezInt16>());
    return;
  case ezVariant::Type::UInt16:
    WriteUInt32(value.Get<ezUInt16>());
    return;
  case ezVariant::Type::Int32:
    WriteInt32(value.Get<ezInt32>());
    return;
  case ezVariant::Type::UInt32:
    WriteUInt32(value.Get<ezUInt32>());
    return;
  case ezVariant::Type::Int64:
    WriteInt64(value.Get<ezInt64>());
    return;
  case ezVariant::Type::UInt64:
    WriteUInt64(value.Get<ezUInt64>());
    return;
  case ezVariant::Type::Float:
    WriteFloat(value.Get<float>());
    return;
  case ezVariant::Type::Double:
    WriteDouble(value.Get<double>());
    return;
  case ezVariant::Type::Color:
    WriteColor(value.Get<ezColor>());
    return;
  case ezVariant::Type::Vector2:
    WriteVec2(value.Get<ezVec2>());
    return;
  case ezVariant::Type::Vector3:
    WriteVec3(value.Get<ezVec3>());
    return;
  case ezVariant::Type::Vector4:
    WriteVec4(value.Get<ezVec4>());
    return;
  case ezVariant::Type::Quaternion:
    WriteQuat(value.Get<ezQuat>());
    return;
  case ezVariant::Type::Matrix3:
    WriteMat3(value.Get<ezMat3>());
    return;
  case ezVariant::Type::Matrix4:
    WriteMat4(value.Get<ezMat4>());
    return;
  case ezVariant::Type::String:
    WriteString(value.Get<ezString>().GetData());
    return;
  case ezVariant::Type::Time:
    WriteTime(value.Get<ezTime>());
    return;
  case ezVariant::Type::Uuid:
    WriteUuid(value.Get<ezUuid>());
    return;
  case ezVariant::Type::VariantArray:
    {
      BeginArray();

      const auto& ar = value.Get<ezVariantArray>();

      for (const auto& val : ar)
      {
        WriteVariant(val);
      }

      EndArray();
    }
    return;

  default:
    break;
  }

  EZ_REPORT_FAILURE("The Variant Type %i is not supported by ezJSONWriter::WriteVariant.", value.GetType());
}




EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_JSONWriter);

