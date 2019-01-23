using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using BuildShared;
using Newtonsoft.Json;

namespace BuildMachine
{
  /// <summary>
  /// This build machine class handles communication with the CNC server and starts the actual builds.
  /// </summary>
  public class ezBuildMachine
  {
    public ezBuildMachine(string sAbsCMakeWorkspace)
    {
      _process = new BuildProcess(sAbsCMakeWorkspace);
    }

    /// <summary>
    /// If false is returned, the settings.json file in the CMake workspace is probably not configured
    /// or another fatal error occurred, check the console output for more information..
    /// </summary>
    /// <returns></returns>
    public bool Init()
    {
      bool bRes = _process.Init();
      if (bRes)
        _ServerAddress = new IPEndPoint(IPAddress.Parse(_process.Settings.ServerAddress), 12345);
      return bRes;
    }

    /// <summary>
    /// Blocking calls that runs the build machine for all eternity.
    /// </summary>
    public void Run()
    {
      // The server is checked for new work every 15 sec.
      _Timer.Elapsed += Timer_Elapsed;
      _Timer.Interval = 1000 * 15;
      _Timer.Enabled = true;

      SendSettings();

      while (true)
      {
        // How do we stop this thing?
        System.Threading.Thread.Sleep(5000);
      }
    }

    #region Event Handler

    void Timer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
    {
      bool bIsBuilding = false;
      lock (_Lock)
      {
        bIsBuilding = _bIsBuilding;
      }
      if (bIsBuilding)
      {
        PingServer();
      }
      else
      {
        RequestWork();
      }
    }

    #endregion Event Handler

    #region Message Handling

    void PingServer()
    {
      string url = ServerUrl(ezBuildRequestMessageType.GETPing);
      HttpHelper.ResponseResult response = HttpHelper.GET(url);

      if (response == null)
      {
        Console.WriteLine("PingServer: Server not responding at URL '{0}'.", url);
        return;
      }

      if (response.StatusCode != HttpStatusCode.OK)
      {
        Console.WriteLine("PingServer: Server returned statusCode '{0}'.", response.StatusCode);
        return;
      }
    }

    void RequestWork()
    {
      string url = ServerUrl(ezBuildRequestMessageType.GETWork);
      HttpHelper.ResponseResult response = HttpHelper.GET(url);

      if (response == null)
      {
        Console.WriteLine("RequestWork: Server not responding at URL '{0}'.", url);
        return;
      }

      if (response.StatusCode == HttpStatusCode.BadRequest)
      {
        SendSettings();
        return;
      }

      if (response.StatusCode != HttpStatusCode.OK)
      {
        Console.WriteLine("RequestWork: Server returned StatusCode '{0}'.", response.StatusCode);
        return;
      }

      try
      {
        ezGETWorkResponse workResponse = Newtonsoft.Json.JsonConvert.DeserializeObject<ezGETWorkResponse>(response.Message);
        if (workResponse.Response == ezGETWorkResponse.WorkResponse.RunBuild)
        {
          HandleRunRequest(workResponse.Revision);
          return;
        }
        else if (workResponse.Response == ezGETWorkResponse.WorkResponse.RunBuildAndClean)
        {
          HandleRunRequest(workResponse.Revision, true);
          return;
        }
      }
      catch (Exception ex)
      {
        Console.WriteLine("RequestWork Failed: {0}.", ex.Message);
      }
    }

    void SendSettings()
    {
      try
      {
        string sSettings = Newtonsoft.Json.JsonConvert.SerializeObject(_process.Settings, Formatting.None,
          new JsonSerializerSettings { ContractResolver = new ezPrivateContractResolver() });
        string url = ServerUrl(ezBuildRequestMessageType.POSTConfiguration);
        HttpHelper.ResponseResult response = HttpHelper.POST(url, sSettings);

        if (response == null)
        {
          Console.WriteLine("SendSettings: Server not responding at URL '{0}'.", url);
          return;
        }

        if (response.StatusCode != HttpStatusCode.OK)
        {
          Console.WriteLine("SendSettings: Server returned statusCode '{0}'.", response.StatusCode);
          return;
        }

        lock (_Lock)
        {
          _sID = response.Message;
        }
        Console.WriteLine("SendSettings: New BuidMachine ID is '{0}'.", _sID);
      }
      catch (Exception ex)
      {
        Console.WriteLine("SendSettings Failed: {0}.", ex.Message);
      }
    }

    void HandleRunRequest(int iRevision, bool bClean = false)
    {
      lock (_Lock)
      {
        _bIsBuilding = true;
      }

      string sFilename = _process.GetResultFilename(iRevision);
      string sResults = null;
      // Check whether we have already run this build before.
      try
      {
        string sResultPath = System.IO.Path.Combine(_process.Settings.AbsOutputFolder, sFilename);
        if (System.IO.File.Exists(sResultPath))
        {
          sResults = System.IO.File.ReadAllText(sResultPath, Encoding.UTF8);
        }
      }
      catch(Exception)
      {
      }

      // Run build process as we don't have the result for this revision.
      if (sResults == null)
      {
        bool bRes = _process.Run(iRevision, bClean);
        sResults = _process.GetJSON();
      }

      // Send result to server.
      try
      {
        string url = ServerUrl(ezBuildRequestMessageType.POSTBuildResult) + string.Format("&File={0}", sFilename);
        HttpHelper.ResponseResult response = HttpHelper.POST(url, sResults);

        if (response == null)
        {
          Console.WriteLine("HandleRunRequest: Server not responding at URL '{0}'.", url);
        }
        else if (response.StatusCode != HttpStatusCode.OK)
        {
          Console.WriteLine("HandleRunRequest: Server returned statusCode '{0}'.", response.StatusCode);
        }
        else
          Console.WriteLine("HandleRunRequest: Build results for rev. '{0}' sent successfully.", iRevision);
      }
      catch (Exception ex)
      {
        Console.WriteLine("HandleRunRequest Failed: {0}.", ex.Message);
      }

      lock (_Lock)
      {
        _bIsBuilding = false;
      }
    }
    
    #endregion Message Handling

    #region Private Functions

    private string ServerUrl(ezBuildRequestMessageType eType)
    {
      lock (_Lock)
      {
        return string.Format("http://{0}/?type={1}&ID={2}", _ServerAddress.ToString(), (int)eType, _sID);
      }
    }

    #endregion Private Functions

    #region Member Variables

    BuildProcess _process;

    Object _Lock = new Object();
    bool _bIsBuilding = false;
    System.Timers.Timer _Timer = new System.Timers.Timer();
    IPEndPoint _ServerAddress;
    string _sID = "";

    #endregion Member Variables
  }
}
