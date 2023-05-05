// --------------------------------------------------------------------
// Standard Ipe style (embedded in Ipelib)
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

#include "ipebase.h"
#include "ipestyle.h"
#include "ipeiml.h"

using namespace ipe;

static const char *styleStandard[] = {
  "<ipestyle>",
  "<color name=\"black\" value=\"0 0 0\"/>",
  "<color name=\"white\" value=\"1 1 1\"/>",
  "<pen name=\"normal\" value=\"0.4\"/>",
  "<dashstyle name=\"normal\" value=\"[]0\"/>",
  "<textsize name=\"normal\" value=\"\\normalsize\"/>",
  "<textstretch name=\"normal\" value=\"1\"/>",
  "<symbolsize name=\"normal\" value=\"3\"/>",
  "<arrowsize name=\"normal\" value=\"7\"/>",
  "<symbol name=\"arrow/normal(spx)\">",
  "<path pen=\"sym-pen\" stroke=\"sym-stroke\" fill=\"sym-stroke\">",
  "0 0 m -1.0 0.333 l -1.0 -0.333 l h</path></symbol>",
  "<layout paper=\"595 842\" origin=\"0 0\" frame=\"595 842\"/>",
  "<titlestyle pos=\"0 800\" size=\"large\" color=\"black\" ",
  "halign=\"left\" valign=\"baseline\"/>\n",
  "<pagenumberstyle pos=\"10 10\" size=\"normal\" color=\"black\"/>",
  "<pathstyle cap=\"0\" join=\"1\" fillrule=\"eofill\"/>",
  "<textstyle name=\"normal\" begin=\"\" end=\"\"/>",
  "<textstyle name=\"normal\" type=\"label\" begin=\"\" end=\"\"/>",
  "<textstyle name=\"math\" type=\"label\" begin=\"$\" end=\"$\"/>",
  "<opacity name=\"opaque\" value=\"1\"/>",
  "<textpad left=\"1\" right=\"1\" top=\"1\" bottom=\"1\"/>",
  "</ipestyle>",
  nullptr };

class StandardStyleSource : public DataSource {
public:
  StandardStyleSource(const char **lines)
    : iLine(lines), iChar(lines[0]) { /* nothing */ }
  int getChar();
private:
  const char **iLine;
  const char *iChar;
};

int StandardStyleSource::getChar()
{
  if (!*iLine)
    return EOF;
  // not yet at end of data
  if (!*iChar) {
    iLine++;
    iChar = *iLine;
    return '\n'; // important: iChar may be 0 now!
  }
  return *iChar++;
}

//! Create standard built-in style sheet.
StyleSheet *StyleSheet::standard()
{
  // ipeDebug("creating standard stylesheet");
  StandardStyleSource source(styleStandard);
  ImlParser parser(source);
  StyleSheet *sheet = parser.parseStyleSheet();
  assert(sheet);
  sheet->iStandard = true;
  sheet->iName = "standard";
  return sheet;
}

// --------------------------------------------------------------------
