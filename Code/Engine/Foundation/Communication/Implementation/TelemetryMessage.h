#pragma once

#include <Foundation/IO/MemoryStream.h>

class EZ_FOUNDATION_DLL ezTelemetryMessage
{
public:
  ezTelemetryMessage();
  ezTelemetryMessage(const ezTelemetryMessage& rhs);
  ~ezTelemetryMessage();

  void operator=(const ezTelemetryMessage& rhs);

  EZ_FORCE_INLINE ezStreamReaderBase& GetReader() { return m_Reader; }
  EZ_FORCE_INLINE ezStreamWriterBase& GetWriter() { return m_Writer; }

  EZ_FORCE_INLINE ezUInt32 GetSystemID()  const { return m_uiSystemID; }
  EZ_FORCE_INLINE ezUInt32 GetMessageID() const { return m_uiMsgID; }

  EZ_FORCE_INLINE void SetMessageID(ezUInt32 uiSystemID, ezUInt32 uiMessageID) { m_uiSystemID = uiSystemID; m_uiMsgID = uiMessageID; }

private:
  friend class ezTelemetry;

  ezUInt32 m_uiSystemID;
  ezUInt32 m_uiMsgID;

  ezMemoryStreamStorage m_Storage;
  ezMemoryStreamReader m_Reader;
  ezMemoryStreamWriter m_Writer;
};

