// --------------------------------------------------------------------
// Ipe6upgrade
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2023 Otfried Cheong

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

#include "ipexml.h"
#include "ipeattributes.h"
#include <cstdlib>

using namespace ipe;

// --------------------------------------------------------------------

struct Attributes {
  String textsize;
  String marksize;
  String markshape;
  String stroke;
  String fill;
  String dashStyle;
  String pen;
  String cap;
  String join;
  String fillrule;
};

// --------------------------------------------------------------------

static const char *standardSheet =
  "<ipestyle name=\"ipe6\">\n"
  "<color name=\"red\" value=\"1 0 0\"/>\n"
  "<color name=\"green\" value=\"0 1 0\"/>\n"
  "<color name=\"blue\" value=\"0 0 1\"/>\n"
  "<color name=\"yellow\" value=\"1 1 0\"/>\n"
  "<color name=\"gray1\" value=\"0.125\"/>\n"
  "<color name=\"gray2\" value=\"0.25\"/>\n"
  "<color name=\"gray3\" value=\"0.375\"/>\n"
  "<color name=\"gray4\" value=\"0.5\"/>\n"
  "<color name=\"gray5\" value=\"0.625\"/>\n"
  "<color name=\"gray6\" value=\"0.75\"/>\n"
  "<color name=\"gray7\" value=\"0.875\"/>\n"
  "<dashstyle name=\"dashed\" value=\"[4] 0\"/>\n"
  "<dashstyle name=\"dotted\" value=\"[1 3] 0\"/>\n"
  "<dashstyle name=\"dash dotted\" value=\"[4 2 1 2] 0\"/>\n"
  "<dashstyle name=\"dash dot dotted\" value=\"[4 2 1 2 1 2] 0\"/>\n"
  "<pen name=\"heavier\" value=\"0.8\"/>\n"
  "<pen name=\"fat\" value=\"1.2\"/>\n"
  "<pen name=\"ultrafat\" value=\"2\"/>\n"
  "<textsize name=\"large\" value=\"\\large\"/>\n"
  "<textsize name=\"Large\" value=\"\\Large\"/>\n"
  "<textsize name=\"LARGE\" value=\"\\LARGE\"/>\n"
  "<textsize name=\"huge\" value=\"\\huge\"/>\n"
  "<textsize name=\"Huge\" value=\"\\Huge\"/>\n"
  "<textsize name=\"small\" value=\"\\small\"/>\n"
  "<textsize name=\"footnote\" value=\"\\footnotesize\"/>\n"
  "<textsize name=\"tiny\" value=\"\\tiny\"/>\n"
  "<symbolsize name=\"small\" value=\"2\"/>\n"
  "<symbolsize name=\"tiny\" value=\"1.1\"/>\n"
  "<symbolsize name=\"large\" value=\"5\"/>\n"
  "<arrowsize name=\"small\" value=\"5\"/>\n"
  "<arrowsize name=\"tiny\" value=\"3\"/>\n"
  "<arrowsize name=\"large\" value=\"10\"/>\n"
  "<symbol name=\"mark/circle(sx)\" transformations=\"translations\">\n"
  "<path fill=\"sym-stroke\">\n"
  "0.6 0 0 0.6 0 0 e 0.4 0 0 0.4 0 0 e\n"
  "</path></symbol>\n"
  "<symbol name=\"mark/disk(sx)\" transformations=\"translations\">\n"
  "<path fill=\"sym-stroke\">\n"
  "0.6 0 0 0.6 0 0 e\n"
  "</path></symbol>\n"
  "<symbol name=\"mark/fdisk(sfx)\" transformations=\"translations\">\n"
  "<group><path fill=\"sym-fill\">\n"
  "0.5 0 0 0.5 0 0 e\n"
  "</path><path fill=\"sym-stroke\" fillrule=\"eofill\">\n"
  "0.6 0 0 0.6 0 0 e 0.4 0 0 0.4 0 0 e\n"
  "</path></group></symbol>\n"
  "<symbol name=\"mark/box(sx)\" transformations=\"translations\">\n"
  "<path fill=\"sym-stroke\" fillrule=\"eofill\">\n"
  "-0.6 -0.6 m 0.6 -0.6 l 0.6 0.6 l -0.6 0.6 l h "
  "-0.4 -0.4 m 0.4 -0.4 l 0.4 0.4 l -0.4 0.4 l h"
  "</path></symbol>\n"
  "<symbol name=\"mark/square(sx)\" transformations=\"translations\">\n"
  "<path fill=\"sym-stroke\">\n"
  "-0.6 -0.6 m 0.6 -0.6 l 0.6 0.6 l -0.6 0.6 l h"
  "</path></symbol>\n"
  "<symbol name=\"mark/fsquare(sfx)\" transformations=\"translations\">\n"
  "<group><path fill=\"sym-fill\">\n"
  "-0.5 -0.5 m 0.5 -0.5 l 0.5 0.5 l -0.5 0.5 l h"
  "</path><path fill=\"sym-stroke\" fillrule=\"eofill\">\n"
  "-0.6 -0.6 m 0.6 -0.6 l 0.6 0.6 l -0.6 0.6 l h"
  "-0.4 -0.4 m 0.4 -0.4 l 0.4 0.4 l -0.4 0.4 l h"
  "</path></group></symbol>\n"
  "<symbol name=\"mark/cross(sx)\" transformations=\"translations\">\n"
  "<group><path fill=\"sym-stroke\">\n"
  "-0.43 -0.57 m 0.57 0.43 l 0.43 0.57 l -0.57 -0.43 l h</path>"
  "<path fill=\"sym-stroke\">\n"
  "-0.43 0.57 m 0.57 -0.43 l 0.43 -0.57 l -0.57 0.43 l h</path>\n"
  "</group></symbol>\n"
  "<textstyle name=\"center\" begin=\"\\begin{center}\"\n"
  "end=\"\\end{center}\"/>\n"
  "<textstyle name=\"itemize\" begin=\"\\begin{itemize}\"\n"
  "end=\"\\end{itemize}\"/>\n"
  "<textstyle name=\"item\" begin=\"\\begin{itemize}\\item{}\"\n"
  "end=\"\\end{itemize}\"/>\n"
  "</ipestyle>\n";

