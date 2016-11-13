#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlParser.h>

/// \brief The base class for OpenDDL writers.
///
/// Declares a common interface for writing OpenDDL files.
class EZ_FOUNDATION_DLL ezOpenDdlWriter
{
public:

  /// \brief Modes to configure how much whitespace the writer will output
  enum class WhitespaceMode
  {
    All,              ///< All whitespace is output. This is the default, it should be used for files that are read by humans.
    LessIndentation,  ///< Saves some space by using less space for indentation
    NoIndentation,    ///< Saves even more space by dropping all indentation from the output. The result will be noticeably less readable.
    NewlinesOnly,     ///< All unnecessary whitespace, except for newlines, is not output.
    None,             ///< No whitespace, not even newlines, is output. This should be used when it is used for data exchange, but probably not read by humans.
  };

  /// \brief Constructor
  ezOpenDdlWriter();

  virtual ~ezOpenDdlWriter() { }

  /// \brief All output is written to this binary stream.
  void SetOutputStream(ezStreamWriter* pOutput) { m_pOutput = pOutput; }

  /// \brief Configures how much whitespace is output.
  void SetWhitespaceMode(WhitespaceMode wsm) { m_WhitespaceMode = wsm; }

  /// \brief Begins outputting an object.
  void BeginObject(const char* szType, const char* szName = nullptr, bool bGlobalName = false);

  void OutputObjectName(const char* szName, bool bGlobalName);

  /// \brief Ends outputting an object.
  void EndObject();

  /// \brief Begins outputting a list of primitives of the given type.
  void BeginPrimitiveList(ezOpenDdlPrimitiveType type, const char* szName = nullptr, bool bGlobalName = false);

  /// \brief Ends outputting the list of primitives.
  void EndPrimitiveList();

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteBool(const bool* pValues, ezUInt32 count = 1);

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt8(const ezInt8* pValues, ezUInt32 count = 1);

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt16(const ezInt16* pValues, ezUInt32 count = 1);

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt32(const ezInt32* pValues, ezUInt32 count = 1);

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt64(const ezInt64* pValues, ezUInt32 count = 1);

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt8(const ezUInt8* pValues, ezUInt32 count = 1);

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt16(const ezUInt16* pValues, ezUInt32 count = 1);

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt32(const ezUInt32* pValues, ezUInt32 count = 1);

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt64(const ezUInt64* pValues, ezUInt32 count = 1);

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteFloat(const float* pValues, ezUInt32 count = 1);

  /// \brief Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteDouble(const double* pValues, ezUInt32 count = 1);

  /// \brief Writes a single string to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteString(const char* szString);


protected:
  enum State
  {
    Invalid = -3,
    Empty = -2,
    Object = -1,
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
    DdlState()
    {
      m_bPrimitivesWritten = false;
    }

    State m_State;
    bool m_bPrimitivesWritten;
  };

  EZ_FORCE_INLINE void OutputString(const char* sz) { m_pOutput->WriteBytes(sz, ezStringUtils::GetStringElementCount(sz)); }
  EZ_FORCE_INLINE void OutputString(const char* sz, ezUInt32 uiElementCount) { m_pOutput->WriteBytes(sz, uiElementCount); }
  void OutputEscapedString(const char* sz);
  void OutputIndentation();
  void OutputPrimitiveTypeName(ezOpenDdlPrimitiveType type);
  void WritePrimitiveType(ezOpenDdlWriter::State exp);

  ezInt32 m_iIndentation;
  WhitespaceMode m_WhitespaceMode;
  ezStreamWriter* m_pOutput;
  ezStringBuilder m_Temp;

  ezHybridArray<DdlState, 16> m_StateStack;
};
