#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiActionQueue.h>
#include <Core/World/GameObject.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Debug/DebugRenderer.h>

ezAiActionQueue::ezAiActionQueue() = default;

ezAiActionQueue::~ezAiActionQueue()
{
  InterruptAndClear();
}

bool ezAiActionQueue::IsEmpty() const
{
  return m_Queue.IsEmpty();
}

void ezAiActionQueue::InterruptAndClear()
{
  for (auto pCmd : m_Queue)
  {
    pCmd->Destroy();
  }

  m_Queue.Clear();
}

void ezAiActionQueue::CancelCurrentActions(ezGameObject& owner)
{
  for (ezUInt32 i = 0; i < m_Queue.GetCount(); ++i)
  {
    m_Queue[i]->Cancel(owner);
  }
}

void ezAiActionQueue::QueueAction(ezAiAction* pAction)
{
  m_Queue.PushBack(pAction);
}

void ezAiActionQueue::Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog, ezStringBuilder* out_pFailedAction)
{
  EZ_LOG_BLOCK("AiActionExecution");
  EZ_PROFILE_SCOPE("AiActionExecution");

  while (!m_Queue.IsEmpty())
  {
    ezAiAction* pCmd = m_Queue[0];

    const ezAiActionResult res = pCmd->Execute(owner, tDiff, pLog);
    if (res == ezAiActionResult::Succeded)
      return;

    if (res == ezAiActionResult::Failed)
    {
      ezStringBuilder str;
      pCmd->GetDebugDesc(str);
      ezLog::Error(pLog, "AI cmd failed: {}", str);

      if (out_pFailedAction != nullptr)
      {
        *out_pFailedAction = str;
      }

      CancelCurrentActions(owner);
    }

    pCmd->Destroy();
    m_Queue.RemoveAtAndCopy(0);
  }
}

void ezAiActionQueue::PrintDebugInfo(ezGameObject& owner)
{
  ezStringBuilder str;

  if (m_Queue.IsEmpty())
  {
    str = "<AI action queue empty>";
    ezDebugRenderer::DrawInfoText(owner.GetWorld(), ezDebugTextPlacement::BottomRight, "AI", str, ezColor::Orange);
  }
  else
  {
    m_Queue[0]->GetDebugDesc(str);
    ezDebugRenderer::DrawInfoText(owner.GetWorld(), ezDebugTextPlacement::BottomRight, "AI", str);
  }
}

void ezAiActionQueue::GetDebugDesc(ezStringBuilder& out_sText) const
{
  if (m_Queue.IsEmpty())
  {
    out_sText = "<no action>";
  }
  else
  {
    m_Queue[0]->GetDebugDesc(out_sText);

    if (m_Queue.GetCount() > 1)
    {
      out_sText.AppendFormat(" (+{})", m_Queue.GetCount() - 1);
    }
  }
}
