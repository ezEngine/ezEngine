#pragma once

#include <Foundation/IO/MemoryStream.h>

class EZ_FOUNDATION_DLL ezTelemetryMessage
{
public:
  ezTelemetryMessage();
  ezTelemetryMessage(const ezTelemetryMessage& rhs);
  ~ezTelemetryMessage();

  void operator=(const ezTelemetryMessage& rhs);

  EZ_ALWAYS_INLINE ezStreamReader& GetReader() { return m_Reader; }
  EZ_ALWAYS_INLINE ezStreamWriter& GetWriter() { return m_Writer; }

  EZ_ALWAYS_INLINE ezUInt32 GetSystemID() const { return m_uiSystemID; }
  EZ_ALWAYS_INLINE ezUInt32 GetMessageID() const { return m_uiMsgID; }

  EZ_ALWAYS_INLINE void SetMessageID(ezUInt32 uiSystemID, ezUInt32 uiMessageID)
  {
    m_uiSystemID = uiSystemID;
    m_uiMsgID = uiMessageID;
  }

  ezUInt64 GetMessageSize() const { return m_Storage.GetStorageSize(); }

private:
  friend class ezTelemetry;

  ezUInt32 m_uiSystemID;
  ezUInt32 m_uiMsgID;

  ezMemoryStreamStorage m_Storage;
  ezMemoryStreamReader m_Reader;
  ezMemoryStreamWriter m_Writer;
};
