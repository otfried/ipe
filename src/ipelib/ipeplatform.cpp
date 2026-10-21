// --------------------------------------------------------------------
// Platform dependent methods
// --------------------------------------------------------------------
/*

	This file is part of the extensible drawing editor Ipe.
	Copyright (c) 1993-2026 Otfried Cheong

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

#include "ipeattributes.h"
#include "ipebase.h"

#ifdef WIN32
#define NTDDI_VERSION 0x06000000
// #include <direct.h>
#include <windows.h>
// must be before these
#include <gdiplus.h>
#include <shlobj.h>
#else
#include <dirent.h>
#include <sys/wait.h>
#endif
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <xlocale.h>
// sys/param.h triggers legacy mode for realpath, so that the
// last component of the path does not need to exist.
#include <sys/errno.h>
#include <sys/param.h>
#endif

#ifdef IPE_GSL
#include <gsl/gsl_errno.h>
#include <gsl/gsl_version.h>
#endif

// for version info only
#include <spiroentrypoints.h>

#include <cerrno>
#include <clocale>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <filesystem>

#ifdef IPEWASM
#include <emscripten.h>
#include <emscripten/val.h>
#endif

using namespace ipe;

namespace fs = std::filesystem;

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
int Platform::libVersion() { return IPELIB_VERSION; }

// --------------------------------------------------------------------

static bool initialized = false;
static bool showDebug = false;
static Platform::DebugHandler debugHandler = nullptr;

#ifdef WIN32
static ULONG_PTR gdiplusToken = 0;
_locale_t ipeLocale;
#else
locale_t ipeLocale;
#endif

#ifdef IPEWASM
inline bool usePreloader() { return getenv("IPEPRELOADER") != nullptr; }
inline bool useJSLatex() { return getenv("IPEJSLATEX") != nullptr; }
#endif

// --------------------------------------------------------------------

// the Windows drive on which Ipe is installed
static String ipeDrive;

// path names without the trailing separator
static String folders[FOLDER_NUM];

// --------------------------------------------------------------------

inline String sp(fs::path const & p) { return String(p.generic_u8string()); }

// overwrites value of path from environment variable 'envvar'
// and replaces leading "ipe:" by drive letter on windows
// return true if the environment variable existed
static bool getenv(const char * envvar, const wchar_t * wenvvar, String & path) {
    fs::path pp{path.s8()};
#ifdef WIN32
    const wchar_t * p = _wgetenv(wenvvar);
    if (p) pp = fs::path(p);
    std::wstring native = pp.native();
    if (native.starts_with(L"ipe:")) {
	native.replace(0, 4, fs::path(::ipeDrive.s8()).native());
	pp = fs::path(native);
    }
#else
    const char * p = getenv(envvar);
    if (p) pp = fs::path(reinterpret_cast<const char8_t *>(p));
#endif
    path = sp(pp);
    return (p != nullptr);
}

String Platform::folder(IpeFolder ft, const char * fname) {
    fs::path p{folders[int(ft)].s8()};
    if (fname) p = p / fs::path{reinterpret_cast<const char8_t *>(fname)};
    return sp(p);
}

static void readIpeConf(fs::path fname) {
#ifndef IPEWASM
    String conf = Platform::readFile(sp(fname));
    if (conf.empty()) return;
    ipeDebug("ipe.conf = %s", conf.z());
    int i = 0;
    while (i < conf.size()) {
	String line = conf.getLine(i);
#ifdef WIN32
	_wputenv(line.w().data());
#else
	putenv(strdup(line.z())); // a leak, but necessary
#endif
    }
#endif
}

static void setupFolders() {
    String home1{"/home/ipe"};
    getenv("HOME", L"HOME", home1);
    fs::path home{home1.s8()};

    // SYSTEM folders first

#ifndef IPEBUNDLE
    folders[FolderLua] = IPELUADIR;
    folders[FolderIcons] = IPEICONDIR;
    folders[FolderIpelets] = IPELETDIR;
    folders[FolderStyles] = IPESTYLEDIR;
    folders[FolderScripts] = IPESCRIPTDIR;
    folders[FolderDoc] = IPEDOCDIR;
#else
#ifdef WIN32
    wchar_t exename[OFS_MAXPATHNAME];
    GetModuleFileNameW(nullptr, exename, OFS_MAXPATHNAME);
    String exe(exename);
    if (exe.size() > 2 && exe[1] == ':') ipeDrive = exe.left(2);
#elif defined(__APPLE__)
    char path[PATH_MAX], rpath[PATH_MAX];
    uint32_t size = sizeof(path);
    String exe;
    if (_NSGetExecutablePath(path, &size) == 0 && realpath(path, rpath) != nullptr)
	exe = String(rpath);
    else
	ipeDebug("setupFolders: buffer too small; need size %u", size);
#else
    String exe{"/opt/ipe/bin/ipe"}; // fake, bundle at /opt/ipe
#endif
    fs::path bundle = fs::path(exe.s8()).parent_path().parent_path();
#ifdef __APPLE__
    folders[FolderLua] = sp(bundle / "Resources" / "lua");
    folders[FolderIcons] = sp(bundle / "Resources" / "icons");
    folders[FolderIpelets] = sp(bundle / "Resources" / "ipelets");
    folders[FolderStyles] = sp(bundle / "Resources" / "styles");
    folders[FolderScripts] = sp(bundle / "Resources" / "scripts");
    folders[FolderDoc] = sp(bundle / "SharedSupport" / "doc");
#else
    folders[FolderLua] = sp(bundle / "lua");
    folders[FolderIcons] = sp(bundle / "icons");
    folders[FolderIpelets] = sp(bundle / "ipelets");
    folders[FolderStyles] = sp(bundle / "styles");
    folders[FolderScripts] = sp(bundle / "scripts");
    folders[FolderDoc] = sp(bundle / "doc");
#endif
#endif

    // USER folders

#ifdef __APPLE__
    fs::path local = fs::path(home.s8()) / "Library" / "Ipe";
    folders[FolderConfig] = sp(local);
    folders[FolderUserStyles] = sp(local / "styles");
    folders[FolderUserIpelets] = sp(local / "ipelets");
    folders[FolderUserScripts] = sp(local / "scripts");
    folders[FolderLatex] = sp(local / "cache");
#elif defined(WIN32)
    // only set Config, Latex, and maybe UserIpelets
    folders[FolderConfig] = sp(bundle);
    if (const wchar_t * p = _wgetenv(L"USERPROFILE"); p)
	folders[FolderUserIpelets] = sp(fs::path{p} / "Ipelets");
    wchar_t * path;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) {
	folders[FolderLatex] = sp(fs::path{path} / "ipe");
	CoTaskMemFree(path);
    } else {
	const wchar_t * p = _wgetenv(L"LOCALAPPDATA");
	if (p)
	    folders[FolderLatex] = sp(fs::path{p} / "ipe");
	else
	    folders[FolderLatex] = sp(bundle / "latexrun");
    }
#else
    // Linux: XDG base directory specification
    // https://specifications.freedesktop.org/basedir-spec/latest/
    String dataHome1 = sp(home / ".local" / "share");
    String configHome1 = sp(home / ".config");
    String cacheHome1 = sp(home / ".cache");
    getenv("XDG_DATA_HOME", nullptr, dataHome1);
    getenv("XDG_CONFIG_HOME", nullptr, configHome1);
    getenv("XDG_CACHE_HOME", nullptr, cacheHome1);
    fs::path dataHome = fs::path(dataHome1.s8()) / "ipe";
    fs::path configHome = fs::path(configHome1.s8()) / "ipe";
    fs::path cacheHome = fs::path(cacheHome1.s8()) / "ipe";
    folders[FolderConfig] = sp(configHome);
    folders[FolderUserStyles] = sp(dataHome / "styles");
    folders[FolderUserIpelets] = sp(dataHome / "ipelets");
    folders[FolderUserScripts] = sp(dataHome / "scripts");
    folders[FolderLatex] = sp(cacheHome);
#endif
    // read ipe.conf before interpreting environment variables
    readIpeConf(fs::path(folders[FolderConfig].s8()) / "ipe.conf");

    getenv("IPEDOCDIR", L"IPEDOCDIR", folders[FolderDoc]);
    getenv("IPEICONDIR", L"IPEICONDIR", folders[FolderIcons]);
    getenv("IPELATEXDIR", L"IPELATEXDIR", folders[FolderLatex]);

    ipeDebug("Configured folders:");
    ipeDebug("Lua: %s", folders[FolderLua].z());
    ipeDebug("Icons: %s", folders[FolderIcons].z());
    ipeDebug("Ipelets: %s", folders[FolderIpelets].z());
    ipeDebug("Styles: %s", folders[FolderStyles].z());
    ipeDebug("Scripts: %s", folders[FolderScripts].z());
    ipeDebug("Doc: %s", folders[FolderDoc].z());
    ipeDebug("Config: %s", folders[FolderConfig].z());
    ipeDebug("UserIpelets: %s", folders[FolderUserIpelets].z());
    ipeDebug("UserStyles: %s", folders[FolderUserStyles].z());
    ipeDebug("UserScripts: %s", folders[FolderUserScripts].z());
    ipeDebug("Latex: %s", folders[FolderLatex].z());
}

// --------------------------------------------------------------------

static void debugHandlerImpl(const char * msg) {
    if (showDebug) {
	fprintf(stderr, "%s\n", msg);
#ifdef WIN32
	fflush(stderr);
	OutputDebugStringA(msg);
#endif
    }
}

static void shutdownIpelib() {
#ifdef WIN32
    Gdiplus::GdiplusShutdown(gdiplusToken);
    _free_locale(ipeLocale);
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
void Platform::initLib(int version) {
    if (initialized) return;
    initialized = true;
    showDebug = getenv("IPEDEBUG") != nullptr;
    if (showDebug) fprintf(stderr, "Debug messages enabled\n");
    debugHandler = debugHandlerImpl;
    setupFolders();
#ifdef WIN32
    ipeLocale = _create_locale(LC_NUMERIC, "C");
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    // registers the RICHEDIT50W window class, used for spell-checked text edits
    LoadLibraryA("Msftedit.dll");
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
    if (version == IPELIB_VERSION) return;
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
void Platform::setDebug(bool debug) { showDebug = debug; }

// --------------------------------------------------------------------

void ipeDebug(const char * msg, ...) noexcept {
    if (debugHandler) {
	char buf[8196];
	va_list ap;
	va_start(ap, msg);
	std::vsprintf(buf, msg, ap);
	va_end(ap);
	debugHandler(buf);
    }
}

void ipeDebugBuffer(Buffer data, int maxsize) {
    if (!showDebug) return;
    int size = std::min(data.size(), maxsize);
    String s;
    StringStream ss(s);
    for (int i = 0; i < size; ++i) {
	ss.putHexByte(data[i]);
	ss << " ";
    }
    debugHandler(s.z());
    ipeDebug("Checksum: %x", data.checksum());
}

// --------------------------------------------------------------------

//! Returns drive on which Ipe executable exists.
/*! On Linux and OSX, returns empty string. */
String Platform::ipeDrive() { return ::ipeDrive; }