// --------------------------------------------------------------------

class Parser : public XmlParser {
public:
  enum { ESuccess = 0, ENotIpe6 = 1, ESyntaxError = 2 };
  explicit Parser(DataSource &source, Stream &out);
  int parseDocument();
  bool parsePage();
  bool parseObject(String tag, const Attributes &a);
  bool parseGroup(const XmlAttributes &attr, const Attributes &a);
  bool parseStyle();
  bool parseBitmap();
private:
  void writeAttr(const XmlAttributes &att, String s, String grs = String());
  void writeAttr(const XmlAttributes &att);
  void writeTag(String tag, const XmlAttributes &att, String data = String());
private:
  Stream &iStream;
  Rect iMediaBox;
  bool iUseCropBox;
};

// --------------------------------------------------------------------

Parser::Parser(DataSource &source, Stream &out)
  : XmlParser(source), iStream(out)
{
  // nothing
}

int Parser::parseDocument()
{
  String tag = parseToTag();
  if (tag == "?xml") {
    XmlAttributes attr;
    if (!parseAttributes(attr))
      return ESyntaxError;
    tag = parseToTag();
  }
  if (tag != "ipe")
    return ESyntaxError;

  XmlAttributes attr;
  if (!parseAttributes(attr))
    return ESyntaxError;

  String str;

  iStream << "<?xml version=\"1.0\"?>\n";
  iStream << "<!DOCTYPE ipe SYSTEM \"ipe.dtd\">\n";
  iStream << "<ipe";
  if (attr.has("version", str)) {
    int  v = Lex(str).getInt();
    if (v >= 70000)
      return ENotIpe6;
  }

  iStream << " version=\"70000\"";
  iStream << " creator=\"ipe6upgrade\">\n";

  // up to pre27 media size was in <info>
  bool needMediaSheet = false;
  iUseCropBox = true;
  Vector paper;

  tag = parseToTag();
  if (tag == "info") {
    XmlAttributes att;
    if (!parseAttributes(att))
      return ESyntaxError;

    if (att.has("media", str)) {
      Lex lex(str);
      Vector mbmin;
      lex >> mbmin.x >> mbmin.y >> paper.x >> paper.y;
      needMediaSheet = true;
    }

    iUseCropBox = (att.has("bbox", str) && str == "yes");

    iStream << "<info";
    writeAttr(att, "title");
    writeAttr(att, "author");
    writeAttr(att, "subject");
    writeAttr(att, "keywords");
    writeAttr(att, "pagemode");
    writeAttr(att, "numberpages");
    writeAttr(att, "created");
    writeAttr(att, "modified");
    iStream << "/>\n";

    tag = parseToTag();
  }
  if (tag == "preamble") {
    XmlAttributes att;
    if (!parseAttributes(att))
      return false;
    if (!parsePCDATA("preamble", str))
      return false;
    iStream << "<preamble>";
    iStream.putXmlString(str);
    iStream << "</preamble>\n";

    tag = parseToTag();
  }

  iStream << standardSheet;

  if (needMediaSheet) {
    iStream << "<ipestyle>\n";
    iStream << "<layout paper=\"" << paper
	    << "\" origin=\"0 0\" frame=\"" << paper << "\"/>\n";
    iStream << "</ipestyle>\n";
  }

  while (tag == "ipestyle" || tag == "bitmap") {
    if (tag == "ipestyle") {
      if (!parseStyle())
	return ESyntaxError;
    } else { // tag == "bitmap"
      if (!parseBitmap())
	return ESyntaxError;
    }
    tag = parseToTag();
  }

  while (tag == "page") {
    if (!parsePage())
      return ESyntaxError;
    tag = parseToTag();
  }

  if (tag != "/ipe")
    return ESyntaxError;
  iStream << "</ipe>\n";
  return ESuccess;
}

