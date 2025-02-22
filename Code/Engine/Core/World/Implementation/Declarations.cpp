#include <Core/CorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/World/World.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGameObjectHandle, ezNoBase, 1, ezRTTIDefaultAllocator<ezGameObjectHandle>)
EZ_END_STATIC_REFLECTED_TYPE;
EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezGameObjectHandle);

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezComponentHandle, ezNoBase, 1, ezRTTIDefaultAllocator<ezComponentHandle>)
EZ_END_STATIC_REFLECTED_TYPE;
EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezComponentHandle);

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezObjectMode, 1)
  EZ_ENUM_CONSTANTS(ezObjectMode::Automatic, ezObjectMode::ForceDynamic)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezComponentMode, 1)
  EZ_ENUM_CONSTANTS(ezComponentMode::Static, ezComponentMode::Dynamic)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezObjectMsgQueueType, 1)
  EZ_ENUM_CONSTANTS(ezObjectMsgQueueType::PostAsync, ezObjectMsgQueueType::PostTransform, ezObjectMsgQueueType::NextFrame, ezObjectMsgQueueType::AfterInitialized)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezOnComponentFinishedAction, 1)
  EZ_ENUM_CONSTANTS(ezOnComponentFinishedAction::None, ezOnComponentFinishedAction::DeleteComponent, ezOnComponentFinishedAction::DeleteGameObject)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezOnComponentFinishedAction2, 1)
  EZ_ENUM_CONSTANTS(ezOnComponentFinishedAction2::None, ezOnComponentFinishedAction2::DeleteComponent, ezOnComponentFinishedAction2::DeleteGameObject, ezOnComponentFinishedAction2::Restart)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

void operator<<(ezStreamWriter& inout_stream, const ezGameObjectHandle& hValue)
{
  EZ_IGNORE_UNUSED(inout_stream);
  EZ_IGNORE_UNUSED(hValue);
  EZ_ASSERT_DEV(false, "This function should not be called. Use ezWorldWriter::WriteGameObjectHandle instead.");
}

void operator>>(ezStreamReader& inout_stream, ezGameObjectHandle& ref_hValue)
{
  EZ_IGNORE_UNUSED(inout_stream);
  EZ_IGNORE_UNUSED(ref_hValue);
  EZ_ASSERT_DEV(false, "This function should not be called. Use ezWorldReader::ReadGameObjectHandle instead.");
}

void operator<<(ezStreamWriter& inout_stream, const ezComponentHandle& hValue)
{
  EZ_IGNORE_UNUSED(inout_stream);
  EZ_IGNORE_UNUSED(hValue);
  EZ_ASSERT_DEV(false, "This function should not be called. Use ezWorldWriter::WriteComponentHandle instead.");
}

void operator>>(ezStreamReader& inout_stream, ezComponentHandle& ref_hValue)
{
  EZ_IGNORE_UNUSED(inout_stream);
  EZ_IGNORE_UNUSED(ref_hValue);
  EZ_ASSERT_DEV(false, "This function should not be called. Use ezWorldReader::ReadComponentHandle instead.");
}

//////////////////////////////////////////////////////////////////////////

namespace
{
  template <typename T>
  void HandleFinishedActionImpl(ezComponent* pComponent, typename T::Enum action)
  {
    if (action == T::DeleteGameObject)
    {
      // Send a message to the owner object to check whether another component wants to delete this object later.
      // Can't use ezGameObject::SendMessage because the object would immediately delete itself and furthermore the sender component needs to be
      // filtered out here.
      ezMsgDeleteGameObject msg;

      for (ezComponent* pComp : pComponent->GetOwner()->GetComponents())
      {
        if (pComp == pComponent)
          continue;

        pComp->SendMessage(msg);
        if (msg.m_bCancel)
        {
          action = T::DeleteComponent;
          break;
        }
      }

      if (action == T::DeleteGameObject)
      {
        pComponent->GetWorld()->DeleteObjectDelayed(pComponent->GetOwner()->GetHandle());
        return;
      }
    }

    if (action == T::DeleteComponent)
    {
      pComponent->GetOwningManager()->DeleteComponent(pComponent->GetHandle());
    }
  }

  template <typename T>
  void HandleDeleteObjectMsgImpl(ezMsgDeleteGameObject& ref_msg, ezEnum<T>& ref_action)
  {
    if (ref_action == T::DeleteComponent)
    {
      ref_msg.m_bCancel = true;
      ref_action = T::DeleteGameObject;
    }
    else if (ref_action == T::DeleteGameObject)
    {
      ref_msg.m_bCancel = true;
    }
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

void ezOnComponentFinishedAction::HandleFinishedAction(ezComponent* pComponent, ezOnComponentFinishedAction::Enum action)
{
  HandleFinishedActionImpl<ezOnComponentFinishedAction>(pComponent, action);
}

void ezOnComponentFinishedAction::HandleDeleteObjectMsg(ezMsgDeleteGameObject& ref_msg, ezEnum<ezOnComponentFinishedAction>& ref_action)
{
  HandleDeleteObjectMsgImpl(ref_msg, ref_action);
}

//////////////////////////////////////////////////////////////////////////

void ezOnComponentFinishedAction2::HandleFinishedAction(ezComponent* pComponent, ezOnComponentFinishedAction2::Enum action)
{
  HandleFinishedActionImpl<ezOnComponentFinishedAction2>(pComponent, action);
}

void ezOnComponentFinishedAction2::HandleDeleteObjectMsg(ezMsgDeleteGameObject& ref_msg, ezEnum<ezOnComponentFinishedAction2>& ref_action)
{
  HandleDeleteObjectMsgImpl(ref_msg, ref_action);
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_Declarations);
