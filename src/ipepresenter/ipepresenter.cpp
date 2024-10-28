// --------------------------------------------------------------------
// IpePresenter common base
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

#include "ipepresenter.h"

using namespace ipe;

// --------------------------------------------------------------------

bool Presenter::load(const char * fname) {
    std::FILE * pdfFile = Platform::fopen(fname, "rb");
    if (!pdfFile) return false;

    FileSource source(pdfFile);
    std::unique_ptr<PdfFile> pdf = std::make_unique<PdfFile>();
    bool okay = pdf->parse(source);
    std::fclose(pdfFile);

    if (!okay) return false;

    iPdf = std::move(pdf);

    iFileName = fname;
    iPdfPageNo = 0;

    collectAnnotations();
    makePageLabels();
    collectDestinations();

    iResources = std::make_unique<PdfFileResources>(iPdf.get());
    iFonts = std::make_unique<Fonts>(iResources.get());
    iType3WarningShown = false;

    return true;
}

// read annotations from PDF
void Presenter::collectAnnotations() {
    iAnnotations.clear();
    iLinks.clear();
    for (int i = 0; i < iPdf->countPages(); ++i) {
	String notes;
	std::vector<SLink> links;
	const PdfDict * page = iPdf->page(i);
	const PdfArray * annots = page->getArray("Annots", iPdf.get());
	if (annots) {
	    for (int j = 0; j < annots->count(); ++j) {
		const PdfObj * a = annots->obj(j, iPdf.get());
		if (a && a->dict()) {
		    const PdfDict * d = a->dict();
		    const PdfObj * type = d->get("Type", iPdf.get());
		    const PdfObj * subtype = d->get("Subtype", iPdf.get());
		    const PdfObj * contents = d->get("Contents", iPdf.get());
		    if (type && type->name() && type->name()->value() == "Annot") {
			if (subtype && subtype->name()
			    && subtype->name()->value() == "Text" && contents
			    && contents->string()) {
			    if (!notes.empty()) notes += "\n";
			    notes += contents->string()->decode();
			} else if (subtype && subtype->name()
				   && subtype->name()->value() == "Link") {
			    std::vector<double> drect;
			    Rect rect;
			    if (d->getNumberArray("Rect", iPdf.get(), drect)
				&& drect.size() == 4) {
				rect.addPoint(Vector(drect[0], drect[1]));
				rect.addPoint(Vector(drect[2], drect[3]));
			    }
			    const PdfDict * action = d->getDict("A", iPdf.get());
			    if (action) links.push_back({rect, action});
			}
		    }
		}
	    }
	}
	iAnnotations.push_back(notes);
	iLinks.emplace_back(links);
    }
}

// read destinations from PDF
void Presenter::collectDestinations() {
    iDestinations.clear();
    const PdfDict * d1 = iPdf->catalog()->getDict("Names", iPdf.get());
    if (!d1) return;
    const PdfDict * d2 = d1->getDict("Dests", iPdf.get());
    if (!d2) return;
    collectDestinations(d2);
}

void Presenter::collectDestinations(const PdfDict * d) {
    const PdfArray * kids = d->getArray("Kids", iPdf.get());
    if (kids) { // recurse
	for (int i = 0; i < kids->count(); ++i) {
	    const PdfObj * kid = kids->obj(i, iPdf.get());
	    if (kid->dict()) collectDestinations(kid->dict());
	}
    } else {
	const PdfArray * names = d->getArray("Names", iPdf.get());
	for (int i = 0; i < names->count(); i += 2) {
	    const PdfObj * key = names->obj(i, iPdf.get());
	    const PdfObj * value = names->obj(i + 1, iPdf.get());
	    // ipeDebug("%s: %s", key->repr().z(), value->repr().z());
	    if (key->string()) {
		String dest = key->string()->value();
		if (value->dict()) value = value->dict()->get("D", iPdf.get());
		if (value->array()) {
		    const PdfObj * target = value->array()->obj(0, nullptr);
		    if (target->ref())
			iDestinations.push_back({dest, target->ref()->value()});
		}
	    }
	}
    }
}

