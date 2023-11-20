#pragma once

#include <Foundation/Math/Vec3.h>
#include <AiPlugin/UtilityAI/Framework/AiPerception.h>

class EZ_AIPLUGIN_DLL ezAiPerceptionPOI : public ezAiPerception
{
public:
  ezAiPerceptionPOI() = default;
  ~ezAiPerceptionPOI() = default;

  ezVec3 m_vGlobalPosition = ezVec3::MakeZero();
};


class EZ_AIPLUGIN_DLL ezAiPerceptionWander : public ezAiPerception
{
public:
  ezAiPerceptionWander() = default;
  ~ezAiPerceptionWander() = default;

  ezVec3 m_vGlobalPosition = ezVec3::MakeZero();
};

class EZ_AIPLUGIN_DLL ezAiPerceptionCheckpoint : public ezAiPerception
{
public:
  ezAiPerceptionCheckpoint() = default;
  ~ezAiPerceptionCheckpoint() = default;

  ezVec3 m_vGlobalPosition = ezVec3::MakeZero();
};
