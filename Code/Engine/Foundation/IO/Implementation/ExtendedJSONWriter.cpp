#include <Foundation/PCH.h>
#include <Foundation/IO/ExtendedJSONWriter.h>

void ezExtendedJSONWriter::WriteInt32(ezInt32 value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.Format("{0}", value);
  ezEndianHelper::NativeToLittleEndian((ezUInt32*) &value, 1);

  WriteBinaryData("int32", &value, sizeof(value), s.GetData());
}

void ezExtendedJSONWriter::WriteUInt32(ezUInt32 value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.Format("{0}", value);
  ezEndianHelper::NativeToLittleEndian((ezUInt32*) &value, 1);

  WriteBinaryData("uint32", &value, sizeof(value), s.GetData());
}

void ezExtendedJSONWriter::WriteInt64(ezInt64 value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.Format("{0}", value);
  ezEndianHelper::NativeToLittleEndian((ezUInt64*) &value, 1);

  WriteBinaryData("int64", &value, sizeof(value), s.GetData());
}

void ezExtendedJSONWriter::WriteUInt64(ezUInt64 value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.Format("{0}", value);
  ezEndianHelper::NativeToLittleEndian((ezUInt64*) &value, 1);

  WriteBinaryData("uint64", &value, sizeof(value), s.GetData());
}

void ezExtendedJSONWriter::WriteFloat(float value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.Format("{0}", ezArgF(value, 4));
  ezEndianHelper::NativeToLittleEndian((ezUInt32*) &value, 1);

  WriteBinaryData("float", &value, sizeof(value), s.GetData());
}

void ezExtendedJSONWriter::WriteDouble(double value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.Format("{0}", ezArgF(value, 8));
  ezEndianHelper::NativeToLittleEndian((ezUInt64*) &value, 1);

  WriteBinaryData("double", &value, sizeof(value), s.GetData());
}

void ezExtendedJSONWriter::WriteTime(ezTime value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.Format("{0}", ezArgF(value.GetSeconds(), 4));
  ezEndianHelper::NativeToLittleEndian((ezUInt64*) &value, 1);

  WriteBinaryData("time", &value, sizeof(value), s.GetData());
}

void ezExtendedJSONWriter::WriteAngle(ezAngle value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.Format("{0}", ezArgF(value.GetDegree(), 4));

  ezEndianHelper::NativeToLittleEndian((ezUInt32*)&value, 1);

  WriteBinaryData("angle", &value, sizeof(value), s.GetData());
}


EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_ExtendedJSONWriter);

