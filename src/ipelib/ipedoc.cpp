// --------------------------------------------------------------------
// The Ipe document.
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
#include "ipeiml.h"
#include "ipelatex.h"
#include "ipepainter.h"
#include "ipepdfparser.h"
#include "ipepdfwriter.h"
#include "ipereference.h"
#include "ipestyle.h"
#include "ipeutils.h"

#include <errno.h>

using namespace ipe;

// --------------------------------------------------------------------

/*! \defgroup doc Ipe Document
  \brief The classes managing an Ipe document.

  The main class, Document, represents an entire Ipe document, and
  allows you to load, save, access, and modify such a document.

  Other classes represent pages, layers, and views of a document.
  Another important class is the StyleSheet, which maps symbolic
  attributes to absolute values.
*/

/*! \class ipe::Document
  \ingroup doc
  \brief The model for an Ipe document.

  The Document class represents the contents of an Ipe document, and
  all the methods necessary to load, save, and modify it.
*/

//! Construct an empty document for filling by a client.
/*! As constructed, it has no pages, A4 media, and
  only the standard style sheet. */
Document::Document() {
    iResources = nullptr;
    iCascade = new Cascade();
    iCascade->insert(0, StyleSheet::standard());
}

//! Destructor.
Document::~Document() {
    for (int i = 0; i < countPages(); ++i) delete page(i);
    delete iCascade;
    delete iResources;
}

//! Copy constructor.
Document::Document(const Document & rhs) {
    iCascade = new Cascade(*rhs.iCascade);
    for (int i = 0; i < rhs.countPages(); ++i) iPages.push_back(new Page(*rhs.page(i)));
    iProperties = rhs.iProperties;
    iResources = nullptr;
}

// ---------------------------------------------------------------------

String readLine(DataSource & source) {
    String s;
    int ch = source.getChar();
    while (ch != EOF && ch != '\n') {
	s += char(ch);
	ch = source.getChar();
    }
    return s;
}

//! Determine format of file in \a source.
FileFormat Document::fileFormat(DataSource & source) {
    String s1 = readLine(source);
    String s2 = readLine(source);
    if (s1.substr(0, 5) == "<?xml" || s1.substr(0, 9) == "<!DOCTYPE"
	|| s1.substr(0, 4) == "<ipe")
	return FileFormat::Xml;
    if (s1.substr(0, 4) == "%PDF")
	return FileFormat::Pdf; // let's assume it contains an Ipe stream
    return FileFormat::Unknown;
}

//! Determine format of file from filename \a fn.
FileFormat Document::formatFromFilename(String fn) {
    if (fn.size() < 5) return FileFormat::Unknown;
    // fn = fn.toLower();
    String s = fn.right(4);
    if (s == ".xml" || s == ".ipe")
	return FileFormat::Xml;
    else if (s == ".pdf")
	return FileFormat::Pdf;
    else
	return FileFormat::Unknown;
}

// --------------------------------------------------------------------

class PdfStreamParser : public ImlParser {
public:
    explicit PdfStreamParser(PdfFile & loader, DataSource & source);
    virtual Buffer pdfStream(int objNum);

private:
    PdfFile & iLoader;
};

PdfStreamParser::PdfStreamParser(PdfFile & loader, DataSource & source)
    : ImlParser(source)
    , iLoader(loader) {
    // nothing
}

Buffer PdfStreamParser::pdfStream(int objNum) {
    const PdfObj * obj = iLoader.object(objNum);
    if (!obj || !obj->dict() || obj->dict()->stream().size() == 0) return Buffer();
    return obj->dict()->stream();
}

// --------------------------------------------------------------------

Document * doParse(Document * self, ImlParser & parser, int & reason) {
    int res = parser.parseDocument(*self);
    if (res) {
	delete self;
	self = nullptr;
	if (res == ImlParser::ESyntaxError)
	    reason = parser.parsePosition();
	else
	    reason = -res;
    }
    return self;
}

Document * doParseXml(DataSource & source, int & reason) {
    Document * self = new Document;
    ImlParser parser(source);
    return doParse(self, parser, reason);
}

