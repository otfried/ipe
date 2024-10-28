// --------------------------------------------------------------------
// ipeextract
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

#include "ipepdfparser.h"
#include "ipeutils.h"
#include "ipexml.h"
#include <cstdlib>

using namespace ipe;

// ---------------------------------------------------------------------

enum TFormat { EXml, EPdf, EEps, EIpe5, EUnknown };

String readALine(DataSource & source) {
    String s;
    int ch = source.getChar();
    while (ch != EOF && ch != '\n') {
	s += char(ch);
	ch = source.getChar();
    }
    return s;
}

//! Determine format of file in \a source.
TFormat fileFormat(DataSource & source) {
    String s1 = readALine(source);
    String s2 = readALine(source);
    if (s1.substr(0, 5) == "<?xml" || s1.substr(0, 4) == "<ipe") return EXml;
    if (s1.substr(0, 4) == "%PDF") return EPdf; // let's assume it contains an Ipe stream
    if (s1.substr(0, 4) == "%!PS") {
	if (s2.substr(0, 11) != "%%Creator: ") return EUnknown;
	if (s2.substr(11, 6) == "Ipelib" || s2.substr(11, 4) == "xpdf") return EEps;
	if (s2.substr(11, 3) == "Ipe") return EIpe5;
	return EUnknown;
    }
    if (s1.substr(0, 5) == "%\\Ipe" || s1.substr(0, 6) == "%\\MIPE") return EIpe5;
    return EUnknown;
}

// --------------------------------------------------------------------

class StreamParser : public XmlParser {
public:
    explicit StreamParser(DataSource & source, std::FILE * out)
	: XmlParser(source)
	, iOut(out) { /* nothing */ }
    bool parse();
    virtual Buffer image(int objNum) = 0;
    void writeAttributes(const XmlAttributes & attr);
    bool parseBitmap();

private:
    std::FILE * iOut;
};

bool StreamParser::parse() {
    while (!eos()) {
	bool lt = (iCh == '<');
	fputc(iCh, iOut);
	getChar();
	// look out for <bitmap> tag
	if (lt && iCh == 'b') {
	    String tag;
	    while (isTagChar(iCh)) {
		tag += char(iCh);
		fputc(iCh, iOut);
		getChar();
	    }
	    // at char after tag
	    if (tag == "bitmap" && !parseBitmap()) return false;
	}
    }
    return true;
}

// write out attributes, but drop 'pdfObject'
void StreamParser::writeAttributes(const XmlAttributes & attr) {
    for (XmlAttributes::const_iterator it = attr.begin(); it != attr.end(); ++it)
	if (it->first != "pdfObject")
	    fprintf(iOut, " %s=\"%s\"", it->first.z(), it->second.z());
    fprintf(iOut, ">\n");
}

static void writeBits(FILE * out, Buffer bits) {
    const char * data = bits.data();
    const char * fin = data + bits.size();
    int col = 0;
    while (data != fin) {
	fprintf(out, "%02x", (*data++ & 0xff));
	if (++col == 36) {
	    fputc('\n', out);
	    col = 0;
	}
    }
    if (col > 0) fputc('\n', out);
}

bool StreamParser::parseBitmap() {
    XmlAttributes attr;
    if (!parseAttributes(attr)) return false;
    String objNumStr;
    if (attr.slash() && attr.has("pdfObject", objNumStr)) {
	Lex lex(objNumStr);
	Buffer bits = image(lex.getInt());
	Buffer alpha;
	lex.skipWhitespace();
	if (!lex.eos()) {
	    alpha = image(lex.getInt());
	    fprintf(iOut, " alphaLength=\"%d\"", alpha.size());
	}
	fprintf(iOut, " length=\"%d\"", bits.size());
	writeAttributes(attr);
	writeBits(iOut, bits);
	if (alpha.size() > 0) writeBits(iOut, alpha);
	fprintf(iOut, "</bitmap>\n");
    } else {
	// just write out attributes
	writeAttributes(attr);
    }
    return true;
}

// --------------------------------------------------------------------

class StreamParserPdf : public StreamParser {
public:
    explicit StreamParserPdf(PdfFile & loader, DataSource & source, std::FILE * out)
	: StreamParser(source, out)
	, iLoader(loader) { /* nothing */ }
    virtual Buffer image(int objNum);

private:
    PdfFile & iLoader;
};

Buffer StreamParserPdf::image(int objNum) {
    const PdfObj * obj = iLoader.object(objNum);
    if (!obj || !obj->dict() || obj->dict()->stream().size() == 0) return Buffer();
    return obj->dict()->stream();
}

// --------------------------------------------------------------------

class PsSource : public DataSource {
public:
    PsSource(DataSource & source)
	: iSource(source) { /* nothing */ }
    bool skipToXml();
    String readLine();
    Buffer image(int index) const;
    int getNext() const;
    inline bool deflated() const { return iDeflated; }

    virtual int getChar();

private:
    DataSource & iSource;
    std::vector<Buffer> iImages;
    bool iEos;
    bool iDeflated;
};

int PsSource::getChar() {
    int ch = iSource.getChar();
    if (ch == '\n') iSource.getChar(); // remove '%'
    return ch;
}

String PsSource::readLine() {
    String s;
    int ch = iSource.getChar();
    while (ch != EOF && ch != '\n') {
	s += char(ch);
	ch = iSource.getChar();
    }
    iEos = (ch == EOF);
    return s;
}

Buffer PsSource::image(int index) const {
    if (1 <= index && index <= int(iImages.size()))
	return iImages[index - 1];
    else
	return Buffer();
}

