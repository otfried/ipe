// --------------------------------------------------------------------
// Platform dependent methods
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
#include "ipeattributes.h"

#ifdef WIN32
#define NTDDI_VERSION 0x06000000
#include <windows.h>
#include <shlobj.h>
#include <direct.h>
#include <gdiplus.h>
#else
#include <sys/wait.h>
#include <dirent.h>
#endif
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <xlocale.h>
// sys/param.h triggers legacy mode for realpath, so that the
// last component of the path does not need to exist.
#include <sys/param.h>
#include <sys/errno.h>
#endif

#ifdef IPE_GSL
#include <gsl/gsl_errno.h>
#include <gsl/gsl_version.h>
#endif

// for version info only
#include <spiroentrypoints.h>

#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdarg>
#include <unistd.h>
#include <clocale>
#include <cstring>
#include <cerrno>

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::Platform
  \ingroup base
  \brief Platform dependent methods.
*/

//! Return the Ipelib version.
/*! This is available as a function so that one can verify what
  version of Ipelib one has actually linked with (as opposed to the
  header files used during compilation).
*/
int Platform::libVersion()
{
  return IPELIB_VERSION;
}

// --------------------------------------------------------------------

static bool initialized = false;
static bool showDebug = false;
static Platform::DebugHandler debugHandler = nullptr;

#ifdef WIN32
static ULONG_PTR gdiplusToken = 0;
_locale_t ipeLocale;
// Windows 7 does not have these functions, so we load them dynamically.
typedef _locale_t (*LPCreateLocale)(int category, const char *locale);
static LPCreateLocale p_create_locale = nullptr;
typedef void (*LPFreeLocale)(_locale_t locale);
static LPFreeLocale p_free_locale = nullptr;
typedef double (*LPStrtodL)(const char * s,char ** fin,_locale_t locale);
static LPStrtodL p_strtod_l = nullptr;
#else
locale_t ipeLocale;
#endif

#ifndef WIN32
static String dotIpe()
{
  const char *home = getenv("HOME");
  if (!home)
    return String();
  String res = String(home) + "/.ipe";
  if (!Platform::fileExists(res) && mkdir(res.z(), 0700) != 0)
    return String();
  return res + "/";
}
#endif

#ifdef WIN32
static void readIpeConf()
{
  String fname = Platform::ipeDir("", nullptr);
  fname += "\\ipe.conf";
  String conf = Platform::readFile(fname);
  if (conf.empty())
    return;
  ipeDebug("ipe.conf = %s", conf.z());
  int i = 0;
  while (i < conf.size()) {
    String line = conf.getLine(i);
    _wputenv(line.w().data());
  }
}
#else
static void readIpeConf()
{
  String fname = dotIpe() + "/ipe.conf";
  String conf = Platform::readFile(fname);
  if (conf.empty())
    return;
  ipeDebug("ipe.conf = %s", conf.z());
  int i = 0;
  while (i < conf.size()) {
    String line = conf.getLine(i);
    putenv(strdup(line.z()));  // a leak, but necessary
  }
}
#endif

static void debugHandlerImpl(const char *msg)
{
  if (showDebug) {
    fprintf(stderr, "%s\n", msg);
#ifdef WIN32
    fflush(stderr);
    OutputDebugStringA(msg);
#endif
  }
}

static void shutdownIpelib()
{
#ifdef WIN32
  Gdiplus::GdiplusShutdown(gdiplusToken);
  if (p_create_locale != nullptr && p_free_locale != nullptr)
    p_free_locale(ipeLocale);
#else
  freelocale(ipeLocale);
#endif
  Repository::cleanup();
}

