// --------------------------------------------------------------------
// Interface with Pdflatex
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

#include "ipestyle.h"
#include "ipegroup.h"
#include "ipereference.h"

#include "ipelatex.h"

#include <cstdlib>

using namespace ipe;

/*! \class ipe::Latex
  \brief Object that converts latex source to PDF format.

  This object is responsible for creating the PDF representation of
  text objects.

*/

//! Create a converter object.
Latex::Latex(const Cascade *sheet, LatexType latexType, bool sequentialText)
{
  iCascade = sheet;
  iResources = new PdfResources;
  iLatexType = latexType;
  iXetex = (latexType == LatexType::Xetex);
  iSequentialText = sequentialText;
}

//! Destructor.
Latex::~Latex()
{
  for (auto &it : iXForms)
    delete it;
  delete iResources;
}

// --------------------------------------------------------------------

//! Return the newly created PdfResources and pass ownership to caller.
PdfResources *Latex::takeResources()
{
  PdfResources *r = iResources;
  iResources = nullptr;
  return r;
}

// --------------------------------------------------------------------

class ipe::TextCollectingVisitor : public Visitor {
public:
  TextCollectingVisitor(Latex::TextList *list);
  virtual void visitText(const Text *obj);
  virtual void visitGroup(const Group *obj);
  virtual void visitReference(const Reference *obj);
public:
  bool iTextFound;
private:
  Latex::TextList *iList;
};

TextCollectingVisitor::TextCollectingVisitor(Latex::TextList *list)
  : iList(list)
{
  // nothing
}

void TextCollectingVisitor::visitText(const Text *obj)
{
  Latex::SText s;
  s.iText = obj;
  s.iSize = obj->size();
  iList->push_back(s);
  iTextFound = true;
}

void TextCollectingVisitor::visitGroup(const Group *obj)
{
  for (Group::const_iterator it = obj->begin(); it != obj->end(); ++it)
    (*it)->accept(*this);
}

void TextCollectingVisitor::visitReference(const Reference *)
{
  // need to figure out what to do for symbols
}

// --------------------------------------------------------------------

/*! Scan an object and insert all text objects into Latex's list.
  Returns total number of text objects found so far. */
int Latex::scanObject(const Object *obj)
{
  TextCollectingVisitor visitor(&iTextObjects);
  obj->accept(visitor);
  return iTextObjects.size();
}

/*! Scan a page and insert all text objects into Latex's list.
  Returns total number of text objects found so far. */
int Latex::scanPage(Page *page)
{
  page->applyTitleStyle(iCascade);
  TextCollectingVisitor visitor(&iTextObjects);
  const Text *title = page->titleText();
  if (title)
    title->accept(visitor);
  for (int i = 0; i < page->count(); ++i) {
    visitor.iTextFound = false;
    page->object(i)->accept(visitor);
    if (visitor.iTextFound)
      page->invalidateBBox(i);
  }
  return iTextObjects.size();
}

//! Create Text object to represent the page number of this view.
void Latex::addPageNumber(int pno, int vno, int npages, int nviews)
{
  const StyleSheet::PageNumberStyle *pns = iCascade->findPageNumberStyle();
  AllAttributes attr;
  attr.iStroke = pns->iColor;
  attr.iTextSize = pns->iSize;
  attr.iHorizontalAlignment = pns->iHorizontalAlignment;
  attr.iVerticalAlignment = pns->iVerticalAlignment;
  char latex[256];
  sprintf(latex, "\\def\\ipeNumber#1#2{#%d}"
	  "\\setcounter{ipePage}{%d}\\setcounter{ipeView}{%d}"
	  "\\setcounter{ipePages}{%d}\\setcounter{ipeViews}{%d}",
	  (nviews > 1 ? 2 : 1), pno + 1, vno + 1, npages, nviews);
  String data = pns->iText.empty() ?
    "\\ipeNumber{\\arabic{ipePage}}{\\arabic{ipePage} - \\arabic{ipeView}}" :
    pns->iText;
  Text *t = new Text(attr, String(latex) + data, pns->iPos, Text::ELabel);
  SText s;
  s.iText = t;
  s.iSize = t->size();
  iTextObjects.push_back(s);
  PdfResources::SPageNumber pn;
  pn.page = pno;
  pn.view = vno;
  pn.text.reset(t);
  iResources->addPageNumber(pn);
}

