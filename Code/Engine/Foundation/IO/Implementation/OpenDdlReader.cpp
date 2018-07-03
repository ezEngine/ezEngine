#include <PCH.h>

#include <Foundation/IO/OpenDdlReader.h>

ezOpenDdlReader::ezOpenDdlReader()
{
  m_pCurrentChunk = nullptr;
  m_uiBytesInChunkLeft = 0;
}

ezOpenDdlReader::~ezOpenDdlReader()
{
  ClearDataChunks();
}

ezResult ezOpenDdlReader::ParseDocument(ezStreamReader& stream, ezUInt32 uiFirstLineOffset, ezLogInterface* pLog, ezUInt32 uiCacheSizeInKB)
{
  EZ_ASSERT_DEBUG(m_ObjectStack.IsEmpty(), "A reader can only be used once.");

  SetLogInterface(pLog);
  SetCacheSize(uiCacheSizeInKB);
  SetInputStream(stream, uiFirstLineOffset);

  m_TempCache.Reserve(s_uiChunkSize);

  ezOpenDdlReaderElement* pElement = &m_Elements.ExpandAndGetRef();
  pElement->m_pFirstChild = nullptr;
  pElement->m_pLastChild = nullptr;
  pElement->m_PrimitiveType = ezOpenDdlPrimitiveType::Custom;
  pElement->m_pSiblingElement = nullptr;
  pElement->m_szCustomType = CopyString("root");
  pElement->m_szName = nullptr;
  pElement->m_uiNumChildElements = 0;

  m_ObjectStack.PushBack(pElement);

  return ParseAll();
}

const ezOpenDdlReaderElement* ezOpenDdlReader::GetRootElement() const
{
  EZ_ASSERT_DEBUG(!m_ObjectStack.IsEmpty(), "The reader has not parsed any document yet or an error occurred during parsing.");

  return m_ObjectStack[0];
}


const ezOpenDdlReaderElement* ezOpenDdlReader::FindElement(const char* szGlobalName) const
{
  return m_GlobalNames.GetValueOrDefault(szGlobalName, nullptr);
}

const char* ezOpenDdlReader::CopyString(const ezStringView& string)
{
  if (string.IsEmpty())
    return nullptr;

  // no idea how to make this more efficient without running into lots of other problems
  m_Strings.PushBack(string);
  return m_Strings.PeekBack().GetData();
}

ezOpenDdlReaderElement* ezOpenDdlReader::CreateElement(ezOpenDdlPrimitiveType type, const char* szType, const char* szName, bool bGlobalName)
{
  ezOpenDdlReaderElement* pElement = &m_Elements.ExpandAndGetRef();
  pElement->m_pFirstChild = nullptr;
  pElement->m_pLastChild = nullptr;
  pElement->m_PrimitiveType = type;
  pElement->m_pSiblingElement = nullptr;
  pElement->m_szCustomType = szType;
  pElement->m_szName = CopyString(szName);
  pElement->m_uiNumChildElements = 0;

  if (bGlobalName)
  {
    pElement->m_uiNumChildElements = EZ_BIT(31);
  }

  if (bGlobalName && !ezStringUtils::IsNullOrEmpty(szName))
  {
    m_GlobalNames[szName] = pElement;
  }

  ezOpenDdlReaderElement* pParent = m_ObjectStack.PeekBack();
  pParent->m_uiNumChildElements++;

  if (pParent->m_pFirstChild == nullptr)
  {
    pParent->m_pFirstChild = pElement;
    pParent->m_pLastChild = pElement;
  }
  else
  {
    ((ezOpenDdlReaderElement*)pParent->m_pLastChild)->m_pSiblingElement = pElement;
    pParent->m_pLastChild = pElement;
  }

  m_ObjectStack.PushBack(pElement);

  return pElement;
}


void ezOpenDdlReader::OnBeginObject(const char* szType, const char* szName, bool bGlobalName)
{
  CreateElement(ezOpenDdlPrimitiveType::Custom, CopyString(szType), szName, bGlobalName);
}

void ezOpenDdlReader::OnEndObject()
{
  m_ObjectStack.PopBack();
}

void ezOpenDdlReader::OnBeginPrimitiveList(ezOpenDdlPrimitiveType type, const char* szName, bool bGlobalName)
{
  CreateElement(type, nullptr, szName, bGlobalName);

  m_TempCache.Clear();
}

