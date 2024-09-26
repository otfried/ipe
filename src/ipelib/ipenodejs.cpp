// --------------------------------------------------------------------
// When running under NodeJS
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

#ifdef IPENODEJS

#include "ipebase.h"

#include <emscripten.h>
#include <cstdlib>

using namespace ipe;

EM_JS(int, getArgc, (), {
    const { argv } =  require('node:process');
    return argv.length - 1;
});

EM_JS(char*, getArgv, (int i), {
    const { argv } =  require('node:process');
    return stringToNewUTF8(argv[i+1]);
});

EM_JS(char*, getEnv, (const char * s), {
    const { env } =  require('node:process');
    const value = env[UTF8ToString(s)];
    if (value)
      return stringToNewUTF8(value);
    else
      return null;
});

std::pair<int, std::vector<const char *>> Platform::setupNodeJs()
{
  // copy useful environment variables
  const auto copyEnv = [](const char *k) {
    const char *s = getEnv(k);
    if (s) {
      String line(k);
      line += "=";
      line += s;
      putenv(strdup(line.z()));  // a leak, but necessary
      std::free((void *) s);
    }
  };
  copyEnv("HOME");
  copyEnv("IPEDEBUG");
  copyEnv("IPELATEXDIR");
  copyEnv("IPELATEXPATH");
  copyEnv("IPETEXFORMAT");
  copyEnv("IPELETPATH");
  copyEnv("IPESTYLES");
  copyEnv("EDITOR");
  copyEnv("IPESCRIPTS");
  // and get the command line arguments
  const int argc = getArgc();
  std::vector<const char *> argv;
  for (int i = 0; i < argc; ++i)
    argv.push_back(getArgv(i));
  return std::pair{argc, argv};
}

// --------------------------------------------------------------------
#endif
