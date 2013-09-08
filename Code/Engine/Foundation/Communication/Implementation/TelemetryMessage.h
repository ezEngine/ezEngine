#pragma once

#include <Core/Basics.h>
#include <Foundation/IO/MemoryStream.h>

class EZ_FOUNDATION_DLL ezTelemetryMessage
{
public:
  ezTelemetryMessage();
  ezTelemetryMessage(const ezTelemetryMessage& rhs);
  ~ezTelemetryMessage();

  void operator=(const ezTelemetryMessage& rhs);

  EZ_FORCE_INLINE ezIBinaryStreamReader& GetReader() { return m_Reader; }
  EZ_FORCE_INLINE ezIBinaryStreamWriter& GetWriter() { return m_Writer; }

  EZ_FORCE_INLINE ezUInt64 GetSystemID()  const { return m_uiSystemID; }
  EZ_FORCE_INLINE ezUInt32 GetMessageID() const { return m_uiMsgID; }

  EZ_FORCE_INLINE void SetMessageID(ezUInt64 uiSystemID, ezUInt32 uiMessageID) { m_uiSystemID = uiSystemID; m_uiMsgID = uiMessageID; }

private:
  friend class ezTelemetry;

  ezUInt64 m_uiSystemID;
  ezUInt32 m_uiMsgID;

  ezMemoryStreamStorage m_Storage;
  ezMemoryStreamReader m_Reader;
  ezMemoryStreamWriter m_Writer;
};
