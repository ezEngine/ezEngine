#include <Mcp/McpPCH.h>

#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>
#include <Mcp/McpServer.h>
#include <Mcp/McpToolRegistry.h>

#include <Foundation/IO/JSONReader.h>
#include <Foundation/IO/MemoryStream.h>

namespace
{
  /// Writes the JSON-RPC 'id' back out exactly as the type it came in as.
  ///
  /// The spec allows numbers, strings and null, and the client is entitled to reject a response whose
  /// id doesn't match what it sent.
  void WriteId(ezMcpJsonWriter& ref_writer, const ezVariantDictionary& msg)
  {
    ref_writer.BeginVariable("id");

    const ezVariant* pId = nullptr;

    if (!msg.TryGetValue("id", pId) || !pId->IsValid())
    {
      ref_writer.WriteNULL();
    }
    else if (pId->IsA<ezString>())
    {
      ref_writer.WriteString(pId->Get<ezString>().GetView());
    }
    else
    {
      // the JSON reader represents every number as a double, but ids are conventionally integers,
      // so write them without a fractional part
      ref_writer.WriteInt64(static_cast<ezInt64>(pId->ConvertTo<double>()));
    }

    ref_writer.EndVariable();
  }
} // namespace

ezMcpServer* ezMcpServer::s_pInstance = nullptr;

ezMcpServer::ezMcpServer(ezStringView sServerName)
  : m_sServerName(sServerName)
{
  // only ever one of these, created by the plugin - the last one wins rather than asserting, because
  // taking the host down over a reporting detail would be worse than reporting the wrong port
  s_pInstance = this;
}

ezMcpServer::~ezMcpServer()
{
  Stop();

  if (s_pInstance == this)
  {
    s_pInstance = nullptr;
  }
}

ezResult ezMcpServer::Start(ezUInt16 uiPort)
{
  Stop();

  if (m_Transport.Start(uiPort, ezMakeDelegate(&ezMcpServer::HandleRequest, this)).Failed())
    return EZ_FAILURE;

  // plugins that were loaded after this one may bring their own tools along
  ezMcpToolRegistry::UpdateProviders();

  ezLog::Success("MCP server listening on http://127.0.0.1:{}/mcp, {} tools available.", m_Transport.GetPort(), ezMcpToolRegistry::GetTools().GetCount());
  return EZ_SUCCESS;
}

void ezMcpServer::Stop()
{
  if (!m_Transport.IsRunning())
    return;

  m_Transport.Stop();

  ezLog::Info("MCP server stopped.");
}

void ezMcpServer::HandleRequest(const ezMcpHttpRequest& request, ezMcpHttpResponse& ref_response)
{
  if (request.m_sPath != "/mcp")
  {
    ref_response.m_sStatus = "404 Not Found";
    return;
  }

  if (request.m_sMethod != "POST")
  {
    // GET would be the SSE stream and DELETE would end a session - we support neither
    ref_response.m_sStatus = "405 Method Not Allowed";
    return;
  }

  ezRawMemoryStreamReader reader(request.m_sBody.GetData(), request.m_sBody.GetElementCount());

  ezJSONReader json;
  if (json.Parse(reader).Failed())
  {
    ref_response.m_sStatus = "400 Bad Request";
    ref_response.m_sBody = "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32700,\"message\":\"Parse error\"}}";
    return;
  }

  ezStringBuilder sResponse;
  bool bDeferred = false;

  if (HandleMessage(json.GetTopLevelObject(), sResponse, bDeferred).Failed())
  {
    // this was a notification - it must not be answered with a JSON-RPC response
    ref_response.m_sStatus = "202 Accepted";
    return;
  }

  if (bDeferred)
  {
    // The tool is not done. Nothing goes out; the whole request is parsed and run again next time the
    // host pumps. Re-parsing costs a JSON read of a small body per attempt, which is cheaper than
    // teaching the transport to hold a half-finished response.
    ref_response.m_bDeferred = true;
    return;
  }

  ref_response.m_sStatus = "200 OK";
  ref_response.m_sBody = sResponse;
}

