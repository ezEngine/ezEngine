#include <CoreUtils/PCH.h>
#include <CoreUtils/ImageWriters/BMPWriter.h>
#include <Foundation/IO/IBinaryStream.h>

ezResult ezBMPWriter::Write(ezArrayPtr<const ezUInt8> data, ezUInt32 uiWidth, ezUInt32 uiHeight, DataFormat::Enum format, ezIBinaryStreamWriter& outStream)
{
  BMPHeader header;
  BitmapInfoHeader infoHeader;

  header.uiOffsetToData = sizeof(header) + sizeof(infoHeader);
  infoHeader.iWidth = uiWidth;
  infoHeader.iHeight = uiHeight;
  infoHeader.uiBitsPerPixel = 24;
  ezUInt32 uiSrcBytesPerLine = (format == DataFormat::Grayscale8) ? uiWidth : uiWidth * 3;
  EZ_ASSERT(uiHeight * uiSrcBytesPerLine == data.GetCount(), "data size is not correct");
  ezUInt32 uiDstBytesPerLine = uiWidth * 3;
  ezUInt32 uiPaddingPerLine = (uiDstBytesPerLine % 4 == 0) ? 0 : 4 - (uiDstBytesPerLine % 4);
  ezUInt32 uiTotalPadding = uiHeight * uiPaddingPerLine;
  infoHeader.uiDataSize = data.GetCount() + uiTotalPadding;
  header.uiFileSize = sizeof(header) + sizeof(infoHeader) + infoHeader.uiDataSize;

  if (outStream.WriteBytes(&header, sizeof(header)) != EZ_SUCCESS)
    return EZ_FAILURE;
  if (outStream.WriteBytes(&infoHeader, sizeof(infoHeader)) != EZ_SUCCESS)
    return EZ_FAILURE;

  ezUInt32 uiNull = 0;

  ezHybridArray<ezUInt8, 1024 * 4> lineData;
  lineData.SetCount(uiDstBytesPerLine);

  /// \todo This code needs a lot of cleanup.
  
  //ezArrayPtr<ezUInt8> lineData((ezUInt8*)alloca(uiDstBytesPerLine), uiDstBytesPerLine);
  for (ezUInt32 line = 1; line <= uiHeight; line++)
  {
    memcpy(&lineData[0], data.GetSubArray((uiHeight - line) * uiSrcBytesPerLine, uiSrcBytesPerLine).GetPtr(), uiSrcBytesPerLine);
    if (format == DataFormat::Grayscale8)
    {
      for (ezUInt32 i=1; i <= uiWidth; i++)
      {
        ezUInt32 invI = uiWidth - i;
        lineData[invI * 3 + 2] = lineData[invI];
        lineData[invI * 3 + 1] = lineData[invI];
        lineData[invI * 3 + 0] = lineData[invI];
      }
    }
    for (ezUInt32 i=0; i < uiDstBytesPerLine; i += 3)
    {
      ezMath::Swap(lineData[i], lineData[i+2]);
    }
    if (outStream.WriteBytes(&lineData[0], lineData.GetCount()) != EZ_SUCCESS)
      return EZ_FAILURE;
    if (outStream.WriteBytes(&uiNull, uiPaddingPerLine) != EZ_SUCCESS)
      return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(CoreUtils, CoreUtils_ImageWriters_Implementation_BMPWriter);