//! Return path for the directory containing pdflatex and xelatex.
/*! If empty means look on PATH. */
String Platform::latexPath() {
    String result;
#ifdef WIN32
    const wchar_t * p = _wgetenv(L"IPELATEXPATH");
    if (p) {
	std::wstring native = fs::path(p).native();
	if (native.starts_with(L"ipe:")) {
	    native.replace(0, 4, fs::path(::ipeDrive.s8()).native());
	}
	result = sp(fs::path(native));
    }
#else
    const char * p = getenv("IPELATEXPATH");
    if (p) result = sp(fs::path(reinterpret_cast<const char8_t *>(p)));
#endif
    return result;
}

//! Determine whether file exists.
bool Platform::fileExists(String fname) {
    std::error_code error;

#if defined(IPEWASM)
    if (!usePreloader() || fname.hasPrefix("/tmp") || fname.hasPrefix("/opt/ipe"))
	return fs::exists(fs::path(fname.s8()), error);
    emscripten::val fileExistsCache =
	emscripten::val::global("window")["ipeui"]["fileExistsCache"];
    return !fileExistsCache[fname.z()].isUndefined();
#else
    return fs::exists(fs::path(fname.s8()), error);
#endif
}

//! Read entire file into string.
/*! Returns an empty string if file cannot be found or read.
  There is no way to distinguish an empty file from this. */
