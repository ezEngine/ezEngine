#include <PCH.h>
#include <FileservePlugin/Network/NetworkMessage.h>

ezNetworkMessage::ezNetworkMessage()
  : m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
  m_uiSystemID = 0;
  m_uiMsgID = 0;
}

ezNetworkMessage::ezNetworkMessage(const ezNetworkMessage& rhs)
  : m_Storage(rhs.m_Storage)
  , m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID = rhs.m_uiMsgID;
}


ezNetworkMessage::ezNetworkMessage(ezUInt32 uiSystemID, ezUInt32 uiMessageID)
  : m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
  m_uiSystemID = uiSystemID;
  m_uiMsgID = uiMessageID;
}

void ezNetworkMessage::operator=(const ezNetworkMessage& rhs)
{
  m_Storage = rhs.m_Storage;
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID = rhs.m_uiMsgID;
  m_Reader.SetStorage(&m_Storage);
  m_Writer.SetStorage(&m_Storage);
}

ezNetworkMessage::~ezNetworkMessage()
{
  m_Reader.SetStorage(nullptr);
  m_Writer.SetStorage(nullptr);
}