/*! Create a Latex source file with all the text objects collected
  before.  The client should have prepared a directory for the
  Pdflatex run, and pass the name of the Latex source file to be
  written by Latex.

  Returns the number of text objects that did not yet have an XForm,
  or a negative error code.
*/
int Latex::createLatexSource(Stream &stream, String preamble)
{
  int count = 0;
  if (preamble.hasPrefix("%&")) {
    int i = preamble.find('\n');
    if (i < 0) {
      stream << preamble << "\n";
      preamble.erase();
    } else {
      stream << preamble.left(i+1);
      preamble = preamble.substr(i+1);
    }
  }
  stream << "\\nonstopmode\n";
  if (!iXetex) {
    // for parser testing: \\pdfobjcompresslevel2\\pdfminorversion5
    stream << "\\expandafter\\ifx\\csname pdfobjcompresslevel\\endcsname"
	   << "\\relax\\else\\pdfobjcompresslevel0\\fi\n";
    if (iLatexType == LatexType::Luatex)
      // load luatex85 for new versions of Luatex
      stream << "\\expandafter\\ifx\\csname pdfcolorstack\\endcsname\\relax"
	     << "\\RequirePackage{luatex85}\\fi\n";
  }
  stream << "\\documentclass{article}\n"
	 << "\\newdimen\\ipefs\n"
	 << "\\newcounter{ipePage}\\newcounter{ipeView}\n"
	 << "\\newcounter{ipePages}\\newcounter{ipeViews}\n"
	 << "\\newcommand{\\PageTitle}[1]{#1}\n"
	 << "\\newcommand{\\ipesymbol}[4]{$\\bullet$}\n";
  stream << "\\def\\ipedefinecolors#1{\\ipecolorpreamble{#1}\\let\\ipecolorpreamble\\relax}\n"
	 << "\\def\\ipecolorpreamble#1{\\usepackage[#1]{xcolor}\n";
  AttributeSeq colors;
  iCascade->allNames(EColor, colors);
  for (AttributeSeq::const_iterator it = colors.begin();
       it != colors.end(); ++it) {
    // only symbolic names (not black, white, void)
    String name = it->string();
    Color value = iCascade->find(EColor, *it).color();
    if (value.isGray())
      stream << "\\definecolor{" << name << "}{gray}{"
	     << value.iRed << "}\n";
    else
      stream << "\\definecolor{" << name << "}{rgb}{"
	     << value.iRed << "," << value.iGreen << ","
	     << value.iBlue << "}\n";
  }
  stream << "}\n";
  if (iXetex) {
    stream << "\\def\\ipesetcolor#1#2#3{\\special{pdf:bc [#1 #2 #3]}}\n"
	   << "\\def\\iperesetcolor{\\special{pdf:ec}}\n";
  } else {
    stream << "\\makeatletter\n"
	   << "\\def\\ipesetcolor#1#2#3{\\def\\current@color{#1 #2 #3 rg #1 #2 #3 RG}"
	   << "\\pdfcolorstack\\@pdfcolorstack push{\\current@color}}\n"
	   << "\\def\\iperesetcolor{\\pdfcolorstack\\@pdfcolorstack pop}\n"
	   << "\\makeatother\n";
  }
  stream << iCascade->findPreamble() << "\n"
	 << preamble << "\n"
	 << "\\ipedefinecolors{}\n"
	 << "\\pagestyle{empty}\n"
	 << "\\newcount\\bigpoint\\dimen0=0.01bp\\bigpoint=\\dimen0\n"
	 << "\\begin{document}\n"
	 << "\\begin{picture}(500,500)\n";

  if (iXetex)
    stream << "\\special{pdf:obj @ipeforms []}\n";

  // generate Latex source for each text object
  for (auto &it : iTextObjects) {
    StringStream source(it.iSource);

    const Text *text = it.iText;

    if (!text->getXForm())
      count++;

    Attribute fsAttr = iCascade->find(ETextSize, it.iSize);

    // compute x-stretch factor from textstretch
    it.iStretch = Fixed(1);
    if (it.iSize.isSymbolic())
      it.iStretch = iCascade->find(ETextStretch, it.iSize).number();
    if (text->isMinipage()) {
      source << "\\begin{minipage}{" <<
	text->width()/it.iStretch.toDouble() << "bp}";
    }

    if (fsAttr.isNumber()) {
      Fixed fs = fsAttr.number();
      source << "\\fontsize{" << fs << "}"
	     << "{" << fs.mult(6, 5) << "bp}\\selectfont\n";
    } else
      source << fsAttr.string() << "\n";
    Color col = iCascade->find(EColor, text->stroke()).color();
    source << "\\ipesetcolor{" << col.iRed.toDouble()
	   << "}{" << col.iGreen.toDouble()
	   << "}{" << col.iBlue.toDouble()
	   << "}%\n";

    Attribute absStyle =
      iCascade->find(text->isMinipage() ? ETextStyle : ELabelStyle,
		     text->style());
    String style = absStyle.string();
    int sp = 0;
    while (sp < style.size() && style[sp] != '\0')
      ++sp;
    source << style.substr(0, sp);

    String txt = text->text();
    source << txt;

    if (text->isMinipage()) {
      if (!txt.empty() && txt[txt.size() - 1] != '\n')
	source << "\n";
      source << style.substr(sp + 1);
      source << "\\end{minipage}";
    } else
      source << style.substr(sp + 1) << "%\n";
  }

  if (!iSequentialText)
    std::sort(iTextObjects.begin(), iTextObjects.end(),
	      [](const SText &a, const SText &b) {
		return a.iSource < b.iSource;
	      });

  for (int i = 0; i < size(iTextObjects); ++i) {
    auto & it = iTextObjects[i];
    if (!iSequentialText && i > 0 && it.iSource == iTextObjects[i-1].iSource)
      continue;
    stream << "\\setbox0=\\hbox{";
    stream << it.iSource;
    stream << "\\iperesetcolor}\n"
	   << "\\count0=\\dp0\\divide\\count0 by \\bigpoint\n";
    int curnum = i + 1;
    if (iXetex) {
      stream << "\\special{ pdf:bxobj @ipeform" << curnum << "\n"
	     << "width \\the\\wd0 \\space "
	     << "height \\the\\ht0 \\space "
	     << "depth \\the\\dp0}%\n"
	     << "\\usebox0%\n"
	     << "\\special{pdf:exobj}%\n"
	     << "\\special{pdf:obj @ipeinfo" << curnum << " <<"
	     << " /IpeId " << curnum
	     << " /IpeStretch " << it.iStretch.toDouble()
	     << " /IpeDepth \\the\\count0"
	     << " /IpeXForm @ipeform" << curnum << " >>}\n"
	     << "\\special{pdf:close @ipeinfo" << curnum << "}\n"
	     << "\\special{pdf:put @ipeforms @ipeinfo" << curnum << "}\n"
	     << "\\put(0,0){\\special{pdf:uxobj @ipeform" << curnum << "}}\n";
    } else {
      stream << "\\pdfxform attr{/IpeId " << curnum
	     << " /IpeStretch " << it.iStretch.toDouble()
	     << " /IpeDepth \\the\\count0}"
	     << "0\\put(0,0){\\pdfrefxform\\pdflastxform}\n";
    }
  }
  stream << "\\end{picture}\n";
  if (iXetex)
    stream << "\\special{pdf:close @ipeforms}\n"
	   << "\\special{pdf:put @resources << /Ipe @ipeforms >>}\n";
  stream << "\\end{document}\n";
  return count;
}

