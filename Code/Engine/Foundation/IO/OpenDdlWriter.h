#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlParser.h>

// TODO
// Write primitives in HEX (esp. float)

/// \brief Writes OpenDDL documents to a stream with configurable formatting
///
/// OpenDDL (Open Data Description Language) is a text format for describing structured data.
/// This writer generates OpenDDL text from programmatically created object hierarchies and primitive data.
/// Supports various output modes including compact/verbose formatting, different type name styles,
/// and precise floating-point representation. Objects and primitive lists can be nested arbitrarily.
/// Call SetOutputStream() first, then use BeginObject/EndObject and BeginPrimitiveList/EndPrimitiveList pairs.
class EZ_FOUNDATION_DLL ezOpenDdlWriter
{
public:
  /// \brief Controls how primitive type names are written in the output
  enum class TypeStringMode
  {
    Compliant,            ///< All primitive types are written as the OpenDDL standard defines them (very verbose)
    ShortenedUnsignedInt, ///< unsigned_intX is shortened to uintX
    Shortest              ///< All primitive type names are shortened to one or two characters: i1, i2, i3, i4, u1, u2, u3, u4, b, s, f, d (int, uint, bool,
                          ///< string, float, double)
  };

  /// \brief Controls how floating-point values are formatted in the output
  enum class FloatPrecisionMode
  {
    Readable, ///< Float values are printed as readable numbers. Precision might get lost though.
    Exact,    ///< Float values are printed as HEX, representing the exact binary data.
  };

  /// \brief Constructor
  ezOpenDdlWriter();

  virtual ~ezOpenDdlWriter() = default;

  /// \brief All output is written to this binary stream.
  void SetOutputStream(ezStreamWriter* pOutput) { m_pOutput = pOutput; } // [tested]

  /// \brief Configures how much whitespace is output.
  void SetCompactMode(bool bCompact) { m_bCompactMode = bCompact; } // [tested]

  /// \brief Configures how verbose the type strings are going to be written.
  void SetPrimitiveTypeStringMode(TypeStringMode mode) { m_TypeStringMode = mode; }

  /// \brief Configures how float values are output.
  void SetFloatPrecisionMode(FloatPrecisionMode mode) { m_FloatPrecisionMode = mode; }

  /// \brief Returns how float values are output.
  FloatPrecisionMode GetFloatPrecisionMode() const { return m_FloatPrecisionMode; }

  /// \brief Sets the base indentation level for output formatting
  ///
  /// Negative values are allowed to delay indentation until deeper nesting levels.
  /// For example, setting -2 means indentation only starts at nesting level 3.
  void SetIndentation(ezInt8 iIndentation) { m_iIndentation = iIndentation; }

  /// \brief Begins outputting an object.
  void BeginObject(ezStringView sType, ezStringView sName = {}, bool bGlobalName = false, bool bSingleLine = false); // [tested]

  /// \brief Ends outputting an object.
  void EndObject(); // [tested]

  /// \brief Begins outputting a list of primitives of the given type.
  void BeginPrimitiveList(ezOpenDdlPrimitiveType type, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Ends outputting the list of primitives.
  void EndPrimitiveList(); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteBool(const bool* pValues, ezUInt32 uiCount = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt8(const ezInt8* pValues, ezUInt32 uiCount = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt16(const ezInt16* pValues, ezUInt32 uiCount = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt32(const ezInt32* pValues, ezUInt32 uiCount = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt64(const ezInt64* pValues, ezUInt32 uiCount = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt8(const ezUInt8* pValues, ezUInt32 uiCount = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt16(const ezUInt16* pValues, ezUInt32 uiCount = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt32(const ezUInt32* pValues, ezUInt32 uiCount = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt64(const ezUInt64* pValues, ezUInt32 uiCount = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteFloat(const float* pValues, ezUInt32 uiCount = 1); // [tested]

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteDouble(const double* pValues, ezUInt32 uiCount = 1); // [tested]

  /// \brief Writes a single string to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteString(const ezStringView& sString); // [tested]

  /// \brief Writes binary data as a hexadecimal string to the primitive list
  ///
  /// Converts the binary data to a hex string representation and writes it as a string primitive.
  /// Useful for embedding binary data within OpenDDL text format.
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
    State m_State = State::Empty;
    bool m_bPrimitivesWritten = false;
  };

  EZ_ALWAYS_INLINE void OutputString(ezStringView s) { m_pOutput->WriteBytes(s.GetStartPointer(), s.GetElementCount()).IgnoreResult(); }
  EZ_ALWAYS_INLINE void OutputString(ezStringView s, ezUInt32 uiElementCount) { m_pOutput->WriteBytes(s.GetStartPointer(), uiElementCount).IgnoreResult(); }
  void OutputEscapedString(const ezStringView& string);
  void OutputIndentation();
  void OutputPrimitiveTypeNameCompliant(ezOpenDdlPrimitiveType type);
  void OutputPrimitiveTypeNameShort(ezOpenDdlPrimitiveType type);
  void OutputPrimitiveTypeNameShortest(ezOpenDdlPrimitiveType type);
  void WritePrimitiveType(ezOpenDdlWriter::State exp);
  void OutputObjectName(ezStringView sName, bool bGlobalName);
  void WriteBinaryAsHex(const void* pData, ezUInt32 uiBytes);
  void OutputObjectBeginning();

  ezInt32 m_iIndentation = 0;
  bool m_bCompactMode = false;
  TypeStringMode m_TypeStringMode = TypeStringMode::ShortenedUnsignedInt;
  FloatPrecisionMode m_FloatPrecisionMode = FloatPrecisionMode::Exact;
  ezStreamWriter* m_pOutput = nullptr;
  ezStringBuilder m_sTemp;

  ezHybridArray<DdlState, 16> m_StateStack;
};
