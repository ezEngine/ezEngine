#include <PCH.h>
#include <Foundation/Communication/Message.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMessage, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezMessageId ezMessage::s_uiNextMsgId = 0;


void ezMessage::PackageForTransfer(const ezMessage& msg, ezStreamWriter& stream)
{
  const ezRTTI* pRtti = msg.GetDynamicRTTI();

  stream << pRtti->GetTypeNameHash();
  stream << (ezUInt8)pRtti->GetTypeVersion();

  msg.Serialize(stream);
}

ezMessage* ezMessage::ReplicatePackedMessage(ezStreamReader& stream)
{
  ezUInt32 uiTypeHash = 0;
  stream >> uiTypeHash;

  ezUInt8 uiTypeVersion = 0;
  stream >> uiTypeVersion;

  static ezHashTable<ezUInt32, const ezRTTI*, ezHashHelper<ezUInt32>, ezStaticAllocatorWrapper> MessageTypes;

  const ezRTTI* pRtti = nullptr;
  if (!MessageTypes.TryGetValue(uiTypeHash, pRtti))
  {
    for (pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
    {
      if (pRtti->GetTypeNameHash() == uiTypeHash)
      {
        MessageTypes[uiTypeHash] = pRtti;
        break;
      }
    }
  }

  if (pRtti == nullptr || !pRtti->GetAllocator()->CanAllocate())
    return nullptr;

  ezMessage* pMsg = reinterpret_cast<ezMessage*>(pRtti->GetAllocator()->Allocate());

  /// \todo It would be nice if we could pack this into a ezUniquePtr such that it correctly deallocates the message

  pMsg->Deserialize(stream, uiTypeVersion);

  return pMsg;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Message);



