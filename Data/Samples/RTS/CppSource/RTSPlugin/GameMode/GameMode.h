#pragma once

class ezWorld;
class ezCamera;
class RTSGameState;
class ezRmlUiContext;

struct RtsMouseInputState
{
  ezVec2U32 m_MousePos;
  ezVec2U32 m_MousePosLeftClick;
  ezVec2U32 m_MousePosRightClick;
  ezKeyState::Enum m_LeftClickState;
  ezKeyState::Enum m_RightClickState;
  bool m_bLeftMouseMoved = false;
  bool m_bRightMouseMoved = false;

  static bool HasMouseMoved(ezVec2U32 vStart, ezVec2U32 vNow);
};

class RtsGameMode
{
public:
  RtsGameMode();
  virtual ~RtsGameMode();

  void ActivateMode(ezWorld* pMainWorld, ezViewHandle hView, ezCamera* pMainCamera);
  void DeactivateMode();
  void ProcessInput(const RtsMouseInputState& mouseInput);
  void BeforeWorldUpdate();

  //////////////////////////////////////////////////////////////////////////
  // Game Mode Interface
public:
  virtual void AfterProcessInput() {}

protected:
  virtual void OnFirstActivation() {}
  virtual void OnActivateMode() {}
  virtual void OnDeactivateMode() {}
  virtual void OnProcessInput(const RtsMouseInputState& MouseInput, bool bUiWantsInput) {}
  virtual void OnBeforeWorldUpdate() {}

  RTSGameState* m_pGameState = nullptr;
  ezWorld* m_pMainWorld = nullptr;
  ezViewHandle m_hMainView;

private:
  bool m_bFirstActivation = true;

  //////////////////////////////////////////////////////////////////////////
  // Camera
protected:
  void DoDefaultCameraInput(const RtsMouseInputState& MouseInput);

  ezCamera* m_pMainCamera = nullptr;

  //////////////////////////////////////////////////////////////////////////
  // User Interface
public:
  static ezColor GetTeamColor(ezUInt16 uiTeam);
  static ezRmlUiContext* SetUiActive(ezWorld* pWorld, ezTempHashedString sName, bool bActive);

protected:
  void SetupSelectModeUI();

  ezComponentHandle m_hSelectModeUIComponent;
};