String Platform::readFile(String fname) {
    std::FILE * file = Platform::fopen(fname.z(), "rb");
    if (!file) return String();
    String s;
    int ch;
    while ((ch = std::fgetc(file)) != EOF) s.append(ch);
    std::fclose(file);
    return s;
}

// amazingly, this actually works in NodeJS as is.
// to make this async, need to run it on a different thread
//! Returns command to run latex on file ipetemp.tex in given directory.
/*! directory of docname is added to TEXINPUTS if its non-empty. */
String Platform::howToRunLatex(LatexType engine, String docname) noexcept {
    fs::path dir(folders[FolderLatex].s8());
    const char * latex = (engine == LatexType::Xetex)    ? "xelatex"
			 : (engine == LatexType::Luatex) ? "lualatex"
							 : "pdflatex";
#ifdef IPEWASM
    if (useJSLatex()) {
	String how("runlatex:");
	how += latex;
	return how;
    }
#endif
    String url = Platform::readFile(sp(dir / "url1.txt"));
    bool online = (url.left(4) == "http");
    String texinputs;
    if (!online && !docname.empty()) {
	fs::path doc1 = fs::absolute(fs::path(docname.s8()));
	texinputs = sp(doc1.parent_path());
    }
#ifdef WIN32
    if (!online && getenv("IPETEXFORMAT")) {
	latex = (engine == LatexType::Xetex)    ? "xetex ^&latex"
		: (engine == LatexType::Luatex) ? "luatex ^&latex"
						: "pdftex ^&pdflatex";
    }

    dir.make_preferred();

    String bat;
    // try to change codepage to UTF-8
    bat += "chcp 65001\r\n";
    if (folders[FolderLatex].size() > 2 && folders[FolderLatex][1] == ':') {
	bat += folders[FolderLatex].substr(0, 2);
	bat += "\r\n";
    }
    bat += "cd \"";
    bat += dir.u8string();
    bat += "\"\r\n";
    if (!texinputs.empty()) {
	bat += "setlocal\r\n";
	bat += "set TEXINPUTS=.;";
	bat += texinputs;
	bat += ";%TEXINPUTS%\r\n";
    }
    if (online) {
	bat += "\"";
	fs::path curl = fs::path(folders[FolderConfig].s8()) / "bin" / "ipecurl.exe";
	curl.make_preferred(); // use Windows style path separators
	bat += curl.u8string();
	bat += "\" ";
	bat += latex;
	bat += "\r\n";
    } else {
	String path = latexPath();
	if (!path.empty()) {
	    fs::path p(path.s8());
	    p.make_preferred();
	    bat += "PATH ";
	    bat += p.u8string();
	    bat += ";%PATH%\r\n";
	}
	bat += latex;
	bat += " ipetemp.tex\r\n";
    }
    if (!texinputs.empty()) bat += "endlocal\r\n";
    // bat += "pause\r\n";  // alternative for Wine

    String s = sp(dir / "runlatex.bat");
    std::FILE * f = Platform::fopen(s.z(), "wb");
    if (!f) return String();
    std::fwrite(bat.data(), 1, bat.size(), f);
    std::fclose(f);

    dir = dir / "runlatex.bat";
    return String("cmd /c call \"") + dir.u8string() + String("\"");
#else
    if (!online && getenv("IPETEXFORMAT")) {
	latex = (engine == LatexType::Xetex)    ? "xetex \\&latex"
		: (engine == LatexType::Luatex) ? "luatex \\&latex"
						: "pdftex \\&pdflatex";
    }
    String s("cd \"");
    s += sp(dir);
    s += "\"; rm -f ipetemp.log; ";
    if (!texinputs.empty()) {
	s += "export TEXINPUTS=\"";
	s += texinputs;
	s += ":$TEXINPUTS\"; ";
    }
    if (online) {
#if defined(__APPLE__) && defined(IPEBUNDLE)
	s += "\"";
	s += sp(fs::path(folders[FolderLua].s8()).parent_path().parent_path() / "MacOS"
		/ "ipecurl");
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
	    s += String("\"") + sp(fs::path{path.s8()} / latex) + "\"";
	s += " ipetemp.tex";
    }
    s += " > /dev/null";
    return s;
#endif
}

