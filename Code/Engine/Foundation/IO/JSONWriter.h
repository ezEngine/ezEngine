#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/Variant.h>

/// \brief The base class for JSON writers.
///
/// Declares a common interface for writing JSON files. Also implements some utility functions built on top of the interface (AddVariable()).
class EZ_FOUNDATION_DLL ezJSONWriter
{
public:
  /// \brief Modes to configure how much whitespace the JSON writer will output
  enum class WhitespaceMode
  {
    All,             ///< All whitespace is output. This is the default, it should be used for files that are read by humans.
    LessIndentation, ///< Saves some space by using less space for indentation
    NoIndentation,   ///< Saves even more space by dropping all indentation from the output. The result will be noticeably less readable.
    NewlinesOnly,    ///< All unnecessary whitespace, except for newlines, is not output.
    None,            ///< No whitespace, not even newlines, is output. This should be used when JSON is used for data exchange, but probably not read by humans.
  };

  /// \brief Modes to configure how arrays are written.
  enum class ArrayMode
  {
    InOneLine,      ///< All array items are written in a single line in the file.
    OneLinePerItem, ///< Each array item is put on a separate line.
  };

  /// \brief Constructor
  ezJSONWriter();

  /// \brief Destructor
  virtual ~ezJSONWriter();

  /// \brief Configures how much whitespace is output.
  void SetWhitespaceMode(WhitespaceMode whitespaceMode) { m_WhitespaceMode = whitespaceMode; }

  /// \brief Configures how arrays are written.
  void SetArrayMode(ArrayMode arrayMode) { m_ArrayMode = arrayMode; }

