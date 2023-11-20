#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Components/AiComponent.h>
#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/Time/Timestamp.h>
#include <GameEngine/Messages/DamageMessage.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
#  include <GameEngine/DearImgui/DearImgui.h>
#endif

ezCVarBool cvar_AiDebugView("Ai.DebugView", false, ezCVarFlags::Default, "Shows the scores, actions and recent behavior switches of the AI agent closest to the screen center (ImGui).");
ezCVarBool cvar_AiRecording("Ai.Recording", true, ezCVarFlags::Default, "Every AI agent keeps its last decisions, see 'Ai.DumpRecording'.");
ezCVarBool cvar_AiDumpRecording("Ai.DumpRecording", false, ezCVarFlags::Default, "Writes the recorded decisions of all AI agents to ':appdata/AiRecordings/' as JSON, then resets itself. Same as pressing F6.");

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAiComponent, 4, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("Controller", m_hController)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_AiController", ezDependencyFlags::Package), new ezRequiredAttribute()),
    EZ_MEMBER_PROPERTY("DecisionInterval", m_DecisionInterval)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(0.5)), new ezClampValueAttribute(ezTime::MakeZero(), ezVariant())),
    EZ_MEMBER_PROPERTY("DebugInfo", m_bDebugInfo),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgDamage, OnMsgDamage),
  }
  EZ_END_MESSAGEHANDLERS;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Components"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiComponentManager::ezAiComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezAiComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

ezAiComponentManager::~ezAiComponentManager() = default;

void ezAiComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezAiComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = true;

  RegisterUpdateFunction(desc);
}

void ezAiComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto it = m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->Update();
    }
  }

  if (cvar_AiDebugView)
  {
    UpdateDebugView();
  }

  if (cvar_AiDumpRecording || ezInputManager::GetInputSlotState(ezInputSlot_KeyF6) == ezKeyState::Pressed)
  {
    cvar_AiDumpRecording = false;
    DumpRecordings();
  }
}

void ezAiComponentManager::DumpRecordings()
{
  const ezDateTime dt = ezDateTime::MakeFromTimestamp(ezTimestamp::CurrentTimestamp());

  ezStringBuilder sPath;
  sPath.SetFormat(":appdata/AiRecordings/{}-{}-{}_{}-{}-{}.json", dt.GetYear(), ezArgU(dt.GetMonth(), 2, true), ezArgU(dt.GetDay(), 2, true), ezArgU(dt.GetHour(), 2, true), ezArgU(dt.GetMinute(), 2, true), ezArgU(dt.GetSecond(), 2, true));

  ezFileWriter file;
  if (file.Open(sPath).Failed())
  {
    ezLog::Error("AI recording: can't write '{}'.", sPath);
    return;
  }

  ezStandardJSONWriter json;
  json.SetOutputStream(&file);
  json.SetWhitespaceMode(ezJSONWriter::WhitespaceMode::NewlinesOnly);

  json.BeginObject();
  json.AddVariableFloat("time", GetWorld()->GetClock().GetAccumulatedTime().AsFloatInSeconds());
  json.AddVariableUInt32("maxEventsPerAgent", ezAiDecisionRecorder::c_uiMaxEvents);

  ezUInt32 uiAgents = 0;
  ezStringBuilder sTmp;

  json.BeginArray("agents");
  for (auto it = m_ComponentStorage.GetIterator(); it.IsValid(); ++it)
  {
    const ezAiComponent* pComp = it;
    if (!pComp->IsActiveAndInitialized() || pComp->m_pControllerDesc == nullptr)
      continue;

    ++uiAgents;

    json.BeginObject();
    sTmp.SetFormat("{}", ezArgP(pComp));
    json.AddVariableString("agent", sTmp);
    json.AddVariableString("name", pComp->GetOwner()->GetName());
    json.AddVariableString("controller", pComp->m_hController.GetResourceID());
    pComp->m_Recorder.WriteJson(json, *pComp->m_pControllerDesc);
    json.EndObject();
  }
  json.EndArray();

  json.EndObject();
  file.Close();

  ezStringBuilder sAbsPath;
  if (ezFileSystem::ResolvePath(sPath, &sAbsPath, nullptr).Failed())
  {
    sAbsPath = sPath;
  }

  ezLog::Info("AI recording of {} agents written to '{}'.", uiAgents, sAbsPath);
}