void ezOpenDdlReader::OnEndPrimitiveList()
{
  // if we had to temporarily store the primitive data, copy it into a new destination
  if (!m_TempCache.IsEmpty())
  {
    ezUInt8* pTarget = AllocateBytes(m_TempCache.GetCount());
    m_ObjectStack.PeekBack()->m_pFirstChild = pTarget;

    ezMemoryUtils::Copy(pTarget, m_TempCache.GetData(), m_TempCache.GetCount());
  }

  m_ObjectStack.PopBack();
}

void ezOpenDdlReader::StorePrimitiveData(bool bThisIsAll, ezUInt32 bytecount, const ezUInt8* pData)
{
  ezUInt8* pTarget = nullptr;

  if (!bThisIsAll || !m_TempCache.IsEmpty())
  {
    // if this is not all, accumulate the data in a temp buffer
    ezUInt32 offset = m_TempCache.GetCount();
    m_TempCache.SetCountUninitialized(m_TempCache.GetCount() + bytecount);
    pTarget = &m_TempCache[offset]; // have to index m_TempCache after the resize, otherwise it could be empty and not like it
  }
  else
  {
    // otherwise, allocate the final storage immediately
    pTarget = AllocateBytes(bytecount);
    m_ObjectStack.PeekBack()->m_pFirstChild = pTarget;
  }

  ezMemoryUtils::Copy(pTarget, pData, bytecount);
}


