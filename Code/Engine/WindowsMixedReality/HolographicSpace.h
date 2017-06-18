#pragma once

#include <WindowsMixedReality/Basics.h>
#include <Core/Input/DeviceTypes/Controller.h>
#include <Foundation/Configuration/Singleton.h>


namespace ABI
{
  namespace Windows
  {
    namespace Graphics
    {
      namespace Holographic
      {
        struct IHolographicSpaceStatics;
        struct IHolographicSpace;
      }
    }
  }
}

class ezWindow;

/// \brief Integration of Windows HolographicSpace (WinRT).
///
/// Right now window holographic is the only vr/mr platform we support.
/// In the future this may be the implementation of an abstract interface that allows the use of arbitrary vr/mr device.
/// For now however, this is entirely platform specific.
class EZ_WINDOWSMIXEDREALITY_DLL ezWindowsHolographicSpace
{
  EZ_DECLARE_SINGLETON(ezWindowsHolographicSpace);

public:
  ezWindowsHolographicSpace();
  ~ezWindowsHolographicSpace();

  /// Initializes the holographic space for a given window.
  ezResult InitForWindow(const ezWindow& window);

  /// Wheather vr/mr headsets are supported at all.
  ///
  /// True for all x64 Windows beyond Creator's Update if the headset was setup already. 
  //bool IsSupported() const;

  /// \brief Wheather a headset is ready for rendering.
  ///
  /// Can be called *before* being initialized with a window.
  bool IsAvailable() const;

private:
  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicSpaceStatics> m_pHolographicSpaceStatics;
  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicSpace> m_pHolographicSpace;
};

