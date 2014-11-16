#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Pipeline/RenderHelper.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <System/Window/Window.h>

class QWidget;

class ezEmbeddedWindow : public ezWindowBase
{
public:
  virtual ezSizeU32 GetClientAreaSize() const override;
  virtual ezWindowHandle GetNativeWindowHandle() const override;

  QWidget* m_pWidget;
};

struct TestCB
{
  ezMat4 mvp;
};

namespace DontUse
{
  class MayaObj
  {
  public:

    struct Vertex
    {
      ezVec3 pos;
      ezVec3 norm;
      ezVec2 tex0;
    };

    static MayaObj* LoadFromFile(const char* szPath, ezGALDevice* pDevice, int i);

    ezMeshBufferResourceHandle m_hMeshBuffer;

  protected:

    MayaObj(const ezArrayPtr<Vertex>& pVertices, const ezArrayPtr<ezUInt16>& pIndices, ezGALDevice* pDevice, int iMesh);


  };
}

class ez3DViewWidget : public QWidget
{
public:
  ez3DViewWidget(QWidget* pParent, ezDocumentWindow* pDocument) : QWidget(pParent), m_bResized(false), m_pDocument(pDocument) { }

  bool HasBeenResized() { bool b = m_bResized; m_bResized = false; return b; }

protected:
  virtual void resizeEvent(QResizeEvent* event) override
  {
    m_bResized = true;
    m_pDocument->TriggerRedraw();
  }

  bool m_bResized;
  ezDocumentWindow* m_pDocument;
};

class ezTestDocumentWindow : public ezDocumentWindow
{
  Q_OBJECT

public:
  ezTestDocumentWindow(ezDocumentBase* pDocument);
  ~ezTestDocumentWindow();

private slots:

private:
  static void InitDevice();
  void InitWindow();
  void DeinitWindow();
  void RecreateRenderTarget();
  virtual void InternalRedraw() override;
  ezEmbeddedWindow m_Window;

  static ezGALDevice* s_pDevice;
  ezGALRenderTargetConfigHandle m_hBBRT;
  ezGALBufferHandle m_hCB;
  ezShaderResourceHandle m_hShader;
  ezGALRasterizerStateHandle m_hRasterizerState;
  ezGALDepthStencilStateHandle m_hDepthStencilState;
  ezGALSwapChainHandle m_hPrimarySwapChain;
  float m_fRotY;
  ezColor m_Color;
  static ezUInt32 s_uiInstances;
  ez3DViewWidget* m_pCenterWidget;

  static const int MaxObjs = 7;

  ezInt32 m_iCurObject;
  DontUse::MayaObj* m_pObj[MaxObjs];
};