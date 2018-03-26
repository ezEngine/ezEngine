#pragma once

#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <QTreeView>

class ezQtPropertyAnimModel;
class QItemSelection;
class QItemSelectionModel;
class ezQtCurve1DEditorWidget;
class ezQtEventTrackEditorWidget;
struct ezDocumentObjectPropertyEvent;
struct ezDocumentObjectStructureEvent;
class ezPropertyAnimAssetDocument;
class ezQtColorGradientEditorWidget;
class ezColorGradientAssetData;
class ezQtQuadViewWidget;
class ezQtTimeScrubberToolbar;
struct ezPropertyAnimAssetDocumentEvent;
class QKeyEvent;
class ezQtDocumentPanel;

class ezQtPropertyAnimAssetTreeView : public QTreeView
{
  Q_OBJECT

public:
  ezQtPropertyAnimAssetTreeView(QWidget* parent);
  void initialize();

signals:
  void DeleteSelectedItemsEvent();
  void FrameSelectedItemsEvent();
  void RebindSelectedItemsEvent();

protected slots:
  void onBeforeModelReset();
  void onAfterModelReset();

protected:
  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void contextMenuEvent(QContextMenuEvent *event) override;
  void storeExpandState(const QModelIndex& parent);
  void restoreExpandState(const QModelIndex& parent, QModelIndexList& newSelection);

  QSet<QString> m_notExpandedState;
  QSet<QString> m_selectedItems;
};

class ezQtPropertyAnimAssetDocumentWindow : public ezQtGameObjectDocumentWindow, public ezGameObjectGizmoInterface
{
  Q_OBJECT

public:
  ezQtPropertyAnimAssetDocumentWindow(ezPropertyAnimAssetDocument* pDocument);
  ~ezQtPropertyAnimAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "PropertyAnimAsset"; }

public slots:
  void ToggleViews(QWidget* pView);

public:
  /// \name ezGameObjectGizmoInterface implementation
  ///@{
  virtual ezObjectAccessorBase* GetObjectAccessor() override;
  virtual bool CanDuplicateSelection() const override;
  virtual void DuplicateSelection() override;
  ///@}

protected:
  virtual void InternalRedraw() override;
  void PropertyAnimAssetEventHandler(const ezPropertyAnimAssetDocumentEvent& e);

private slots:
  void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void onScrubberPosChanged(ezUInt64 uiTick);
  void onDeleteSelectedItems();
  void onRebindSelectedItems();
  void onPlaybackTick();
  void onPlayPauseClicked();
  void onRepeatClicked();
  void onAdjustDurationClicked();
  void onDurationChangedEvent(double duration);
  void onTreeItemDoubleClicked(const QModelIndex& index);
  void onFrameSelectedTracks();

  //////////////////////////////////////////////////////////////////////////
  // Curve editor events

  void onCurveInsertCpAt(ezUInt32 uiCurveIdx, ezInt64 tickX, double newPosY);
  void onCurveCpMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, ezInt64 iTickX, double newPosY);
  void onCurveCpDeleted(ezUInt32 curveIdx, ezUInt32 cpIdx);
  void onCurveTangentMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void onLinkCurveTangents(ezUInt32 curveIdx, ezUInt32 cpIdx, bool bLink);
  void onCurveTangentModeChanged(ezUInt32 curveIdx, ezUInt32 cpIdx, bool rightTangent, int mode);

  void onCurveBeginOperation(QString name);
  void onCurveEndOperation(bool commit);
  void onCurveBeginCpChanges(QString name);
  void onCurveEndCpChanges();

  //////////////////////////////////////////////////////////////////////////
  // Color gradient editor events

  void onGradientColorCpAdded(double posX, const ezColorGammaUB& color);
  void onGradientAlphaCpAdded(double posX, ezUInt8 alpha);
  void onGradientIntensityCpAdded(double posX, float intensity);
  void MoveGradientCP(ezInt32 idx, double newPosX, const char* szArrayName);
  void onGradientColorCpMoved(ezInt32 idx, double newPosX);
  void onGradientAlphaCpMoved(ezInt32 idx, double newPosX);
  void onGradientIntensityCpMoved(ezInt32 idx, double newPosX);
  void RemoveGradientCP(ezInt32 idx, const char* szArrayName);
  void onGradientColorCpDeleted(ezInt32 idx);
  void onGradientAlphaCpDeleted(ezInt32 idx);
  void onGradientIntensityCpDeleted(ezInt32 idx);
  void onGradientColorCpChanged(ezInt32 idx, const ezColorGammaUB& color);
  void onGradientAlphaCpChanged(ezInt32 idx, ezUInt8 alpha);
  void onGradientIntensityCpChanged(ezInt32 idx, float intensity);
  void onGradientBeginOperation();
  void onGradientEndOperation(bool commit);
  //void onGradientNormalizeRange();

  //////////////////////////////////////////////////////////////////////////
  // Event track editor events
  void onEventTrackInsertCpAt(ezInt64 tickX, QString value);
  void onEventTrackCpMoved(ezUInt32 cpIdx, ezInt64 iTickX);
  void onEventTrackCpDeleted(ezUInt32 cpIdx);
  void onEventTrackBeginOperation(QString name);
  void onEventTrackEndOperation(bool commit);
  void onEventTrackBeginCpChanges(QString name);
  void onEventTrackEndCpChanges();

  //////////////////////////////////////////////////////////////////////////

private:
  ezPropertyAnimAssetDocument* GetPropertyAnimDocument();
  //void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void SelectionEventHandler(const ezSelectionManagerEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);
  void UpdateCurveEditor();
  void UpdateGradientEditor();
  void UpdateEventTrackEditor();
  void UpdateSelectionData();

  ezQtQuadViewWidget* m_pQuadViewWidget;
  ezCurveGroupData m_CurvesToDisplay;
  ezColorGradientAssetData* m_pGradientToDisplay = nullptr;
  ezInt32 m_iMapGradientToTrack = -1;
  ezDynamicArray<ezInt32> m_MapSelectionToTrack;
  ezQtPropertyAnimAssetTreeView* m_pPropertyTreeView = nullptr;
  ezQtPropertyAnimModel* m_pPropertiesModel;
  QItemSelectionModel* m_pSelectionModel = nullptr;
  ezQtCurve1DEditorWidget* m_pCurveEditor = nullptr;
  ezQtEventTrackEditorWidget* m_pEventTrackEditor = nullptr;
  ezQtColorGradientEditorWidget* m_pGradientEditor = nullptr;
  ezQtTimeScrubberToolbar* m_pScrubberToolbar = nullptr;
  ezQtDocumentPanel* m_pCurvePanel = nullptr;
  ezQtDocumentPanel* m_pColorGradientPanel = nullptr;
  ezQtDocumentPanel* m_pEventTrackPanel = nullptr;
  bool m_bAnimTimerInFlight = false;
};
