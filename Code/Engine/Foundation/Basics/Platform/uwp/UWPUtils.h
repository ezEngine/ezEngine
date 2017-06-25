#pragma once

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
  #error "uwp util header should only be included in UWP builds!"
#endif

#include <Foundation/Types/Types.h>
#include <Foundation/Basics/Platform/Win/HResultUtils.h>

#include <windows.foundation.h>
#include <wrl/wrappers/corewrappers.h>
#include <wrl/client.h>
#include <wrl/implements.h>

using namespace Microsoft::WRL::Wrappers;
using namespace Microsoft::WRL;

/// Helper function to iterate over all elements of a ABI::Windows::Foundation::Collections::IVectorView
/// \param callback
///   Callable of signature bool(UINT index, const ComPtr<Interface>& pElement). Return value of false means discontinue.
template<typename ElementQueryType, typename ElementType, typename Callback>
HRESULT ezWinRtIterateIVectorView(const ComPtr<ABI::Windows::Foundation::Collections::IVectorView<ElementType>>& pVectorView, const Callback& callback)
{
  UINT numElements = 0;
  HRESULT result = pVectorView->get_Size(&numElements);
  if (FAILED(result))
    return result;

  for (UINT i = 0; i < numElements; ++i)
  {
    ElementQueryType pElement;
    result = pVectorView->GetAt(i, &pElement);
    if (FAILED(result))
      return result;

    if (!callback(i, pElement))
      return S_OK;
  }

  return result;
}