//! Parse an Bitmap.
/*! On calling, stream must be just past \c bitmap. */
bool Parser::parseBitmap()
{
  XmlAttributes att;
  if (!parseAttributes(att))
    return false;
  String bits;
  if (!att.slash() && !parsePCDATA("bitmap", bits))
    return false;
  writeTag("bitmap", att, bits);
  return true;
}

//! Parse an Page.
/*! On calling, stream must be just past \c page. */
bool Parser::parsePage()
{
  XmlAttributes att;
  if (!parseAttributes(att))
    return false;

  iStream << "<page";
  writeAttr(att, "title");
  writeAttr(att, "section");
  writeAttr(att, "subsection");
  iStream << ">\n";

  String tag = parseToTag();
  while (tag == "layer" || tag == "view") {
    XmlAttributes att;
    if (!parseAttributes(att))
      return false;
    if (tag == "layer") {
      iStream << "<layer";
      writeAttr(att, "name");
      writeAttr(att, "edit");
      iStream << "/>\n";
    } else {
      // effects are not supported by ipe6upgrade
      iStream << "<view";
      writeAttr(att, "layers");
      writeAttr(att, "active");
      iStream << "/>\n";
    }
    tag = parseToTag();
  }

  Attributes a;

  for (;;) {
    if (tag == "/page") {
      iStream << "</page>\n";
      return true;
    }
    if (!parseObject(tag, a))
      return false;
    tag = parseToTag();
  }
  return true;
}

bool Parser::parseGroup(const XmlAttributes &attr, const Attributes &a)
{
  iStream << "<group";
  writeAttr(attr, "matrix");
  writeAttr(attr, "layer");
  writeAttr(attr, "pin");
  iStream << ">\n";

  Attributes a1 = a;
  if (a.textsize.empty())
    a1.textsize = attr["textsize"];
  if (a.marksize.empty())
    a1.marksize = attr["marksize"];
  if (a.markshape.empty())
    a1.markshape = attr["markshape"];
  if (a.stroke.empty())
    a1.stroke = attr["stroke"];
  if (a.fill.empty())
    a1.fill = attr["fill"];
  if (a.dashStyle.empty())
    a1.dashStyle = attr["dash"];
  if (a.pen.empty())
    a1.pen = attr["pen"];
  if (a.cap.empty())
    a1.cap = attr["cap"];
  if (a.join.empty())
    a1.join = attr["join"];
  if (a.fillrule.empty())
    a1.fillrule = attr["fillrule"];

  String tag = parseToTag();
  for (;;) {
    if (tag == "/group") {
      iStream << "</group>\n";
      return true;
    }
    if (!parseObject(tag, a1))
      return false;
    tag = parseToTag();
  }
}

static const char *const marktypes[] = {
  "circle(sx)", "disk(sx)", "box(sx)", "square(sx)", "cross(sx)",
  "fdisk(sfx)", "fsquare(sfx)" };