// create the page labels
void Presenter::makePageLabels() {
    iPageLabels.clear();
    const PdfDict * d1 = iPdf->catalog()->getDict("PageLabels", iPdf.get());
    if (d1) {
	collectPageLabels(d1);
    } else {
	for (int pno = 0; pno < iPdf->countPages(); ++pno) {
	    char buf[16];
	    sprintf(buf, "%d", pno + 1);
	    iPageLabels.push_back(std::make_pair(String(buf), -1));
	}
    }
}

// this is not a complete implementation,
// just meant to work for beamer output and Ipe
void Presenter::collectPageLabels(const PdfDict * d) {
    const PdfArray * nums = d->getArray("Nums", iPdf.get());
    if (!nums) return;
    int prevNum = 0;
    String prevLabel;
    for (int j = 0; j < nums->count() - 1; j += 2) {
	const PdfObj * num = nums->obj(j, iPdf.get());
	const PdfObj * label = nums->obj(j + 1, iPdf.get());
	if (num->number() && label->dict()) {
	    int newNum = int(num->number()->value());
	    const PdfObj * p = label->dict()->get("P", iPdf.get());
	    String newLabel;
	    if (p && p->string()) newLabel = p->string()->decode();
	    bool moreThanOne = (newNum - prevNum) > 1;
	    while (size(iPageLabels) < newNum)
		iPageLabels.push_back(std::make_pair(
		    prevLabel, moreThanOne ? iPageLabels.size() - prevNum : -1));
	    prevNum = newNum;
	    prevLabel = newLabel;
	}
    }
    bool moreThanOne = (iPdf->countPages() - iPageLabels.size()) > 1;
    while (size(iPageLabels) < iPdf->countPages())
	iPageLabels.push_back(
	    std::make_pair(prevLabel, moreThanOne ? iPageLabels.size() - prevNum : -1));
}

// --------------------------------------------------------------------

static const char * const type3Warning =
    "It appears your document uses a Type3 font.\n\n"
    "These are bitmapped fonts, typically created by Latex from a Metafont source.\n\n"
    "Ipe cannot display these fonts (you'll see a box instead).\n\n"
    "A modern Latex installation should not normally use Type3 fonts. You could "
    "try to install the 'cm-super' package to avoid using Type3 fonts.";

void Presenter::setViewPage(PdfViewBase * view, int pdfpno) {
    view->setPage(iPdf->page(pdfpno), mediaBox(pdfpno));
    view->updatePdf();
    if (!iType3WarningShown && iFonts->hasType3Font()) {
	showType3Warning(type3Warning);
	iType3WarningShown = true;
    }
}

void Presenter::fitBox(const Rect & box, PdfViewBase * view) {
    if (box.isEmpty()) return;
    double xfactor = box.width() > 0.0 ? (view->viewWidth() / box.width()) : 20.0;
    double yfactor = box.height() > 0.0 ? (view->viewHeight() / box.height()) : 20.0;
    double zoom = (xfactor > yfactor) ? yfactor : xfactor;
    view->setPan(0.5 * (box.bottomLeft() + box.topRight()));
    view->setZoom(zoom);
    view->updatePdf();
}

// --------------------------------------------------------------------

String Presenter::pageLabel(int pdfno) {
    auto & pl = iPageLabels[pdfno];
    String s;
    ipe::StringStream ss(s);
    ss << pl.first;
    if (pl.second >= 0) {
	if (pl.first.right(1) != "-") ss << "-";
	ss << pl.second + 1;
    }
    return s;
}

String Presenter::currentLabel() {
    String s = iFileName;
    if (iFileName.rfind('/') >= 0) s = iFileName.substr(iFileName.rfind('/') + 1);
    ipe::StringStream ss(s);
    ss << " : " << pageLabel(iPdfPageNo) << " / " << iPageLabels.back().first << " ("
       << iPdfPageNo + 1 << " / " << iPdf->countPages() << ")";
    return s;
}

