#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/OpenDdlParser.h>
#include <Foundation/Logging/Log.h>

/// \brief Represents a single 'object' in a DDL document, e.g. either a custom type or a primitives list.
class EZ_FOUNDATION_DLL ezOpenDdlReaderElement
{
public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Whether this is a custom object type that typically contains sub-elements.
  EZ_ALWAYS_INLINE bool IsCustomType() const { return m_PrimitiveType == ezOpenDdlPrimitiveType::Custom; } // [tested]

  /// \brief Whether this is a custom object type of the requested type.
  EZ_ALWAYS_INLINE bool IsCustomType(const char* szTypeName) const { return m_PrimitiveType == ezOpenDdlPrimitiveType::Custom && ezStringUtils::IsEqual(m_szCustomType, szTypeName); }

  /// \brief Returns the string for the custom type name.
  EZ_ALWAYS_INLINE const char* GetCustomType() const { return m_szCustomType; } // [tested]

  /// \brief Whether the name of the object is non-empty.
  EZ_ALWAYS_INLINE bool HasName() const { return !ezStringUtils::IsNullOrEmpty(m_szName); } // [tested]

  /// \brief Returns the name of the object.
  EZ_ALWAYS_INLINE const char* GetName() const { return m_szName; } // [tested]

  /// \brief Returns whether the element name is a global or a local name.
  EZ_ALWAYS_INLINE bool IsNameGlobal() const { return (m_uiNumChildElements & EZ_BIT(31)) != 0; } // [tested]

  /// \brief How many sub-elements the object has.
  ezUInt32 GetNumChildObjects() const; // [tested]

  /// \brief If this is a custom type element, the returned pointer is to the first child element.
  EZ_ALWAYS_INLINE const ezOpenDdlReaderElement* GetFirstChild() const { return reinterpret_cast<const ezOpenDdlReaderElement*>(m_pFirstChild); } // [tested]

  /// \brief If the parent is a custom type element, the next child after this is returned.
  EZ_ALWAYS_INLINE const ezOpenDdlReaderElement* GetSibling() const { return m_pSiblingElement; } // [tested]

  /// \brief For non-custom types this returns how many primitives are stored at this element.
  ezUInt32 GetNumPrimitives() const; // [tested]

  /// \brief For non-custom types this returns the type of primitive that is stored at this element.
  EZ_ALWAYS_INLINE ezOpenDdlPrimitiveType GetPrimitivesType() const { return m_PrimitiveType; } // [tested]

