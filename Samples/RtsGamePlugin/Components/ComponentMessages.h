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

struct RtsMsgApplyDamage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(RtsMsgApplyDamage, ezMessage);

  ezInt16 m_iDamage;
};

/// Used to inform sub-systems (child nodes etc.) when health changes (taking damage etc.)
/// m_uiCurHealth == 0 means the unit is destroyed now
struct RtsMsgUnitHealthStatus : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(RtsMsgUnitHealthStatus, ezMessage);

  ezUInt16 m_uiCurHealth;
  ezUInt16 m_uiMaxHealth;
  ezInt16 m_iDifference;
};
