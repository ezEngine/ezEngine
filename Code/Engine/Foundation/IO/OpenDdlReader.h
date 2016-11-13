#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlParser.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>

class EZ_FOUNDATION_DLL ezOpenDdlReaderElement
{
public:
  EZ_DECLARE_POD_TYPE();

  EZ_FORCE_INLINE bool IsCustomType() const { return m_PrimitiveType == ezOpenDdlPrimitiveType::Custom; }
  EZ_FORCE_INLINE const char* GetCustomType() const { return m_szCustomType; }
  EZ_FORCE_INLINE bool HasName() const { return !ezStringUtils::IsNullOrEmpty(m_szName); }
  EZ_FORCE_INLINE const char* GetName() const { return m_szName; }
  ezUInt32 GetNumChildObjects() const;
  //EZ_FORCE_INLINE const ezOpenDdlReaderElement* GetChildObjects() const { reinterpret_cast<const ezOpenDdlReaderElement*>(m_pChildElements); }
  ezUInt32 GetNumPrimitives() const;
  EZ_FORCE_INLINE ezOpenDdlPrimitiveType GetPrimitivesType() const { return m_PrimitiveType; }
  EZ_FORCE_INLINE const bool* GetPrimitivesBool() const { return reinterpret_cast<const bool*>(m_pFirstChild); }
  EZ_FORCE_INLINE const ezInt8* GetPrimitivesInt8() const { return reinterpret_cast<const ezInt8*>(m_pFirstChild); }
  EZ_FORCE_INLINE const ezInt16* GetPrimitivesInt16() const { return reinterpret_cast<const ezInt16*>(m_pFirstChild); }
  EZ_FORCE_INLINE const ezInt32* GetPrimitivesInt32() const { return reinterpret_cast<const ezInt32*>(m_pFirstChild); }
  EZ_FORCE_INLINE const ezInt64* GetPrimitivesInt64() const { return reinterpret_cast<const ezInt64*>(m_pFirstChild); }
  EZ_FORCE_INLINE const ezUInt8* GetPrimitivesUInt8() const { return reinterpret_cast<const ezUInt8*>(m_pFirstChild); }
  EZ_FORCE_INLINE const ezUInt16* GetPrimitivesUInt16() const { return reinterpret_cast<const ezUInt16*>(m_pFirstChild); }
  EZ_FORCE_INLINE const ezUInt32* GetPrimitivesUInt32() const { return reinterpret_cast<const ezUInt32*>(m_pFirstChild); }
  EZ_FORCE_INLINE const ezUInt64* GetPrimitivesUInt64() const { return reinterpret_cast<const ezUInt64*>(m_pFirstChild); }
  EZ_FORCE_INLINE const float* GetPrimitivesFloat() const { return reinterpret_cast<const float*>(m_pFirstChild); }
  EZ_FORCE_INLINE const double* GetPrimitivesDouble() const { return reinterpret_cast<const double*>(m_pFirstChild); }
  EZ_FORCE_INLINE const ezStringView* GetPrimitivesString() const { return reinterpret_cast<const ezStringView*>(m_pFirstChild); }
  //const ezOpenDdlReaderElement* FindChildElement(const char* szName) const;

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

class EZ_FOUNDATION_DLL ezOpenDdlReader : public ezOpenDdlParser
{
public:
  ezOpenDdlReader();
  ~ezOpenDdlReader();

  ezResult ParseDocument();

  /// \todo Get by global name
  const ezOpenDdlReaderElement* GetRootElement() const;

private:
  virtual void OnBeginObject(const char* szType, const char* szName, bool bGlobalName) override;
  virtual void OnEndObject() override;

  virtual void OnBeginPrimitiveList(ezOpenDdlPrimitiveType type, const char* szName, bool bGlobalName) override;
  virtual void OnEndPrimitiveList() override;

  virtual void OnPrimitiveBool(ezUInt32 count, const bool* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveInt8(ezUInt32 count, const ezInt8* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt16(ezUInt32 count, const ezInt16* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt32(ezUInt32 count, const ezInt32* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt64(ezUInt32 count, const ezInt64* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveUInt8 (ezUInt32 count, const ezUInt8* pData, bool bThisIsAll) override;
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

  ezDynamicArray<bool> m_BoolData;
  // ...
  ezDeque<ezString> m_Strings;

  ezMap<ezString, ezOpenDdlReaderElement*> m_GlobalNames;
};