Document * doParsePdf(DataSource & source, int & reason) {
    PdfFile loader;
    reason = Document::ENotAnIpeFile;
    if (!loader.parse(source)) // could not parse PDF container
	return nullptr;
    const PdfObj * obj = loader.catalog()->get("PieceInfo", &loader);
    if (obj && obj->dict()) {
	obj = obj->dict()->get("Ipe", &loader);
	if (obj && obj->dict()) obj = obj->dict()->get("Private", &loader);
    }
    if (!obj) obj = loader.object(1);
    // was the object really created by Ipe?
    if (!obj || !obj->dict()) return nullptr;
    const PdfObj * type = obj->dict()->get("Type");
    if (!type || !type->name() || type->name()->value() != "Ipe") return nullptr;

    Buffer buffer = obj->dict()->stream();
    BufferSource xml(buffer);

    Document * self = new Document;
    if (obj->dict()->deflated()) {
	InflateSource xml1(xml);
	PdfStreamParser parser(loader, xml1);
	return doParse(self, parser, reason);
    } else {
	PdfStreamParser parser(loader, xml);
	return doParse(self, parser, reason);
    }
}

//! Construct a document from an input stream.
/*! Returns 0 if the stream couldn't be parsed, and a reason
  explaining that in \a reason.  If \a reason is positive, it is a
  file (stream) offset where parsing failed.  If \a reason is
  negative, it is an error code, see Document::LoadErrors.
*/
Document * Document::load(DataSource & source, FileFormat format, int & reason) {
    if (format == FileFormat::Xml) return doParseXml(source, reason);

    if (format == FileFormat::Pdf) return doParsePdf(source, reason);

    reason = ENotAnIpeFile;
    return nullptr;
}

Document * Document::load(const char * fname, int & reason) {
    reason = EFileOpenError;
    std::FILE * fd = Platform::fopen(fname, "rb");
    if (!fd) return nullptr;
    Platform::changeDirectory(Platform::parentDirectory(fname));
    FileSource source(fd);
    FileFormat format = fileFormat(source);
    std::rewind(fd);
    Document * self = load(source, format, reason);
    std::fclose(fd);
    return self;
}

Document * Document::loadWithErrorReport(const char * fname) {
    int reason;
    Document * doc = load(fname, reason);
    if (doc) return doc;

    fprintf(stderr, "Could not read Ipe file '%s'\n", fname);
    switch (reason) {
    case Document::EVersionTooOld:
	fprintf(stderr, "The Ipe version of this document is too old.\n"
			"Please convert it using 'ipe6upgrade'.\n");
	break;
    case Document::EVersionTooRecent:
	fprintf(stderr, "The document was created by a newer version of Ipe.\n"
			"Please upgrade your Ipe installation.\n");
	break;
    case Document::EFileOpenError: perror("Error opening the file"); break;
    case Document::ENotAnIpeFile:
	fprintf(stderr, "The document was not created by Ipe.\n");
	break;
    default:
	fprintf(stderr, "Error parsing the document at position %d\n.", reason);
	break;
    }
    return nullptr;
}

// --------------------------------------------------------------------

//! Save in a stream.
/*! Returns true if sucessful.
 */
bool Document::save(TellStream & stream, FileFormat format, uint32_t flags) const {
    if (format == FileFormat::Xml) {
	stream << "<?xml version=\"1.0\"?>\n";
	stream << "<!DOCTYPE ipe SYSTEM \"ipe.dtd\">\n";
	saveAsXml(stream);
	return true;
    }

    int compresslevel = 9;
    if (flags & SaveFlag::NoZip) compresslevel = 0;

    if (format == FileFormat::Pdf) {
	PdfWriter writer(stream, this, iResources, flags, 0, -1, compresslevel);
	writer.createPages();
	writer.createBookmarks();
	writer.createNamedDests();
	if (!(flags & SaveFlag::Export)) {
	    String xmlData;
	    StringStream stream(xmlData);
	    if (compresslevel > 0) {
		DeflateStream dfStream(stream, compresslevel);
		// all bitmaps have been embedded and carry correct object number
		saveAsXml(dfStream, true);
		dfStream.close();
		writer.createXmlStream(xmlData, true);
	    } else {
		saveAsXml(stream, true);
		writer.createXmlStream(xmlData, false);
	    }
	}
	writer.createTrailer();
	return true;
    }

    return false;
}

