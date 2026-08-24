#pragma once

#include <McpPlugin/McpPluginDLL.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Time/Time.h>
#include <Mcp/Tools/McpAppTool.h>
#include <Texture/Image/Image.h>

/// \brief The game process' half of the shared app_* tools, plus taking a screenshot.
///
/// The process level questions - port, executable, command line, process id - are answered by the base
/// class, so an agent driving both an editor and the game it runs asks them the same way in both. What
/// is added here is what only a rendering, frame-stepping process can answer.
class ezMcpEngineAppTool : public ezMcpAppTool
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpEngineAppTool, ezMcpAppTool);

public:
  ~ezMcpEngineAppTool();

  virtual void OnDeactivate() override;

  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

protected:
  virtual ezStringView GetHostNoun() const override { return "game"; }
  virtual ezStringView GetRelaunchHint() const override;
  virtual ezStringView GetBuildTimestamp() const override;
  virtual void AddHostInfo(ezMcpJsonWriter& ref_writer) override;
  virtual void RequestQuit(bool bDiscardChanges) override;

private:
  void ExecuteScreenshot(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);

  /// \brief Starts the capture. Fills out_result and returns EZ_FAILURE if it could not be started.
  ezResult BeginScreenshot(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);

  /// \brief Turns a finished capture into a file. Called once m_State is Ready.
  void FinishScreenshot(ezMcpToolResult& out_result);

  /// \brief Polls the pending capture, from inside the frame.
  ///
  /// Subscribed to the frame loop rather than done from the tool call, because a capture only completes
  /// once the frame that renders it is about to be presented - which is exactly where the engine polls
  /// its own screenshots (ezGameApplication::Run_PresentImage). Polling from the tool call instead, at
  /// the point where MCP requests are pumped, is *before* that frame has rendered, and the capture is
  /// then perpetually pending.
  void ExecutionEventHandler(const ezGameApplicationExecutionEvent& e);

  enum class CaptureState
  {
    Idle,
    Pending, ///< Started, waiting for the frame loop to hand back an image.
    Ready,   ///< m_CapturedImage holds it.
    Failed,  ///< Dropped by the renderer - see m_sCaptureError.
  };

  /// Capturing a back buffer is asynchronous by at least one frame, and the tool call runs *inside* a
  /// frame - so the image cannot be ready before that call returns. The request is deferred
  /// (ezMcpToolResult::m_bNotFinished) and this is the state that survives across the re-entries. Only
  /// one request is ever in flight, so a single set of members is enough.
  CaptureState m_State = CaptureState::Idle;
  ezImage m_CapturedImage;
  ezString m_sCaptureError;
  ezString m_sCapturePath;
  ezUInt32 m_uiCaptureMaxWidth = 0;
  ezInt32 m_iCaptureWindow = 0;
  /// Name of the window the capture was started on, so the result can say what was captured. Recorded
  /// up front because a window can go away while the capture is in flight.
  ezString m_sCaptureWindowName;
  ezTime m_CaptureStarted;
  ezEventSubscriptionID m_FrameSubscription = 0;

  /// How long to keep polling before giving up. Generous for a frame or two of latency, short enough
  /// that the answer arrives while an agent is still waiting. The interesting case it bounds is the
  /// editor's engine process, which renders only when the editor asks it to and may never present a
  /// frame at all - without a limit that is indistinguishable from a hang.
  static constexpr ezTime s_CaptureTimeout = ezTime::MakeFromSeconds(10);
};
