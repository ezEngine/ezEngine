#include <DTest/PCH.h>
#include <Foundation/Types/Delegate.h>

int TargetFunction(float arg1, double arg2)
{
  return (int)arg1 + (int)arg2;
}

int CallDelegate(ezDelegate<int(ezDelegate<int(float, double)>, float, double)> del, float arg1, double arg2)
{
  return del(ezMakeDelegate(&TargetFunction), arg1, arg2);
}