Rect Presenter::mediaBox(int pdfpno) const {
    if (pdfpno == -1)
	pdfpno = iPdfPageNo;
    else if (pdfpno == -2)
	pdfpno = (iPdfPageNo < iPdf->countPages() - 1) ? iPdfPageNo + 1 : iPdfPageNo;
    return iPdf->mediaBox(iPdf->page(pdfpno));
}

const PdfDict * Presenter::findLink(const Vector & pos) const {
    for (const auto & link : iLinks[iPdfPageNo]) {
	if (link.rect.contains(pos)) return link.action;
    }
    return nullptr;
}

void Presenter::interpretAction(const PdfDict * action) {
    String type = action->getName("S", iPdf.get());
    if (type == "URI") {
	const PdfObj * uriObj = action->get("URI", iPdf.get());
	if (uriObj && uriObj->string()) browseLaunch(false, uriObj->string()->value());
    } else if (type == "Launch") {
	const PdfObj * fObj = action->get("F", iPdf.get());
	if (fObj && fObj->string()) browseLaunch(true, fObj->string()->value());
    } else if (type == "GoTo") {
	gotoDestination(action->get("D", iPdf.get()));
    } else if (type == "Named") {
	String op = action->getName("N", iPdf.get());
	if (op == "NextPage")
	    nextView(1);
	else if (op == "PrevPage")
	    nextView(-1);
	else if (op == "FirstPage")
	    firstView();
	else if (op == "LastPage")
	    lastView();
	else
	    ipeDebug("Named action /%s", op.z());
    }
}

void Presenter::gotoDestination(const PdfObj * dest) {
    if (!dest) return;
    if (dest->string()) {
	String sdest = dest->string()->value();
	for (const auto & d : iDestinations) {
	    if (d.first == sdest) {
		int p = iPdf->findPageFromPageObjectNumber(d.second);
		if (p >= 0) {
		    iPdfPageNo = p;
		    return;
		}
	    }
	}
    } else if (dest->array() && dest->array()->count() > 0) {
	const PdfObj * d1 = dest->array()->obj(0, nullptr); // do not follow reference
	if (!d1->ref()) return;
	int p = iPdf->findPageFromPageObjectNumber(d1->ref()->value());
	if (p >= 0) {
	    iPdfPageNo = p;
	    return;
	}
    }
    ipeDebug("GoTo with unknown destination %s", dest->repr().z());
}

// --------------------------------------------------------------------

void Presenter::jumpToPage(String page) {
    if (page.empty()) return;
    for (int i = 0; i < size(iPageLabels); ++i) {
	auto pl = pageLabel(i);
	if (page == pl || (page + "-1" == pl) || page == (pl + "-1")) {
	    iPdfPageNo = i;
	    return;
	}
    }
}

void Presenter::nextView(int delta) {
    int npno = iPdfPageNo + delta;
    if (0 <= npno && npno < iPdf->countPages()) { iPdfPageNo = npno; }
}

void Presenter::nextPage(int delta) {
    String now = iPageLabels[iPdfPageNo].first;
    while (iPageLabels[iPdfPageNo].first == now && 0 <= iPdfPageNo + delta
	   && iPdfPageNo + delta < iPdf->countPages())
	iPdfPageNo += delta;
    if (delta < 0) {
	// go back to first view of the same page
	String cur = iPageLabels[iPdfPageNo].first;
	while (0 < iPdfPageNo && iPageLabels[iPdfPageNo - 1].first == cur)
	    iPdfPageNo += delta;
    }
}

void Presenter::firstView() { iPdfPageNo = 0; }

void Presenter::lastView() { iPdfPageNo = iPdf->countPages() - 1; }

// --------------------------------------------------------------------
