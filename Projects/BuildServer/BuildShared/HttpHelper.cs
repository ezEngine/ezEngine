using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;

namespace BuildShared
{
  /// <summary>
  /// This class contains static helper methods for handling http GET / POST messages containing utf8 strings.
  /// </summary>
  public static class HttpHelper
  {
    /// <summary>
    /// Returned by the POST and GET methods. Message is the response stream interpreted as utf8.
    /// </summary>
    public class ResponseResult
    {
      public ResponseResult()
      {
        StatusCode = HttpStatusCode.ServiceUnavailable;
        Message = "";
      }

      public HttpStatusCode StatusCode { get; set; }
      public string Message { get; set; }
    }

    /// <summary>
    /// Sends a blocking POST request to the given address.
    /// </summary>
    /// <param name="url">URL to which the message should be send to. E.g. http://127.0.0.1/?test=true </param>
    /// <param name="sMessage">Text message that is passed to the request stream.</param>
    /// <returns>null if the server could not be reached.</returns>
    public static ResponseResult POST(string url, string sMessage)
    {
      ResponseResult res = new ResponseResult();
      try
      {
        HttpWebRequest request = (HttpWebRequest)WebRequest.Create(url);
        request.Proxy = null;
        request.Method = "POST";
        request.ContentType = "application/json";
        byte[] bytes = System.Text.Encoding.UTF8.GetBytes(sMessage);
        request.ContentLength = bytes.Length;
        System.IO.Stream os = request.GetRequestStream();
        os.Write(bytes, 0, bytes.Length);
        os.Close();
        using (HttpWebResponse response = (HttpWebResponse)request.GetResponse())
        {
          if (response == null)
            return null;

          res.StatusCode = response.StatusCode;
          using (var reader = new System.IO.StreamReader(response.GetResponseStream(), UTF8Encoding.UTF8))
          {
            res.Message = reader.ReadToEnd();
          }
          return res;
        }
      }
      catch (System.Net.WebException ex)
      {
        if (ex.Response != null)
        {
          res.StatusCode = ((HttpWebResponse)ex.Response).StatusCode;
          return res;
        }
        else
        {
          Console.WriteLine("http POST at '{0}' Failed: {1}.", url, ex.Message);
          return null;
        }
      }
      catch (Exception ex)
      {
        Console.WriteLine("http POST at '{0}' Failed: {1}.", url, ex.Message);
        return null;
      }
    }

    /// <summary>
    /// Sends a blocking GET request to the given address.
    /// </summary>
    /// <param name="url">URL to which the message should be send to. E.g. http://127.0.0.1/?test=true </param>
    /// <returns>null if the server could not be reached.</returns>
    public static ResponseResult GET(string url)
    {
      ResponseResult res = new ResponseResult();
      try
      {
        HttpWebRequest request = (HttpWebRequest)WebRequest.Create(url);
        request.Proxy = null;
        using (HttpWebResponse response = (HttpWebResponse)request.GetResponse())
        {
          if (response == null)
            return null;

          res.StatusCode = response.StatusCode;
          using (var reader = new System.IO.StreamReader(response.GetResponseStream(), UTF8Encoding.UTF8))
          {
            res.Message = reader.ReadToEnd();
          }
          return res;
        }
      }
      catch (System.Net.WebException ex)
      {
        if (ex.Response != null)
        {
          res.StatusCode = ((HttpWebResponse)ex.Response).StatusCode;
          return res;
        }
        else
        {
          Console.WriteLine("http GET at '{0}' Failed: {1}.", url, ex.Message);
          return null;
        }
      }
      catch (Exception ex)
      {
        Console.WriteLine("http GET at '{0}' Failed: {1}.", url, ex.Message);
        return null;
      }
    }
  }
}