bool Latex::getXForm(String key, const PdfDict *ipeInfo)
{
  /*
     /Type /XObject
     /Subtype /Form
     /Id /abcd1234
     /Depth 246
     /Stretch [ 3 3 ]
     /BBox [0 0 4.639 4.289]
     /FormType 1
     /Matrix [1 0 0 1 0 0]
     /Resources 11 0 R
  */
  Text::XForm *xf = new Text::XForm;
  iXForms.push_back(xf);
  const PdfObj *xform = iXetex ? ipeInfo->get("IpeXForm") :
    iResources->findResource("XObject", key);
  int xformNum = -1;
  if (xform && xform->ref()) {
    xformNum = xform->ref()->value();
    xform = iResources->object(xformNum);
  }
  if (!xform || !xform->dict())
    return false;
  const PdfDict *xformd = xform->dict();
  if (iXetex) {
    // determine key
    const PdfDict *d = iResources->resourcesOfKind("XObject");
    for (int i = 0; i < d->count(); ++i) {
      const PdfObj *obj = d->value(i);
      if (obj->ref() && obj->ref()->value() == xformNum) {
	xf->iName = d->key(i);
	break;
      }
    }
    if (xf->iName.empty())
      return false;
  } else {
    xf->iName = key;
    ipeInfo = xformd;
  }
  // Get  id
  int ipeId = ipeInfo->getInteger("IpeId", &iPdf);
  int ipeDepth = ipeInfo->getInteger("IpeDepth", &iPdf);
  if (ipeId < 0 || ipeDepth < 0)
    return false;
  xf->iRefCount = ipeId;  // abusing refcount field
  xf->iDepth = ipeDepth;
  double val;
  if (!ipeInfo->getNumber("IpeStretch", val, &iPdf))
    return false;
  xf->iStretch = val;

  // Get BBox
  std::vector<double> a;
  if (!xformd->getNumberArray("BBox", &iPdf, a) || a.size() != 4)
    return false;
  xf->iBBox.addPoint(Vector(a[0], a[1]));
  xf->iBBox.addPoint(Vector(a[2], a[3]));

  if (!xformd->getNumberArray("Matrix", &iPdf, a) || a.size() != 6)
    return false;
  if (a[0] != 1.0 || a[1] != 0.0 || a[2] != 0.0 || a[3] != 1.0) {
    ipeDebug("PDF XObject has a non-trivial transformation");
    return false;
  }
  xf->iTranslation = Vector(-a[4], -a[5]) - xf->iBBox.bottomLeft();
  return true;
}

