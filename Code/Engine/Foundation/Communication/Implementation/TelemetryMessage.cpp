#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/TelemetryMessage.h>

ezTelemetryMessage::ezTelemetryMessage()
  : m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
  m_uiSystemID = 0;
  m_uiMsgID = 0;
}

ezTelemetryMessage::ezTelemetryMessage(const ezTelemetryMessage& rhs)
  : m_Storage(rhs.m_Storage)
  , m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID = rhs.m_uiMsgID;
}

void ezTelemetryMessage::operator=(const ezTelemetryMessage& rhs)
{
  m_Storage = rhs.m_Storage;
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID = rhs.m_uiMsgID;
  m_Reader.SetStorage(&m_Storage);
  m_Writer.SetStorage(&m_Storage);
}

ezTelemetryMessage::~ezTelemetryMessage()
{
  m_Reader.SetStorage(nullptr);
  m_Writer.SetStorage(nullptr);
}
