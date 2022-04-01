#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <RendererFoundation/Device/SwapChain.h>

class ezXRInterface;

class EZ_GAMEENGINE_DLL ezGALXRSwapChain : public ezGALSwapChain
{
public:
  using Functor = ezDelegate<ezGALSwapChainHandle(ezXRInterface*)>;
  static void SetFactoryMethod(Functor factory);

  static ezGALSwapChainHandle Create(ezXRInterface* pXrInterface);

public:
  ezGALXRSwapChain(ezXRInterface* pXrInterface);

protected:
  static Functor s_Factory;

protected:
  ezXRInterface* m_pXrInterface = nullptr;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezGALXRSwapChain);
