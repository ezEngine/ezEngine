#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Message.h>

struct EZ_CORE_DLL ezMsgDeleteGameObject : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgDeleteGameObject, ezMessage);

  /// \brief If set to true, any parent/ancestor that has no other children or components will also be deleted.
  bool m_bDeleteEmptyParents = true;

  /// \brief This is used by ezOnComponentFinishedAction to orchestrate when an object shall really be deleted.
  bool m_bCancel = false;
};
