#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <QGraphicsPathItem>

class ezVisualGraphPin;
class ezQtVisualGraphConnection;

/// Visual feedback state for pins during connection dragging
enum class ezQtVisualGraphPinHighlight
{
  None,
  CannotConnect,
  CannotConnectSameDirection,
  CanAddConnection,
  CanReplaceConnection,
};

/// Qt graphics item representing a pin (connection point) on a visual graph node.
///
/// Displays the pin shape, color, and label. Manages visual feedback during connection operations
/// and maintains references to all connections attached to this pin.
class EZ_GUIFOUNDATION_DLL ezQtVisualGraphPin : public QGraphicsPathItem
{
public:
  ezQtVisualGraphPin();
  ~ezQtVisualGraphPin();
  virtual int type() const override { return ezQtVisualGraphScene::Pin; }

  void AddConnection(ezQtVisualGraphConnection* pConnection);
  void RemoveConnection(ezQtVisualGraphConnection* pConnection);
  ezArrayPtr<ezQtVisualGraphConnection*> GetConnections() { return m_Connections; }
  bool HasAnyConnections() const { return !m_Connections.IsEmpty(); }

  const ezVisualGraphPin* GetPin() const { return m_pPin; }
  virtual void SetPin(const ezVisualGraphPin& pin);
  virtual void ConnectedStateChanged(bool bConnected);

  virtual QPointF GetPinPos() const;
  virtual QPointF GetPinDir() const;
  virtual QRectF GetPinRect() const;
  virtual void UpdateConnections();
  void SetHighlightState(ezQtVisualGraphPinHighlight state);

  void SetActive(bool bActive);

  virtual void ExtendContextMenu(QMenu& ref_menu) {}
  virtual void keyPressEvent(QKeyEvent* pEvent) override {}

protected:
  virtual bool UpdatePinColors(const ezColorGammaUB* pOverwriteColor = nullptr);
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  ezQtVisualGraphPinHighlight m_HighlightState = ezQtVisualGraphPinHighlight::None;
  QGraphicsTextItem* m_pLabel;
  QPointF m_PinCenter;

  bool m_bTranslatePinName = true;

private:
  bool m_bIsActive = true;

  const ezVisualGraphPin* m_pPin = nullptr;
  ezHybridArray<ezQtVisualGraphConnection*, 6> m_Connections;
};
