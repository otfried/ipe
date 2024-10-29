## Ipe extensible drawing editor

### Introduction

This is the extensible drawing editor Ipe.  Ipe allows you to create
figures in PDF format, for inclusion into LaTeX (or plain TeX)
documents as well as stand-alone PDF documents, for instance to print
transparencies or for on-line presentations.

See the [home page](https://ipe.otfried.org/) or the
[manual](https://otfried.github.io/ipe/) for an introduction.

### Download Ipe

You can try Ipe, without downloading anything,
in the [web edition](https://ipe-web.otfried.org).

#### Windows

A Windows package for Ipe is available on the [home page](https://ipe.otfried.org/). 
You only need to unpack the archive, and you are ready to run.

#### MacOS

If you have homebrew, you can install Ipe by saying
```
brew install --cask ipe
```
You can also download the application directly from the 
[home page](https://ipe.otfried.org/).
Open it, drag `Ipe.app` to your computer, and you are ready to run.

#### Linux

The file "doc/install.txt" explains how to build and install Ipe
on Unix.


### Copyright

Ipe is copyright (c) 1993-2024 Otfried Cheong

Ipe is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3 of the License, or (at your
option) any later version.
	
As a special exception, you have permission to link Ipe with the CGAL
library and distribute executables, as long as you follow the
requirements of the Gnu General Public License in regard to all of the
software in the executable aside from CGAL.

Ipe is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Ipe; if not, you can find it at
"http://www.gnu.org/copyleft/gpl.html", or write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


### Acknowledgements

Ipe uses the Zlib library (www.gzip.org/zlib), the Freetype 2 library
(www.freetype.org), the Cairo library (www.cairographics.org), the
libspiro library (http://github.com/fontforge/libspiro), as well as some code
from Xpdf (www.foolabs.com/xpdf).  Ipe contains the Lua 5.4
interpreter (www.lua.org), and relies on Pdflatex for rendering text,
either locally on your computer or in a cloud service.

This project is tested with [BrowserStack](https://www.browserstack.com/).


### Reporting bugs

Before reporting a bug, check that you have the latest Ipe version,
and check that it is not yet mentioned in the FAQ on the [Ipe
wiki](https://github.com/otfried/ipe-wiki/wiki).

You can report bugs on the [Ipe issue
tracker](https://github.com/otfried/ipe/issues).  Check the existing
reports to see whether your bug has already been reported.  Please
**include your operating system** (Linux, Windows, MacOS, Web) in your report.

Please do not send bug reports directly to me (the first thing I would
do with the report is to enter it into the bug tracking system).


### Getting in contact

Suggestions for features, or random comments on Ipe can be sent to the
Ipe discussion mailing list at <ipe-discuss@lists.science.uu.nl>.  If
you have problems installing or using Ipe and cannot find a guru among
your real-life friends, the Ipe discussion mailing list would also be
the best place to ask.

You can send suggestions or comments directly to me by Email, but you
should then not expect a reply.  I cannot dedicate much time to Ipe,
and the little time I have I prefer to put into development.  I'm much
more likely to get involved in a discussion of desirable features on
the mailing list, where anyone interested can participate, rather than
by direct Email.

If you write interesting ipelets that might be useful to others,
please put a link or copy (as you prefer) on the [Ipe
wiki](https://github.com/otfried/ipe-wiki/wiki).  Feel free to
advertise them on the Ipe discussion list!

Otfried Cheong <ipe@otfried.org>