  /// \brief Returns true if the element stores the requested type of primitives AND has at least the desired amount of them, so that accessing the data array at certain indices is safe.
  bool HasPrimitives(ezOpenDdlPrimitiveType type, ezUInt32 uiMinNumberOfPrimitives = 1) const;

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  EZ_ALWAYS_INLINE const bool* GetPrimitivesBool() const { return reinterpret_cast<const bool*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  EZ_ALWAYS_INLINE const ezInt8* GetPrimitivesInt8() const { return reinterpret_cast<const ezInt8*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  EZ_ALWAYS_INLINE const ezInt16* GetPrimitivesInt16() const { return reinterpret_cast<const ezInt16*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  EZ_ALWAYS_INLINE const ezInt32* GetPrimitivesInt32() const { return reinterpret_cast<const ezInt32*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  EZ_ALWAYS_INLINE const ezInt64* GetPrimitivesInt64() const { return reinterpret_cast<const ezInt64*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  EZ_ALWAYS_INLINE const ezUInt8* GetPrimitivesUInt8() const { return reinterpret_cast<const ezUInt8*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  EZ_ALWAYS_INLINE const ezUInt16* GetPrimitivesUInt16() const { return reinterpret_cast<const ezUInt16*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  EZ_ALWAYS_INLINE const ezUInt32* GetPrimitivesUInt32() const { return reinterpret_cast<const ezUInt32*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  EZ_ALWAYS_INLINE const ezUInt64* GetPrimitivesUInt64() const { return reinterpret_cast<const ezUInt64*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  EZ_ALWAYS_INLINE const float* GetPrimitivesFloat() const { return reinterpret_cast<const float*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  EZ_ALWAYS_INLINE const double* GetPrimitivesDouble() const { return reinterpret_cast<const double*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  EZ_ALWAYS_INLINE const ezStringView* GetPrimitivesString() const { return reinterpret_cast<const ezStringView*>(m_pFirstChild); } // [tested]

  /// \brief Searches for a child with the given name. It does not matter whether the object's name is 'local' or 'global'.
  /// \a szName is case-sensitive.
  const ezOpenDdlReaderElement* FindChild(const char* szName) const; // [tested]

  /// \brief Searches for a child element that has the given type, name and if it is a primitives list, at least the desired number of primitives.
  const ezOpenDdlReaderElement* FindChildOfType(ezOpenDdlPrimitiveType type, const char* szName, ezUInt32 uiMinNumberOfPrimitives = 1) const;

  /// \brief Searches for a child element with the given type and optionally also a certain name.
  const ezOpenDdlReaderElement* FindChildOfType(const char* szType, const char* szName = nullptr) const;

private:
  friend class ezOpenDdlReader;

  ezOpenDdlPrimitiveType m_PrimitiveType;
  ezUInt32 m_uiNumChildElements;
  const void* m_pFirstChild;
  const ezOpenDdlReaderElement* m_pLastChild;
  const char* m_szCustomType;
  const char* m_szName;
  const ezOpenDdlReaderElement* m_pSiblingElement;
};

/// \brief An OpenDDL reader parses an entire DDL document and creates an in-memory representation of the document structure.
class EZ_FOUNDATION_DLL ezOpenDdlReader : public ezOpenDdlParser
{
public:
  ezOpenDdlReader();
  ~ezOpenDdlReader();

  /// \brief Parses the given document, returns EZ_FAILURE if an unrecoverable parsing error was encountered.
  ///
  /// \param stream is the input data.
  /// \param uiFirstLineOffset allows to adjust the reported line numbers in error messages, in case the given stream represents a sub-section of a larger file.
  /// \param pLog is used for outputting details about parsing errors. If nullptr is given, no details are logged.
  /// \param uiCacheSizeInKB is the internal cache size that the parser uses. If the parsed documents contain primitives lists with several thousand elements in a single list,
  /// increasing the cache size can improve performance, but typically this doesn't need to be adjusted.
  ezResult ParseDocument(ezStreamReader& stream, ezUInt32 uiFirstLineOffset = 0, ezLogInterface* pLog = ezLog::GetThreadLocalLogSystem(), ezUInt32 uiCacheSizeInKB = 4); // [tested]

  /// \brief Every document has exactly one root element.
  const ezOpenDdlReaderElement* GetRootElement() const; // [tested]

  /// \brief Searches for an element with a global name. NULL if there is no such element.
  const ezOpenDdlReaderElement* FindElement(const char* szGlobalName) const; // [tested]

protected:
  virtual void OnBeginObject(const char* szType, const char* szName, bool bGlobalName) override;
  virtual void OnEndObject() override;

  virtual void OnBeginPrimitiveList(ezOpenDdlPrimitiveType type, const char* szName, bool bGlobalName) override;
  virtual void OnEndPrimitiveList() override;

  virtual void OnPrimitiveBool(ezUInt32 count, const bool* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveInt8(ezUInt32 count, const ezInt8* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt16(ezUInt32 count, const ezInt16* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt32(ezUInt32 count, const ezInt32* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt64(ezUInt32 count, const ezInt64* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveUInt8(ezUInt32 count, const ezUInt8* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveUInt16(ezUInt32 count, const ezUInt16* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveUInt32(ezUInt32 count, const ezUInt32* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveUInt64(ezUInt32 count, const ezUInt64* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveFloat(ezUInt32 count, const float* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveDouble(ezUInt32 count, const double* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveString(ezUInt32 count, const ezStringView* pData, bool bThisIsAll) override;

  virtual void OnParsingError(const char* szMessage, bool bFatal, ezUInt32 uiLine, ezUInt32 uiColumn) override;

protected:
  ezOpenDdlReaderElement* CreateElement(ezOpenDdlPrimitiveType type, const char* szType, const char* szName, bool bGlobalName);
  const char* CopyString(const ezStringView& string);
  void StorePrimitiveData(bool bThisIsAll, ezUInt32 bytecount, const ezUInt8* pData);

  void ClearDataChunks();
  ezUInt8* AllocateBytes(ezUInt32 uiNumBytes);

  static const ezUInt32 s_uiChunkSize = 1000 * 4; // 4 KiB

  ezHybridArray<ezUInt8*, 16> m_DataChunks;
  ezUInt8* m_pCurrentChunk;
  ezUInt32 m_uiBytesInChunkLeft;

  ezDynamicArray<ezUInt8> m_TempCache;

  ezDeque<ezOpenDdlReaderElement> m_Elements;
  ezHybridArray<ezOpenDdlReaderElement*, 16> m_ObjectStack;

  ezDeque<ezString> m_Strings;

  ezMap<ezString, ezOpenDdlReaderElement*> m_GlobalNames;
};
