#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Connection.h>

class ezQtNodeScene;
class ezQtNodeView;

class ezQtVisualShaderScene : public ezQtNodeScene
{
  Q_OBJECT

public:
  ezQtVisualShaderScene(QObject* parent = nullptr);
  ~ezQtVisualShaderScene();

protected:
  virtual void ConnectPinsAction(const ezPin* pSourcePin, const ezPin* pTargetPin) override;
};

class ezQtVisualShaderPin : public ezQtPin
{
public:
  ezQtVisualShaderPin();

  virtual void SetPin(const ezPin* pPin) override;
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;


  virtual void ConnectedStateChanged(bool bConnected) override;
};

class ezQtVisualShaderConnection : public ezQtConnection
{
public:
  ezQtVisualShaderConnection(QGraphicsItem* parent = 0);

  virtual QPen DeterminePen() const override;
};

class ezQtVisualShaderNode : public ezQtNode
{
public:
  ezQtVisualShaderNode();

  virtual void InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject, const char* szHeaderText = nullptr) override;

};

