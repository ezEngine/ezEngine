#include <PCH.h>

#include <TexConvLib/Processing/Processor.h>

 ezTexConvProcessor::ezTexConvProcessor() = default;

ezResult ezTexConvProcessor::Process()
{
  EZ_SUCCEED_OR_RETURN(LoadInputImages());

  EZ_SUCCEED_OR_RETURN(AdjustTargetFormat());

  return EZ_SUCCESS;
}
