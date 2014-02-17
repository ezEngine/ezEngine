#include <Foundation/PCH.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <Foundation/Memory/EndianHelper.h>

void ezExtendedJSONWriter::WriteInt32(ezInt32 value)
{
  CommaWriter cw(this);

  ezEndianHelper::NativeToLittleEndian((ezUInt32*) &value, 1);

  ezStringBuilder s;
  s.Format("%i", value);

  WriteBinaryData("int32", &value, sizeof(value), s.GetData());
}

void ezExtendedJSONWriter::WriteUInt32(ezUInt32 value)
{
  CommaWriter cw(this);

  ezEndianHelper::NativeToLittleEndian((ezUInt32*) &value, 1);

  ezStringBuilder s;
  s.Format("%u", value);

  WriteBinaryData("uint32", &value, sizeof(value), s.GetData());
}

void ezExtendedJSONWriter::WriteInt64(ezInt64 value)
{
  CommaWriter cw(this);

  ezEndianHelper::NativeToLittleEndian((ezUInt64*) &value, 1);

  ezStringBuilder s;
  s.Format("%lli", value);

  WriteBinaryData("int64", &value, sizeof(value), s.GetData());
}

void ezExtendedJSONWriter::WriteUInt64(ezUInt64 value)
{
  CommaWriter cw(this);

  ezEndianHelper::NativeToLittleEndian((ezUInt64*) &value, 1);

  ezStringBuilder s;
  s.Format("%llu", value);

  WriteBinaryData("uint64", &value, sizeof(value), s.GetData());
}

void ezExtendedJSONWriter::WriteFloat(float value)
{
  CommaWriter cw(this);

  ezEndianHelper::NativeToLittleEndian((ezUInt32*) &value, 1);

  ezStringBuilder s;
  s.Format("%.4f", value);

  WriteBinaryData("float", &value, sizeof(value), s.GetData());
}

void ezExtendedJSONWriter::WriteDouble(double value)
{
  CommaWriter cw(this);

  ezEndianHelper::NativeToLittleEndian((ezUInt64*) &value, 1);

  ezStringBuilder s;
  s.Format("%.8f", value);

  WriteBinaryData("double", &value, sizeof(value), s.GetData());
}

void ezExtendedJSONWriter::WriteTime(ezTime value)
{
  CommaWriter cw(this);

  ezEndianHelper::NativeToLittleEndian((ezUInt64*) &value, 1);

  ezStringBuilder s;
  s.Format("%.4f", value.GetSeconds());

  WriteBinaryData("time", &value, sizeof(value), s.GetData());
}



EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_ExtendedJSONWriter);

