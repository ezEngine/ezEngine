module ez.Script.Serialization.Serialization;
import std.format;
import ez.Foundation.Memory.AllocatorBase;
import ez.Script.Reflection.Reflection;
import core.stdc.string : memcpy;

enum DataType : uint
{
  Byte   = BasicType.Type.Byte,
  Short  = BasicType.Type.Short,
  Int    = BasicType.Type.Int,
  Long   = BasicType.Type.Long,
  Ubyte  = BasicType.Type.Ubyte,
  Ushort = BasicType.Type.Ushort,
  Uint   = BasicType.Type.Uint,
  Ulong  = BasicType.Type.Ulong,
  Char   = BasicType.Type.Char,
  Wchar  = BasicType.Type.Wchar,
  Dchar  = BasicType.Type.Dchar,
  Bool   = BasicType.Type.Bool,
  Array,
  Class,
  Memcpy
}

size_t GetDataTypeSize(DataType type)
{
  final switch(type)
  {
    case DataType.Byte:
      return byte.sizeof;
    case DataType.Short:
      return short.sizeof;
    case DataType.Int:
      return int.sizeof;
    case DataType.Long:
      return long.sizeof;
    case DataType.Ubyte:
      return ubyte.sizeof;
    case DataType.Ushort:
      return ushort.sizeof;
    case DataType.Uint:
      return uint.sizeof;
    case DataType.Ulong:
      return ulong.sizeof;
    case DataType.Char:
      return char.sizeof;
    case DataType.Wchar:
      return wchar.sizeof;
    case DataType.Dchar:
      return dchar.sizeof;
    case DataType.Bool:
      return bool.sizeof;
    case DataType.Array:
      return (void[]).sizeof;
    case DataType.Class:
      return Object.sizeof;
    case DataType.Memcpy:
      return 0;
  }
}

struct SerializationStep
{
  uint readOffset;
  uint writeOffset;
  uint size;
  DataType type;
}

void Serialize(void[] read, void[] write, SerializationStep[] instructions)
{
  foreach(ref inst; instructions)
  {
    assert(inst.readOffset + GetDataTypeSize(inst.type) <= read.length);
    assert(inst.writeOffset + GetDataTypeSize(inst.type) <= write.length);
    final switch(inst.type)
    {
      case DataType.Byte:
        *cast(byte*)(write.ptr + inst.writeOffset) = *cast(byte*)(read.ptr + inst.readOffset);
        break;
      case DataType.Short:
        *cast(short*)(write.ptr + inst.writeOffset) = *cast(short*)(read.ptr + inst.readOffset);
        break;
      case DataType.Int:
        *cast(int*)(write.ptr + inst.writeOffset) = *cast(int*)(read.ptr + inst.readOffset);
        break;
      case DataType.Long:
        *cast(long*)(write.ptr + inst.writeOffset) = *cast(long*)(read.ptr + inst.readOffset);
        break;
      case DataType.Ubyte:
        *cast(ubyte*)(write.ptr + inst.writeOffset) = *cast(ubyte*)(read.ptr + inst.readOffset);
        break;
      case DataType.Ushort:
        *cast(ushort*)(write.ptr + inst.writeOffset) = *cast(ushort*)(read.ptr + inst.readOffset);
        break;     
      case DataType.Uint:
        *cast(uint*)(write.ptr + inst.writeOffset) = *cast(uint*)(read.ptr + inst.readOffset);
        break;
      case DataType.Ulong:
        *cast(ulong*)(write.ptr + inst.writeOffset) = *cast(ulong*)(read.ptr + inst.readOffset);
        break;
      case DataType.Char:
        *cast(char*)(write.ptr + inst.writeOffset) = *cast(char*)(read.ptr + inst.readOffset);
        break;
      case DataType.Wchar:
        *cast(wchar*)(write.ptr + inst.writeOffset) = *cast(wchar*)(read.ptr + inst.readOffset);
        break;
      case DataType.Dchar:
        *cast(dchar*)(write.ptr + inst.writeOffset) = *cast(dchar*)(read.ptr + inst.readOffset);
        break;
      case DataType.Bool:
        *cast(bool*)(write.ptr + inst.writeOffset) = *cast(bool*)(read.ptr + inst.readOffset);
        break;
      case DataType.Array:
        *cast(void[]*)(write.ptr + inst.writeOffset) = *cast(void[]*)(read.ptr + inst.readOffset);
        break;
      case DataType.Class:
        *cast(Object*)(write.ptr + inst.writeOffset) = *cast(Object*)(read.ptr + inst.readOffset);
        break;
      case DataType.Memcpy:
        assert(inst.readOffset + inst.size <= read.length);
        assert(inst.writeOffset + inst.size <= write.length);
        memcpy(write.ptr, read.ptr, inst.size);
        break;
    }
  }
}

void BuildSerializationInstructions(ezAllocatorBase allocator, StructType st)
{
  // if we already have serialization information skip
  // if the struct does not have any members, skip
  if(st.serialization.length > 0 || (st.curType !is null && st.curType.members.length == 0))
    return;
  // Is it a unreflected type?
  if(st.curType is null)
  {
    st.serialization = allocator.NewArray!SerializationStep(1);
    with(st.serialization[0])
    {
      readOffset = 0;
      writeOffset = 0;
      type = DataType.Memcpy;
      size = cast(uint)st.size;
    }
  }
  else
  {
    auto oldMembers = st.curType.members;
    auto newMembers = (st.newType is null) ? st.curType.members : st.newType.members;
    st.serialization = BuildSerializationInstructions(allocator, oldMembers, newMembers);
  }
}

SerializationStep[] BuildSerializationInstructions(ezAllocatorBase allocator, Member[] oldMembers, Member[] newMembers)
{
  size_t numSteps = 0;
  outer: foreach(ref oldMember; oldMembers)
  {
    foreach(ref newMember; newMembers)
    {
      if(newMember.name == oldMember.name)
      {
        assert(newMember.type is oldMember.type, format("Type conversions are not supported yet"));
        if(oldMember.type.metaType == ReflectedType.Type.StructType)
        {
          StructType st = oldMember.type.castTo!StructType;
          BuildSerializationInstructions(allocator, st);
          numSteps += st.serialization.length;
        }
        else
          numSteps++;
        continue outer;
      }
    }
    assert(false, "Couldn't find matching member for " ~ oldMember.name);
  }

  SerializationStep[] steps = allocator.NewArray!SerializationStep(numSteps);
  numSteps = 0;

  outer2: foreach(ref oldMember; oldMembers)
  {
    foreach(ref newMember; newMembers)
    {
      if(newMember.name == oldMember.name)
      {
        assert(newMember.type is oldMember.type, format("Type conversions are not supported yet"));
        if(oldMember.type.metaType == ReflectedType.Type.StructType)
        {
          StructType st = oldMember.type.castTo!StructType;
          steps[numSteps..numSteps+st.serialization.length] = st.serialization[];
          numSteps += st.serialization.length;
        }
        else
        {
        }
        continue outer2;
      }
    }
  }
  return steps;
}