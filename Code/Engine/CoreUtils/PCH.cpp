#include <CoreUtils/PCH.h>

EZ_STATICLINK_LIBRARY(CoreUtils)
{
if(bReturn)
  return;

  EZ_STATICLINK_REFERENCE(CoreUtils_ImageWriters_Implementation_BMPWriter);
}

