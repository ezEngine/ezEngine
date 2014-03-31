using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Web.Script.Serialization;
using Newtonsoft.Json.Linq;

namespace BuildShared
{
  /// <summary>
  /// Supported http request codes by the CNC tool.
  /// Passed as "type=%d" parameter to the server.
  /// </summary>
  public enum ezBuildRequestMessageType
  {
    POSTConfiguration = 0,
    GETPing = 1,
    GETWork = 2,
    POSTBuildResult = 3,
    GETStatus = 4,
    GETCheckHEADRevision = 5,
    GETPostToAddress = 6,
    GETPause = 7,
    GETResume = 8,
    GETEnableHibernateOnIdle = 9,
    GETDisableHibernateOnIdle = 10,
    GETCleanBuild = 11,
    INVALID_REQUEST
  }

  /// <summary>
  /// Response to the GETWork http request.
  /// </summary>
  public class ezGETWorkResponse
  {
    public ezGETWorkResponse()
    {
      Response = WorkResponse.Idle;
      Revision = -1;
    }
    public enum WorkResponse
    {
      Idle,
      RunBuild,
      RunBuildAndClean,
    }

    public int Revision { get; set; }
    public WorkResponse Response { get; set; }
  }
}

