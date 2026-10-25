#pragma once

#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Basics.h>

class ezViewMarqueePickingResultMsgToEditor;
class ezQtGameObjectDocumentWindow;
class ezOrthoGizmoContext;
class ezContextMenuContext;
class ezSelectionContext;
class ezCameraMoveContext;

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtGameObjectViewWidget(QWidget* pParent, ezQtGameObjectDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig);
  ~ezQtGameObjectViewWidget();

  ezOrthoGizmoContext* m_pOrthoGizmoContext;
  ezSelectionContext* m_pSelectionContext;
  ezCameraMoveContext* m_pCameraMoveContext;

  virtual void SyncToEngine() override;

  /// Rotates the camera around the orbit point of the camera move context, until it looks into vDirection.
  ///
  /// The orbit point is the same one that Alt + left mouse button orbits around (e.g. the selection after 'Focus on Selection').
  void OrbitCameraTo(const ezVec3& vDirection, const ezVec3& vUp);

  /// Rotates the camera around the orbit point by yaw (around the world up axis) and pitch (around the camera's right axis).
  ///
  /// If a previous orbit is still being interpolated, the rotation is added to where that one ends, so repeated calls add up exactly.
  void OrbitCameraBy(ezAngle yaw, ezAngle pitch);

protected:
  virtual void HandleMarqueePickingResult(const ezViewMarqueePickingResultMsgToEditor* pMsg) override;

  /// Returns the orbit point and the camera orientation that the next orbit should start from.
  void GetOrbitStart(ezVec3& out_vOrbitPoint, ezVec3& out_vDirection, ezVec3& out_vUp) const;

  ezUInt32 m_uiLastMarqueeActionID = 0;
  ezDeque<ezUuid> m_MarqueeBaseSelection;
};
