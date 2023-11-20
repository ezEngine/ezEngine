#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/UtilityAI/Framework/AiConsideration.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Time/Time.h>

class ezAiControllerDesc;
class ezAiInputPin;

/// Evaluates the considerations of one agent and caches the results per decision update.
///
/// The considerations themselves are owned by the ezAiControllerDesc, this only tracks the per-agent state.
/// Behaviors use it through ezAiBehavior::MakeScore(), not directly.
class EZ_AIPLUGIN_DLL ezAiConsiderationManager
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAiConsiderationManager);

public:
  ezAiConsiderationManager();
  ~ezAiConsiderationManager();

  /// instanceData must stay valid until Deinitialize() is called.
  void Initialize(const ezAiControllerDesc& desc, ezByteBlobPtr instanceData);
  void Deinitialize();

  /// Lets the considerations connected to the pin flag the perceptions they need.
  void FlagNeededPerceptions(const ezAiInputPin& pin, ezAiPerceptionManager& ref_PerceptionManager) const;

  /// Returns the cached result, if the consideration was already evaluated for this candidate at ctx.m_Now.
  ///
  /// uiConsiderationIndex is an index into ezAiControllerDesc::GetConsiderations(), typically from an ezAiInputPin.
  float Evaluate(const ezAiContext& ctx, ezUInt32 uiConsiderationIndex, const ezAiPerception* pCandidate);

  const ezAiConsideration* GetConsideration(ezUInt32 uiConsiderationIndex) const;

private:
  void* GetInstanceData(ezUInt32 uiConsiderationIndex) const;

  struct CacheEntry
  {
    const ezAiPerception* m_pCandidate = nullptr;
    float m_fValue = 0.0f;
  };

  struct State
  {
    ezTime m_CacheTime = ezTime::MakeFromHours(-1); ///< The cache is only valid while this equals ezAiContext::m_Now.
    ezSmallArray<CacheEntry, 4> m_Cache;
  };

  const ezAiControllerDesc* m_pDesc = nullptr;
  ezByteBlobPtr m_InstanceData;
  ezHybridArray<State, 8> m_States; ///< One per consideration in the controller desc.
};
