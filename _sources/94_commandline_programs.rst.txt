The command line programs
=========================

Ipe
---

Ipe supports the following command line options:

:samp:`-sheet {style_sheet_name}`
  Adds the designated style sheet to any newly created documents.
  
``-show-configuration``
  With this option, Ipe will display the current configuration options
  on stdout, and terminate.

In addition, you can specify the name of an Ipe file to open on the
command line.  


Ipetoipe: converting Ipe file formats
-------------------------------------

The auxiliary program :program:`ipetoipe` converts between the different
Ipe file formats:

.. code-block::

  ipetoipe ( -xml | -pdf ) { <options> } infile [ outfile ]

The first argument determines the format of the output file.  If no
output filename is provided, Ipe will try to guess it by appending one
of the extensions ``ipe`` or ``pdf`` to the input file's basename.

For example, the command line syntax

.. code-block::

   ipetoipe -pdf figure1.ipe

converts :file:`figure1.ipe` to :file:`figure1.pdf`.

Ipetoipe understands the following options:

``-export``
  No Ipe markup is included in the resulting
  output file.  Ipe will not be able to open a file created that way,
  so make sure you keep your original!

``-markedview`` (PDF only)
  Only the marked views of marked Ipe pages will be created in PDF
  format.  If all views of a marked page are unmarked, the last view
  is exported. This is convenient to make handouts for slides.

:samp:`-pages {from-to}` (PDF only)
  Restrict exporting to PDF to this page range.  This implies  the
  ``-export`` option.

:samp:`-view {page-view}`
  Only export this single view from the  document.   This  implies
  the ``-export`` option.

``-runlatex``
  Run  Latex even for XML output. This has the effect of including
  the dimensions of each text object in the XML file.

``-nozip``
  Do not compress streams in PDF output.

.. _iperender:  

Iperender: exporting to a bitmap, EPS, or SVG
---------------------------------------------

The program :program:`iperender` exports a page of the document to a
bitmap in PNG format, to a figure in Encapsulated Postscript (EPS), or
to scalable vector graphics in SVG format.  (Of course the result
contains no Ipe markup, so make sure you keep your original!)  For
instance, the following command line

.. code-block::
   
  iperender -png -page 3 -resolution 150 presentation.pdf pres3.png

converts page 3 of the Ipe document :file:`presentation.pdf` to a
bitmap, with resolution 150 pixels per inch.

Ipescript: running Ipe scripts
------------------------------

Ipescript runs an Ipe script written in the Lua language with bindings
for the Ipe objects, such as the script *update-master*.  Ipescript
automatically finds the script in Ipe's script directories.  On Unix,
you can place your own scripts in :file:`~/.ipe/scripts`.

The Ipe distribution contains the following scripts:

* *update-master*, explained :ref:`earlier <sharing>`;

* *page-labels*, explained :ref:`earlier <multiple>`;
  
* *onepage* collapses pages of an Ipe document into layers of a single
  page;
  
* *add-style* to add a stylesheet to Ipe figures;
  
* *update-styles* to update the stylesheets in Ipe figures (in the
  same way that Ipe does it using the *Update stylesheets* function).


Ipeextract: extract XML stream from Ipe file
--------------------------------------------

Ipeextract extracts the XML stream from an PDF or EPS file made by
Ipe 6 or 7 and saves it in a file.  It will work even if Ipe cannot
actually parse the file, so you can use this tool to debug problems
where Ipe fails to open your document.

.. code-block::

   ipeextract infile [ outfile ]

If not provided, the outfile is guessed by appending ``xml`` to the
infile's basename.


Ipe6upgrade: convert Ipe 6 files to Ipe 7 file format
-----------------------------------------------------

Ipe6upgrade takes as input a file created by any version of Ipe 6, and
saves in the format of Ipe 7.0.0.

.. code-block::

   ipe6upgrade infile [ outfile ]

If not provided, the outfile is guessed by adding the extension
``ipe`` to the infile's basename.

To reuse an Ipe 6 document in EPS or PDF format, you first run
:program:`ipeextract`, which extracts the XML stream inside the
document and saves it as an XML file.  The Ipe 6 XML document can then
be converted to Ipe 7 format using :program:`ipe6upgrade`.

If your old figure is :file:`figure.pdf`, then the command

.. code-block::

   ipeextract figure.pdf

will save the XML stream as :file:`figure.xml`.  Then run 

.. code-block::

   ipe6upgrade figure.xml

which will save your document in Ipe 7 format as :file:`figure.ipe`.
All contents of the original document should have been preserved.


Importing other formats
-----------------------

Svgtoipe: Importing SVG figures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The auxiliary program :program:`svgtoipe` converts an SVG figure to Ipe
format. It cannot handle all SVG features (many SVG features are not
supported by Ipe anyway), but it works for gradients.

:program:`svgtoipe` is not part of the Ipe source distribution. You can
download it separately.


Pdftoipe: Importing Postscript and PDF
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can convert arbitrary Postscript or PDF files into Ipe documents,
making them editable.  The auxiliary program :program:`pdftoipe`
converts (pages from) a PDF file into an Ipe XML-file. (If your source
is Postscript, you have to first convert it to PDF using Acrobat
Distiller or :command:`ps2pdf`.)  Once converted to XML, the file can
be opened from Ipe as usual.

The conversion process handles many kinds of graphics in the PDF file
fine, but doesn't do very well on text---Ipe's text model is just too
different.

:program:`pdftoipe` is not part of the Ipe source distribution. You can
download and build it separately.


Ipe5toxml: convert Ipe 5 files to Ipe 6 file format
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you still have figures that were created with Ipe 5, you can use
:program:`ipe5toxml` to convert them to Ipe 6 format.  You can then use
:program:`ipe6upgrade` to convert them to Ipe 7 format.

:program:`ipe5toxml` is not part of the Ipe distribution, but available as
a separate download.


Figtoipe: Importing FIG figures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The auxiliary program :program:`figtoipe` converts a figure in FIG format
into an Ipe XML-file.  This is useful if you used to make figures with
Xfig before discovering Ipe, of if your co-authors made figures for
your article with Xfig (converting them will have the added benefit of
forcing your co-authors to learn to use Ipe).  Finally, there are
quite a number of programs that can export to FIG format, and
:program:`figtoipe` effectively turns that into the possibility of
exporting to Ipe.

However, :program:`figtoipe` is not quite complete.  The drawing models of
FIG and Ipe are also somewhat different, which makes it impossible to
properly render some FIG files in Ipe.  In particular, Ipe does not
support FIG's interpolating splines, depth ordering independent of
grouping, pattern fill, and Postscript fonts.

You may therefore have to edit the file after conversion.

:program:`figtoipe` is not part of the Ipe distribution. You can download
and build it separately. :program:`figtoipe` is now maintained by
Alexander Bürger.

