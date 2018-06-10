#pragma once

struct RtsMsgNavigateTo : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(RtsMsgNavigateTo, ezMessage);

  ezVec2 m_vTargetPosition;
};

struct RtsMsgSetTarget : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(RtsMsgSetTarget, ezMessage);

  ezVec2 m_vPosition;
  ezGameObjectHandle m_hObject;
};

