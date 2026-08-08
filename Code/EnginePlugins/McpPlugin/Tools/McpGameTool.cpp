#include <McpPlugin/McpPluginPCH.h>

#include <McpPlugin/McpEngineHost.h>
#include <McpPlugin/Tools/McpGameTool.h>

#include <Core/GameState/GameStateBase.h>
#include <Core/World/World.h>
#include <GameEngine/GameState/GameState.h>

#include <Foundation/Time/Clock.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpGameTool, 1, ezRTTIDefaultAllocator<ezMcpGameTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezMcpGameTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "game_info";
    desc.m_sDescription =
      "Returns what the game is currently doing: whether a game state is active and of which type, the world it is "
      "playing in with its simulation state and time, and the global clock's speed and paused state.\n"
      "Start here. 'gameStateActive' false means nothing is being played yet - in the editor's engine process that is "
      "the normal state until play-the-game is started, and it explains why input does nothing and why screenshots "
      "time out. Process level facts - port, executable, log file, windows - are in app_info instead.";

    desc.m_sInputSchema = R"({"type":"object","properties":{}})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "game_wait";
    desc.m_sDescription =
      "Lets the game run for a number of frames and returns once they have happened. This is how to observe anything: "
      "input is consumed one frame at a time and a screenshot captures a frame that was already presented, so "
      "'press a key, wait, look' is the sequence that shows an effect.\n"
      "It does not change how the game runs - it only waits. If frames are not being produced at all, it gives up "
      "after a timeout and says so rather than blocking forever; the usual cause is the editor's engine process, "
      "which renders only while the editor asks it to.";

    desc.m_sInputSchema = R"({"type":"object","properties":{"frames":{"type":"number","description":"How many frames to wait for. Default 1."},"timeout":{"type":"number","description":"Give up after this many seconds if the frames do not happen. Default 30."}}})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "game_pause";
    desc.m_sDescription =
      "Pauses or resumes the global clock. A paused game keeps rendering and keeps answering tools - it is the world "
      "that stops advancing - so this is how to hold a moment still while inspecting it, and screenshots and input "
      "still work while paused.\n"
      "Beware what 'paused' means for a particular game: it stops the clock the engine hands out, which is not "
      "necessarily the same thing as that game's own notion of simulation time.";

    desc.m_sInputSchema = R"({"type":"object","properties":{"paused":{"type":"boolean","description":"True to pause, false to resume. Omit to just report the current state."}}})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "game_speed";
    desc.m_sDescription =
      "Reads or sets the global clock's speed factor. 1 is real time, 0.1 slows everything down by ten, 5 speeds it "
      "up. Useful either way: slowing down makes a fast event observable frame by frame, speeding up gets through a "
      "long wait without spending the frames.\n"
      "This scales time, not frames - the game still renders as fast as it did, each frame just covers more or less "
      "simulated time.";

    desc.m_sInputSchema = R"({"type":"object","properties":{"speed":{"type":"number","description":"The new speed factor. Must be greater than 0. Omit to just report the current one."}}})";
  }
}

void ezMcpGameTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "game_info")
  {
    ExecuteInfo(arguments, out_result);
  }
  else if (sToolName == "game_wait")
  {
    ExecuteWait(arguments, out_result);
  }
  else if (sToolName == "game_pause")
  {
    ExecutePause(arguments, out_result);
  }
  else if (sToolName == "game_speed")
  {
    ExecuteSpeed(arguments, out_result);
  }
}

void ezMcpGameTool::WriteClockState(ezMcpJsonWriter& ref_writer)
{
  const ezClock* pClock = ezClock::GetGlobalClock();

  ref_writer.BeginObject("clock");
  ref_writer.AddVariableBool("paused", pClock->GetPaused());
  ref_writer.AddVariableDouble("speed", pClock->GetSpeed());
  ref_writer.AddVariableDouble("accumulatedTimeSeconds", pClock->GetAccumulatedTime().GetSeconds());
  ref_writer.AddVariableDouble("lastTimeStepSeconds", pClock->GetTimeDiff().GetSeconds());

  // Non-zero means the clock reports a constant time step regardless of how long the frame really took.
  // Worth reporting because it makes 'wait N frames' mean a fixed amount of simulated time.
  if (pClock->GetFixedTimeStep().IsPositive())
  {
    ref_writer.AddVariableDouble("fixedTimeStepSeconds", pClock->GetFixedTimeStep().GetSeconds());
  }

  ref_writer.EndObject();
}