ezResult ezMcpServer::HandleMessage(const ezVariantDictionary& msg, ezStringBuilder& out_sResponse, bool& out_bDeferred)
{
  const ezStringView sMethod = ezMcpJson::GetString(msg, "method");
  const ezVariantDictionary* pParams = ezMcpJson::GetDict(msg, "params");

  // notifications carry no 'id' and must be answered with an empty body, never with a JSON-RPC response
  if (sMethod.StartsWith("notifications/"))
    return EZ_FAILURE;

  ezMcpJsonWriter writer;
  writer.BeginObject();

  writer.AddVariableString("jsonrpc", "2.0");
  WriteId(writer, msg);

  if (sMethod == "initialize")
  {
    // echo the client's protocol version back, rather than guessing at one
    const ezStringView sVersion = pParams ? ezMcpJson::GetString(*pParams, "protocolVersion", "2025-06-18") : "2025-06-18";

    writer.BeginObject("result");
    writer.AddVariableString("protocolVersion", sVersion);

    writer.BeginObject("capabilities");
    writer.BeginObject("tools");
    writer.EndObject();
    writer.EndObject();

    writer.BeginObject("serverInfo");
    writer.AddVariableString("name", m_sServerName);
    writer.AddVariableString("version", "0.1");
    writer.EndObject();

    writer.EndObject();
  }
  else if (sMethod == "tools/list")
  {
    writer.BeginObject("result");
    writer.BeginArray("tools");

    for (const ezMcpToolDesc& tool : ezMcpToolRegistry::GetTools())
    {
      writer.BeginObject();
      writer.AddVariableString("name", tool.m_sName);
      writer.AddVariableString("description", tool.m_sDescription);

      // the schema is raw JSON authored by the tool, so it is spliced in rather than escaped
      writer.AddVariableRawJson("inputSchema", tool.m_sInputSchema.IsEmpty() ? ezStringView("{\"type\":\"object\"}") : tool.m_sInputSchema.GetView());
      writer.EndObject();
    }

    writer.EndArray();
    writer.EndObject();
  }
  else if (sMethod == "tools/call")
  {
    const ezStringView sToolName = pParams ? ezMcpJson::GetString(*pParams, "name") : ezStringView();
    const ezVariantDictionary* pArgs = pParams ? ezMcpJson::GetDict(*pParams, "arguments") : nullptr;

    // a call without an 'arguments' object is legal for tools that take none
    const ezVariantDictionary emptyArgs;

    ezMcpToolResult result;

    if (ezMcpToolRegistry::Execute(sToolName, pArgs != nullptr ? *pArgs : emptyArgs, result).Failed())
    {
      result.SetError(ezStringBuilder("Unknown tool: ", sToolName));
    }

    if (result.m_bNotFinished)
    {
      // EndAll() rather than just returning: the writer asserts on destruction if the object it opened
      // above was never closed, and the half-written result is thrown away anyway.
      writer.EndAll();
      out_bDeferred = true;
      return EZ_SUCCESS;
    }

    writer.BeginObject("result");

    if (result.m_bIsError)
    {
      // a failed tool call is reported inside the result, not as a JSON-RPC error - that way the
      // client hands the message to the model instead of treating it as a protocol failure
      writer.AddVariableBool("isError", true);
    }

    writer.BeginArray("content");
    writer.BeginObject();
    writer.AddVariableString("type", "text");
    writer.AddVariableString("text", result.m_sText);
    writer.EndObject();
    writer.EndArray();

    writer.EndObject();
  }
  else
  {
    ezStringBuilder sMessage("Method not found: ", sMethod);

    writer.BeginObject("error");
    writer.AddVariableInt32("code", -32601);
    writer.AddVariableString("message", sMessage);
    writer.EndObject();
  }

  writer.EndObject();

  out_sResponse = writer.GetResult();

  return EZ_SUCCESS;
}
