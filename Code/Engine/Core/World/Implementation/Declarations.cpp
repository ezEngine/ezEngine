#include <CorePCH.h>

#include <Core/World/World.h>
#include <Core/Messages/DeleteObjectMessage.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezObjectMode, 1)
  EZ_ENUM_CONSTANTS(ezObjectMode::Automatic, ezObjectMode::ForceDynamic)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezOnComponentFinishedAction, 1)
  EZ_ENUM_CONSTANTS(ezOnComponentFinishedAction::None, ezOnComponentFinishedAction::DeleteComponent, ezOnComponentFinishedAction::DeleteGameObject)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezOnComponentFinishedAction2, 1)
  EZ_ENUM_CONSTANTS(ezOnComponentFinishedAction2::None, ezOnComponentFinishedAction2::DeleteComponent, ezOnComponentFinishedAction2::DeleteGameObject, ezOnComponentFinishedAction2::Restart)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

namespace
{
  template <typename T>
  void HandleFinishedActionImpl(ezComponent* pComponent, typename T::Enum action)
  {
    if (action == T::DeleteGameObject)
    {
      // Send a message to the owner object to check whether another component wants to delete this object later.
      // Can't use ezGameObject::SendMessage because the object would immediately delete itself and furthermore the sender component needs to be filtered out here.
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
  void HandleDeleteObjectMsgImpl(ezMsgDeleteGameObject& msg, ezEnum<T>& action)
  {
    if (action == T::DeleteComponent)
    {
      msg.m_bCancel = true;
      action = T::DeleteGameObject;
    }
    else if (action == T::DeleteGameObject)
    {
      msg.m_bCancel = true;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

void ezOnComponentFinishedAction::HandleFinishedAction(ezComponent* pComponent, ezOnComponentFinishedAction::Enum action)
{
  HandleFinishedActionImpl<ezOnComponentFinishedAction>(pComponent, action);
}

void ezOnComponentFinishedAction::HandleDeleteObjectMsg(ezMsgDeleteGameObject& msg, ezEnum<ezOnComponentFinishedAction>& action)
{
  HandleDeleteObjectMsgImpl(msg, action);
}

//////////////////////////////////////////////////////////////////////////

void ezOnComponentFinishedAction2::HandleFinishedAction(ezComponent* pComponent, ezOnComponentFinishedAction2::Enum action)
{
  HandleFinishedActionImpl<ezOnComponentFinishedAction2>(pComponent, action);
}

void ezOnComponentFinishedAction2::HandleDeleteObjectMsg(ezMsgDeleteGameObject& msg, ezEnum<ezOnComponentFinishedAction2>& action)
{
  HandleDeleteObjectMsgImpl(msg, action);
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_Declarations);