//! Initialize Ipelib.
/*! This method must be called before Ipelib is used.

  It creates a LC_NUMERIC locale set to 'C', which is necessary for
  correct loading and saving of Ipe objects.  The method also checks
  that the correct version of Ipelib is loaded, and aborts with an
  error message if the version is not correct.  Also enables ipeDebug
  messages if environment variable IPEDEBUG is defined.  (You can
  override this using setDebug).
*/
void Platform::initLib(int version)
{
  if (initialized)
    return;
  initialized = true;
  readIpeConf();
  showDebug = false;
  if (getenv("IPEDEBUG")) {
    showDebug = true;
    fprintf(stderr, "Debug messages enabled\n");
  }
  debugHandler = debugHandlerImpl;
#ifdef WIN32
  HMODULE hDll = LoadLibraryA("msvcrt.dll");
  if (hDll) {
    p_create_locale = (LPCreateLocale) GetProcAddress(hDll, "_create_locale");
    p_free_locale = (LPFreeLocale) GetProcAddress(hDll, "_free_locale");
    p_strtod_l = (LPStrtodL) GetProcAddress(hDll, "_strtod_l");
  }
  if (p_create_locale != nullptr)
    ipeLocale = p_create_locale(LC_NUMERIC, "C");
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
#else
  ipeLocale = newlocale(LC_NUMERIC_MASK, "C", nullptr);
#endif
  atexit(shutdownIpelib);

#ifdef IPE_GSL
  // The GSL default error handler aborts the program.
  // This is useful for debugging, but should never be in a release.
  // Instead, Ipe handles error codes returned by GSL.
  gsl_set_error_handler_off();
#endif

#ifndef IPE_NO_IPELIB_VERSION_CHECK
  if (version == IPELIB_VERSION)
    return;
  fprintf(stderr,
	  "Ipetoipe has been compiled with header files for Ipelib %d\n"
	  "but is dynamically linked against libipe %d.\n"
	  "Check with 'ldd' which libipe is being loaded, and "
	  "replace it by the correct version or set LD_LIBRARY_PATH.\n",
	  version, IPELIB_VERSION);
  exit(99);
#endif
}

//! Enable or disable display of ipeDebug messages.
void Platform::setDebug(bool debug)
{
  showDebug = debug;
}

// --------------------------------------------------------------------

void ipeDebug(const char *msg, ...) noexcept
{
  if (debugHandler) {
    char buf[8196];
    va_list ap;
    va_start(ap, msg);
    std::vsprintf(buf, msg, ap);
    va_end(ap);
    debugHandler(buf);
  }
}

void ipeDebugBuffer(Buffer data, int maxsize)
{
  if (!showDebug) return;
  int size = std::min(data.size(), maxsize);
  String s;
  StringStream ss(s);
  for (int i = 0; i < size; ++i) {
    ss.putHexByte(data[i]);
    ss << " ";
  }
  debugHandler(s.z());
}

// --------------------------------------------------------------------

//! Returns current working directory.
/*! Returns empty string if something fails. */
String Platform::currentDirectory()
{
#ifdef WIN32
  wchar_t *buffer = _wgetcwd(nullptr, 0);
  return String(buffer);
#else
  char buffer[1024];
  if (getcwd(buffer, 1024) != buffer)
    return String();
  return String(buffer);
#endif
}

//! Returns drive on which Ipe executable exists.
/*! On Linux and OSX, returns empty string. */
String Platform::ipeDrive()
{
#ifdef WIN32
  String fname = ipeDir("", nullptr);
  if (fname.size() > 2 && fname[1] == ':')
    return fname.left(2);
#endif
  return String();
}

#ifdef IPEBUNDLE
String Platform::ipeDir(const char *suffix, const char *fname)
{
#ifdef WIN32
  wchar_t exename[OFS_MAXPATHNAME];
  GetModuleFileNameW(nullptr, exename, OFS_MAXPATHNAME);
  String exe(exename);
#else
  char path[PATH_MAX], rpath[PATH_MAX];
  uint32_t size = sizeof(path);
  String exe;
  if (_NSGetExecutablePath(path, &size) == 0 && realpath(path, rpath) != nullptr)
    exe = String(rpath);
  else
    ipeDebug("ipeDir: buffer too small; need size %u", size);
#endif
  int i = exe.rfind(IPESEP);
  if (i >= 0) {
    exe = exe.left(i);    // strip filename
    i = exe.rfind(IPESEP);
    if (i >= 0) {
      exe = exe.left(i);  // strip bin directory name
    }
  }
#ifdef __APPLE__
  if (!strcmp(suffix, "doc"))
    exe += "/SharedSupport/";
  else
    exe += "/Resources/";
#else
  exe += IPESEP;
#endif
  exe += suffix;
  if (fname) {
    exe += IPESEP;
    exe += fname;
  }
  return exe;
}
#endif

