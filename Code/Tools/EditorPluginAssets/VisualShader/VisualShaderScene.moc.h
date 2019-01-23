#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Connection.h>

class ezQtNodeView;

class ezQtVisualShaderScene : public ezQtNodeScene
{
  Q_OBJECT

public:
  ezQtVisualShaderScene(QObject* parent = nullptr);
  ~ezQtVisualShaderScene();

};

class ezQtVisualShaderPin : public ezQtPin
{
public:
  ezQtVisualShaderPin();

  virtual void SetPin(const ezPin* pPin) override;
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};

class ezQtVisualShaderNode : public ezQtNode
{
public:
  ezQtVisualShaderNode();

  virtual void InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject) override;

  virtual void UpdateTitle() override;
};

