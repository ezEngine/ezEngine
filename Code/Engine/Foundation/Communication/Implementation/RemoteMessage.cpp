#include <PCH.h>
#include <Foundation/Communication/RemoteMessage.h>

ezRemoteMessage::ezRemoteMessage()
  : m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
}

ezRemoteMessage::ezRemoteMessage(const ezRemoteMessage& rhs)
  : m_Storage(rhs.m_Storage)
  , m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID = rhs.m_uiMsgID;
}


ezRemoteMessage::ezRemoteMessage(ezUInt32 uiSystemID, ezUInt32 uiMessageID)
  : m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
  m_uiSystemID = uiSystemID;
  m_uiMsgID = uiMessageID;
}

void ezRemoteMessage::operator=(const ezRemoteMessage& rhs)
{
  m_Storage = rhs.m_Storage;
  m_uiApplicationID = rhs.m_uiApplicationID;
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID = rhs.m_uiMsgID;
  m_Reader.SetStorage(&m_Storage);
  m_Writer.SetStorage(&m_Storage);
}

ezRemoteMessage::~ezRemoteMessage()
{
  m_Reader.SetStorage(nullptr);
  m_Writer.SetStorage(nullptr);
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcessMessage, 1, ezRTTIDefaultAllocator<ezProcessMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_RemoteMessage);

