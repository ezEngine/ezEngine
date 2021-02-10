#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class ezImageDataAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezImageDataAssetProperties, ezReflectedClass);

public:
  ezString m_sInputFile;

  // TODO: more ezImageData options
  // * maximum resolution
  // * 1, 2, 3, 4 channels
  // * compression: lossy (jpg), lossless (png), uncompressed
  // * HDR data ?
  // * combine from multiple images (channel mapping)
};