//! Parse an Object.
/*! On calling, stream must be just past the tag. */
bool Parser::parseObject(String tag, const Attributes &a)
{
  XmlAttributes attr;
  if (!parseAttributes(attr))
    return false;

  if (tag == "group")
    return parseGroup(attr, a);

  String pcdata;
  if (!attr.slash() && !parsePCDATA(tag, pcdata))
    return false;

  if (tag == "image") {
    writeTag(tag, attr, pcdata);
  } else if (tag == "ref") {
    writeTag("use", attr, pcdata);
  } else if (tag == "mark") {
    String t;
    if (a.markshape.empty())
      t = attr["shape"];
    else
      t = a.markshape;
    int typ = Lex(t).getInt();
    if (!a.fill.empty() || !attr["fill"].empty()) {
      if (typ == 1)
	typ = 6;
      else if (typ == 3)
	typ = 7;
    }
    iStream << "<use name=\"mark/" << marktypes[typ - 1] << "\"";
    writeAttr(attr, "pos");
    writeAttr(attr, "layer");
    writeAttr(attr, "matrix");
    writeAttr(attr, "pin");
    writeAttr(attr, "size", a.marksize);
    writeAttr(attr, "stroke", a.stroke);
    writeAttr(attr, "fill", a.fill);
    iStream << "/>\n";
  } else if (tag == "text") {
    iStream << "<text";
    writeAttr(attr, "layer");
    writeAttr(attr, "stroke", a.stroke);
    writeAttr(attr, "matrix");
    writeAttr(attr, "pos");
    String type = attr["type"];
    String pin = attr["pin"];
    if (type == "textbox") {
      type = "minipage";
      pin = "h";
    }
    writeAttr(attr, "type", type);
    writeAttr(attr, "pin", pin);
    if (attr["transformable"] == "yes")
      iStream << " transformations=\"affine\"";
    else
      iStream << " transformations=\"translations\"";
    writeAttr(attr, "width");
    String style = attr["style"];
    if (style == "default")
      style = "normal";
    writeAttr(attr, "style", style);
    writeAttr(attr, "halign");
    writeAttr(attr, "valign");
    writeAttr(attr, "size", a.textsize);
    iStream << ">";
    iStream.putXmlString(pcdata);
    iStream << "</text>\n";
  } else if (tag == "path") {
    iStream << "<path";
    writeAttr(attr, "layer");
    String dash = a.dashStyle;
    if (dash.empty())
      dash = attr["dash"];
    String stroke = a.stroke;
    if (stroke.empty())
      stroke = attr["stroke"];
    if (dash != "void" && stroke != "void") {
      writeAttr(attr, "stroke", a.stroke);
      // in Ipe7 "solid" is "normal" and is the default anyway
      if (dash != "" and dash != "solid")
	iStream << " dash=\"" << dash << "\"";
      writeAttr(attr, "pen", a.pen);
      writeAttr(attr, "cap", a.cap);
      writeAttr(attr, "join", a.join);
    }
    writeAttr(attr, "matrix");
    writeAttr(attr, "pin");
    writeAttr(attr, "arrow");
    String rarrow = attr["backarrow"];
    if (!rarrow.empty())
      iStream << " rarrow=\"" << rarrow << "\"";
    writeAttr(attr, "fill", a.fill);
    writeAttr(attr, "fillrule", a.fillrule);
    iStream << ">";
    iStream.putXmlString(pcdata);
    iStream << "</path>\n";
  } else
    return false;
  return true;
}

// these tags are left unchanged
static const char *const styledefs[] = {
  "titlestyle", "layout", "textstyle",
  "pathstyle", "color", "dashstyle", "textsize",
  "textstretch", "marksize", "arrowsize" };

