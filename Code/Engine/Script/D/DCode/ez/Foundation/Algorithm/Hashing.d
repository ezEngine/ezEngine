module ez.Foundation.Algorithm.Hashing;
import ez.Foundation.Basics;
import std.traits : isPointer;

/// \brief This class provides implementations of different hashing algorithms.
class ezHashing
{
public:
  /// \brief helper struct to wrap a string pointer
  static struct StringWrapper
  {
    this(const(char)* str)
    {
      m_str = str;
    }
    const(char)* m_str;
  };

  /// \brief Calculates the CRC32 checksum of the given key.
  extern(C++) static ezUInt32 CRC32Hash(const(void)* pKey, size_t uiSizeInBytes); // [tested]

  /// \brief Calculates the 32bit murmur hash of the given key.
  extern(C++) static ezUInt32 MurmurHash(const(void)* pKey, size_t uiSizeInByte, ezUInt32 uiSeed = 0); // [tested]

  /// \brief Calculates the 64bit murmur hash of the given key.
  extern(C++) static ezUInt64 MurmurHash64(const(void)* pKey, size_t uiSizeInByte, ezUInt64 uiSeed = 0); // [tested]

  /// \brief Calculates the 32bit murmur hash of a string pointer during runtime. Encoding does not matter here.
  ///
  /// We cannot pass a string pointer directly since a string constant would be treated as pointer as well.
  extern(C++) static ezUInt32 MurmurHash(StringWrapper str, ezUInt32 uiSeed = 0); // [tested]
};

struct ezHashHelper(T)
{
  static ezUInt32 Hash(ref const(T) value)
  {
    static if(is(T == ezUInt32) || is(T == ezInt32) || is(T == ezUInt64) || is(T == ezInt64))
    {
      return cast(ezUInt32)(value * 2654435761U);
    }
    else static if(is(T == const(char)*))
    {
      return ezHashing.MurmurHash(ezHashing.StringWrapper(szValue));
    }
    else static if(isPointer!T || is(T == class))
    {
      return cast(ezUInt32)((cast(size_t)cast(void*)(value) << 4) * 2654435761U);
    }
    else
    {
      return ezHashing.MurmurHash(&value, T.sizeof);
    }
  }

  static bool Equal(ref const(T) a,ref const(T) b)
  {
    static if(is(T == const(char)*))
    {
      return ezStringUtils.IsEqual(a, b);
    }
    else static if(isPointer!T || is(T == class))
    {
      return a is b;
    }
    else
    {
      return a == b;
    }
  }
}