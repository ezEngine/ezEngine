#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/JSONParser.h>
#include <Foundation/Types/Variant.h>

/// \brief This JSON reader will read an entire JSON document into a hierarchical structure of ezVariants.
///
/// The reader will parse the entire document and create a data structure of ezVariants, which can then be traversed easily.
/// Note that this class is much less efficient at reading large JSON documents, as it will dynamically allocate and copy objects around
/// quite a bit. For small to medium sized documents that might be good enough, for large files one should prefer to write a dedicated
/// class derived from ezJSONParser.
class EZ_FOUNDATION_DLL ezJSONReader : public ezJSONParser
{
public:
  enum class ElementType : ezInt8
  {
    None,       ///< The JSON document is entirely empty (not even containing an empty object or array)
    Dictionary, ///< The top level element in the JSON document is an object
    Array,      ///< The top level element in the JSON document is an array
  };

  ezJSONReader();

  /// \brief Reads the entire stream and creates the internal data structure that represents the JSON document.
  ///
  /// Parses the complete JSON document from the input stream, building the variant tree structure.
  /// The entire document must be valid JSON - parsing stops on the first error encountered.
  ///
  /// \param ref_input Stream containing the JSON document to parse
  /// \param uiFirstLineOffset Line number offset for error reporting (useful when JSON is embedded)
  /// \return EZ_SUCCESS if parsing completed without errors, EZ_FAILURE if any parsing error occurred
  ///
  /// \note After successful parsing, use GetTopLevelObject() or GetTopLevelArray() to access the data.
  ezResult Parse(ezStreamReader& ref_input, ezUInt32 uiFirstLineOffset = 0);

  /// \brief Returns the top-level object of the JSON document.
  const ezVariantDictionary& GetTopLevelObject() const { return m_Stack.PeekBack().m_Dictionary; }

  /// \brief Returns the top-level array of the JSON document.
  const ezVariantArray& GetTopLevelArray() const { return m_Stack.PeekBack().m_Array; }

  /// \brief Returns whether the top level element is an array or an object.
  ElementType GetTopLevelElementType() const { return m_Stack.PeekBack().m_Mode; }

private:
  /// \brief This function can be overridden to skip certain variables, however the overriding function must still call this.
  virtual bool OnVariable(ezStringView sVarName) override;

  /// \brief [internal] Do not override further.
  virtual void OnReadValue(ezStringView sValue) override;

  /// \brief [internal] Do not override further.
  virtual void OnReadValue(double fValue) override;

  /// \brief [internal] Do not override further.
  virtual void OnReadValue(bool bValue) override;

  /// \brief [internal] Do not override further.
  virtual void OnReadValueNULL() override;

  /// \brief [internal] Do not override further.
  virtual void OnBeginObject() override;

  /// \brief [internal] Do not override further.
  virtual void OnEndObject() override;

  /// \brief [internal] Do not override further.
  virtual void OnBeginArray() override;

  /// \brief [internal] Do not override further.
  virtual void OnEndArray() override;

  virtual void OnParsingError(ezStringView sMessage, bool bFatal, ezUInt32 uiLine, ezUInt32 uiColumn) override;

protected:
  struct Element
  {
    ezString m_sName;
    ElementType m_Mode = ElementType::None;
    ezVariantArray m_Array;
    ezVariantDictionary m_Dictionary;
  };

  ezHybridArray<Element, 32> m_Stack;

  bool m_bParsingError = false;
  ezString m_sLastName;
};
