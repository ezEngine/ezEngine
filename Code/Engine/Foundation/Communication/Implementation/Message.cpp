#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Message.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMessage, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

ezMessageId ezMessage::s_NextMsgId = 0;


void ezMessage::PackageForTransfer(const ezMessage& msg, ezStreamWriter& inout_stream)
{
  const ezRTTI* pRtti = msg.GetDynamicRTTI();

  inout_stream << pRtti->GetTypeNameHash();
  inout_stream << (ezUInt8)pRtti->GetTypeVersion();

  msg.Serialize(inout_stream);
}

ezUniquePtr<ezMessage> ezMessage::ReplicatePackedMessage(ezStreamReader& inout_stream)
{
  ezUInt64 uiTypeHash = 0;
  inout_stream >> uiTypeHash;

  ezUInt8 uiTypeVersion = 0;
  inout_stream >> uiTypeVersion;

  static ezHashTable<ezUInt64, const ezRTTI*, ezHashHelper<ezUInt64>, ezStaticAllocatorWrapper> MessageTypes;

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

  auto pMsg = pRtti->GetAllocator()->Allocate<ezMessage>();

  pMsg->Deserialize(inout_stream, uiTypeVersion);

  return pMsg;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Message);
