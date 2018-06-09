#pragma once

class ezWorld;
class ezCamera;
class RtsGameState;

struct RtsMouseInputState
{
  ezVec2U32 m_MousePos;
  ezVec2U32 m_MousePosLeftClick;
  ezVec2U32 m_MousePosRightClick;
  ezKeyState::Enum m_LeftClickState;
  ezKeyState::Enum m_RightClickState;
  bool m_bLeftMouseMoved = false;
  bool m_bRightMouseMoved = false;

  static bool HasMouseMoved(ezVec2U32 start, ezVec2U32 now);
};

class RtsGameMode
{
public:
  RtsGameMode();
  virtual ~RtsGameMode();

  void ActivateMode(ezWorld* pMainWorld, ezViewHandle hView, ezCamera* pMainCamera);
  void DeactivateMode();
  void ProcessInput(const RtsMouseInputState& MouseInput);
  void BeforeWorldUpdate();

  //////////////////////////////////////////////////////////////////////////
  // Game Mode Interface
public:
  virtual void AfterProcessInput() {}

protected:
  virtual void OnActivateMode() {}
  virtual void OnDeactivateMode() {}
  virtual void RegisterInputActions() {}
  virtual void OnProcessInput(const RtsMouseInputState& MouseInput) {}
  virtual void OnBeforeWorldUpdate() {}

  RtsGameState* m_pGameState = nullptr;
  ezWorld* m_pMainWorld = nullptr;
  ezViewHandle m_hMainView;

private:
  bool m_bInitialized = false;

  //////////////////////////////////////////////////////////////////////////
  // Camera
protected:
  void DoDefaultCameraInput(const RtsMouseInputState& MouseInput);

  ezCamera* m_pMainCamera = nullptr;
};
