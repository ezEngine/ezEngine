using System;
using System.Collections.Generic;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using BuildShared;

namespace CommandAndControl
{
  class Program
  {
    static void Main(string[] args)
    {
      ezCNC CNC = new ezCNC();
      if (!CNC.Init())
        return;

      CNC.Run();
    }
  }
}
