#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <QGraphicsWidget>

// Avoid conflicts with windows.
#ifdef GetObject
#  undef GetObject
#endif

class ezQtPin;
class ezDocumentNodeManager;
class QLabel;
class ezDocumentObject;
class QGraphicsGridLayout;
class QGraphicsTextItem;
class QGraphicsDropShadowEffect;

struct ezNodeFlags
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None = 0,
    Moved = EZ_BIT(0),
    UpdateTitle = EZ_BIT(1),
    Default = None
  };

  struct Bits
  {
    StorageType Moved : 1;
    StorageType UpdateTitle : 1;
  };
};

class EZ_GUIFOUNDATION_DLL ezQtNode : public QGraphicsPathItem
{
public:
  ezQtNode();
  ~ezQtNode();
  virtual int type() const override { return ezQtNodeScene::Node; }

  const ezDocumentObject* GetObject() const { return m_pObject; }
  virtual void InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject);

  virtual void UpdateGeometry();

  void CreatePins();

  ezQtPin* GetInputPin(const ezPin& pin);
  ezQtPin* GetOutputPin(const ezPin& pin);

  ezBitflags<ezNodeFlags> GetFlags() const;
  void ResetFlags();

  void EnableDropShadow(bool enable);
  virtual void UpdateState();

  const ezHybridArray<ezQtPin*, 6>& GetInputPins() const { return m_Inputs; }
  const ezHybridArray<ezQtPin*, 6>& GetOutputPins() const { return m_Outputs; }

  void SetActive(bool active);

  virtual void ExtendContextMenu(QMenu& menu) {}

protected:
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  QColor m_HeaderColor;
  QRectF m_HeaderRect;
  QGraphicsTextItem* m_pLabel = nullptr;

private:
  const ezDocumentNodeManager* m_pManager = nullptr;
  const ezDocumentObject* m_pObject = nullptr;
  ezBitflags<ezNodeFlags> m_DirtyFlags;

  bool m_bIsActive = true;

  QGraphicsDropShadowEffect* m_pShadow = nullptr;

  // Pins
  ezHybridArray<ezQtPin*, 6> m_Inputs;
  ezHybridArray<ezQtPin*, 6> m_Outputs;
};
