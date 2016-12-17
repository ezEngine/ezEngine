#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <Foundation/Containers/HybridArray.h>
#include <QGraphicsWidget>

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
    SelectionChanged = EZ_BIT(0),
    Moved = EZ_BIT(1),
    Default = None
  };

  struct Bits
  {
    StorageType SelectionChanged : 1;
    StorageType Moved : 1;
  };
};

class EZ_GUIFOUNDATION_DLL ezQtNode : public QGraphicsPathItem
{
public:
  ezQtNode();
  ~ezQtNode();
  virtual int type() const override { return ezQtNodeScene::Node; }

  const ezDocumentObject* GetObject() const { return m_pObject; }
  virtual void InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject, const char* szHeaderText = nullptr);
  void CreatePins();

  ezQtPin* GetInputPin(const ezPin* pPin);
  ezQtPin* GetOutputPin(const ezPin* pPin);

  ezBitflags<ezNodeFlags> GetFlags() const;
  void ResetFlags();

  void EnableDropShadow(bool enable);

protected:
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

  QColor m_HeaderColor;
  QGraphicsTextItem* m_pLabel;

private:
  const ezDocumentNodeManager* m_pManager;
  const ezDocumentObject* m_pObject;
  ezBitflags<ezNodeFlags> m_DirtyFlags;

  // Header
  QRectF m_HeaderRect;
  QGraphicsDropShadowEffect* m_pShadow;

  // Pins
  ezHybridArray<ezQtPin*, 6> m_Inputs;
  ezHybridArray<ezQtPin*, 6> m_Outputs;
};