void ezAiComponentManager::UpdateDebugView()
{
  // the editor view is used for simulating in the editor
  const ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView, GetWorld());
  if (pView == nullptr)
    return;

  const ezVec3 vCamPos = pView->GetCullingCamera()->GetCenterPosition();
  const ezVec3 vCamDir = pView->GetCullingCamera()->GetCenterDirForwards();

  ezAiComponent* pSelected = nullptr;
  if (m_hDebugViewAgent.IsInvalidated() || !TryGetComponent(m_hDebugViewAgent, pSelected) || !pSelected->IsActiveAndInitialized() || pSelected->m_pControllerDesc == nullptr)
  {
    pSelected = nullptr;
    m_bDebugViewPinned = false;
  }

  if (ezInputManager::GetInputSlotState(ezInputSlot_KeyF7) == ezKeyState::Pressed)
  {
    m_bDebugViewPinned = !m_bDebugViewPinned && pSelected != nullptr;
  }

  if (!m_bDebugViewPinned)
  {
    // the agent closest to the screen center, within this angle
    // the selected one stays selected when no other agent is there, or when they are only slightly closer to the center (so it doesn't flicker)
    const ezAngle maxAngle = ezAngle::MakeFromDegree(25);
    const ezAngle keepAngle = ezAngle::MakeFromDegree(5);

    ezAiComponent* pBest = nullptr;
    ezAngle bestAngle = maxAngle;

    for (auto it = m_ComponentStorage.GetIterator(); it.IsValid(); ++it)
    {
      ezAiComponent* pComp = it;
      if (!pComp->IsActiveAndInitialized() || pComp->m_pControllerDesc == nullptr)
        continue;

      ezVec3 vDir = pComp->GetOwner()->GetGlobalPosition() + ezVec3(0, 0, 1.0f) - vCamPos;
      if (vDir.NormalizeIfNotZero().Failed())
        continue;

      ezAngle angle = vCamDir.GetAngleBetween(vDir);
      if (pComp == pSelected)
      {
        angle -= keepAngle;
      }

      if (angle < bestAngle)
      {
        bestAngle = angle;
        pBest = pComp;
      }
    }

    if (pBest != nullptr)
    {
      pSelected = pBest;
    }
  }

  m_hDebugViewAgent = pSelected ? pSelected->GetHandle() : ezComponentHandle();

  if (pSelected != nullptr)
  {
    const ezAiBehaviorManager& bm = pSelected->m_BehaviorManager;
    const ezVec3 vAgentPos = pSelected->GetOwner()->GetGlobalPosition();

    ezHybridArray<ezDebugRendererLine, 32> lines;

    auto AddLine = [&](const ezVec3& vStart, const ezVec3& vEnd, const ezColor& color)
    {
      auto& line = lines.ExpandAndGetRef();
      line.m_start = vStart;
      line.m_end = vEnd;
      line.m_startColor = color;
      line.m_endColor = color;
    };

    // box around the agent
    {
      const ezColor color = m_bDebugViewPinned ? ezColor::Orange : ezColor::Yellow;
      const ezBoundingBox box = ezBoundingBox::MakeFromMinMax(vAgentPos + ezVec3(-0.4f, -0.4f, 0), vAgentPos + ezVec3(0.4f, 0.4f, 2.0f));

      ezVec3 corners[8];
      box.GetCorners(corners);

      // in GetCorners() each axis is one bit of the index (set = max), the edges connect corners that differ in one bit
      for (ezUInt32 a = 0; a < 8; ++a)
      {
        for (ezUInt32 uiBit = 1; uiBit < 8; uiBit <<= 1)
        {
          if ((a & uiBit) == 0)
          {
            AddLine(corners[a], corners[a | uiBit], color);
          }
        }
      }
    }

    // the best candidate of every behavior that got scored, the active one highlighted
    for (ezUInt32 i = 0; i < bm.GetBehaviorCount(); ++i)
    {
      const auto& state = bm.GetBehaviorState(i);
      if (!bm.WasAvailable(i) || !state.m_bLastCandidateHasPosition)
        continue;

      const ezColor color = (i == bm.GetActiveBehaviorIndex()) ? ezColor::LimeGreen : (state.m_fLastScore > 0.0f ? ezColor::LightGray : ezColor::DimGray);

      AddLine(vAgentPos + ezVec3(0, 0, 1.0f), state.m_vLastCandidatePosition, color);
      ezDebugRenderer::Draw3DText(GetWorld(), bm.GetBehavior(i)->GetName(), state.m_vLastCandidatePosition, color, 14);
    }

    // visible through walls, but brighter where not occluded
    ezDebugRenderer::DrawLinesOccluded(GetWorld(), lines, ezColor::White);
    ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::White);
  }

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT
  if (ezImgui::GetSingleton() == nullptr)
  {
    EZ_DEFAULT_NEW(ezImgui);
  }

  ezImgui::GetSingleton()->SetCurrentContextForView(pView->GetHandle());

  // the window sizes itself to the content, the table has a fixed width
  const float fTableWidth = 720.0f;
  ImGui::SetNextWindowPos(ImVec2(ezMath::Max(10.0f, ImGui::GetIO().DisplaySize.x - fTableWidth - 30.0f), 10), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowBgAlpha(0.85f);

  // '###' keeps the window ID the same, while the title changes
  const char* szTitle = m_bDebugViewPinned ? "AI Agent - pinned (F7 to unpin)###AiAgent" : "AI Agent - closest to the screen center (F7 to pin)###AiAgent";

  if (ImGui::Begin(szTitle, nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize))
  {
    if (pSelected == nullptr)
    {
      ImGui::TextUnformatted("No AI agent close to the screen center.");
    }
    else
    {
      const ezAiBehaviorManager& bm = pSelected->m_BehaviorManager;
      const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();
      ezStringBuilder sText, sTmp;

      auto GetBehaviorName = [&](ezUInt32 uiIndex) -> ezStringView
      { return uiIndex != ezInvalidIndex ? bm.GetBehavior(uiIndex)->GetName() : ezStringView("<none>"); };

      sText.SetFormat("{} ({})", pSelected->GetOwner()->GetName().IsEmpty() ? ezStringView("<unnamed>") : pSelected->GetOwner()->GetName(), ezArgP(pSelected));
      ImGui::TextUnformatted(sText.GetData());

      sText.SetFormat("Active: {} ({})", GetBehaviorName(bm.GetActiveBehaviorIndex()), ezArgF(pSelected->m_fLastScore, 2));
      ImGui::TextUnformatted(sText.GetData());

      pSelected->m_ActionQueue.GetDebugDesc(sTmp);
      sText.SetFormat("Action: {}", sTmp);
      ImGui::TextUnformatted(sText.GetData());

      ImGui::Separator();

      if (ImGui::BeginTable("Behaviors", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, ImVec2(fTableWidth, 0)))
      {
        ImGui::TableSetupColumn("Behavior", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Score", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Candidate", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Considerations", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (ezUInt32 i = 0; i < bm.GetBehaviorCount(); ++i)
        {
          const auto& state = bm.GetBehaviorState(i);
          const ezAiBehavior* pBehavior = bm.GetBehavior(i);
          const bool bAvailable = bm.WasAvailable(i);

          ImGui::TableNextRow();

          if (i == bm.GetActiveBehaviorIndex())
          {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(40, 110, 40, 255));
          }

          ImGui::TableNextColumn();
          sText = pBehavior->GetName();
          ImGui::TextUnformatted(sText.GetData());

          ImGui::TableNextColumn();
          if (bAvailable)
            sText.SetFormat("{}", ezArgF(state.m_fLastScore, 2));
          else if (state.m_CooldownUntil > tNow)
            sText.SetFormat("cooldown {}s", ezArgF((state.m_CooldownUntil - tNow).GetSeconds(), 1));
          else
            sText = "n/a";
          ImGui::TextUnformatted(sText.GetData());

          ImGui::TableNextColumn();
          sText.Clear();
          if (bAvailable && state.m_pLastCandidateType != nullptr)
          {
            sText = state.m_pLastCandidateType->GetTypeName();
            sText.TrimWordStart("ezAiPerception");
          }
          ImGui::TextUnformatted(sText.GetData());

          ImGui::TableNextColumn();
          sText.Clear();
          const ezArrayPtr<const ezUInt32> considerations = pBehavior->m_Considerations.GetConnections();
          if (bAvailable && state.m_LastConsiderationValues.GetCount() == considerations.GetCount())
          {
            for (ezUInt32 c = 0; c < considerations.GetCount(); ++c)
            {
              const ezStringView sName = pSelected->m_pControllerDesc->GetConsiderations()[considerations[c]].m_pConsideration->GetDisplayName();
              const float fValue = state.m_LastConsiderationValues[c];

              if (c > 0)
                sText.Append(", ");

              if (fValue < 0.0f)
                sText.AppendFormat("{} -", sName);
              else
                sText.AppendFormat("{} {}", sName, ezArgF(fValue, 2));
            }
          }
          ImGui::TextUnformatted(sText.GetData());
        }

        ImGui::EndTable();
      }

      ImGui::Separator();
      ImGui::TextUnformatted("Recent switches:");

      for (ezUInt32 i = pSelected->m_RecentSwitches.GetCount(); i > 0; --i)
      {
        const auto& sw = pSelected->m_RecentSwitches[i - 1];
        sText.SetFormat("  {}s ago: {} -> {}", ezArgF((tNow - sw.m_Time).GetSeconds(), 1), GetBehaviorName(sw.m_uiFromBehavior), GetBehaviorName(sw.m_uiToBehavior));
        if (sw.m_uiToBehavior != ezInvalidIndex)
          sText.AppendFormat(" ({})", ezArgF(sw.m_fScore, 2));

        ImGui::TextUnformatted(sText.GetData());
      }
    }
  }

  ImGui::End();
#endif
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiComponent::ezAiComponent() = default;

ezAiComponent::~ezAiComponent()
{
  ClearController();
}

void ezAiComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_bDebugInfo;
  s << m_DecisionInterval;
  s << m_hController;
}

void ezAiComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  if (uiVersion < 4)
  {
    // older data still has the removed behavior set enum
    s >> m_bDebugInfo;
    if (uiVersion >= 2)
    {
      ezUInt8 uiBehaviorSet = 0;
      s >> uiBehaviorSet;
      s >> m_DecisionInterval;
    }
    if (uiVersion >= 3)
    {
      s >> m_hController;
    }
    return;
  }

  s >> m_bDebugInfo;
  s >> m_DecisionInterval;
  s >> m_hController;
}

void ezAiComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ClearController();

  if (!m_hController.IsValid())
  {
    ezLog::Warning("AI component on '{}' has no controller resource", GetOwner()->GetName());
    return;
  }

  {
    ezResourceLock<ezAiControllerResource> pController(m_hController, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pController.GetAcquireResult() != ezResourceAcquireResult::Final || pController->GetDesc() == nullptr)
    {
      ezLog::Error("AI component on '{}': controller resource '{}' could not be loaded", GetOwner()->GetName(), m_hController.GetResourceID());
      return;
    }

    m_pControllerDesc = pController->GetDesc();
  }

  m_InstanceData = m_pControllerDesc->AllocateInstanceData();

  m_SensorManager.Initialize(*m_pControllerDesc, m_InstanceData.GetByteBlobPtr());
  m_PerceptionManager.Initialize(*m_pControllerDesc, m_InstanceData.GetByteBlobPtr());
  m_ConsiderationManager.Initialize(*m_pControllerDesc, m_InstanceData.GetByteBlobPtr());
  m_BehaviorManager.Initialize(*m_pControllerDesc, m_InstanceData.GetByteBlobPtr());
  m_fLastScore = 0.0f;

  // stagger the decisions of multiple agents
  const double fOffset = GetWorld()->GetRandomNumberGenerator().DoubleZeroToOneExclusive();
  m_LastAiUpdate = GetWorld()->GetClock().GetAccumulatedTime() - m_DecisionInterval * fOffset;
}

