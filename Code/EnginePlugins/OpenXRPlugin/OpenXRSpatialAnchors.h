#pragma once

#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>
#include <GameEngine/XR/XRSpatialAnchorsInterface.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/IdTable.h>

class ezOpenXR;


class EZ_OPENXRPLUGIN_DLL ezOpenXRSpatialAnchors : public ezXRSpatialAnchorsInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezOpenXRSpatialAnchors, ezXRSpatialAnchorsInterface);

public:
  ezOpenXRSpatialAnchors(ezOpenXR* pOpenXR);
  ~ezOpenXRSpatialAnchors();

  ezXRSpatialAnchorID CreateAnchor(const ezTransform& globalTransform) override;
  ezResult DestroyAnchor(ezXRSpatialAnchorID id) override;
  ezResult TryGetAnchorTransform(ezXRSpatialAnchorID id, ezTransform& out_globalTransform) override;

private:
  friend class ezOpenXR;
  struct AnchorData
  {
    EZ_DECLARE_POD_TYPE();
    XrSpatialAnchorMSFT m_Anchor;
    XrSpace m_Space;
  };

  ezOpenXR* m_pOpenXR = nullptr;

  ezIdTable<ezXRSpatialAnchorID, AnchorData> m_Anchors;

};
