#pragma once

#include <PCH.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <Core/Input/Declarations.h>

class ezWorld;
class ezCamera;

class RtsGameMode
{
public:
  RtsGameMode();
  virtual ~RtsGameMode();

  void ActivateMode(ezWorld* pMainWorld, ezViewHandle hView, ezCamera* pMainCamera);
  void DeactivateMode();
  void ProcessInput(ezUInt32 uiMousePosX, ezUInt32 uiMousePosY, ezKeyState::Enum LeftClickState, ezKeyState::Enum RightClickState);
  void BeforeWorldUpdate();

  //////////////////////////////////////////////////////////////////////////
  // Picking
public:
  ezResult ComputePickingRay(ezVec3& out_vRayStart, ezVec3& out_vRayDir) const;
  ezResult PickGroundPlanePosition(const ezVec3& vRayStart, const ezVec3& vRayDir, ezVec3& out_vPositon) const;

  //////////////////////////////////////////////////////////////////////////
  // Spawning Objects
public:
  ezGameObject* SpawnNamedObjectAt(const ezTransform& transform, const char* szObjectName, ezUInt16 uiTeamID);

  //////////////////////////////////////////////////////////////////////////
  // Game Mode Interface
public:
  virtual void AfterProcessInput() {}

protected:
  virtual void OnActivateMode() {}
  virtual void OnDeactivateMode() {}
  virtual void RegisterInputActions() {}
  virtual void OnProcessInput() {}
  virtual void OnBeforeWorldUpdate() {}

  ezUInt32 m_uiMousePosX;
  ezUInt32 m_uiMousePosY;
  ezKeyState::Enum m_LeftClickState;
  ezKeyState::Enum m_RightClickState;
  ezWorld* m_pMainWorld = nullptr;
  ezViewHandle m_hMainView;
  ezCamera* m_pMainCamera = nullptr;

private:
  bool m_bInitialized = false;
};
