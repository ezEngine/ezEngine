#pragma once

#include <TestFramework/Framework/TestFramework.h>

#include <iostream>
#include <fstream>
#include <sstream>

struct ezOutputToHTML
{
  static std::ofstream htmlFile;

  static void OutputToHTML(ezTestOutput::Enum Type, const char* szMsg)
  {
    if (Type != ezTestOutput::StartOutput && !htmlFile.is_open())
      return;

    static ezInt32 iIndentation = 0;
    static bool bError = false;
    static bool bDetails = false;
    static std::string sSubTest;
    static std::string sDuration;
    static std::ostringstream details;

    switch (Type)
    {
    case ezTestOutput::StartOutput:
      {
        ezTestFramework::GetInstance()->CreateOutputFolder();

        std::string sOutputFile = std::string(ezTestFramework::GetInstance()->GetAbsOutputPath()) + "/UnitTestsLog.htm";
        const char* szTestName = ezTestFramework::GetInstance()->GetTestName();

        const char* szStyle = "body { margin: 0; padding: 20px; font-size: 12px; font-family: Arial, Sans-Serif; background-color: #fff; text-align: center; }"
          "#container { margin: 20px auto; width: 900px; text-align: left; }"
          "table { border-collapse: collapse; width: 100%; }"
          "table, td { font-size: 12px; border: solid #000 1px; padding: 5px; }"
          "td { background-color: #66ff66; }"
          "td.category { background-color: #ccc; font-weight: bold; }"
          "td.title { background-color: #fff; }"
          "td.error { background-color: #ff6666; }"
          "td.details { background-color: #ffff00; }";

        htmlFile.open(sOutputFile.c_str());
        if (htmlFile.is_open())
        {
          htmlFile << "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n<head>"
            "<title>Log - " << szTestName << "</title>\n"
            "<style type=\"text/css\" media=\"screen\">\n" << szStyle << "</style></head>\n"
            "<body>\n<div id=\"container\">\n<h1>Log - " << szTestName << "</h1>\n"
            "<table>\n<tr>\n<td class=\"title\">Testmethod</td>\n<td class=\"title\">Result</td>"
            "<td class=\"title\">Duration</td>\n<td class=\"title\">Details</td>\n</tr>";
        }
      }
      break;
    case ezTestOutput::BeginBlock:
      iIndentation++;

      if (iIndentation == 1)
      {
        htmlFile << "<tr>\n<td colspan=\"4\" class=\"category\" />" << szMsg << "\n</tr>";
      }
      else if (iIndentation == 2)
      {
        bError = false;
        bDetails = false;
        details.str("");
        sSubTest = szMsg;
      }
      break;

    case ezTestOutput::EndBlock:
      if (iIndentation == 2)
      {
        if (bError)
        {
          htmlFile << "<tr>\n<td class=\"error\">" << sSubTest << "</td>\n"
            "<td class=\"error\">Failed</td>\n<td class=\"error\">" << sDuration << "</td>\n"
            "<td class=\"error\">";
        }
        else
        if (bDetails)
        {
          htmlFile << "<tr>\n<td class=\"details\">" << sSubTest << "</td>\n"
            "<td class=\"details\">Passed</td>\n<td class=\"details\">" << sDuration << "</td>\n"
            "<td class=\"details\">";
        }
        else
        {
          htmlFile << "<tr>\n<td>" << sSubTest << "</td>\n<td>Passed</td>\n<td>" <<
            sDuration << "</td>\n<td>";
        }

        htmlFile << details.str();
        htmlFile << "</td>\n</tr>";
      }
      else if (iIndentation == 3)
      {
        details << "<br/>";
      }

      iIndentation--;
      break;

    case ezTestOutput::Duration:
      sDuration = szMsg;
      break;

    case ezTestOutput::Error:
      bError = true;
      // fall through

    case ezTestOutput::Message:
    case ezTestOutput::ImportantInfo:
      bDetails = true;
    
    case ezTestOutput::Details:
      details << szMsg << "<br/>";
      break;

    case ezTestOutput::Success:
      // nothing to do here
      break;

    case ezTestOutput::FinalResult:
      {
        htmlFile << "</table>\n<h2>" << szMsg << "</h2>";
        htmlFile << "</div>\n</body>\n</html>";
        htmlFile.close();

        std::string sOutputFile = std::string(ezTestFramework::GetInstance()->GetAbsOutputPath()) + "/UnitTestsLog.htm";

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
        TestSettings settings = ezTestFramework::GetInstance()->GetSettings();
        if (settings.m_bOpenHtmlOutput)
        {
          // opens the html file in a browser
          ShellExecuteA(nullptr, "open", sOutputFile.c_str(), nullptr, nullptr, SW_SHOW);
        }
#endif
      }
      break;
    default:
      break;
    }
  }
};

std::ofstream ezOutputToHTML::htmlFile;

