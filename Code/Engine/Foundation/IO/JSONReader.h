#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/JSONParser.h>
#include <Foundation/Basics/Types/Variant.h>

/// \brief This JSON reader will read an entire JSON document into a hierarchical structure of ezVariants.
///
/// The reader will parse the entire document and create a data structure of ezVariants, which can then be traversed easily.
/// Note that this class is much less efficient at reading large JSON documents, as it will dynamically allocate and copy objects around
/// quite a bit. For small to medium sized documents that might be good enough, for large files one should prefer to write a dedicated
/// class derived from ezJSONParser.
class EZ_FOUNDATION_DLL ezJSONReader : public ezJSONParser
{
public:

  /// \brief Reads the entire stream and creates the internal data structure that represents the JSON document. Returns EZ_FAILURE if any parsing error occurred.
  ezResult Parse(ezStreamReaderBase& pInput);

  /// \brief Returns the top-level object of the JSON document.
  const ezVariantDictionary& GetTopLevelObject() const
  {
    return m_Stack.PeekBack().m_Dictionary;
  }

private:

  /// \brief This function can be overridden to skip certain variables, however the overriding function must still call this.
  virtual bool OnVariable(const char* szVarName) EZ_OVERRIDE;

  /// \brief [internal] Do not call override further.
  virtual void OnReadValue(const char* szValue) EZ_OVERRIDE;

  /// \brief [internal] Do not call override further.
  virtual void OnReadValue(double fValue) EZ_OVERRIDE;

  /// \brief [internal] Do not call override further.
  virtual void OnReadValue(bool bValue) EZ_OVERRIDE;

  /// \brief [internal] Do not call override further.
  virtual void OnBeginObject() EZ_OVERRIDE;

  /// \brief [internal] Do not call override further.
  virtual void OnEndObject() EZ_OVERRIDE;

  /// \brief [internal] Do not call override further.
  virtual void OnBeginArray() EZ_OVERRIDE;

  /// \brief [internal] Do not call override further.
  virtual void OnEndArray() EZ_OVERRIDE;

  virtual void OnParsingError(const char* szMessage, bool bFatal) { m_bParsingError = true;  }

private:
  struct Element
  {
    ezString m_sName;

    ezInt8 m_iMode; // 0 == Array, 1 == Dictionary
    ezVariantArray m_Array;
    ezVariantDictionary m_Dictionary;
  };

  ezHybridArray<Element, 32> m_Stack;

  bool m_bParsingError;
  ezString m_sLastName;
};