//! Read the PDF file created by Pdflatex.
/*! Must have performed the call to Pdflatex, and pass the name of the
  resulting output file.
*/
bool Latex::readPdf(DataSource &source)
{
  if (!iPdf.parse(source)) {
    warn("Ipe cannot parse the PDF file produced by Pdflatex.");
    return false;
  }

  const PdfDict *page1 = iPdf.page();

  const PdfObj *res = page1->get("Resources", &iPdf);
  if (!res || !res->dict())
    return false;

  if (!iResources->collect(res->dict(), &iPdf))
    return false;

  if (iXetex) {
    const PdfObj *obj = res->dict()->get("Ipe", &iPdf);
    if (!obj || !obj->array()) {
      warn("Page 1 has no /Ipe link.");
      return false;
    }
    for (int i = 0; i < obj->array()->count(); i++) {
      const PdfObj *info = obj->array()->obj(i, &iPdf);
      if (!info || !info->dict())
	return false;
      const PdfObj *ref = info->dict()->get("IpeXForm");
      if (!ref || !ref->ref())
	return false;
      iResources->setIpeXForm(ref->ref()->value());
      if (!getXForm(String(), info->dict()))
	return false;
    }
  } else {
    const PdfObj *obj = res->dict()->get("XObject", &iPdf);
    if (!obj || !obj->dict()) {
      warn("Page 1 has no XForms.");
      return false;
    }
    const PdfDict *xo = obj->dict();
    for (int i = 0; i < xo->count(); i++) {
      String key = xo->key(i);
      if (!xo->value(i)->ref())
	return false;
      iResources->setIpeXForm(xo->value(i)->ref()->value());
      if (!getXForm(key, nullptr))
	return false;
    }
  }
  // iResources->show();
  return true;
}

//! Notify all text objects about their updated PDF code.
/*! Returns true if successful. */
bool Latex::updateTextObjects()
{
  std::sort(iXForms.begin(), iXForms.end(),
	    [](const Text::XForm *a, const Text::XForm *b) {
	      return a->iRefCount < b->iRefCount;
	    });

  int curXForm = 0;
  Text::XForm *xf = nullptr;
  for (int i = 0; i < size(iTextObjects); ++i) {
    if (!iSequentialText && i > 0 && iTextObjects[i].iSource == iTextObjects[i-1].iSource) {
      if (xf == nullptr)
	return false;
      iTextObjects[i].iText->setXForm(xf);
    } else {
      if (i + 1 != iXForms[curXForm]->iRefCount)
	return false;
      xf = iXForms[curXForm];
      iXForms[curXForm] = nullptr;
      xf->iRefCount = 0;
      iTextObjects[i].iText->setXForm(xf);
      ++curXForm;
    }
  }
  return true;
}

/*! Messages about the (mis)behaviour of Pdflatex, probably
  incomprehensible to the user. */
void Latex::warn(String msg)
{
  ipeDebug(msg.z());
}

// --------------------------------------------------------------------