bool PsSource::skipToXml() {
    iDeflated = false;

    String s1 = readLine();
    String s2 = readLine();

    if (s1.substr(0, 11) != "%!PS-Adobe-" || s2.substr(0, 11) != "%%Creator: ")
	return false;

    if (s2.substr(11, 6) == "Ipelib") {
	// the 'modern' file format of Ipe 6.0 preview 17 and later
	do {
	    s1 = readLine();
	    if (s1.substr(0, 17) == "%%BeginIpeImage: ") {
		Lex lex(s1.substr(17));
		int num, len;
		lex >> num >> len;
		if (num != int(iImages.size() + 1)) return false;
		(void)readLine(); // skip 'image'
		Buffer buf(len);
		A85Source a85(iSource);
		char * p = buf.data();
		char * p1 = p + buf.size();
		while (p < p1) {
		    int ch = a85.getChar();
		    if (ch == EOF) return false;
		    *p++ = char(ch);
		}
		iImages.push_back(buf);
	    }
	} while (!iEos && s1.substr(0, 13) != "%%BeginIpeXml");

	iDeflated = (s1.substr(13, 14) == ": /FlateDecode");

    } else {
	// the 'old' file format generated through pdftops
	do { s1 = readLine(); } while (!iEos && s1.substr(0, 10) != "%%EndSetup");
    }
    if (iEos) return false;
    (void)iSource.getChar(); // skip '%' before <ipe>
    return true;
}

// --------------------------------------------------------------------

class StreamParserPs : public StreamParser {
public:
    explicit StreamParserPs(PsSource & loader, DataSource & source, std::FILE * out)
	: StreamParser(source, out)
	, iLoader(loader) { /* nothing */ }
    virtual Buffer image(int objNum);

private:
    PsSource & iLoader;
};

Buffer StreamParserPs::image(int objNum) { return iLoader.image(objNum); }

// --------------------------------------------------------------------

static bool extractPs(DataSource & source, std::FILE * out) {
    PsSource psSource(source);
    if (!psSource.skipToXml()) {
	fprintf(stderr, "Could not find XML stream.\n");
	return false;
    }

    if (psSource.deflated()) {
	A85Source a85(psSource);
	InflateSource source(a85);
	StreamParserPs parser(psSource, source, out);
	return parser.parse();
    } else {
	StreamParserPs parser(psSource, psSource, out);
	return parser.parse();
    }
    return false;
}

static bool extractPdf(DataSource & source, std::FILE * out) {
    PdfFile loader;
    if (!loader.parse(source)) {
	fprintf(stderr, "Error parsing PDF file - probably not an Ipe file.\n");
	return false;
    }

    // try ancient format version first (early previews of Ipe 6.0)
    const PdfObj * obj = loader.catalog()->get("Ipe", &loader);

    // otherwise try most recent format (>= 7.2.11)
    if (!obj) {
	obj = loader.catalog()->get("PieceInfo", &loader);
	if (obj && obj->dict()) {
	    obj = obj->dict()->get("Ipe", &loader);
	    if (obj && obj->dict()) obj = obj->dict()->get("Private", &loader);
	}
    }

    if (!obj) obj = loader.object(1);

    if (!obj || !obj->dict()) {
	fprintf(stderr, "Input file does not contain an Ipe XML stream.\n");
	return false;
    }

    const PdfObj * type = obj->dict()->get("Type");
    if (!type || !type->name() || type->name()->value() != "Ipe") {
	fprintf(stderr, "Input file does not contain an Ipe XML stream.\n");
	return false;
    }

    Buffer buffer = obj->dict()->stream();
    BufferSource xml(buffer);

    if (obj->dict()->deflated()) {
	InflateSource xml1(xml);
	StreamParserPdf parser(loader, xml1, out);
	return parser.parse();
    } else {
	StreamParserPdf parser(loader, xml, out);
	return parser.parse();
    }
}

// --------------------------------------------------------------------

static void usage() {
    fprintf(stderr, "Usage: ipeextract ( <input.pdf> | <input.eps> ) [<output.xml>]\n"
		    "Ipeextract extracts the XML stream from a PDF or Postscript file\n"
		    "generated by any version of Ipe 6 or Ipe 7.\n");
    exit(1);
}

int main(int argc, char * argv[]) {
    Platform::initLib(IPELIB_VERSION);

    // ensure one or two arguments
    if (argc != 2 && argc != 3) usage();

    const char * src = argv[1];
    String dst;

    if (argc == 3) {
	dst = argv[2];
    } else {
	String s = src;
	if (s.right(4) == ".pdf" || s.right(4) == ".eps")
	    dst = s.left(s.size() - 3) + "xml";
	else
	    dst = s + ".xml";
    }

    std::FILE * fd = Platform::fopen(src, "rb");
    if (!fd) {
	std::fprintf(stderr, "Could not open '%s'\n", src);
	exit(1);
    }
    FileSource source(fd);
    TFormat format = fileFormat(source);
    if (format == EXml) {
	fprintf(stderr, "Input file is already in XML format.\n");
    } else if (format == EIpe5) {
	fprintf(stderr, "Input file is in Ipe5 format.\n"
			"Run 'ipe5toxml' to convert it to XML format.\n");
    } else {
	std::rewind(fd);
	std::FILE * out = Platform::fopen(dst.z(), "wb");
	if (!out) {
	    fprintf(stderr, "Could not open '%s' for writing.\n", dst.z());
	} else {
	    bool res =
		(format == EPdf) ? extractPdf(source, out) : extractPs(source, out);
	    if (!res) fprintf(stderr, "Error during extraction of XML stream.\n");
	    std::fclose(out);
	}
    }
    std::fclose(fd);
    return 0;
}

// --------------------------------------------------------------------