bool Document::save(const char * fname, FileFormat format, uint32_t flags) const {
    std::FILE * fd = Platform::fopen(fname, "wb");
    if (!fd) return false;

    String new_base = Platform::parentDirectory(Platform::realPath(fname));
    BitmapFinder bmf;
    findBitmaps(bmf);
    for (auto & bm : bmf.iBitmaps) {
	if (bm.isExternal()) bm.changeExternalPathRelativeBase(new_base);
    }

    FileStream stream(fd);
    bool result = save(stream, format, flags);
    std::fclose(fd);
    return result;
}

//! Export a single view to PDF
bool Document::exportView(const char * fname, FileFormat format, uint32_t flags, int pno,
			  int vno) const {
    if (format != FileFormat::Pdf) return false;

    int compresslevel = 9;
    if (flags & SaveFlag::NoZip) compresslevel = 0;

    std::FILE * fd = Platform::fopen(fname, "wb");
    if (!fd) return false;
    FileStream stream(fd);

    PdfWriter writer(stream, this, iResources, flags, pno, pno, compresslevel);
    writer.createPageView(pno, vno);
    writer.createTrailer();
    std::fclose(fd);
    return true;
}

//! Export a range of pages to PDF.
bool Document::exportPages(const char * fname, uint32_t flags, int fromPage,
			   int toPage) const {
    int compresslevel = 9;
    if (flags & SaveFlag::NoZip) compresslevel = 0;
    std::FILE * fd = Platform::fopen(fname, "wb");
    if (!fd) return false;
    FileStream stream(fd);
    PdfWriter writer(stream, this, iResources, flags, fromPage, toPage, compresslevel);
    writer.createPages();
    writer.createTrailer();
    std::fclose(fd);
    return true;
}

// --------------------------------------------------------------------

//! Create a list of all bitmaps in the document.
void Document::findBitmaps(BitmapFinder & bm) const {
    for (int i = 0; i < countPages(); ++i) bm.scanPage(page(i));
    // also need to look at all templates
    AttributeSeq seq;
    iCascade->allNames(ESymbol, seq);
    for (AttributeSeq::iterator it = seq.begin(); it != seq.end(); ++it) {
	const Symbol * symbol = iCascade->findSymbol(*it);
	symbol->iObject->accept(bm);
    }
    std::sort(bm.iBitmaps.begin(), bm.iBitmaps.end());
}

//! Save in XML format into an Stream.
void Document::saveAsXml(Stream & stream, bool usePdfBitmaps) const {
    stream << "<ipe version=\"" << FILE_FORMAT << "\"";
    if (!iProperties.iCreator.empty())
	stream << " creator=\"" << iProperties.iCreator << "\"";
    stream << ">\n";
    String info;
    StringStream infoStr(info);
    infoStr << "<info";
    if (!iProperties.iCreated.empty())
	infoStr << " created=\"" << iProperties.iCreated << "\"";
    if (!iProperties.iModified.empty())
	infoStr << " modified=\"" << iProperties.iModified << "\"";
    if (!iProperties.iTitle.empty()) {
	infoStr << " title=\"";
	infoStr.putXmlString(iProperties.iTitle);
	infoStr << "\"";
    }
    if (!iProperties.iAuthor.empty()) {
	infoStr << " author=\"";
	infoStr.putXmlString(iProperties.iAuthor);
	infoStr << "\"";
    }
    if (!iProperties.iSubject.empty()) {
	infoStr << " subject=\"";
	infoStr.putXmlString(iProperties.iSubject);
	infoStr << "\"";
    }
    if (!iProperties.iKeywords.empty()) {
	infoStr << " keywords=\"";
	infoStr.putXmlString(iProperties.iKeywords);
	infoStr << "\"";
    }
    if (!iProperties.iLanguage.empty()) {
	infoStr << " language=\"";
	infoStr.putXmlString(iProperties.iLanguage);
	infoStr << "\"";
    }
    if (iProperties.iFullScreen) { infoStr << " pagemode=\"fullscreen\""; }
    if (iProperties.iNumberPages) { infoStr << " numberpages=\"yes\""; }
    if (iProperties.iSequentialText) { infoStr << " sequentialtext=\"yes\""; }
    switch (iProperties.iTexEngine) {
    case LatexType::Pdftex: infoStr << " tex=\"pdftex\""; break;
    case LatexType::Xetex: infoStr << " tex=\"xetex\""; break;
    case LatexType::Luatex: infoStr << " tex=\"luatex\""; break;
    case LatexType::Default:
    default: break;
    }
    infoStr << "/>\n";
    if (info.size() > 10) stream << info;

    if (!iProperties.iPreamble.empty()) {
	stream << "<preamble>";
	stream.putXmlString(iProperties.iPreamble);
	stream << "</preamble>\n";
    }

    // save bitmaps
    BitmapFinder bm;
    findBitmaps(bm);
    if (!bm.iBitmaps.empty()) {
	int id = 1;
	Bitmap prev;
	for (std::vector<Bitmap>::iterator it = bm.iBitmaps.begin();
	     it != bm.iBitmaps.end(); ++it) {
	    if (!it->equal(prev)) {
		if (usePdfBitmaps) {
		    it->saveAsXml(stream, it->objNum(), it->objNum());
		} else {
		    it->saveAsXml(stream, id);
		    it->setObjNum(id);
		}
	    } else
		it->setObjNum(prev.objNum()); // noop if prev == it
	    prev = *it;
	    ++id;
	}
    }

    // now save style sheet
    iCascade->saveAsXml(stream);

    // save pages
    for (int i = 0; i < countPages(); ++i) page(i)->saveAsXml(stream);
    stream << "</ipe>\n";
}

