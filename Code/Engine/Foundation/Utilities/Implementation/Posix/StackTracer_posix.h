
#ifdef EZ_STACKTRACER_POSIX_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_STACKTRACER_POSIX_INL_H_INCLUDED

//static
ezUInt32 ezStackTracer::GetStackTrace(ezArrayPtr<void*>& trace)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

//static
void ezStackTracer::DumpStackTrace(const ezArrayPtr<void*>& trace)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}
