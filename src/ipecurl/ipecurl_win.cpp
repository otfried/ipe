// --------------------------------------------------------------------
// ipecurl for Win32
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2024 Otfried Cheong

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, you have permission to link Ipe with the
    CGAL library and distribute executables, as long as you follow the
    requirements of the Gnu General Public License in regard to all of
    the software in the executable aside from CGAL.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "ipebase.h"

#include <windows.h>
#include <winhttp.h>

using namespace ipe;

#define TEXNAME "ipetemp.tex"
#define PDFNAME "ipetemp.pdf"
#define LOGNAME "ipetemp.log"
#define URLNAME "url1.txt"

// --------------------------------------------------------------------

static void usage()
{
  fprintf(stderr,
	  "Usage: ipecurl [ pdflatex | xelatex | lualatex ]\n"
	  "Ipecurl runs a Latex compilation on a cloud service given in 'url1.txt'.\n"
	  );
  exit(1);
}

static BOOL readData(FILE *out, HINTERNET hRequest)
{
  DWORD dwSize;
  do {
    dwSize = 0;
    if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
      fprintf(stderr, "Error %lu in WinHttpQueryDataAvailable.\n", GetLastError());
      return FALSE;
    }

    Buffer data(dwSize + 1);
    DWORD dwDownloaded = 0;
    if (!WinHttpReadData(hRequest, (LPVOID) data.data(), dwSize, &dwDownloaded)) {
      fprintf(stderr, "Error %lu in WinHttpReadData.\n", GetLastError());
      return FALSE;
    } else {
      ipeDebug("Received %d bytes.", dwDownloaded);
      fwrite(data.data(), 1, dwDownloaded, out);
    }
  } while (dwSize > 0);
  return TRUE;
}

// --------------------------------------------------------------------

int main(int argc, char *argv[])
{
  ipe::Platform::initLib(ipe::IPELIB_VERSION);

  // ensure one arguments
  if (argc != 2)
    usage();

  String command = argv[1];

  if (command != "pdflatex" && command != "xelatex" && command != "lualatex") {
    fprintf(stderr, "Illegal Latex command.\n");
    return -1;
  }

  String url = Platform::readFile(URLNAME);
  if (url.empty() || url.left(4) != "http") {
    fprintf(stderr, "Cannot find url for cloud service in '%s'.\n", URLNAME);
    return -2;
  }

  String tex = Platform::readFile(TEXNAME);

  if (tex.empty()) {
    fprintf(stderr, "Cannot read Latex source from '%s'.\n", TEXNAME);
    return -3;
  }

  FILE * out = fopen(PDFNAME, "wb");
  if (!out) {
    fprintf(stderr, "Cannot open '%s' for writing.\n", PDFNAME);
    return -4;
  }

  if (!url.hasPrefix("https://")) {
    fprintf(stderr, "URL '%s' must start with 'https://'.\n", url.z());
    return -11;
  }

  String host = url.substr(8);
  while (host.right(1) == "\n" || host.right(1) == " " || host.right(1) == "\r")
    host = host.left(host.size() - 1);


  String path = "/data?target=ipetemp.tex&command=";
  path += command;

  ipeDebug("Host '%s', path '%s'", host.z(), path.z());

  std::wstring whost = host.w();
  std::wstring wpath = path.w();

  wchar_t agent[32];
  wsprintf(agent, L"ipecurl_win %d", Platform::libVersion());

  HINTERNET hSession = WinHttpOpen(agent,
				   WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
				   nullptr, // WINHTTP_NO_PROXY_NAME,
				   nullptr, // WINHTTP_NO_PROXY_BYPASS,
				   0);
  if (!hSession) {
    fprintf(stderr, "Error %lu in WinHttpOpen.\n", GetLastError());
    return -5;
  }

  HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(),
				      INTERNET_DEFAULT_HTTPS_PORT, 0);
  if (!hConnect) {
    fprintf(stderr, "Error %lu in WinHttpConnect.\n", GetLastError());
    WinHttpCloseHandle(hSession);
    return -6;
  }

  HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wpath.c_str(),
					  nullptr, nullptr, // WINHTTP_NO_REFERER,
					  nullptr, // WINHTTP_DEFAULT_ACCEPT_TYPES,
					  WINHTTP_FLAG_SECURE);
  if (!hRequest) {
    fprintf(stderr, "Error %lu in WinHttpOpenRequest.\n", GetLastError());
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return -7;
  }

  String boundary{"------------------------f0324ce8daa3cc53"};
  String mime;
  StringStream ss(mime);
  ss << "--" << boundary << "\r\n"
     << "Content-Disposition: form-data; name=\"file\"; filename=\"latexTarball.tar\"\r\n"
     << "Content-Type: text/plain\r\n\r\n";
  // need to send Latex source as a tarball
  ss << Platform::createTarball(tex) << "\r\n";
  ss << "--" << boundary << "--\r\n";

  String additionalHeaders;
  StringStream hs(additionalHeaders);
  hs << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n"
     << "Content-Length: " << mime.size() << "\r\n";

  std::wstring wAdditionalHeaders = additionalHeaders.w();

  BOOL bResults = WinHttpSendRequest(hRequest,
				     wAdditionalHeaders.data(), -1,
				     (LPVOID) mime.data(),
				     mime.size(), mime.size(), 0);

  if (bResults)
    bResults = WinHttpReceiveResponse(hRequest, nullptr);

  if (!bResults)
    fprintf(stderr, "Error %lu in WinHttpSendRequest or WinHttpReceiveResponse.\n", GetLastError());
  else
    bResults = readData(out, hRequest);

  fclose(out);

  WinHttpCloseHandle(hRequest);
  WinHttpCloseHandle(hConnect);
  WinHttpCloseHandle(hSession);

  if (!bResults)
    return -8;

  // generate logfile
  FILE *log = Platform::fopen(LOGNAME, "wb");
  fprintf(log, "entering extended mode: using latexonline at 'https://%s'\n", host.z());

  String pdf = Platform::readFile(PDFNAME);
  if (pdf.left(4) != "%PDF") {
    // an error happened during Latex run: pdf is actually a log
    fwrite(pdf.data(), 1, pdf.size(), log);
    unlink(PDFNAME); // kill pdf file
  }
  fclose(log);
  return 0;
}

// --------------------------------------------------------------------