// --------------------------------------------------------------------

//! Set document properties.
void Document::setProperties(const SProperties & props) { iProperties = props; }

//! Replace the entire style sheet cascade.
/*! Takes ownership of \a cascade, and returns the original cascade. */
Cascade * Document::replaceCascade(Cascade * sheets) {
    Cascade * old = iCascade;
    iCascade = sheets;
    return old;
}

//! Check all symbolic attributes in the document.
/*!  This function verifies that all symbolic attributes in the
  document are defined in the style sheet. It appends to \a seq all
  symbolic attributes (in no particular order, but without duplicates)
  that are NOT defined.

  Returns \c true if there are no undefined symbolic attributes in the
  document.
*/
bool Document::checkStyle(AttributeSeq & seq) const {
    for (int i = 0; i < countPages(); ++i) {
	for (int j = 0; j < page(i)->count(); ++j) {
	    page(i)->object(j)->checkStyle(cascade(), seq);
	}
    }
    return (seq.size() == 0);
}

//! Update the PDF resources (after running latex).
/*! Takes ownership. */
void Document::setResources(PdfResources * resources) {
    delete iResources;
    iResources = resources;
}

//! Return total number of views in all pages.
int Document::countTotalViews() const {
    int views = 0;
    for (int i = 0; i < countPages(); ++i) {
	int nviews = page(i)->countViews();
	views += (nviews > 0) ? nviews : 1;
    }
    return views;
}

//! Return page index given a section title or page number.
/*! Input page numbers are 1-based strings.
  Returns -1 if page not found. */
int Document::findPage(String s) const {
    if (s.empty()) return -1;
    if ('0' <= s[0] && s[0] <= '9') {
	int no = Lex(s).getInt();
	if (no <= 0 || no > countPages()) return -1;
	return no - 1;
    }
    for (int i = 0; i < countPages(); ++i) {
	if (s == page(i)->section(0)) return i;
    }
    return -1;
}

//! Insert a new page.
/*! The page is inserted at index \a no. */
void Document::insert(int no, Page * page) { iPages.insert(iPages.begin() + no, page); }

//! Append a new page.
void Document::push_back(Page * page) { iPages.push_back(page); }

//! Replace page.
/*! Returns the original page. */
Page * Document::set(int no, Page * page) {
    Page * p = iPages[no];
    iPages[no] = page;
    return p;
}

//! Remove a page.
/*! Returns the page that has been removed.  */
Page * Document::remove(int no) {
    Page * p = iPages[no];
    iPages.erase(iPages.begin() + no);
    return p;
}

// --------------------------------------------------------------------

