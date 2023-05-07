About Ipe files
===============

Ipe 7 creates PDF files.  These files can be used in any way that PDF
files are used, such as viewed with a PDF viewer, edited with a PDF
editor, or included in Latex/Pdflatex documents.  However, Ipe cannot
read arbitrary PDF files, only files it has created itself.  This is
because files created by Ipe contain a special hidden stream that
describes the Ipe objects.  (So if you edit your Ipe-generated PDF
file in a different program such as Adobe Acrobat or MacOS Preview,
Ipe will not be able to read the file again afterwards.)

You decide in what format to store a figure when saving it for the
first time. Ipe gives you the option of saving with extensions ``pdf``
(PDF), and ``ipe`` (XML).  Files saved with extension ``ipe`` are XML
files and contain no PDF information.  The precise XML format used by
Ipe is documented :doc:`later in this manual <90_file_format>`.  XML
files can be read by any XML-aware parser, and it is easy for other
programs to generate XML output to be read by Ipe.  You probably don't
want to keep your figures in XML format, but it is excellent for
communicating with other programs, and for converting figures between
programs.

There are a few interesting uses for Ipe documents:

Figures for Latex documents
---------------------------

Ipe was originally written to make it easy to make figures for Latex
documents.  If you are not familiar with including figures in Latex,
you can find details in the :doc:`92_using_ipe_figures` section.

Presentations
-------------

Ipe is not a presentation tool like Powerpoint or Keynote.  An Ipe
presentation is simply a PDF file that has been created by Ipe, and
which you present during your talk using any PDF reader (for instance
Acrobat Reader).

However, Ipe now comes with a presentation tool IpePresenter to make
presentations more comfortable.  The :doc:`70_presentations` section
explains Ipe features meant for making presentations.

SVG files
---------

Figures in SVG format can be used to include scalable figures in web
pages.  Ipe does not save in SVG format directly, but the tool
:program:`iperender` allows you to convert an Ipe document to SVG format.  This
conversion is one-way, although the auxiliary tool :program:`svgtoipe` also
allows you to convert SVG figures to Ipe format.

Bitmaps
-------

Sometimes Ipe can be useful for creating bitmap images.  Again, the
:program:`iperender` tool can render an Ipe document as a bitmap in
PNG format.