//! Parse a style sheet.
/*! On calling, stream must be just past the style tag. */
bool Parser::parseStyle()
{
  XmlAttributes att;
  if (!parseAttributes(att))
    return false;

  String tag = parseToTag();

  if (tag == "/ipestyle")  // empty sheet, return without writing anything
    return true;

  iStream << "<ipestyle";
  writeAttr(att);
  iStream << ">\n";

  while (tag != "/ipestyle") {
    if (tag == "bitmap") {
      if (!parseBitmap())
	return false;
    } else if (tag == "template") {
      if (!parseAttributes(att))
	return false;
      iStream << "<symbol";
      writeAttr(att);
      iStream << ">\n";
      String tag1 = parseToTag();
      Attributes a;
      if (!parseObject(tag1, a))
	return false;
      if (parseToTag() != "/template")
	return false;
      iStream << "</symbol>\n";
    } else if (tag == "preamble") {
      if (!parseAttributes(att))
	return false;
      String pcdata;
      if (!att.slash() && !parsePCDATA(tag, pcdata))
	return false;
      writeTag("preamble", att, pcdata);
    } else if (tag == "textmatrix" || tag == "media" ||
	       tag == "margins" || tag == "shading") {
      // Keep old style sheets parsing correctly, but ignore them
      if (!parseAttributes(att) || !att.slash())
	return false;
    } else if (tag == "marksize") {
      if (!parseAttributes(att) || !att.slash())
	return false;
      writeTag("symbolsize", att);
    } else if (tag == "angle") {
      if (!parseAttributes(att) || !att.slash())
	return false;
      writeTag("anglesize", att);
    } else if (tag == "grid") {
      if (!parseAttributes(att) || !att.slash())
	return false;
      writeTag("gridsize", att);
    } else if (tag == "linewidth") {
      if (!parseAttributes(att) || !att.slash())
	return false;
      writeTag("pen", att);
    } else if (tag == "layout") {
      if (!parseAttributes(att) || !att.slash())
	return false;
      if (!iUseCropBox)
	att.add("crop", "no");
      writeTag("layout", att);
    } else {
      // only standard tags remain
      size_t i = 0;
      while (i < (sizeof(styledefs)/sizeof(const char *)) &&
	     tag != styledefs[i])
	++i;
      if (i == (sizeof(styledefs)/sizeof(const char *)))
	return false;
      if (!parseAttributes(att) || !att.slash())
	return false;
      if (tag == "color") {
	if (att["value"] == "black")
	  att.add("value", "0");
	else if (att["value"] == "white")
	  att.add("value", "1");
      }
      writeTag(tag, att);
    }
    tag = parseToTag();
  }
  iStream << "</ipestyle>\n";
  return true;
}

void Parser::writeAttr(const XmlAttributes &att, String s, String grs)
{
  String str = grs;
  if (!str.empty() || att.has(s, str)) {
    iStream << " " << s << "=\"";
    iStream.putXmlString(str);
    iStream << "\"";
  }
}

void Parser::writeAttr(const XmlAttributes &att)
{
  for (XmlAttributes::const_iterator it = att.begin(); it != att.end(); ++it) {
    String name = it->first;
    String value = it->second;
    iStream << " " << name << "=\"";
    iStream.putXmlString(value);
    iStream << "\"";
  }
}

void Parser::writeTag(String tag, const XmlAttributes &att, String data)
{
  iStream << "<" << tag;
  writeAttr(att);
  if (data.empty()) {
    iStream << "/>\n";
  } else {
    iStream << ">";
    iStream.putXmlString(data);
    iStream << "</" << tag << ">\n";
  }
}

// --------------------------------------------------------------------

static void usage()
{
  fprintf(stderr,
	  "Usage: ipe6upgrade <input.xml> [ <output.ipe> ]\n"
	  "Ipe6upgrade reads the XML format generated by any version of"
	  " Ipe 6,\nand writes the XML Format of Ipe 7.0.\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  Platform::initLib(IPELIB_VERSION);

  // ensure two or three arguments
  if (argc != 2 && argc != 3)
    usage();

  const char *src = argv[1];
  String dst;

  if (argc == 3) {
    dst = argv[2];
  } else {
    String s = src;
    if (s.right(4) == ".xml")
      dst = s.left(s.size() - 3) + "ipe";
    else
      dst = s + ".ipe7";
  }

  std::FILE *fd = Platform::fopen(src, "rb");
  if (!fd) {
    std::fprintf(stderr, "Could not open '%s'\n", src);
    exit(1);
  }
  std::FILE *out = Platform::fopen(dst.z(), "wb");
  if (!out) {
    fprintf(stderr, "Could not open '%s' for writing.\n", dst.z());
    exit(3);
  }

  FileSource source(fd);
  FileStream sink(out);
  Parser parser(source, sink);
  int result = parser.parseDocument();
  std::fclose(fd);
  sink.close();
  std::fclose(out);

  if (result == Parser::ENotIpe6) {
    fprintf(stderr, "The input file was not created by a version of Ipe 6.\n");
    exit(1);
  }
  if (result == Parser::ESyntaxError) {
    fprintf(stderr, "Error parsing at position %d\n", parser.parsePosition());
    exit(2);
  }

  return 0;
}

// --------------------------------------------------------------------