  /// \brief Shorthand for "BeginVariable(szName); WriteBool(value); EndVariable(); "
  void AddVariableBool(ezStringView sName, bool value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteInt32(value); EndVariable(); "
  void AddVariableInt32(ezStringView sName, ezInt32 value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteUInt32(value); EndVariable(); "
  void AddVariableUInt32(ezStringView sName, ezUInt32 value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteInt64(value); EndVariable(); "
  void AddVariableInt64(ezStringView sName, ezInt64 value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteUInt64(value); EndVariable(); "
  void AddVariableUInt64(ezStringView sName, ezUInt64 value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteFloat(value); EndVariable(); "
  void AddVariableFloat(ezStringView sName, float value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteDouble(value); EndVariable(); "
  void AddVariableDouble(ezStringView sName, double value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteString(value); EndVariable(); "
  void AddVariableString(ezStringView sName, ezStringView value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteNULL(value); EndVariable(); "
  void AddVariableNULL(ezStringView sName); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteTime(value); EndVariable(); "
  void AddVariableTime(ezStringView sName, ezTime value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteUuid(value); EndVariable(); "
  void AddVariableUuid(ezStringView sName, ezUuid value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteAngle(value); EndVariable(); "
  void AddVariableAngle(ezStringView sName, ezAngle value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteColor(value); EndVariable(); "
  void AddVariableColor(ezStringView sName, const ezColor& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteColorGamma(value); EndVariable(); "
  void AddVariableColorGamma(ezStringView sName, const ezColorGammaUB& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec2(value); EndVariable(); "
  void AddVariableVec2(ezStringView sName, const ezVec2& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec3(value); EndVariable(); "
  void AddVariableVec3(ezStringView sName, const ezVec3& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec4(value); EndVariable(); "
  void AddVariableVec4(ezStringView sName, const ezVec4& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec2I32(value); EndVariable(); "
  void AddVariableVec2I32(ezStringView sName, const ezVec2I32& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec3I32(value); EndVariable(); "
  void AddVariableVec3I32(ezStringView sName, const ezVec3I32& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec4I32(value); EndVariable(); "
  void AddVariableVec4I32(ezStringView sName, const ezVec4I32& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteQuat(value); EndVariable(); "
  void AddVariableQuat(ezStringView sName, const ezQuat& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteMat3(value); EndVariable(); "
  void AddVariableMat3(ezStringView sName, const ezMat3& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteMat4(value); EndVariable(); "
  void AddVariableMat4(ezStringView sName, const ezMat4& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteDataBuffer(value); EndVariable(); "
  void AddVariableDataBuffer(ezStringView sName, const ezDataBuffer& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVariant(value); EndVariable(); "
  void AddVariableVariant(ezStringView sName, const ezVariant& value); // [tested]


  /// \brief Writes a bool to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteBool(bool value) = 0;

  /// \brief Writes an int32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteInt32(ezInt32 value) = 0;

  /// \brief Writes a uint32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteUInt32(ezUInt32 value) = 0;

  /// \brief Writes an int64 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteInt64(ezInt64 value) = 0;

  /// \brief Writes a uint64 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteUInt64(ezUInt64 value) = 0;

  /// \brief Writes a float to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteFloat(float value) = 0;

  /// \brief Writes a double to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteDouble(double value) = 0;

  /// \brief Writes a string to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteString(ezStringView value) = 0;

  /// \brief Writes the value 'null' to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteNULL() = 0;

  /// \brief Writes a time value to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteTime(ezTime value) = 0;

  /// \brief Writes an ezColor to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteColor(const ezColor& value) = 0;

  /// \brief Writes an ezColorGammaUB to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteColorGamma(const ezColorGammaUB& value) = 0;

  /// \brief Writes an ezVec2 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec2(const ezVec2& value) = 0;

  /// \brief Writes an ezVec3 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec3(const ezVec3& value) = 0;

  /// \brief Writes an ezVec4 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec4(const ezVec4& value) = 0;

  /// \brief Writes an ezVec2I32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec2I32(const ezVec2I32& value) = 0;

  /// \brief Writes an ezVec3I32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec3I32(const ezVec3I32& value) = 0;

  /// \brief Writes an ezVec4I32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec4I32(const ezVec4I32& value) = 0;

  /// \brief Writes an ezQuat to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteQuat(const ezQuat& value) = 0;

  /// \brief Writes an ezMat3 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteMat3(const ezMat3& value) = 0;

  /// \brief Writes an ezMat4 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteMat4(const ezMat4& value) = 0;

  /// \brief Writes an ezUuid to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteUuid(const ezUuid& value) = 0;

  /// \brief Writes an ezAngle to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteAngle(ezAngle value) = 0; // [tested]

  /// \brief Writes an ezDataBuffer to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteDataBuffer(const ezDataBuffer& value) = 0; // [tested]

  /// \brief The default implementation dispatches all supported types to WriteBool, WriteInt32, etc. and asserts on the more complex types.
  ///
  /// A derived class may override this function to implement support for the remaining variant types, if required.
  virtual void WriteVariant(const ezVariant& value); // [tested]

  /// \brief Outputs a chunk of memory in some JSON form that can be interpreted as binary data when reading it again.
  ///
  /// How exactly the raw data is represented in JSON is up to the derived class. \a szDataType allows to additionally output a string
  /// that identifies the type of data.
  virtual void WriteBinaryData(ezStringView sDataType, const void* pData, ezUInt32 uiBytes, ezStringView sValueString = nullptr) = 0;

  /// \brief Begins outputting a variable. \a szName is the variable name.
  ///
  /// Between BeginVariable() and EndVariable() you can call the WriteXYZ functions once to write out the variable's data.
  /// You can also call BeginArray() and BeginObject() without a variable name to output an array or object variable.
  virtual void BeginVariable(ezStringView sName) = 0;

  /// \brief Ends outputting a variable.
  virtual void EndVariable() = 0;

  /// \brief Begins outputting an array variable.
  ///
  /// If szName is nullptr this will create an anonymous array, which is necessary when you want to put an array as a value into another array.
  /// BeginArray() with a non-nullptr value for \a szName is identical to calling BeginVariable() first. In this case EndArray() will also
  /// end the variable definition, so no additional call to EndVariable() is required.
  virtual void BeginArray(ezStringView sName = nullptr) = 0;

  /// \brief Ends outputting an array variable.
  virtual void EndArray() = 0;

  /// \brief Begins outputting an object variable.
  ///
  /// If szName is nullptr this will create an anonymous object, which is necessary when you want to put an object as a value into an array.
  /// BeginObject() with a non-nullptr value for \a szName is identical to calling BeginVariable() first. In this case EndObject() will also
  /// end the variable definition, so no additional call to EndVariable() is required.
  virtual void BeginObject(ezStringView sName = nullptr) = 0;

  /// \brief Ends outputting an object variable.
  virtual void EndObject() = 0;

  /// \brief Indicates if an error was encountered while writing
  ///
  /// If any error was encountered at any time during writing, this will return true
  bool HadWriteError() const;

protected:
  WhitespaceMode m_WhitespaceMode = WhitespaceMode::All;
  ArrayMode m_ArrayMode = ArrayMode::InOneLine;

  /// \brief called internally when there was an error during writing
  void SetWriteErrorState();

private:
  bool m_bHadWriteError = false;
};


/// \brief Standard-compliant JSON writer implementation with MongoDB-style binary data extension.
///
/// StandardJSONWriter produces fully compliant JSON output that can be read by any standard JSON parser.
/// It extends the base functionality with support for binary data using MongoDB's binary data convention,
/// allowing ezEngine-specific types to be represented in a standardized way.
///
/// Binary data handling:
/// - ezEngine types (Vec2, Vec3, matrices, etc.) encoded as MongoDB binary objects
/// - Format: {"$type": "typename", "$binary": "hexdata"}
/// - Little-endian hex encoding for cross-platform compatibility
/// - Custom data types supported via WriteBinaryData()
class EZ_FOUNDATION_DLL ezStandardJSONWriter : public ezJSONWriter
{
public:
  /// \brief Constructor.
  ezStandardJSONWriter(); // [tested]

  /// \brief Destructor.
  ~ezStandardJSONWriter(); // [tested]

  /// \brief All output is written to this binary stream.
  void SetOutputStream(ezStreamWriter* pOutput); // [tested]

  /// \brief \copydoc ezJSONWriter::WriteBool()
  virtual void WriteBool(bool value) override; // [tested]

  /// \brief \copydoc ezJSONWriter::WriteInt32()
  virtual void WriteInt32(ezInt32 value) override; // [tested]

  /// \brief \copydoc ezJSONWriter::WriteUInt32()
  virtual void WriteUInt32(ezUInt32 value) override; // [tested]

  /// \brief \copydoc ezJSONWriter::WriteInt64()
  virtual void WriteInt64(ezInt64 value) override; // [tested]

  /// \brief \copydoc ezJSONWriter::WriteUInt64()
  virtual void WriteUInt64(ezUInt64 value) override; // [tested]

  /// \brief \copydoc ezJSONWriter::WriteFloat()
  virtual void WriteFloat(float value) override; // [tested]

  /// \brief \copydoc ezJSONWriter::WriteDouble()
  virtual void WriteDouble(double value) override; // [tested]

  /// \brief \copydoc ezJSONWriter::WriteString()
  virtual void WriteString(ezStringView value) override; // [tested]

  /// \brief \copydoc ezJSONWriter::WriteNULL()
  virtual void WriteNULL() override; // [tested]

  /// \brief Writes the time value as a double (i.e. redirects to WriteDouble()).
  virtual void WriteTime(ezTime value) override; // [tested]

  /// \brief Outputs the value via WriteVec4().
  virtual void WriteColor(const ezColor& value) override; // [tested]

  /// \brief Outputs the value via WriteVec4().
  virtual void WriteColorGamma(const ezColorGammaUB& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec2(const ezVec2& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec3(const ezVec3& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec4(const ezVec4& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec2I32(const ezVec2I32& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec3I32(const ezVec3I32& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec4I32(const ezVec4I32& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteQuat(const ezQuat& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteMat3(const ezMat3& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteMat4(const ezMat4& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteUuid(const ezUuid& value) override; // [tested]

  /// \brief \copydoc ezJSONWriter::WriteFloat()
  virtual void WriteAngle(ezAngle value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteDataBuffer(const ezDataBuffer& value) override; // [tested]

  /// \brief Implements the MongoDB way of writing binary data. First writes a "$type" variable, then a "$binary" variable that represents the raw
  /// data (Hex encoded, little endian).
  virtual void WriteBinaryData(ezStringView sDataType, const void* pData, ezUInt32 uiBytes, ezStringView sValueString = nullptr) override; // [tested]

  /// \brief \copydoc ezJSONWriter::BeginVariable()
  virtual void BeginVariable(ezStringView sName) override; // [tested]

  /// \brief \copydoc ezJSONWriter::EndVariable()
  virtual void EndVariable() override; // [tested]

  /// \brief \copydoc ezJSONWriter::BeginArray()
  virtual void BeginArray(ezStringView sName = {}) override; // [tested]

  /// \brief \copydoc ezJSONWriter::EndArray()
  virtual void EndArray() override; // [tested]

  /// \brief \copydoc ezJSONWriter::BeginObject()
  virtual void BeginObject(ezStringView sName = {}) override; // [tested]

  /// \brief \copydoc ezJSONWriter::EndObject()
  virtual void EndObject() override; // [tested]

protected:
  void End();

  enum State
  {
    Invalid,
    Empty,
    Variable,
    Object,
    NamedObject,
    Array,
    NamedArray,
  };

  struct JSONState
  {
    JSONState();

    State m_State;
    bool m_bRequireComma;
    bool m_bValueWasWritten;
  };

  struct CommaWriter
  {
    CommaWriter(ezStandardJSONWriter* pWriter);
    ~CommaWriter();

    ezStandardJSONWriter* m_pWriter;
  };

  void OutputString(ezStringView s);
  void OutputEscapedString(ezStringView s);
  void OutputIndentation();

  ezInt32 m_iIndentation;
  ezStreamWriter* m_pOutput;

  ezHybridArray<JSONState, 16> m_StateStack;
};
