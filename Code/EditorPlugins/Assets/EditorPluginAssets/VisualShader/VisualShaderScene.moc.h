#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class ezQtVisualGraphView;

class ezQtVisualShaderScene : public ezQtVisualGraphScene
{
  Q_OBJECT

public:
  ezQtVisualShaderScene(QObject* pParent = nullptr);
  ~ezQtVisualShaderScene();
};

class ezQtVisualShaderPin : public ezQtVisualGraphPin
{
public:
  ezQtVisualShaderPin();

  virtual void SetPin(const ezVisualGraphPin& pin) override;
  virtual void paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;
};

class ezQtVisualShaderNode : public ezQtVisualGraphNode
{
public:
  ezQtVisualShaderNode();

  virtual void InitNode(const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pObject) override;

  virtual void UpdateState() override;
};
