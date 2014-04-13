using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading;
using BuildShared;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace CommandAndControl
{
  /// <summary>
  /// Main class of the command and control application. It creates a http server that listens on the port 12345.
  /// Build machines can register to it by referencing the IP of the CNC application in their settings.json file.
  /// 
  /// Make sure to call this with admin rights first to allow this application to use port 12345:
  /// "netsh http add urlacl url=http://*:12345/ user=Everyone"
  /// Also make sure the firewall allows port 12345 to pass through.
  /// </summary>
  public class ezCNC
  {
    #region Public Functions

    public ezCNC()
    {
    }

    /// <summary>
    /// If false is returned the settings in the settings.json file need to be set first and the app restarted
    /// or the SVN server could not be queried for the new revision.
    /// </summary>
    /// <returns></returns>
    public bool Init()
    {
      if (!LoadSettings())
        return false;

      if (!CheckForNewRevision())
        return false;

      string sAddress = "http://*:12345/";
      Console.WriteLine("Server listening on address: {0}", sAddress);
      _Listener.Prefixes.Add(sAddress);
      return true;
    }

    /// <summary>
    /// Blocks and handles the http server for all eternity.
    /// </summary>
    public void Run()
    {
      _Listener.Start();

      _Timer.Elapsed += Timer_Elapsed;
      _Timer.Interval = 1000 * iTimerInterval;
      _Timer.Enabled = true;

      while (true)
      {
        try
        {
          var context = _Listener.GetContext();
          ThreadPool.QueueUserWorkItem(o => HandleRequest(context));
        }
        catch (Exception ex)
        {
          Console.WriteLine("Run Failed: {0}", ex.Message);
        }
      }
    }

    #endregion Public Functions

    #region Event Handler

    void Timer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
    {
      CheckBuildMachineAliveStatus();

      // Check SVN server every 60 seconds for a new HEAD revision.
      if (_iLastSVNCheckTimestamp + iSVNRevisionCheckInterval < LinuxDateTime.Now())
      {
        CheckForNewRevision();
      }

      lock (_Lock)
      {
        if (IsHibernationPossible())
        {
          // Conditions for hibernation are met - increase hibernation timer
          _iHibernationTime += iTimerInterval;
          if (_iHibernationTime > iHibernationDelay)
          {
            _iHibernationTime = 0;
            // We have been idle for 15min (iHibernationDelay), thus we now go into hibernation. 
            ezProcessHelper.RunExternalExe("shutdown", "-h", _Settings.AbsOutputFolder, null);
          }
        }
        else
        {
          // Reset hibernation timer
          _iHibernationTime = 0;
        }
      }
    }

    private bool IsHibernationPossible()
    {
      if (_bHibernateOnIdle && !_bIsPaused)
      {
        // If we have no working machine and no machine is not at head we hibernate the PC.
        if (_NextMachine != null)
          return false;

        foreach (KeyValuePair<string, BuildMachine> entry in _Machines)
        {
          BuildMachine machine = entry.Value;
          if (machine.State != BuildMachine.BuildMachineState.Idle)
            return false;

          if (machine.Settings.Revision != _HeadRevision)
            return false;
        }
        return true;
      }
      return false;
    }
    private void HandleRequest(object state)
    {
      HttpListenerContext context = (HttpListenerContext)state;
      try
      {
        HttpListenerRequest request = context.Request;
        // Get message text
        string sMessageText;
        using (var reader = new StreamReader(request.InputStream, UTF8Encoding.UTF8))
        {
          sMessageText = reader.ReadToEnd();
        }

        string sResponseMessage = null;
        // If no query was given, show help page
        if (request.QueryString.AllKeys.Count() == 0)
        {
          sResponseMessage = ShowHelpPage();
        }
        else
        {
          // Retrieve type of message.
          ezBuildRequestMessageType eType = ezBuildRequestMessageType.INVALID_REQUEST;
          try
          {
            if (request.QueryString.AllKeys.Contains("type"))
              eType = (ezBuildRequestMessageType)Convert.ToInt32(request.QueryString["type"]);
          }
          catch (FormatException)
          {
            eType = ezBuildRequestMessageType.INVALID_REQUEST;
            Console.WriteLine("'type' of request invalid or not present!");
            context.Response.StatusCode = 400; // Bad Request
            context.Response.OutputStream.Close();
            return;
          }

          // Handle message
          switch (eType)
          {
            case ezBuildRequestMessageType.POSTConfiguration:
              {
                BuildMachineSettings settings = Newtonsoft.Json.JsonConvert.DeserializeObject<BuildMachineSettings>(sMessageText);
                sResponseMessage = HandlePOSTConfiguration(settings);
              }
              break;
            case ezBuildRequestMessageType.GETPing:
              {
                string sID = request.QueryString["ID"];
                sResponseMessage = HandleGETPing(sID);
              }
              break;
            case ezBuildRequestMessageType.GETWork:
              {
                string sID = request.QueryString["ID"];
                sResponseMessage = HandleGETWork(sID);
              }
              break;
            case ezBuildRequestMessageType.POSTBuildResult:
              {
                string sID = request.QueryString["ID"];
                string sFilename = request.QueryString["File"];
                sResponseMessage = POSTBuildResult(sID, sFilename, sMessageText);
              }
              break;
            case ezBuildRequestMessageType.GETStatus:
              {
                sResponseMessage = HandleGetStatus();
              }
              break;
            case ezBuildRequestMessageType.GETCheckHEADRevision:
              {
                _iLastSVNCheckTimestamp = 0;
                sResponseMessage = "Checking SVN Request received.";
              }
              break;
            case ezBuildRequestMessageType.GETPostToAddress:
              {
                string sAddress = request.QueryString["TO"];
                int iStartRevision = 0;
                if (request.QueryString.AllKeys.Contains("StartRevision"))
                  iStartRevision = Convert.ToInt32(request.QueryString["StartRevision"]);
                sResponseMessage = HandleGetPostToAddress(sAddress, iStartRevision);
              }
              break;
            case ezBuildRequestMessageType.GETPause:
              {
                sResponseMessage = HandleGetPause(true);
              }
              break;
            case ezBuildRequestMessageType.GETResume:
              {
                sResponseMessage = HandleGetPause(false);
              }
              break;
            case ezBuildRequestMessageType.GETEnableHibernateOnIdle:
              {
                sResponseMessage = HandleGetHibernate(true);
              }
              break;
            case ezBuildRequestMessageType.GETDisableHibernateOnIdle:
              {
                sResponseMessage = HandleGetHibernate(false);
              }
              break;
            case ezBuildRequestMessageType.GETCleanBuild:
              {
                string sID = request.QueryString["ID"];
                sResponseMessage = HandleGetCleanBuild(sID);
              }
              break;
            default:
              Console.WriteLine("HandleRequest: invalid message type: '{0}'!", eType);
              break;
          }
        }

        // Send response
        if (sResponseMessage == null)
        {
          context.Response.StatusCode = 400; // Bad Request
          context.Response.OutputStream.Close();
        }
        else
        {
          context.Response.StatusCode = 200; // OK
          context.Response.SendChunked = true;
          var bytes = Encoding.UTF8.GetBytes(sResponseMessage);
          context.Response.OutputStream.Write(bytes, 0, bytes.Length);
          context.Response.OutputStream.Close();
        }
      }
      catch (Exception ex)
      {
        Console.WriteLine("HandleRequest Failed: {0}", ex.Message);
        context.Response.StatusCode = 400; // Bad Request
        context.Response.OutputStream.Close();
      }
    }

    #endregion Event Handler

    #region Message Handler

    private string ShowHelpPage()
    {
      lock (_Lock)
      {
        // Returns a very simple web page that shows all non-buildMachine related commands to the CNC tool.
        string sHeader =  "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n" +
                          "<html>\n<head>\n" +
                            "<meta content=\"text/html; charset=ISO-8859-1\"\n" +
                            "http-equiv=\"content-type\">\n" +
                            "<title></title>\n" +
                          "</head>\n<body>\n";

        string sFooter = "</body>\n</html>\n";

        string sHelpPage = sHeader;
        sHelpPage += string.Format("<br>HEAD: {0}<br>", _HeadRevision);
        sHelpPage += string.Format("<a href=\"/?type={0}\">Check HEAD revision</a><br>", (int)ezBuildRequestMessageType.GETCheckHEADRevision);

        sHelpPage += string.Format("<a href=\"/?type={0}&TO={1}&StartRevision={2}\">Repost last 5 revisions</a><br>",
          (int)ezBuildRequestMessageType.GETPostToAddress, _Settings.WebsiteServer, (_HeadRevision - 5).ToString());
        sHelpPage += string.Format("<a href=\"/?type={0}\">Machine Status</a><br><br>", (int)ezBuildRequestMessageType.GETStatus);

        sHelpPage += string.Format("<br>Paused: {0}<br>", _bIsPaused ? "yes" : "no");
        sHelpPage += string.Format("<a href=\"/?type={0}\">Pause</a><br>", (int)ezBuildRequestMessageType.GETPause);
        sHelpPage += string.Format("<a href=\"/?type={0}\">Resume</a><br><br>", (int)ezBuildRequestMessageType.GETResume);

        sHelpPage += string.Format("<br>Hibernate on idle: {0}<br>", _bHibernateOnIdle ? "yes" : "no");
        sHelpPage += string.Format("<a href=\"/?type={0}\">Enable Hibernate on Idle</a><br>", (int)ezBuildRequestMessageType.GETEnableHibernateOnIdle);
        sHelpPage += string.Format("<a href=\"/?type={0}\">Disable Hibernate on Idle</a><br><br><br>", (int)ezBuildRequestMessageType.GETDisableHibernateOnIdle);

        foreach (KeyValuePair<string, BuildMachine> entry in _Machines)
        {
          BuildMachine machine = entry.Value;
          sHelpPage += string.Format("<a href=\"/?type={0}&ID={1}\">Clean {1}</a><br>", (int)ezBuildRequestMessageType.GETCleanBuild, machine.Settings.ConfigurationName);
        }

        sHelpPage += sFooter;
        return sHelpPage;
      }
    }

    private string HandlePOSTConfiguration(BuildMachineSettings settings)
    {
      lock (_Lock)
      {
        // We got a new build machine! We use it's configurationName as it's ID as it should be chosen unique by the admin.
        _Machines[settings.ConfigurationName] = new BuildMachine(settings);
        DetermineCurrentRevisionOfBuildMachine(_Machines[settings.ConfigurationName]);
        Console.WriteLine("HandlePOSTConfiguration: New machine '{0}' connected!", settings.ConfigurationName);
        return settings.ConfigurationName;
      }
    }

    private string HandleGETPing(string sID)
    {
      lock (_Lock)
      {
        if (!_Machines.ContainsKey(sID))
        {
          Console.WriteLine("HandleGETPing: Unknown machine '{0}' pinged!", sID);
          return null;
        }

        _Machines[sID].CurrentTimeout = 0;
        return "";
      }
    }

    private string HandleGETWork(string sID)
    {
      lock (_Lock)
      {
        if (!_Machines.ContainsKey(sID))
        {
          Console.WriteLine("HandleGETWork: Unknown machine '{0}' requested work!", sID);
          return null;
        }

        // Update timestamp.
        _Machines[sID].CurrentTimeout = 0;

        DetermineNextBuildMachine();

        ezGETWorkResponse response = new ezGETWorkResponse();
        // If the currently querying build machine is the 'chosen one' we allow it to build the next revision.
       if (_Machines[sID] == _NextMachine && _NextMachine.State == BuildMachine.BuildMachineState.Idle)
        {
          BuildMachine machine = _Machines[sID];
          machine.CurrentTimeout = 0;
          machine.State = machine.NeedClean ? BuildMachine.BuildMachineState.RunBuildAndClean : BuildMachine.BuildMachineState.RunBuild;
          response.Revision = machine.Settings.Revision + 1; // Build next revision in line.
          response.Response = machine.NeedClean ? ezGETWorkResponse.WorkResponse.RunBuildAndClean : ezGETWorkResponse.WorkResponse.RunBuild;
          Console.WriteLine("HandleGETWork: Machine '{0}' is now building!", sID);
        }

        return Newtonsoft.Json.JsonConvert.SerializeObject(response);
      }
    }

    string POSTBuildResult(string sID, string sFilename, string sMessage)
    {
      lock (_Lock)
      {
        try
        {
          // Write result in out output folder.
          string sResultPath = System.IO.Path.Combine(_Settings.AbsOutputFolder, sFilename);
          System.IO.File.WriteAllText(sResultPath, sMessage, Encoding.UTF8);
          if (!_Machines.ContainsKey(sID))
          {
            Console.WriteLine("POSTBuildResult: Unknown machine '{0}' tried to POST build result!", sID);
            return null;
          }

          try
          {
            if (!String.IsNullOrEmpty(_Settings.WebsiteServer))
              PostToAddress(sMessage, _Settings.WebsiteServer + String.Format("?rev={0}", _NextMachine.Settings.Revision + 1));
          }
          catch (Exception ex)
          {
            Console.WriteLine("POSTBuildResult:PostToAddress failed: {0}.", ex.Message);
          }

          // Reset current build machine.
          Debug.Assert(_Machines[sID] == _NextMachine, "We got a build result but not from the machine that is supposed to be building!");
          _NextMachine.CurrentTimeout = 0;
          _NextMachine.Settings.Revision += 1; // The build machine is now one revision higher!
          Debug.Assert(_NextMachine.Settings.Revision <= _HeadRevision);
          if (_NextMachine.State == BuildMachine.BuildMachineState.RunBuildAndClean)
            _NextMachine.NeedClean = false;
          _NextMachine.State = BuildMachine.BuildMachineState.Idle;
          _NextMachine = null;
          Console.WriteLine("POSTBuildResult: Build results written to file: '{0}'.", sResultPath);
          return "";
        }
        catch (Exception ex)
        {
          Console.WriteLine("POSTBuildResult: Writing build result to file failed: {0}.", ex.Message);
        }
        return null;
      }
    }

    string HandleGetStatus()
    {
      lock (_Lock)
      {
        return JsonConvert.SerializeObject(_Machines, Formatting.Indented);
      }
    }

    string HandleGetPostToAddress(string sAddress, int iStartRevision)
    {
      try
      {
        //string sDataTemp = System.IO.File.ReadAllText("E:\\Code\\ezengine\\Trunk\\Output\\CNC\\LinuxMakeGccRelDeb64_401.json", Encoding.UTF8);
        //PostToAddress(sDataTemp, sAddress + "?rev=399");
        //return String.Format("POST to address '{0}' successful", sAddress);

        string[] allFiles = System.IO.Directory.EnumerateFiles(_Settings.AbsOutputFolder, "*_*.json", SearchOption.TopDirectoryOnly).ToArray();
        Array.Sort<string>(allFiles);
        foreach (string sFile in allFiles)
        {
          int iRev = -1;
          string[] parts = Path.GetFileNameWithoutExtension(sFile).Split('_');
          if (parts.Count() == 2)
          {
            try
            {
              iRev = Convert.ToInt32(parts[1]);
            }
            catch (FormatException)
            {
              continue;
            }
          }

          if (iRev < iStartRevision)
            continue;

          string sData = System.IO.File.ReadAllText(sFile, Encoding.UTF8);
          PostToAddress(sData, sAddress);
          Console.WriteLine("{0} posted.", Path.GetFileNameWithoutExtension(sFile));
        }
        return String.Format("POST to address '{0}' successful", sAddress);
      }
      catch (Exception ex)
      {
        return String.Format("POST to address '{0}' failed:\n{1}!", sAddress, ex.Message);
      }
    }

    string HandleGetPause(bool bPause)
    {
      lock (_Lock)
      {
        _bIsPaused = bPause;
        return String.Format("Server is paused: {0}", _bIsPaused ? "yes" : "no");
      }
    }

    string HandleGetHibernate(bool bHibernateOnIdle)
    {
      lock (_Lock)
      {
        _bHibernateOnIdle = bHibernateOnIdle;
        return String.Format("Hibernating on idle is {0}", bHibernateOnIdle ? "on" : "off");
      }
    }

    string HandleGetCleanBuild(string sID)
    {
      string sSafeConfigurationName = sID.Replace("_", "");
      lock (_Lock)
      {
        if (!_Machines.ContainsKey(sSafeConfigurationName))
        {
          Console.WriteLine("HandleGetCleanBuild: Unknown machine '{0}' requested to be cleaned!", sSafeConfigurationName);
          return String.Format("Unknown machine '{0}' requested to be cleaned!", sSafeConfigurationName);
        }

        _Machines[sSafeConfigurationName].NeedClean = true;
        return String.Format("Machine '{0}' will be cleaned on the next build!", sSafeConfigurationName);
      }
    }


    #endregion Message Handler

    #region Private Functions

    void PostToAddress(string sData, string sAddress)
    {
      HttpHelper.ResponseResult response = HttpHelper.POST(sAddress, sData);

      if (response == null)
      {
        throw new System.Exception(String.Format("Server not responding at URL '{0}'", sAddress));
      }

      if (response.StatusCode != HttpStatusCode.OK)
      {
        throw new System.Exception(String.Format("Server '{0}' returned statusCode '{1}'", sAddress, response.StatusCode));
      }
    }

    bool CheckForNewRevision()
    {
      _iLastSVNCheckTimestamp = LinuxDateTime.Now();

      // Call 'svn info' on the server and extract the HEAD revision from the stdout stream.
      const string sRevisionToken = "Revision: ";
      string sArguments = string.Format("info -r HEAD {0} --username {1} --password {2} --non-interactive --trust-server-cert",
        _Settings.SVNServer, _Settings.SVNUsername, _Settings.SVNPassword);

      ezProcessHelper.ProcessResult ProcessRes = ezProcessHelper.RunExternalExe("svn", sArguments, _Settings.AbsOutputFolder, null, 20 * 1000);
      if (ProcessRes.ExitCode != 0)
        return false;

      string[] lines = ProcessRes.StdOut.Split('\n');
      foreach (string line in lines)
      {
        if (line.StartsWith(sRevisionToken))
        {
          string sValue = line.Substring(sRevisionToken.Length);
          try
          {
            int iRev = Convert.ToInt32(sValue);
            if (iRev != _HeadRevision)
            {
              lock (_Lock)
              {
                _HeadRevision = iRev;
                Console.WriteLine("HEAD revision: {0}", _HeadRevision);
              }
            }
          }
          catch (FormatException)
          {
            Console.WriteLine("'CheckForNewRevision' failed to extract revision number from line: '{0}", line);
            return false;
          }
          return true;
        }
      }

      lock (_Lock)
      {
        Console.WriteLine("'CheckForNewRevision' failed to find the revision line in the svn: '{0}{1}", ProcessRes.StdOut, ProcessRes.ErrorOut);
        _HeadRevision = -1;
      }
      return false;
    }

    void CheckBuildMachineAliveStatus()
    {
      lock (_Lock)
      {
        List<string> deadMachines = new List<string>();
        foreach (KeyValuePair<string, BuildMachine> entry in _Machines)
        {
          BuildMachine machine = entry.Value;
          machine.CurrentTimeout += iTimerInterval;
          // Is the machine dead?
          if (machine.CurrentTimeout > iBuildMachineTimeout)
          {
            Console.WriteLine("DetermineNextBuildMachine: Machine '{0}' is dead.", entry.Key);
            deadMachines.Add(entry.Key);

            if (machine == _NextMachine)
            {
              _NextMachine.State = BuildMachine.BuildMachineState.FatalError;
              _NextMachine = null;
            }
            continue;
          }
        }

        // Remove dead machines
        foreach (string sKey in deadMachines)
        {
          _Machines.Remove(sKey);
        }
      }
    }

    void DetermineNextBuildMachine()
    {
      lock (_Lock)
      {
        if (_NextMachine != null)
        {
          // Current machine is still active so we just wait.
          return;
        }

        if (_bIsPaused)
        {
          // Machine is paused. Don't start any builds.
          return;
        }

        // Need to find a new chosen one among our build machines. As they are
        // currently all on one physical machine we don't want them to build in parallel.
        foreach (KeyValuePair<string, BuildMachine> entry in _Machines)
        {
          BuildMachine machine = entry.Value;
         
          // The build machine that is the most far behind the HEAD will be build first, rest is currently random.
          // Also, we can't build beyond the current HEAD revision.
          if ((_NextMachine == null || machine.Settings.Revision < _NextMachine.Settings.Revision) && machine.Settings.Revision < _HeadRevision)
          {
            _NextMachine = machine;
          }
        }
      }
    }

    void DetermineCurrentRevisionOfBuildMachine(BuildMachine machine)
    {
      int iHighestRev = -1;
      string sSafeConfigurationName = machine.Settings.ConfigurationName.Replace("_", "");
      string sSeachCritera = String.Format("{0}_*.*", sSafeConfigurationName);
      // Iterate through all exiting build results and search for the last build revision.
      // If non exists, we start to build at HEAD revision, otherwise we continue to build in order.
      foreach (string file in System.IO.Directory.EnumerateFiles(_Settings.AbsOutputFolder, sSeachCritera, SearchOption.AllDirectories))
      {
        string sFile = Path.GetFileNameWithoutExtension(file);
        string[] parts = sFile.Split('_');
        if (parts.Count() >= 2)
        {
          int iRev = -1;
          try
          {
            iRev = Convert.ToInt32(parts[1]);
            if (iRev > iHighestRev)
            {
              iHighestRev = iRev;
            }
          }
          catch (FormatException)
          {
            continue;
          }
        }
      }

      if (iHighestRev > 0)
      {
        machine.Settings.Revision = iHighestRev;
      }
      else
      {
        // New build machines start to build at HEAD so we just pretend as if we are at the revision before HEAD.
        machine.Settings.Revision = _HeadRevision - 1;
      }
    }

    bool LoadSettings()
    {
      try
      {
        string sResultPath = System.IO.Path.Combine(System.IO.Directory.GetCurrentDirectory(), "settings.json");
        if (!System.IO.File.Exists(sResultPath))
        {
          Console.WriteLine("The CNC settings are not set yet.\n" +
            "Open '{0}', change the settings and restart this application.", sResultPath);
          SaveSettings();
          return false;
        }
        string serializedResult = System.IO.File.ReadAllText(sResultPath, Encoding.UTF8);
        _Settings = JsonConvert.DeserializeObject<CNCSettings>(serializedResult);

        if (string.IsNullOrEmpty(_Settings.AbsOutputFolder) || !System.IO.Directory.Exists(_Settings.AbsOutputFolder))
        {
          Console.WriteLine("'AbsOutputFolder' needs to be specified in the settings.json and exist, change it and restart this application.");
          return false;
        }
        if (string.IsNullOrEmpty(_Settings.SVNPassword) || string.IsNullOrEmpty(_Settings.SVNUsername) || string.IsNullOrEmpty(_Settings.SVNServer))
        {
          Console.WriteLine("'SVNServer', 'SVNUsername' and 'SVNPassword' needs to be specified in the settings.json, change it and restart this application.");
          return false;
        }
      }
      catch (Exception ex)
      {
        Console.WriteLine("Reading json file failed: {0}", ex.Message);
        return false;
      }
      return true;
    }

    bool SaveSettings()
    {
      try
      {
        if (string.IsNullOrEmpty(_Settings.AbsOutputFolder))
          _Settings.AbsOutputFolder = System.IO.Directory.GetCurrentDirectory();

        string serializedResult = JsonConvert.SerializeObject(_Settings, Formatting.Indented);
        string sResultPath = System.IO.Path.Combine(System.IO.Directory.GetCurrentDirectory(), "settings.json");
        System.IO.File.WriteAllText(sResultPath, serializedResult, Encoding.UTF8);
      }
      catch (Exception ex)
      {
        Console.WriteLine("Writing json file failed: {0}", ex.Message);
        return false;
      }
      return true;
    }

    #endregion Private Functions

    /// <summary>
    /// This class stores information about a build machine that previously
    /// connected to us.
    /// </summary>
    public class BuildMachine
    {
      public BuildMachine(BuildMachineSettings settings)
      {
        Settings = settings;
        State = BuildMachineState.Idle;
        CurrentTimeout = 0;
        NeedClean = false;
      }

      public enum BuildMachineState
      {
        Idle,
        RunBuild,
        RunBuildAndClean,
        FatalError,
      }

      public int CurrentTimeout { get; set; }
      public BuildMachineSettings Settings { get; set; }
      public BuildMachineState State { get; set; }
      public bool NeedClean { get; set; }
    }

    /// <summary>
    /// The external settings that allow the CNC tool to access
    /// the SVN server and store the results on disk.
    /// </summary>
    public class CNCSettings
    {
      public string AbsOutputFolder { get; set; }
      public string SVNUsername { get; set; }
      public string SVNPassword { get; set; }
      public string SVNServer { get; set; }
      public string WebsiteServer { get; set; }
    }

    #region Member Variables

    Object _Lock = new Object();
    HttpListener _Listener = new HttpListener();
    Dictionary<string, BuildMachine> _Machines = new Dictionary<string, BuildMachine>();
    int _HeadRevision = -1;
    CNCSettings _Settings = new CNCSettings();
    System.Timers.Timer _Timer = new System.Timers.Timer();
    BuildMachine _NextMachine = null;
    long _iLastSVNCheckTimestamp = 0;
    long _iHibernationTime = 0;
    bool _bIsPaused = false;
    bool _bHibernateOnIdle = false;
    const int iSVNRevisionCheckInterval = 60;
    const int iBuildMachineTimeout = 120;
    const int iHibernationDelay = 15 * 60;
    const int iTimerInterval = 5;

    #endregion Member Variables
  }
}
