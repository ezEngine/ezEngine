#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONWriter.h>

ezJSONWriter::ezJSONWriter() = default;
ezJSONWriter::~ezJSONWriter() = default;

void ezJSONWriter::AddVariableBool(ezStringView sName, bool value)
{
  BeginVariable(sName);
  WriteBool(value);
  EndVariable();
}

void ezJSONWriter::AddVariableInt32(ezStringView sName, ezInt32 value)
{
  BeginVariable(sName);
  WriteInt32(value);
  EndVariable();
}

void ezJSONWriter::AddVariableUInt32(ezStringView sName, ezUInt32 value)
{
  BeginVariable(sName);
  WriteUInt32(value);
  EndVariable();
}

void ezJSONWriter::AddVariableInt64(ezStringView sName, ezInt64 value)
{
  BeginVariable(sName);
  WriteInt64(value);
  EndVariable();
}

void ezJSONWriter::AddVariableUInt64(ezStringView sName, ezUInt64 value)
{
  BeginVariable(sName);
  WriteUInt64(value);
  EndVariable();
}

void ezJSONWriter::AddVariableFloat(ezStringView sName, float value)
{
  BeginVariable(sName);
  WriteFloat(value);
  EndVariable();
}

void ezJSONWriter::AddVariableDouble(ezStringView sName, double value)
{
  BeginVariable(sName);
  WriteDouble(value);
  EndVariable();
}

void ezJSONWriter::AddVariableString(ezStringView sName, ezStringView value)
{
  BeginVariable(sName);
  WriteString(value);
  EndVariable();
}

void ezJSONWriter::AddVariableNULL(ezStringView sName)
{
  BeginVariable(sName);
  WriteNULL();
  EndVariable();
}

void ezJSONWriter::AddVariableTime(ezStringView sName, ezTime value)
{
  BeginVariable(sName);
  WriteTime(value);
  EndVariable();
}

void ezJSONWriter::AddVariableUuid(ezStringView sName, ezUuid value)
{
  BeginVariable(sName);
  WriteUuid(value);
  EndVariable();
}

void ezJSONWriter::AddVariableAngle(ezStringView sName, ezAngle value)
{
  BeginVariable(sName);
  WriteAngle(value);
  EndVariable();
}

void ezJSONWriter::AddVariableColor(ezStringView sName, const ezColor& value)
{
  BeginVariable(sName);
  WriteColor(value);
  EndVariable();
}

void ezJSONWriter::AddVariableColorGamma(ezStringView sName, const ezColorGammaUB& value)
{
  BeginVariable(sName);
  WriteColorGamma(value);
  EndVariable();
}

void ezJSONWriter::AddVariableVec2(ezStringView sName, const ezVec2& value)
{
  BeginVariable(sName);
  WriteVec2(value);
  EndVariable();
}

void ezJSONWriter::AddVariableVec3(ezStringView sName, const ezVec3& value)
{
  BeginVariable(sName);
  WriteVec3(value);
  EndVariable();
}

void ezJSONWriter::AddVariableVec4(ezStringView sName, const ezVec4& value)
{
  BeginVariable(sName);
  WriteVec4(value);
  EndVariable();
}

void ezJSONWriter::AddVariableVec2I32(ezStringView sName, const ezVec2I32& value)
{
  BeginVariable(sName);
  WriteVec2I32(value);
  EndVariable();
}

void ezJSONWriter::AddVariableVec3I32(ezStringView sName, const ezVec3I32& value)
{
  BeginVariable(sName);
  WriteVec3I32(value);
  EndVariable();
}

void ezJSONWriter::AddVariableVec4I32(ezStringView sName, const ezVec4I32& value)
{
  BeginVariable(sName);
  WriteVec4I32(value);
  EndVariable();
}

void ezJSONWriter::AddVariableQuat(ezStringView sName, const ezQuat& value)
{
  BeginVariable(sName);
  WriteQuat(value);
  EndVariable();
}

void ezJSONWriter::AddVariableMat3(ezStringView sName, const ezMat3& value)
{
  BeginVariable(sName);
  WriteMat3(value);
  EndVariable();
}

void ezJSONWriter::AddVariableMat4(ezStringView sName, const ezMat4& value)
{
  BeginVariable(sName);
  WriteMat4(value);
  EndVariable();
}

void ezJSONWriter::AddVariableDataBuffer(ezStringView sName, const ezDataBuffer& value)
{
  BeginVariable(sName);
  WriteDataBuffer(value);
  EndVariable();
}

void ezJSONWriter::AddVariableVariant(ezStringView sName, const ezVariant& value)
{
  BeginVariable(sName);
  WriteVariant(value);
  EndVariable();
}

