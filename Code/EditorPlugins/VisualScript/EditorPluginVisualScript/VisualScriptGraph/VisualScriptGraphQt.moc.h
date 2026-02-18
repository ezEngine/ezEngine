#pragma once

#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

class ezQtVisualScriptPin : public ezQtVisualGraphPin
{
public:
  ezQtVisualScriptPin();

  virtual void SetPin(const ezVisualGraphPin& pin) override;
  virtual bool UpdatePinColors(const ezColorGammaUB* pOverwriteColor = nullptr) override;

private:
  void UpdateTooltip();
};

class ezQtVisualScriptConnection : public ezQtVisualGraphConnection
{
public:
  ezQtVisualScriptConnection();
};

class ezQtVisualScriptNode : public ezQtVisualGraphNode
{
public:
  ezQtVisualScriptNode();

  virtual void UpdateState() override;
};

class ezQtVisualScriptNodeScene : public ezQtVisualGraphScene
{
  Q_OBJECT

public:
  ezQtVisualScriptNodeScene(QObject* pParent = nullptr);
  ~ezQtVisualScriptNodeScene();

  virtual void InitScene(const ezVisualGraphObjectManager* pManager);

  const QPixmap& GetCoroutineIcon() const { return m_CoroutineIcon; }
  const QPixmap& GetLoopIcon() const { return m_LoopIcon; }

private:
  void NodeChangedHandler(const ezDocumentObject* pObject);

  QPixmap m_CoroutineIcon;
  QPixmap m_LoopIcon;
};