void ezAiComponent::OnDeactivated()
{
  ClearController();

  SUPER::OnDeactivated();
}

void ezAiComponent::ClearController()
{
  // actions may reference instance data, so they have to go first
  m_ActionQueue.InterruptAndClear();

  if (m_pControllerDesc == nullptr)
    return;

  m_BehaviorManager.Deinitialize();
  m_ConsiderationManager.Deinitialize();
  m_PerceptionManager.Deinitialize();
  m_SensorManager.Deinitialize();

  m_pControllerDesc->DeallocateInstanceData(m_InstanceData);
  m_pControllerDesc.Clear();
}

void ezAiComponent::OnMsgDamage(ezMsgDamage& ref_msg)
{
  if (m_pControllerDesc == nullptr)
    return;

  ezAiContext ctx{*GetOwner(), GetWorld()->GetClock().GetAccumulatedTime(), ezLog::GetThreadLocalLogSystem(), m_SensorManager, m_PerceptionManager, m_ConsiderationManager, m_ActionQueue};
  m_PerceptionManager.HandleMessage(ctx, ref_msg);
}

void ezAiComponent::Update()
{
  if (m_pControllerDesc == nullptr)
    return;

  // how much better another behavior has to score, to replace the active one (prevents flip-flopping between similar scores)
  constexpr float fSwitchMargin = 0.1f;

  const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  ezAiContext ctx{*GetOwner(), tNow, ezLog::GetThreadLocalLogSystem(), m_SensorManager, m_PerceptionManager, m_ConsiderationManager, m_ActionQueue};

  const auto cs = m_BehaviorManager.ContinueActiveBehavior(ctx);

  if (cs.m_bEndBehavior)
  {
    if (cvar_AiRecording)
    {
      ezStringBuilder sAction;
      m_ActionQueue.GetDebugDesc(sAction);
      m_Recorder.RecordBehaviorEnd(tNow, *GetOwner(), m_BehaviorManager.GetActiveBehaviorIndex(), sAction);
    }

    m_fLastScore = 0.0f;
    SetActiveBehavior(ctx, ezInvalidIndex, nullptr, 0.0f);

    // decide on the next behavior right away
    m_LastAiUpdate = tNow - m_DecisionInterval;
  }

  if (cs.m_bAllowBehaviorSwitch && tNow >= m_LastAiUpdate + m_DecisionInterval)
  {
    if (!m_BehaviorManager.HasActiveBehavior())
    {
      m_fLastScore = 0.0f;
    }

    m_LastAiUpdate = tNow;

    m_BehaviorManager.DetermineAvailableBehaviors(m_LastAiUpdate, ezMath::Floor(m_fLastScore));

    m_BehaviorManager.FlagNeededPerceptions(m_PerceptionManager, m_ConsiderationManager);

    m_PerceptionManager.FlagNeededSensors(m_SensorManager);

    m_SensorManager.UpdateNeededSensors(ctx);

    m_PerceptionManager.UpdateNeededPerceptions(ctx);

    const ezAiBehaviorCandidate candidate = m_BehaviorManager.DetermineBehaviorCandidate(ctx);

    if (candidate.m_fActiveBehaviorScore >= 0.0f)
    {
      // the active behavior was re-evaluated, its score may have dropped
      m_fLastScore = candidate.m_fActiveBehaviorScore;
    }

    const ezUInt32 uiActiveBefore = m_BehaviorManager.GetActiveBehaviorIndex();
    const float fScoreToBeat = m_fLastScore;
    ezAiDecisionRecorder::Result result = ezAiDecisionRecorder::Result::NoCandidate;

    ezStringBuilder sAction;
    if (cvar_AiRecording)
    {
      // before the switch, so that it shows what was running when the decision was made
      m_ActionQueue.GetDebugDesc(sAction);
    }

    if (candidate.m_pBehavior == nullptr)
    {
      // nothing wants to run, keep the active behavior (if any)
    }
    else if (candidate.m_pBehavior == m_BehaviorManager.GetActiveBehavior())
    {
      result = ezAiDecisionRecorder::Result::Kept;
      m_fLastScore = candidate.m_fScore;
      m_BehaviorManager.KeepActiveBehavior(ctx, candidate.m_pPerception);
    }
    else if (candidate.m_fScore > m_fLastScore + fSwitchMargin)
    {
      result = ezAiDecisionRecorder::Result::Switched;
      m_fLastScore = candidate.m_fScore;
      SetActiveBehavior(ctx, candidate.m_uiBehaviorIndex, candidate.m_pPerception, candidate.m_fScore);
    }
    else
    {
      result = ezAiDecisionRecorder::Result::NotEnough;
    }

    if (cvar_AiRecording)
    {
      m_Recorder.RecordDecision(tNow, *GetOwner(), m_BehaviorManager, m_PerceptionManager, uiActiveBefore, fScoreToBeat, candidate, result, sAction);
    }
  }

  ezStringBuilder sFailedAction;
  m_ActionQueue.Execute(*GetOwner(), GetWorld()->GetClock().GetTimeDiff(), ezLog::GetThreadLocalLogSystem(), &sFailedAction);

  if (cvar_AiRecording && !sFailedAction.IsEmpty())
  {
    m_Recorder.RecordActionFailure(tNow, *GetOwner(), m_BehaviorManager.GetActiveBehaviorIndex(), sFailedAction);
  }

  if (m_bDebugInfo)
  {
    DrawDebugInfo();
  }
}

