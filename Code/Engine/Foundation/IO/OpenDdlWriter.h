#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlParser.h>

// TODO
// Write primitives in HEX (esp. float)

/// \brief The base class for OpenDDL writers.
///
/// Declares a common interface for writing OpenDDL files.
class EZ_FOUNDATION_DLL ezOpenDdlWriter
{
public:

  enum class TypeStringMode
  {
    Compliant,            ///< All primitive types are written as the OpenDDL standard defines them (very verbose)
    ShortenedUnsignedInt, ///< unsigned_intX is shortened to uintX
    Shortest              ///< All primitive type names are shortened to one or two characters: i1, i2, i3, i4, u1, u2, u3, u4, b, s, f, d (int, uint, bool, string, float, double)
  };

  enum class FloatPrecisionMode
  {
    Readable,   ///< Float values are printed as readable numbers. Precision might get lost though.
    Exact,      ///< Float values are printed as HEX, representing the exact binary data.
  };

  /// \brief Constructor
  ezOpenDdlWriter();

  virtual ~ezOpenDdlWriter() { }

  /// \brief All output is written to this binary stream.
  void SetOutputStream(ezStreamWriter* pOutput) { m_pOutput = pOutput; } // [tested]

  /// \brief Configures how much whitespace is output.
  void SetCompactMode(bool compact) { m_bCompactMode = compact; } // [tested]

  /// \brief Configures how verbose the type strings are going to be written.
  void SetPrimitiveTypeStringMode(TypeStringMode mode) { m_TypeStringMode = mode; }

  /// \brief Configures how float values are output.
  void SetFloatPrecisionMode(FloatPrecisionMode mode) { m_FloatPrecisionMode = mode; }

  /// \brief Allows to set the indentation. Negative values are possible.
  /// This makes it possible to set the indentation e.g. to -2, thus the output will only have indentation after a level of 3 has been reached.
  void SetIndentation(ezInt8 iIndentation) { m_iIndentation = iIndentation; }

  /// \brief Begins outputting an object.
  void BeginObject(const char* szType, const char* szName = nullptr, bool bGlobalName = false, bool bSingleLine = false); // [tested]

  /// \brief Ends outputting an object.
  void EndObject(); // [tested]

  /// \brief Begins outputting a list of primitives of the given type.
  void BeginPrimitiveList(ezOpenDdlPrimitiveType type, const char* szName = nullptr, bool bGlobalName = false); // [tested]

  /// \brief Ends outputting the list of primitives.
  void EndPrimitiveList(); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteBool(const bool* pValues, ezUInt32 count = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt8(const ezInt8* pValues, ezUInt32 count = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt16(const ezInt16* pValues, ezUInt32 count = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt32(const ezInt32* pValues, ezUInt32 count = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt64(const ezInt64* pValues, ezUInt32 count = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt8(const ezUInt8* pValues, ezUInt32 count = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt16(const ezUInt16* pValues, ezUInt32 count = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt32(const ezUInt32* pValues, ezUInt32 count = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt64(const ezUInt64* pValues, ezUInt32 count = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteFloat(const float* pValues, ezUInt32 count = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteDouble(const double* pValues, ezUInt32 count = 1); // [tested]

  /// \brief Writes a single string to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteString(const ezStringView& string); // [tested]

  /// \brief Writes a single string to the primitive list, but the value is a HEX representation of the given binary data.
  void WriteBinaryAsString(const void* pData, ezUInt32 uiBytes);


protected:
  enum State
  {
    Invalid = -5,
    Empty = -4,
    ObjectSingleLine = -3,
    ObjectMultiLine = -2,
    ObjectStart = -1,
    PrimitivesBool = 0, // same values as in ezOpenDdlPrimitiveType to enable casting
    PrimitivesInt8,
    PrimitivesInt16,
    PrimitivesInt32,
    PrimitivesInt64,
    PrimitivesUInt8,
    PrimitivesUInt16,
    PrimitivesUInt32,
    PrimitivesUInt64,
    PrimitivesFloat,
    PrimitivesDouble,
    PrimitivesString,
  };

  struct DdlState
  {
    DdlState() : m_State(Empty)
    {
      m_bPrimitivesWritten = false;
    }

    State m_State;
    bool m_bPrimitivesWritten;
  };

  EZ_FORCE_INLINE void OutputString(const char* sz) { m_pOutput->WriteBytes(sz, ezStringUtils::GetStringElementCount(sz)); }
  EZ_FORCE_INLINE void OutputString(const char* sz, ezUInt32 uiElementCount) { m_pOutput->WriteBytes(sz, uiElementCount); }
  void OutputEscapedString(const ezStringView& string);
  void OutputIndentation();
  void OutputPrimitiveTypeNameCompliant(ezOpenDdlPrimitiveType type);
  void OutputPrimitiveTypeNameShort(ezOpenDdlPrimitiveType type);
  void OutputPrimitiveTypeNameShortest(ezOpenDdlPrimitiveType type);
  void WritePrimitiveType(ezOpenDdlWriter::State exp);
  void OutputObjectName(const char* szName, bool bGlobalName);
  void WriteBinaryAsHex(const void* pData, ezUInt32 uiBytes);
  void OutputObjectBeginning();

  ezInt32 m_iIndentation;
  bool m_bCompactMode;
  TypeStringMode m_TypeStringMode;
  FloatPrecisionMode m_FloatPrecisionMode;
  ezStreamWriter* m_pOutput;
  ezStringBuilder m_Temp;

  ezHybridArray<DdlState, 16> m_StateStack;
};
