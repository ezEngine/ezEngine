#pragma once

#include <RecastPlugin/RecastPluginDLL.h>

#include <Core/World/WorldModule.h>

class dtCrowd;
struct dtCrowdAgent;
class ezRecastWorldModule;


struct ezDetourCrowdAgentParams
{
  /// \brief Agent radius. [Limit: >= 0]
  float m_fRadius;

  /// \brief Agent height. [Limit: > 0]
  float m_fHeight;

  /// \brief Maximum allowed acceleration. [Limit: >= 0]
  float m_fMaxAcceleration;

  /// \brief Maximum allowed speed. [Limit: >= 0]
  float m_fMaxSpeed;

  /// \brief How aggresive the agent manager should be at avoiding collisions with this agent. [Limit: >= 0]
  float m_fSeparationWeight;

  void* m_pUserData;

  static inline ezDetourCrowdAgentParams Default()
  {
    ezDetourCrowdAgentParams params;
    params.m_fRadius = 0.3f;
    params.m_fHeight = 1.8f;
    params.m_fMaxAcceleration = 10.0f;
    params.m_fMaxSpeed = 3.5f;
    params.m_fSeparationWeight = 2.0f;
    params.m_pUserData = nullptr;
    return params;
  }
};


class EZ_RECASTPLUGIN_DLL ezDetourCrowdWorldModule : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezDetourCrowdWorldModule, ezWorldModule);

public:
  ezDetourCrowdWorldModule(ezWorld* pWorld);
  ~ezDetourCrowdWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  bool IsInitializedAndReady() const;

  const dtCrowdAgent* GetAgentById(ezInt32 iAgentId) const;

  /// \brief Tries to create a new crowd agent and returns its ID or -1.
  ezInt32 CreateAgent(const ezVec3& vPos, const ezDetourCrowdAgentParams& params);

  void DestroyAgent(ezInt32 iAgentId);

  bool SetAgentTargetPosition(ezInt32 iAgentId, const ezVec3& vPos, const ezVec3& vQueryHalfExtents);

  void ClearAgentTargetPosition(ezInt32 iAgentId);

  void UpdateAgentParams(ezInt32 iAgentId, const ezDetourCrowdAgentParams& params);

private:
  void UpdateNavMesh(const UpdateContext& ctx);
  void UpdateCrowd(const UpdateContext& ctx);
  void VisualizeCrowd(const UpdateContext& ctx);

  void FillDtCrowdAgentParams(const ezDetourCrowdAgentParams& params, struct dtCrowdAgentParams& out_params) const;

  ezInt32 m_iMaxAgents = 128;
  float m_fMaxAgentRadius = 2.0f;
  dtCrowd* m_pDtCrowd = nullptr;
  ezRecastWorldModule* m_pRecastModule = nullptr;
};
