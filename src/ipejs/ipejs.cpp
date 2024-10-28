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

using namespace ipe;
using namespace emscripten;

namespace {
Latex * converter = nullptr;

Document * loadWithErrorReport(std::string s) {
    return Document::loadWithErrorReport(s.c_str());
}

int prepareLatexRun(Document * doc) {
    converter = nullptr;
    return doc->prepareLatexRun(&converter);
}

int completeLatexRun(Document * doc) {
    String log;
    return doc->completeLatexRun(log, converter);
}

Page * getPage(Document * doc, int pno) { return doc->page(pno); }

val getBytes(Buffer & buffer) {
    return val(typed_memory_view(buffer.size(), (uint8_t *)buffer.data()));
}
}; // namespace

EMSCRIPTEN_BINDINGS(ipelib) {
    value_object<Vector>("Vector").field("x", &Vector::x).field("y", &Vector::y);

    class_<Buffer>("Buffer").property("size", &Buffer::size).function("data", &getBytes);

    class_<Page>("Page")
	.property("count", &Page::count)
	.property("countLayers", &Page::countLayers)
	.property("countViews", &Page::countViews);

    class_<Cascade>("Cascade").property("count", &Cascade::count);

    class_<Document>("Document")
	.constructor<>()
	.property("countPages", &Document::countPages)
	.function("prepareLatexRun", &prepareLatexRun, allow_raw_pointers())
	.function("completeLatexRun", &completeLatexRun, allow_raw_pointers())
	.function("page", &getPage, allow_raw_pointers())
	.function("cascade", select_overload<Cascade *()>(&Document::cascade),
		  allow_raw_pointers());
    function("loadWithErrorReport", &loadWithErrorReport,
	     return_value_policy::take_ownership());
}

// --------------------------------------------------------------------