void ezAiComponent::SetActiveBehavior(const ezAiContext& ctx, ezUInt32 uiBehaviorIndex, const ezAiPerception* pPerception, float fScore)
{
  const ezUInt32 uiPrevBehavior = m_BehaviorManager.GetActiveBehaviorIndex();

  if (m_bDebugInfo && uiBehaviorIndex != ezInvalidIndex)
  {
    const ezAiBehavior* pPrev = m_BehaviorManager.GetActiveBehavior();
    ezLog::Info("AI {}: {} -> {} ({})", ezArgP(this), pPrev ? pPrev->GetName() : "<none>", m_BehaviorManager.GetBehavior(uiBehaviorIndex)->GetName(), ezArgF(fScore, 2));
  }

  m_BehaviorManager.SetActiveBehavior(ctx, uiBehaviorIndex, pPerception);

  if (uiPrevBehavior == uiBehaviorIndex)
    return;

  if (!m_RecentSwitches.CanAppend())
  {
    m_RecentSwitches.PopFront();
  }

  BehaviorSwitch sw;
  sw.m_Time = ctx.m_Now;
  sw.m_uiFromBehavior = uiPrevBehavior;
  sw.m_uiToBehavior = uiBehaviorIndex;
  sw.m_fScore = fScore;
  m_RecentSwitches.PushBack(sw);
}

void ezAiComponent::DrawDebugInfo()
{
  ezStringBuilder sText, sTmp;

  if (const ezAiBehavior* pActive = m_BehaviorManager.GetActiveBehavior())
  {
    sText.SetFormat("{} ({})\n", pActive->GetName(), ezArgF(m_fLastScore, 2));
  }
  else
  {
    sText = "<no behavior>\n";
  }

  if (!cvar_AiDebugView)
  {
    // the debug view shows the details
    m_ActionQueue.GetDebugDesc(sTmp);
    sText.AppendFormat("{}\n\n", sTmp);

    m_BehaviorManager.GetDebugText(sText);
  }

  ezDebugRenderer::Draw3DText(GetWorld(), sText, GetOwner()->GetGlobalPosition() + ezVec3(0, 0, 1.0f), ezColor::White, 16);
}