#ifdef WIN32
int Platform::system(String cmd) {
    // Declare and initialize process blocks
    PROCESS_INFORMATION processInformation;
    STARTUPINFOW startupInfo;

    memset(&processInformation, 0, sizeof(processInformation));
    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    // Call the executable program
    std::wstring wcmd = cmd.w();

    int result = CreateProcessW(nullptr, wcmd.data(), nullptr, nullptr, FALSE,
				NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, nullptr,
				nullptr, &startupInfo, &processInformation);
    if (result == 0) return -1; // failure to create process

    // Wait until child process exits.
    WaitForSingleObject(processInformation.hProcess, INFINITE);

    // Close process and thread handles.
    CloseHandle(processInformation.hProcess);
    CloseHandle(processInformation.hThread);

    // Apparently WaitForSingleObject doesn't work in Wine
    const char * wine = getenv("IPEWINE");
    if (wine) Sleep(Lex(wine).getInt());
    return 0;
}
#else
int Platform::system(String cmd) {
    int result = std::system(cmd.z());
    if (result != -1) result = WEXITSTATUS(result);
    return result;
}
#endif

#ifdef WIN32
//! Return a wide string including a terminating zero character.
std::wstring String::w() const noexcept {
    if (empty()) return L"\0";
    int rw = MultiByteToWideChar(CP_UTF8, 0, data(), size(), nullptr, 0);
    std::wstring result(rw + 1, wchar_t(0));
    MultiByteToWideChar(CP_UTF8, 0, data(), size(), result.data(), rw);
    return result;
}