//! Return path for the directory containing pdflatex and xelatex.
/*! If empty means look on PATH. */
String Platform::latexPath()
{
  String result;
#ifdef WIN32
  const wchar_t *p = _wgetenv(L"IPELATEXPATH");
  if (p)
    result = String(p);
  if (result.left(4) == "ipe:")
    result = ipeDrive() + result.substr(4);
#else
  char *p = getenv("IPELATEXPATH");
  if (p)
    result = p;
#endif
  return result;
}

//! Returns directory for running Latex.
/*! The directory is created if it does not exist.  Returns an empty
  string if the directory cannot be found or cannot be created.
  The directory returned ends in the path separator.
 */
String Platform::latexDirectory()
{
#ifdef WIN32
  String latexDir;
  const wchar_t *p = _wgetenv(L"IPELATEXDIR");
  if (p) {
    latexDir = String(p);
    if (latexDir.left(4) == "ipe:")
      latexDir = ipeDrive() + latexDir.substr(4);
  } else {
    wchar_t *path;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) {
      latexDir = String(path) + "\\ipe";
      CoTaskMemFree(path);
    } else {
      p = _wgetenv(L"LOCALAPPDATA");
      if (p)
	latexDir = String(p) + "\\ipe";
      else
	latexDir = ipeDir("latexrun");
    }
  }
  if (latexDir.right(1) == "\\")
    latexDir = latexDir.left(latexDir.size() - 1);
  if (!fileExists(latexDir)) {
    if (Platform::mkdir(latexDir.z()) != 0)
      return String();
  }
  latexDir += "\\";
  return latexDir;
#else
  const char *p = getenv("IPELATEXDIR");
  String latexDir;
  if (p) {
    latexDir = p;
    if (latexDir.right(1) == "/")
      latexDir = latexDir.left(latexDir.size() - 1);
  } else {
    latexDir = dotIpe() + "latexrun";
  }
  if (!fileExists(latexDir) && mkdir(latexDir.z(), 0700) != 0)
    return String();
  latexDir += "/";
  return latexDir;
#endif
}

//! Determine whether file exists.
bool Platform::fileExists(String fname)
{
#ifdef WIN32
  return (_waccess(fname.w().data(), F_OK) == 0);
#else
  return (access(fname.z(), F_OK) == 0);
#endif
}

//! Convert relative filename to absolute.
/*! This also works when the filename does not exist, or at least it tries. */
String Platform::realPath(String fname)
{
#ifdef WIN32
  wchar_t wresult[MAX_PATH];
  // this function works also when fname does not exist
  GetFullPathNameW(fname.w().data(), MAX_PATH, wresult, nullptr);
  return String(wresult);
#else
  char rpath[PATH_MAX];
  if (realpath(fname.z(), rpath))
    return String(rpath);
  if (errno != ENOENT || fname.left(1) == "/")
    return fname;  // not much we can do
  if (realpath(".", rpath) == nullptr)
    return fname;  // nothing we can do
  return String(rpath) + "/" + fname;
#endif
}

//! List all files in directory
/*! Return true if successful, false on error. */
bool Platform::listDirectory(String path, std::vector<String> &files)
{
#ifdef WIN32
  String pattern = path + "\\*";
  struct _wfinddata_t info;
  intptr_t h = _wfindfirst(pattern.w().data(), &info);
  if (h == -1L)
    return false;
  files.push_back(String(info.name));
  while (_wfindnext(h, &info) == 0)
    files.push_back(String(info.name));
  _findclose(h);
  return true;
#else
  DIR *dir = opendir(path.z());
  if (dir == nullptr)
    return false;
  struct dirent *entry = readdir(dir);
  while (entry != nullptr) {
    String s(entry->d_name);
    if (s != "." && s != "..")
      files.push_back(s);
    entry = readdir(dir);
  }
  closedir(dir);
  return true;
#endif
}

