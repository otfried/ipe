// --------------------------------------------------------------------
// ipejs - Javascript bindings to ipelib
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

#include "ipedoc.h"

#include <emscripten/bind.h>

using namespace emscripten;

namespace {
  void initLib() {
    putenv(strdup("IPEDEBUG=1"));
    ipe::Platform::initLib(ipe::IPELIB_VERSION);
  }
  ipe::Document *loadWithErrorReport(std::string s) {
    return ipe::Document::loadWithErrorReport(s.c_str());
  }
  ipe::Page *getPage(ipe::Document *doc, int pno) {
    return doc->page(pno);
  }
  val getBytes(ipe::Buffer & buffer) {
    return val(typed_memory_view(buffer.size(), (uint8_t *) buffer.data()));
  }
};

EMSCRIPTEN_BINDINGS(ipe) {
  class_<ipe::Platform>("Platform")
    .class_function("initLib", &initLib);

  value_object<ipe::Vector>("Vector")
    .field("x", &ipe::Vector::x)
    .field("y", &ipe::Vector::y);

  class_<ipe::Buffer>("Buffer")
    .property("size", &ipe::Buffer::size)
    .function("data", &getBytes);

  class_<ipe::Page>("Page")
    .property("count", &ipe::Page::count)
    .property("countLayers", &ipe::Page::countLayers)
    .property("countViews", &ipe::Page::countViews);

  class_<ipe::Cascade>("Cascade")
    .property("count", &ipe::Cascade::count);

  class_<ipe::Document>("Document")
    .constructor<>()
    .property("countPages", &ipe::Document::countPages)
    .function("page", &getPage, allow_raw_pointers())
    .function("cascade", select_overload<ipe::Cascade *()>(&ipe::Document::cascade),
	      allow_raw_pointers());
  function("loadWithErrorReport", &loadWithErrorReport, return_value_policy::take_ownership());

}

// --------------------------------------------------------------------
