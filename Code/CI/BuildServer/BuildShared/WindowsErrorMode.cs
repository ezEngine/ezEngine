using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BuildShared
{
  public class ezWindowsErrorModeSetter : IDisposable
  {
    [Flags]
    internal enum ErrorModes : uint
    {
      SYSTEM_DEFAULT = 0x0,
      SEM_FAILCRITICALERRORS = 0x0001,
      SEM_NOALIGNMENTFAULTEXCEPT = 0x0004,
      SEM_NOGPFAULTERRORBOX = 0x0002,
      SEM_NOOPENFILEERRORBOX = 0x8000
    }

    [System.Runtime.InteropServices.DllImport("kernel32.dll")]
    internal static extern ErrorModes SetErrorMode(ErrorModes mode);

    public ezWindowsErrorModeSetter()
    {
      _oldFlags = SetErrorMode(SetErrorMode(0) | ErrorModes.SEM_NOGPFAULTERRORBOX |
        ErrorModes.SEM_FAILCRITICALERRORS | ErrorModes.SEM_NOOPENFILEERRORBOX);
    }

    public void Dispose()
    {
      SetErrorMode(_oldFlags);
    }

    private ErrorModes _oldFlags;
  }
}
