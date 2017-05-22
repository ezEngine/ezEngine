#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <QGraphicsPathItem>

class ezPin;
class ezQtConnection;

enum class ezQtPinHighlightState
{
  None,
  CannotConnect,
  CannotConnectSameDirection,
  CanAddConnection,
  CanReplaceConnection,
};

class EZ_GUIFOUNDATION_DLL ezQtPin : public QGraphicsPathItem
{
public:
  ezQtPin();
  ~ezQtPin();
  virtual int type() const override { return ezQtNodeScene::Pin; }

  void AddConnection(ezQtConnection* pConnection);
  void RemoveConnection(ezQtConnection* pConnection);
  ezArrayPtr<ezQtConnection*> GetConnections() { return m_Connections; }
  bool HasAnyConnections() const { return !m_Connections.IsEmpty(); }

  const ezPin* GetPin() const { return m_pPin; }
  virtual void SetPin(const ezPin* pPin);
  virtual void ConnectedStateChanged(bool bConnected);

  virtual QPointF GetPinPos() const;
  virtual QPointF GetPinDir() const;
  virtual QRectF GetPinRect() const;
  virtual void UpdateConnections();
  void SetHighlightState(ezQtPinHighlightState state);

protected:
  virtual bool AdjustRenderingForHighlight(ezQtPinHighlightState state);
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  ezQtPinHighlightState m_HighlightState = ezQtPinHighlightState::None;

private:
  ezHybridArray<ezQtConnection*, 6> m_Connections;
  QPointF m_PinCenter;
  const ezPin* m_pPin;
  QGraphicsTextItem* m_pLabel;
};
