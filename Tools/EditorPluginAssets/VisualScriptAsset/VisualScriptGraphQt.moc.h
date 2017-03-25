#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Connection.h>

class ezQtNodeView;

class ezQtVisualScriptAssetScene : public ezQtNodeScene
{
  Q_OBJECT

public:
  ezQtVisualScriptAssetScene(QObject* parent = nullptr);
  ~ezQtVisualScriptAssetScene();

};

class ezQtVisualScriptPin : public ezQtPin
{
public:
  ezQtVisualScriptPin();

  virtual void SetPin(const ezPin* pPin) override;
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;


  virtual void ConnectedStateChanged(bool bConnected) override;
};

class ezQtVisualScriptConnection : public ezQtConnection
{
public:
  ezQtVisualScriptConnection(QGraphicsItem* parent = 0);

  virtual QPen DeterminePen() const override;
};

class ezQtVisualScriptNode : public ezQtNode
{
public:
  ezQtVisualScriptNode();

  virtual void InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject, const char* szHeaderText = nullptr) override;

};