//! Read entire file into string.
/*! Returns an empty string if file cannot be found or read.
  There is no way to distinguish an empty file from this. */
String Platform::readFile(String fname)
{
  std::FILE *file = Platform::fopen(fname.z(), "rb");
  if (!file)
    return String();
  String s;
  int ch;
  while ((ch = std::fgetc(file)) != EOF)
    s.append(ch);
  std::fclose(file);
  return s;
}

#ifndef __EMSCRIPTEN__  // wasm version is elsewhere
//! Runs latex on file ipetemp.tex in given directory.
/*! directory of docname is added to TEXINPUTS if its non-empty. */
int Platform::runLatex(String dir, LatexType engine, String docname) noexcept
{
  const char *latex = (engine == LatexType::Xetex) ?
    "xelatex" : (engine == LatexType::Luatex) ?
    "lualatex" : "pdflatex";
  String url = Platform::readFile(dir + "url1.txt");
  bool online = (url.left(4) == "http");
  String texinputs;
  if (!online && !docname.empty()) {
    docname = realPath(docname);
    int i = docname.size();
    while (i > 0 && docname[i-1] != IPESEP)
      --i;
    if (i > 0)
      texinputs = docname.substr(0, i-1);
  }
#ifdef WIN32
  if (!online && getenv("IPETEXFORMAT")) {
    latex = (engine == LatexType::Xetex) ?
      "xetex ^&latex" : (engine == LatexType::Luatex) ?
      "luatex ^&latex" : "pdftex ^&pdflatex";
  }

  String bat;
  // try to change codepage to UTF-8
  bat += "chcp 65001\r\n";
  if (dir.size() > 2 && dir[1] == ':') {
    bat += dir.substr(0, 2);
    bat += "\r\n";
  }
  bat += "cd \"";
  bat += dir;
  bat += "\"\r\n";
  if (!texinputs.empty()) {
    bat += "setlocal\r\n";
    bat += "set TEXINPUTS=.;";
    bat += texinputs;
    bat += ";%TEXINPUTS%\r\n";
  }
  if (online) {
    bat += "\"";
    bat += ipeDir("bin", "ipecurl.exe");
    bat += "\" ";
    bat += latex;
    bat += "\r\n";
  } else {
    String path = latexPath();
    if (!path.empty()) {
      bat += "PATH ";
      bat += path;
      bat += ";%PATH%\r\n";
    }
    bat += latex;
    bat += " ipetemp.tex\r\n";
  }
  if (!texinputs.empty())
    bat += "endlocal\r\n";
  // bat += "pause\r\n";  // alternative for Wine

  String s = dir + "runlatex.bat";
  std::FILE *f = Platform::fopen(s.z(), "wb");
  if (!f)
    return -1;
  std::fwrite(bat.data(), 1, bat.size(), f);
  std::fclose(f);

  // Declare and initialize process blocks
  PROCESS_INFORMATION processInformation;
  STARTUPINFOW startupInfo;

  memset(&processInformation, 0, sizeof(processInformation));
  memset(&startupInfo, 0, sizeof(startupInfo));
  startupInfo.cb = sizeof(startupInfo);

  // Call the executable program
  String cmd = String("cmd /c call \"") + dir + String("runlatex.bat\"");
  std::wstring wcmd = cmd.w();

  int result = CreateProcessW(nullptr, wcmd.data(), nullptr, nullptr, FALSE,
			      NORMAL_PRIORITY_CLASS|CREATE_NO_WINDOW,
			      nullptr, nullptr, &startupInfo, &processInformation);
  if (result == 0)
    return -1;  // failure to create process

  // Wait until child process exits.
  WaitForSingleObject(processInformation.hProcess, INFINITE);

  // Close process and thread handles.
  CloseHandle(processInformation.hProcess);
  CloseHandle(processInformation.hThread);

  // Apparently WaitForSingleObject doesn't work in Wine
  const char *wine = getenv("IPEWINE");
  if (wine)
    Sleep(Lex(wine).getInt());
  return 0;
#else
  if (!online && getenv("IPETEXFORMAT")) {
    latex = (engine == LatexType::Xetex) ?
      "xetex \\&latex" : (engine == LatexType::Luatex) ?
      "luatex \\&latex" : "pdftex \\&pdflatex";
  }
  String s("cd \"");
  s += dir;
  s += "\"; rm -f ipetemp.log; ";
  if (!texinputs.empty()) {
    s += "export TEXINPUTS=\"";
    s += texinputs;
    s += ":$TEXINPUTS\"; ";
  }
  if (online) {
#if defined(__APPLE__) && defined(IPEBUNDLE)
    s += "\"";
    s += ipeDir("../MacOS", "ipecurl");
    s += "\" ";
#else
    s += "ipecurl ";
#endif
    s += latex;
  } else {
    String path = latexPath();
    if (path.empty())
      s += latex;
    else
      s += String("\"") + path + "/" + latex + "\"";
    s += " ipetemp.tex";
  }
  s += " > /dev/null";
  int result = std::system(s.z());
  if (result != -1)
    result = WEXITSTATUS(result);
  return result;
#endif
}
#endif

