#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Containers/HybridArray.h>

class ezLogInterface;

/// \brief A low level JSON parser that can incrementally parse the structure of a JSON document.
///
/// The document structure is returned through virtual functions that need to be overridden. 
class EZ_FOUNDATION_DLL ezJSONParser
{
public:
  /// \brief Constructor.
  ezJSONParser();

  virtual ~ezJSONParser() { }

  /// \brief Allows to specify an ezLogInterface through which errors and warnings are reported.
  void SetLogInterface(ezLogInterface* pLog) { m_pLogInterface = pLog; }

protected:

  /// \brief Resets the parser to the start state and configures it to read from the given stream.
  void SetInputStream(ezStreamReaderBase& stream, ezUInt32 uiFirstLineOffset = 0);

  /// \brief Does one parsing step.
  ///
  /// While this function returns true, the document has not been parsed completely.
  /// This function may call any of the OnSomething functions through which the structure of the document is obtained.
  /// This function calls at most one such callback, but there is no guarantee that it calls any at all, it might just
  /// advance its internal state.
  bool ContinueParsing();

  /// \brief Calls ContinueParsing() in a loop until that returns false.
  void ParseAll();

  /// \brief Skips the rest of the currently open object. No OnEndArray() and OnEndObject() calls will be done for this object,
  /// cleanup must be done manually.
  void SkipObject();

  /// \brief Skips the rest of the currently open array. No OnEndArray() and OnEndObject() calls will be done for this object,
  /// cleanup must be done manually.
  void SkipArray();

  /// \brief Outputs that a parsing error was detected (via OnParsingError) and stops further parsing, if bFatal is set to true.
  void ParsingError(const char* szMessage, bool bFatal);

  ezLogInterface* m_pLogInterface;

private:

  /// \brief Called whenever a new variable is encountered. The variable name is passed along.
  /// At this point the type of the variable (simple, array, object) is not yet determined.
  ///
  /// The entire variable (independent of whether it is a simple value, an array or an object) can
  /// be skipped by returning false.
  virtual bool OnVariable(const char* szVarName) = 0;

  /// \brief Called whenever a new value is read.
  ///
  /// Directly following a call to OnVariable(), this means that the variable is a simple variable.
  /// In between calls to OnBeginArray() and OnEndArray() it is another value in the array.
  virtual void OnReadValue(const char* szValue) = 0;

  /// \brief \copydoc ezJSONParser::OnReadValue()
  virtual void OnReadValue(double fValue) = 0;

  /// \brief \copydoc ezJSONParser::OnReadValue()
  virtual void OnReadValue(bool bValue) = 0;

  /// \brief \copydoc ezJSONParser::OnReadValue()
  virtual void OnReadValueNULL() = 0;

  /// \brief Called when a new object is encountered.
  ///
  /// Directly following a call to OnVariable(), this means the variable is of type 'object' (and has a name).
  /// In between calls to OnBeginArray() and OnEndArray() it is another value in the array.
  virtual void OnBeginObject() = 0;

  /// \brief Called when the end of an object is encountered.
  virtual void OnEndObject() = 0;

  /// \brief Called when a new array is encountered.
  ///
  /// Directly following a call to OnVariable(), this means the variable is of type 'array' (and has a name).
  /// In between calls to OnBeginArray() and OnEndArray() it is another value in the array.
  virtual void OnBeginArray() = 0;

  /// \brief Called when the end of an array is encountered.
  virtual void OnEndArray() = 0;

  /// \brief Called when something unexpected is encountered in the JSON document.
  /// 
  /// The error message describes what was expected and what was encountered.
  /// If bFatal is true, the error has left the parser in an unrecoverable state and thus it not continue parsing.
  /// In that case client code will need to clean up it's open state, as no further OnEndObject() / OnEndArray() will be called.
  /// If bFatal is false, the document does not contain valid JSON, but the parser is able to continue still.
  virtual void OnParsingError(const char* szMessage, bool bFatal, ezUInt32 uiLine, ezUInt32 uiColumn) { }

private:
  enum State
  {
    NotStarted,
    Finished,
    ReadingObject,
    ReadingArray,
    ReadingValue,
    ReadingVariable,
    ExpectSeparator
  };

  struct JSONState
  {
    JSONState()
    {
    }

    State m_State;
  };

  void StartParsing();
  void SkipWhitespace();
  void SkipString();
  void ReadString();
  double ReadNumber();
  void ReadWord();

  void ContinueObject();
  void ContinueArray();
  void ContinueVariable();
  void ContinueValue();
  void ContinueSeparator();

  bool ReadCharacter(bool bSkipComments);
  void ReadNextByte();

  void SkipStack(State s);

  ezUInt8 m_uiCurByte;
  ezUInt8 m_uiNextByte;
  ezUInt32 m_uiCurLine;
  ezUInt32 m_uiCurColumn;

  ezStreamReaderBase* m_pInput;
  ezHybridArray<JSONState, 32> m_StateStack;
  ezHybridArray<ezUInt8, 4096> m_TempString;

  bool m_bSkippingMode;
  
};

