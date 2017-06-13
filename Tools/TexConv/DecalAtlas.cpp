#include "Main.h"
#include <Foundation/Image/ImageUtils.h>

void ezTexConv::CreateDecalAtlas()
{
  // write the actual decal asset file
  {
    ezAssetFileHeader header;
    header.SetFileHashAndVersion(m_uiAssetHash, m_uiAssetVersion);
    header.Write(m_FileOut);
  }

  SetReturnCode(TexConvReturnCodes::OK);
}