void ezOpenDdlReader::OnPrimitiveBool(ezUInt32 count, const bool* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(bool) * count, (const ezUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void ezOpenDdlReader::OnPrimitiveInt8(ezUInt32 count, const ezInt8* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(ezInt8) * count, (const ezUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void ezOpenDdlReader::OnPrimitiveInt16(ezUInt32 count, const ezInt16* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(ezInt16) * count, (const ezUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void ezOpenDdlReader::OnPrimitiveInt32(ezUInt32 count, const ezInt32* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(ezInt32) * count, (const ezUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void ezOpenDdlReader::OnPrimitiveInt64(ezUInt32 count, const ezInt64* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(ezInt64) * count, (const ezUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void ezOpenDdlReader::OnPrimitiveUInt8(ezUInt32 count, const ezUInt8* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(ezUInt8) * count, (const ezUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void ezOpenDdlReader::OnPrimitiveUInt16(ezUInt32 count, const ezUInt16* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(ezUInt16) * count, (const ezUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void ezOpenDdlReader::OnPrimitiveUInt32(ezUInt32 count, const ezUInt32* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(ezUInt32) * count, (const ezUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void ezOpenDdlReader::OnPrimitiveUInt64(ezUInt32 count, const ezUInt64* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(ezUInt64) * count, (const ezUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void ezOpenDdlReader::OnPrimitiveFloat(ezUInt32 count, const float* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(float) * count, (const ezUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void ezOpenDdlReader::OnPrimitiveDouble(ezUInt32 count, const double* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(double) * count, (const ezUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void ezOpenDdlReader::OnPrimitiveString(ezUInt32 count, const ezStringView* pData, bool bThisIsAll)
{
  const ezUInt32 uiDataSize = count * sizeof(ezStringView);

  const ezUInt32 offset = m_TempCache.GetCount();
  m_TempCache.SetCountUninitialized(m_TempCache.GetCount() + uiDataSize);
  ezStringView* pTarget = (ezStringView*)&m_TempCache[offset];

  for (ezUInt32 i = 0; i < count; ++i)
  {
    const char* szStart = CopyString(pData[i]);
    pTarget[i] = ezStringView(szStart, szStart + pData[i].GetElementCount());
  }

  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}


void ezOpenDdlReader::OnParsingError(const char* szMessage, bool bFatal, ezUInt32 uiLine, ezUInt32 uiColumn)
{
  m_ObjectStack.Clear();
  m_GlobalNames.Clear();
  m_Elements.Clear();

  ClearDataChunks();
}

//////////////////////////////////////////////////////////////////////////

void ezOpenDdlReader::ClearDataChunks()
{
  for (ezUInt32 i = 0; i < m_DataChunks.GetCount(); ++i)
  {
    EZ_DEFAULT_DELETE(m_DataChunks[i]);
  }

  m_DataChunks.Clear();
}

ezUInt8* ezOpenDdlReader::AllocateBytes(ezUInt32 uiNumBytes)
{
  // if the requested data is very large, just allocate it as an individual chunk
  if (uiNumBytes > s_uiChunkSize / 2)
  {
    ezUInt8* pResult = EZ_DEFAULT_NEW_ARRAY(ezUInt8, uiNumBytes).GetPtr();
    m_DataChunks.PushBack(pResult);
    return pResult;
  }

  // if our current chunk is too small, discard the remaining free bytes and just allocate a new chunk
  if (m_uiBytesInChunkLeft < uiNumBytes)
  {
    m_pCurrentChunk = EZ_DEFAULT_NEW_ARRAY(ezUInt8, s_uiChunkSize).GetPtr();
    m_uiBytesInChunkLeft = s_uiChunkSize;
    m_DataChunks.PushBack(m_pCurrentChunk);
  }

  // no fulfill the request from the current chunk
  ezUInt8* pResult = m_pCurrentChunk;
  m_pCurrentChunk += uiNumBytes;
  m_uiBytesInChunkLeft -= uiNumBytes;

  return pResult;
}

//////////////////////////////////////////////////////////////////////////

ezUInt32 ezOpenDdlReaderElement::GetNumChildObjects() const
{
  if (m_PrimitiveType != ezOpenDdlPrimitiveType::Custom)
    return 0;

  return m_uiNumChildElements & (~EZ_BIT(31)); // Bit 31 stores whether the name is global
}

ezUInt32 ezOpenDdlReaderElement::GetNumPrimitives() const
{
  if (m_PrimitiveType == ezOpenDdlPrimitiveType::Custom)
    return 0;

  return m_uiNumChildElements & (~EZ_BIT(31)); // Bit 31 stores whether the name is global
}


bool ezOpenDdlReaderElement::HasPrimitives(ezOpenDdlPrimitiveType type, ezUInt32 uiMinNumberOfPrimitives /*= 1*/) const
{
  /// \test This is new

  if (m_PrimitiveType != type)
    return false;

  return m_uiNumChildElements >= uiMinNumberOfPrimitives;
}

const ezOpenDdlReaderElement* ezOpenDdlReaderElement::FindChild(const char* szName) const
{
  EZ_ASSERT_DEBUG(m_PrimitiveType == ezOpenDdlPrimitiveType::Custom, "Cannot search for a child object in a primitives list");

  const ezOpenDdlReaderElement* pChild = static_cast<const ezOpenDdlReaderElement*>(m_pFirstChild);

  while (pChild)
  {
    if (ezStringUtils::IsEqual(pChild->GetName(), szName))
    {
      return pChild;
    }

    pChild = pChild->GetSibling();
  }

  return nullptr;
}

const ezOpenDdlReaderElement* ezOpenDdlReaderElement::FindChildOfType(ezOpenDdlPrimitiveType type, const char* szName, ezUInt32 uiMinNumberOfPrimitives /* = 1*/) const
{
  /// \test This is new

  EZ_ASSERT_DEBUG(m_PrimitiveType == ezOpenDdlPrimitiveType::Custom, "Cannot search for a child object in a primitives list");

  const ezOpenDdlReaderElement* pChild = static_cast<const ezOpenDdlReaderElement*>(m_pFirstChild);

  while (pChild)
  {
    if (pChild->GetPrimitivesType() == type && ezStringUtils::IsEqual(pChild->GetName(), szName))
    {
      if (type == ezOpenDdlPrimitiveType::Custom || pChild->GetNumPrimitives() >= uiMinNumberOfPrimitives)
        return pChild;
    }

    pChild = pChild->GetSibling();
  }

  return nullptr;
}

const ezOpenDdlReaderElement* ezOpenDdlReaderElement::FindChildOfType(const char* szType, const char* szName /*= nullptr*/) const
{
  const ezOpenDdlReaderElement* pChild = static_cast<const ezOpenDdlReaderElement*>(m_pFirstChild);

  while (pChild)
  {
    if (pChild->GetPrimitivesType() == ezOpenDdlPrimitiveType::Custom &&
        ezStringUtils::IsEqual(pChild->GetCustomType(), szType) &&
        (szName == nullptr || ezStringUtils::IsEqual(pChild->GetName(), szName)))
    {
      return pChild;
    }

    pChild = pChild->GetSibling();
  }

  return nullptr;
}



EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_OpenDdlReader);
