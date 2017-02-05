#pragma once

#include <Foundation/IO/MemoryStream.h>

/// \todo Add move semantics for ezNetworkMessage

/// \brief Encapsulates all the data that is transmitted when sending or receiving a message with ezNetworkInterface
class EZ_FILESERVEPLUGIN_DLL ezNetworkMessage
{
public:
  ezNetworkMessage();
  ezNetworkMessage(ezUInt32 uiSystemID, ezUInt32 uiMessageID);
  ezNetworkMessage(const ezNetworkMessage& rhs);
  ~ezNetworkMessage();
  void operator=(const ezNetworkMessage& rhs);

  /// \name Sending
  ///@{

  /// \brief For setting the message IDs before sending it
  EZ_ALWAYS_INLINE void SetMessageID(ezUInt32 uiSystemID, ezUInt32 uiMessageID) { m_uiSystemID = uiSystemID; m_uiMsgID = uiMessageID; }

  /// \brief Returns a stream writer to append data to the message
  EZ_ALWAYS_INLINE ezStreamWriter& GetWriter() { return m_Writer; }


  ///@}

  /// \name Receiving
  ///@{

  /// \brief Returns a stream reader for reading the message data
  EZ_ALWAYS_INLINE ezStreamReader& GetReader() { return m_Reader; }
  EZ_ALWAYS_INLINE ezUInt32 GetApplicationID()  const { return m_uiApplicationID; }
  EZ_ALWAYS_INLINE ezUInt32 GetSystemID()  const { return m_uiSystemID; }
  EZ_ALWAYS_INLINE ezUInt32 GetMessageID() const { return m_uiMsgID; }
  EZ_ALWAYS_INLINE ezUInt32 GetMessageSize() const { return m_Storage.GetStorageSize(); }
  EZ_ALWAYS_INLINE const ezUInt8* GetMessageData() const { return m_Storage.GetData(); }

  ///@}

private:
  friend class ezNetworkInterface;

  ezUInt32 m_uiApplicationID = 0;
  ezUInt32 m_uiSystemID = 0;
  ezUInt32 m_uiMsgID = 0;

  ezMemoryStreamStorage m_Storage;
  ezMemoryStreamReader m_Reader;
  ezMemoryStreamWriter m_Writer;
};