void ezJSONWriter::WriteColor(const ezColor& value)
{
  EZ_IGNORE_UNUSED(value);
  EZ_REPORT_FAILURE("The complex data type ezColor is not supported by this JSON writer.");
}

void ezJSONWriter::WriteColorGamma(const ezColorGammaUB& value)
{
  EZ_IGNORE_UNUSED(value);
  EZ_REPORT_FAILURE("The complex data type ezColorGammaUB is not supported by this JSON writer.");
}

void ezJSONWriter::WriteVec2(const ezVec2& value)
{
  EZ_IGNORE_UNUSED(value);
  EZ_REPORT_FAILURE("The complex data type ezVec2 is not supported by this JSON writer.");
}

void ezJSONWriter::WriteVec3(const ezVec3& value)
{
  EZ_IGNORE_UNUSED(value);
  EZ_REPORT_FAILURE("The complex data type ezVec3 is not supported by this JSON writer.");
}

void ezJSONWriter::WriteVec4(const ezVec4& value)
{
  EZ_IGNORE_UNUSED(value);
  EZ_REPORT_FAILURE("The complex data type ezVec4 is not supported by this JSON writer.");
}

void ezJSONWriter::WriteVec2I32(const ezVec2I32& value)
{
  EZ_IGNORE_UNUSED(value);
  EZ_REPORT_FAILURE("The complex data type ezVec2I32 is not supported by this JSON writer.");
}

void ezJSONWriter::WriteVec3I32(const ezVec3I32& value)
{
  EZ_IGNORE_UNUSED(value);
  EZ_REPORT_FAILURE("The complex data type ezVec3I32 is not supported by this JSON writer.");
}

void ezJSONWriter::WriteVec4I32(const ezVec4I32& value)
{
  EZ_IGNORE_UNUSED(value);
  EZ_REPORT_FAILURE("The complex data type ezVec4I32 is not supported by this JSON writer.");
}

void ezJSONWriter::WriteQuat(const ezQuat& value)
{
  EZ_IGNORE_UNUSED(value);
  EZ_REPORT_FAILURE("The complex data type ezQuat is not supported by this JSON writer.");
}

void ezJSONWriter::WriteMat3(const ezMat3& value)
{
  EZ_IGNORE_UNUSED(value);
  EZ_REPORT_FAILURE("The complex data type ezMat3 is not supported by this JSON writer.");
}

void ezJSONWriter::WriteMat4(const ezMat4& value)
{
  EZ_IGNORE_UNUSED(value);
  EZ_REPORT_FAILURE("The complex data type ezMat4 is not supported by this JSON writer.");
}

void ezJSONWriter::WriteDataBuffer(const ezDataBuffer& value)
{
  EZ_IGNORE_UNUSED(value);
  EZ_REPORT_FAILURE("The complex data type ezDateBuffer is not supported by this JSON writer.");
}

void ezJSONWriter::WriteVariant(const ezVariant& value)
{
  switch (value.GetType())
  {
    case ezVariant::Type::Invalid:
      // EZ_REPORT_FAILURE("Variant of Type 'Invalid' cannot be written as JSON.");
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
    case ezVariant::Type::ColorGamma:
      WriteColorGamma(value.Get<ezColorGammaUB>());
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
    case ezVariant::Type::Vector2I:
      WriteVec2I32(value.Get<ezVec2I32>());
      return;
    case ezVariant::Type::Vector3I:
      WriteVec3I32(value.Get<ezVec3I32>());
      return;
    case ezVariant::Type::Vector4I:
      WriteVec4I32(value.Get<ezVec4I32>());
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
    case ezVariant::Type::StringView:
    {
      ezStringBuilder s = value.Get<ezStringView>();
      WriteString(s.GetData());
      return;
    }
    case ezVariant::Type::Time:
      WriteTime(value.Get<ezTime>());
      return;
    case ezVariant::Type::Uuid:
      WriteUuid(value.Get<ezUuid>());
      return;
    case ezVariant::Type::Angle:
      WriteAngle(value.Get<ezAngle>());
      return;
    case ezVariant::Type::DataBuffer:
      WriteDataBuffer(value.Get<ezDataBuffer>());
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
    case ezVariant::Type::VariantDictionary:
    {
      BeginObject();

      const auto& dict = value.Get<ezVariantDictionary>();

      for (auto& kv : dict)
      {
        AddVariableVariant(kv.Key(), kv.Value());
      }
      EndObject();
    }
      return;

    default:
      break;
  }

  EZ_REPORT_FAILURE("The Variant Type {0} is not supported by ezJSONWriter::WriteVariant.", value.GetType());
}


bool ezJSONWriter::HadWriteError() const
{
  return m_bHadWriteError;
}

void ezJSONWriter::SetWriteErrorState()
{
  m_bHadWriteError = true;
}