String::String(const wchar_t * wbuf) {
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

#ifdef IPEWASM
FILE * Platform::fopen(const char * fname, const char * mode) {
    if (!usePreloader() || !strncmp(fname, "/tmp", 4) || !strncmp(fname, "/opt/ipe", 8))
	return ::fopen(fname, mode);
    emscripten::val preloadCache =
	emscripten::val::global("window")["ipeui"]["preloadCache"];
    emscripten::val s = preloadCache[fname];
    if (!s.isUndefined()) {
	std::string t = s.as<std::string>();
	return ::fopen(t.c_str(), mode);
    } else if (mode[0] == 'w') {
	char tmpname[] = "/tmp/ipeXXXXXX";
	int fd = ::mkstemp(tmpname);
	if (fd < 0) return nullptr;
	preloadCache.set(fname, tmpname);
	return ::fdopen(fd, mode);
    } else
	return nullptr;
}
#else
FILE * Platform::fopen(const char * fname, const char * mode) {
    fs::path path{reinterpret_cast<const char8_t *>(fname)};
#ifdef _WIN32
    std::wstring wideMode(mode, mode + std::strlen(mode));
    return ::_wfopen(path.c_str(), wideMode.c_str());
#else
    return std::fopen(path.c_str(), mode);
#endif
}
#endif

// package Latex source as a tarball to send to online Latex conversion
String Platform::createTarball(String tex) {
    Buffer tarHeader(512);
    // volatile is necessary to appease compiler in strcpy etc.
    char * volatile p = tarHeader.data();
    memset(p, 0, 512);
    strcpy(p, "ipetemp.tex");
    strcpy(p + 100, "0000644"); // mode
    strcpy(p + 108, "0001750"); // uid 1000
    strcpy(p + 116, "0001750"); // gid 1000
    sprintf(p + 124, "%011o", (unsigned int)tex.size());
    p[136] = '0'; // time stamp, fudge it
    p[156] = '0'; // normal file
    // checksum
    strcpy(p + 148, "        ");
    uint32_t checksum = 0;
    for (const char * q = p; q < p + 512; ++q) checksum += uint8_t(*q);
    sprintf(p + 148, "%06o", checksum);
    p[155] = ' ';

    String tar;
    StringStream ss(tar);
    for (const char * q = p; q < p + 512;) ss.putChar(*q++);
    ss << tex;
    int i = tex.size();
    while ((i & 0x1ff) != 0) { // fill a 512-byte block
	ss.putChar('\0');
	++i;
    }
    for (int i = 0; i < 1024; ++i) // add two empty blocks
	ss.putChar('\0');
    return tar;
}

// --------------------------------------------------------------------

static double ipestrtod(const char * s, char ** fin) {
#ifdef WIN32
    return _strtod_l(s, fin, ipeLocale);
#else
    return strtod_l(s, fin, ipeLocale);
#endif
}

double Platform::toDouble(String s) { return ipestrtod(s.z(), nullptr); }

int Platform::toNumber(String s, int & iValue, double & dValue) {
    char * fin = const_cast<char *>(s.z());
    iValue = std::strtol(s.z(), &fin, 10);
    while (*fin == ' ' || *fin == '\t') ++fin;
    if (*fin == '\0') return 1; // integer
    dValue = ipestrtod(s.z(), &fin);
    while (*fin == ' ' || *fin == '\t') ++fin;
    if (*fin == '\0') return 2; // double
    // error
    return 0;
}

void ipeAssertionFailed(const char * file, int line, const char * assertion) {
    fprintf(stderr, "Assertion failed on line #%d (%s): '%s'\n", line, file, assertion);
    abort();
}

// --------------------------------------------------------------------

String Platform::spiroVersion() {
#ifdef SPIRO_CUBIC_TO_BEZIER
    return LibSpiroVersion();
#else
    return "unknown";
#endif
}

String Platform::gslVersion() {
#ifdef IPE_GSL
    String s = GSL_VERSION;
    s += " / ";
    s += gsl_version;
    return s;
#else
    return "none";
#endif
}

String Platform::imageFolder(String fname) {
    fs::path path(fname.s8());
    if (path.extension() == ".ipe" || path.extension() == ".pdf")
	path.replace_extension();
    path += ".images";
    path /= "";
    return sp(path);
}

// --------------------------------------------------------------------

#ifdef IPEWASM

EMSCRIPTEN_KEEPALIVE
extern "C" void initLib(emscripten::EM_VAL environment) {
    emscripten::val env = emscripten::val::take_ownership(environment);
    int n = env["length"].as<int>();
    for (int i = 0; i < n; ++i) {
	std::string e = env[i].as<std::string>();
	putenv(strdup(e.c_str()));
    }
    Platform::initLib(IPELIB_VERSION);
}

#endif

// ------------------------------------------------------------------------