void ezMcpGameTool::ExecuteInfo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  EZ_IGNORE_UNUSED(arguments);

  ezMcpJsonWriter writer;
  writer.BeginObject();

  ezGameApplicationBase* pApp = ezGameApplicationBase::GetGameApplicationBaseInstance();
  ezGameStateBase* pGameState = pApp != nullptr ? pApp->GetActiveGameState() : nullptr;

  writer.AddVariableBool("gameStateActive", pGameState != nullptr);

  if (pGameState != nullptr)
  {
    // The concrete type is what identifies the game - a project's own game state is where its rules
    // live, and its name is what a caller needs to look for tools or types belonging to it.
    writer.AddVariableString("gameStateType", pGameState->GetDynamicRTTI()->GetTypeName());
  }

  writer.AddVariableUInt64("frameCount", ezMcpEngineHost::GetFrameCount());

  WriteClockState(writer);

  // Reached through the game state rather than by walking ezWorld's global table: ezWorld::GetWorld()
  // takes a raw slot index while GetWorldCount() returns how many slots are *live*, so iterating one
  // against the other reads freed slots as soon as any world has ever been destroyed. The game state's
  // world is also the one the question is actually about.
  const ezGameState* pTypedGameState = ezDynamicCast<const ezGameState*>(pGameState);
  const ezWorld* pWorld = pTypedGameState != nullptr ? const_cast<ezGameState*>(pTypedGameState)->GetMainWorld() : nullptr;

  if (pWorld != nullptr)
  {
    writer.BeginObject("world");
    writer.AddVariableString("name", pWorld->GetName());
    writer.AddVariableBool("simulating", pWorld->GetWorldSimulationEnabled());
    writer.AddVariableDouble("timeSeconds", pWorld->GetClock().GetAccumulatedTime().GetSeconds());
    writer.EndObject();
  }

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

void ezMcpGameTool::ExecuteWait(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezUInt64 uiNow = ezMcpEngineHost::GetFrameCount();

  // First entry. Every later one is a re-entry from the deferral, and must not read the arguments
  // again - the frame count it is waiting for was decided here.
  if (!m_bWaiting)
  {
    const ezInt64 iFrames = ezMcpJson::GetInt(arguments, "frames", 1);

    if (iFrames < 1 || iFrames > s_uiMaxFrames)
    {
      ezStringBuilder sError;
      sError.SetFormat("'frames' must be between 1 and {}, not {}.", s_uiMaxFrames, iFrames);
      out_result.SetError(sError);
      return;
    }

    const ezInt64 iTimeout = ezMcpJson::GetInt(arguments, "timeout", static_cast<ezInt64>(s_DefaultTimeout.GetSeconds()));

    m_bWaiting = true;
    m_uiWaitStartFrame = uiNow;
    m_uiWaitUntilFrame = uiNow + static_cast<ezUInt64>(iFrames);
    m_WaitStarted = ezTime::Now();
    m_WaitTimeout = iTimeout > 0 ? ezTime::MakeFromSeconds(static_cast<double>(iTimeout)) : s_DefaultTimeout;

    out_result.m_bNotFinished = true;
    return;
  }

  const bool bDone = uiNow >= m_uiWaitUntilFrame;
  const bool bTimedOut = !bDone && (ezTime::Now() - m_WaitStarted >= m_WaitTimeout);

  if (!bDone && !bTimedOut)
  {
    out_result.m_bNotFinished = true;
    return;
  }

  const ezUInt64 uiElapsed = uiNow - m_uiWaitStartFrame;
  const ezUInt64 uiRequested = m_uiWaitUntilFrame - m_uiWaitStartFrame;

  m_bWaiting = false;

  if (bTimedOut)
  {
    ezStringBuilder sError;
    sError.SetFormat("Timed out after {} seconds having waited {} of {} frames. The game is not producing frames fast "
                     "enough, or not at all. In the editor's engine process that is the normal state while "
                     "play-the-game is not running - it renders only when the editor asks it to - so start the game "
                     "in the editor first. A minimised window does the same thing.",
      (ezTime::Now() - m_WaitStarted).GetSeconds(), uiElapsed, uiRequested);
    out_result.SetError(sError);
    return;
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableUInt64("framesWaited", uiElapsed);
  writer.AddVariableUInt64("frameCount", uiNow);
  writer.AddVariableDouble("elapsedSeconds", (ezTime::Now() - m_WaitStarted).GetSeconds());
  WriteClockState(writer);
  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpGameTool::ExecutePause(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezClock* pClock = ezClock::GetGlobalClock();

  const ezVariant* pPaused = nullptr;
  if (arguments.TryGetValue("paused", pPaused) && pPaused->IsValid())
  {
    pClock->SetPaused(ezMcpJson::GetBool(arguments, "paused", false));
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();
  WriteClockState(writer);
  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}

void ezMcpGameTool::ExecuteSpeed(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  ezClock* pClock = ezClock::GetGlobalClock();

  const ezVariant* pSpeed = nullptr;
  if (arguments.TryGetValue("speed", pSpeed) && pSpeed->IsValid())
  {
    ezResult conversion = EZ_SUCCESS;
    const double fSpeed = pSpeed->ConvertTo<double>(&conversion);

    if (conversion.Failed())
    {
      out_result.SetError("'speed' is not a number.");
      return;
    }

    if (fSpeed <= 0.0)
    {
      // ezClock asserts on a non-positive speed, and 'stopped' is what game_pause is for.
      out_result.SetError("'speed' must be greater than 0. Use game_pause to stop time entirely.");
      return;
    }

    pClock->SetSpeed(fSpeed);
  }

  ezMcpJsonWriter writer;
  writer.BeginObject();
  WriteClockState(writer);
  writer.EndObject();

  out_result.m_sText = writer.GetResult();
}
