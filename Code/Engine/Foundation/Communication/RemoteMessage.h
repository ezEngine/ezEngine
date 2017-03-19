#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/Reflection.h>

/// \todo Add move semantics for ezRemoteMessage

/// \brief Encapsulates all the data that is transmitted when sending or receiving a message with ezRemoteInterface
class EZ_FOUNDATION_DLL ezRemoteMessage
{
public:
  ezRemoteMessage();
  ezRemoteMessage(ezUInt32 uiSystemID, ezUInt32 uiMessageID);
  ezRemoteMessage(const ezRemoteMessage& rhs);
  ~ezRemoteMessage();
  void operator=(const ezRemoteMessage& rhs);

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
  friend class ezRemoteInterface;

  ezUInt32 m_uiApplicationID = 0;
  ezUInt32 m_uiSystemID = 0;
  ezUInt32 m_uiMsgID = 0;

  ezMemoryStreamStorage m_Storage;
  ezMemoryStreamReader m_Reader;
  ezMemoryStreamWriter m_Writer;
};

/// \brief Base class for IPC messages transmitted by ezIpcChannel.
class EZ_FOUNDATION_DLL ezProcessMessage : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcessMessage, ezReflectedClass);
public:
  ezProcessMessage() {}
};