int Document::prepareLatexRun(Latex ** pConverter) {
    *pConverter = nullptr;
    std::unique_ptr<Latex> converter(
	new Latex(cascade(), iProperties.iTexEngine, iProperties.iSequentialText));
    AttributeSeq seq;
    cascade()->allNames(ESymbol, seq);

    for (AttributeSeq::iterator it = seq.begin(); it != seq.end(); ++it) {
	const Symbol * sym = cascade()->findSymbol(*it);
	if (sym) converter->scanObject(sym->iObject);
    }

    int count = 0;
    for (int i = 0; i < countPages(); ++i) count = converter->scanPage(page(i));

    if (iProperties.iNumberPages) {
	for (int i = 0; i < countPages(); ++i) {
	    int nviews = page(i)->countViews();
	    for (int j = 0; j < nviews; ++j)
		converter->addPageNumber(i, j, countPages(), nviews);
	}
    } else if (count == 0)
	return ErrNoText;

    // First we need a directory
    String latexDir = Platform::folder(FolderLatex);
    if (Platform::mkdirTree(latexDir) != 0) {
	ipeDebug("Latex directory '%s' does not exist and cannot be created!\n",
		 latexDir.z());
	return ErrNoDir;
    }
    latexDir += IPESEP;
    String texFile = latexDir + "ipetemp.tex";
    String pdfFile = latexDir + "ipetemp.pdf";
    String logFile = latexDir + "ipetemp.log";

    std::remove(logFile.z());
    std::remove(pdfFile.z());

    std::FILE * file = Platform::fopen(texFile.z(), "wb");
    if (!file) return ErrWritingSource;
    FileStream stream(file);
    int err = converter->createLatexSource(stream, properties().iPreamble);
    std::fclose(file);

    if (err < 0) return ErrWritingSource;

    *pConverter = converter.release();
    return ErrNone;
}

int Document::completeLatexRun(String & texLog, Latex * converter) {
    texLog = "";
    String pdfFile = Platform::folder(FolderLatex, "ipetemp.pdf");
    String logFile = Platform::folder(FolderLatex, "ipetemp.log");

    // Check log file for Pdflatex version and errors
    texLog = Platform::readFile(logFile);
    if (!texLog.hasPrefix("This is ") && !texLog.hasPrefix("entering extended mode"))
	return ErrRunLatex;
    int i = texLog.find('\n');
    if (i < 0) return ErrRunLatex;
    String version = texLog.substr(8, i);
    ipeDebug("%s", version.z());
    // Check for error
    if (texLog.find("\n!") >= 0) return ErrLatex;

    std::FILE * pdfF = Platform::fopen(pdfFile.z(), "rb");
    if (!pdfF) return ErrLatex;
    FileSource source(pdfF);
    bool okay = converter->readPdf(source);
    std::fclose(pdfF);
    if (!okay) return ErrLatexOutput;

    okay = converter->updateTextObjects();
    if (okay) {
	setResources(converter->takeResources());
	// resources()->show();
    }
    delete converter;
    return okay ? ErrNone : ErrLatexOutput;
}

int Document::runLatex(String docname, String & texLog) {
    Latex * converter = nullptr;
    int err = prepareLatexRun(&converter);
    if (err) return err;
    String cmd = Platform::howToRunLatex(iProperties.iTexEngine, docname);
    if (cmd.empty() || Platform::system(cmd)) return ErrRunLatex;
    return completeLatexRun(texLog, converter);
}

//! Run Pdflatex (suitable for console applications)
/*! Success/error is reported on stderr. */
int Document::runLatex(String docname) {
    String logFile;
    switch (runLatex(docname, logFile)) {
    case ErrNoText:
	fprintf(stderr, "No text objects in document, no need to run Pdflatex.\n");
	return 0;
    case ErrNoDir:
	fprintf(stderr, "Directory '%s' does not exist and cannot be created.\n",
		"latexdir");
	return 1;
    case ErrWritingSource: fprintf(stderr, "Error writing Latex source.\n"); return 1;
    case ErrRunLatex:
	fprintf(stderr, "There was an error trying to run Pdflatex.\n");
	return 1;
    case ErrLatex: fprintf(stderr, "There were Latex errors.\n"); return 1;
    case ErrLatexOutput:
	fprintf(stderr, "There was an error reading the Pdflatex output.\n");
	return 1;
    case ErrNone:
    default: fprintf(stderr, "Pdflatex was run sucessfully.\n"); return 0;
    }
}

// --------------------------------------------------------------------
