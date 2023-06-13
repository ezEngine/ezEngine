#pragma once

#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

class ezQtVisualScriptPin : public ezQtPin
{
public:
  ezQtVisualScriptPin();

  virtual void SetPin(const ezPin& pin) override;
  virtual bool UpdatePinColors(const ezColorGammaUB* pOverwriteColor = nullptr) override;
};

class ezQtVisualScriptConnection : public ezQtConnection
{
public:
  ezQtVisualScriptConnection();
};

class ezQtVisualScriptNode : public ezQtNode
{
public:
  ezQtVisualScriptNode();

  virtual void UpdateState() override;
};

class ezQtVisualScriptNodeScene : public ezQtNodeScene
{
  Q_OBJECT

public:
  ezQtVisualScriptNodeScene(QObject* pParent = nullptr);
  ~ezQtVisualScriptNodeScene();

  virtual void SetDocumentNodeManager(const ezDocumentNodeManager* pManager);

  const QPixmap& GetCoroutineIcon() const { return m_CoroutineIcon; }

private:
  void NodeChangedHandler(const ezDocumentObject* pObject);

  QPixmap m_CoroutineIcon;
};