#ifdef WIN32
FILE *Platform::fopen(const char *fname, const char *mode)
{
  return _wfopen(String(fname).w().data(), String(mode).w().data());
}

int Platform::mkdir(const char *dname)
{
  return _wmkdir(String(dname).w().data());
}

//! Return a wide string including a terminating zero character.
std::wstring String::w() const noexcept
{
  if (empty())
    return L"\0";
  int rw = MultiByteToWideChar(CP_UTF8, 0, data(), size(), nullptr, 0);
  std::wstring result(rw + 1, wchar_t(0));
  MultiByteToWideChar(CP_UTF8, 0, data(), size(), result.data(), rw);
  return result;
}

String::String(const wchar_t *wbuf)
{
  if (!wbuf) {
    iImp = emptyString();
  } else {
    int rm = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, nullptr, 0, nullptr, nullptr);
    iImp = new Imp;
    iImp->iRefCount = 1;
    iImp->iSize = rm - 1; // rm includes the trailing zero
    iImp->iCapacity = (rm + 32) & ~15;
    iImp->iData = new char[iImp->iCapacity];
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, iImp->iData, rm, nullptr, nullptr);
  }
}
#endif

// --------------------------------------------------------------------

static double ipestrtod(const char *s, char ** fin)
{
#ifdef WIN32
  if (p_create_locale != nullptr && p_strtod_l != nullptr)
    return p_strtod_l(s, fin, ipeLocale);
  else
    return strtod(s, fin);
#else
  return strtod_l(s, fin, ipeLocale);
#endif
}

double Platform::toDouble(String s)
{
  return ipestrtod(s.z(), nullptr);
}

int Platform::toNumber(String s, int &iValue, double &dValue)
{
  char *fin = const_cast<char *>(s.z());
  iValue = std::strtol(s.z(), &fin, 10);
  while (*fin == ' ' || *fin == '\t')
    ++fin;
  if (*fin == '\0')
    return 1; // integer
  dValue = ipestrtod(s.z(), &fin);
  while (*fin == ' ' || *fin == '\t')
    ++fin;
  if (*fin == '\0')
    return 2; // double
  // error
  return 0;
}

void ipeAssertionFailed(const char *file, int line, const char *assertion)
{
  fprintf(stderr, "Assertion failed on line #%d (%s): '%s'\n",
	  line, file, assertion);
  abort();
}

// --------------------------------------------------------------------

String Platform::spiroVersion()
{
#ifdef SPIRO_CUBIC_TO_BEZIER
  return LibSpiroVersion();
#else
  return "unknown";
#endif
}

String Platform::gslVersion()
{
#ifdef IPE_GSL
  String s = GSL_VERSION;
  s += " / ";
  s += gsl_version;
  return s;
#else
  return "none";
#endif
}

// --------------------------------------------------------------------

