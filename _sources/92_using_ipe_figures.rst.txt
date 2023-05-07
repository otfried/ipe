Using Ipe figures in Latex
==========================

If---like many Latex users nowadays---you are a user of Pdflatex you
can include Ipe figures in PDF format in your Latex documents
directly.

The standard way of including PDF figures is using the ``graphicx``
package. If you are not familiar with it, here is a quick overview.
In the preamble of your document, add the declaration:

.. code-block::

  \usepackage{graphicx}

One useful attribute to this declaration is ``draft``, which stops
LaTeX from actually including the figures---instead, a rectangle
with the figure filename is shown:

.. code-block::

  \usepackage[draft]{graphicx}

To include the figure :file:`figure1.pdf`, you use the command:

.. code-block::

  \includegraphics{figs/figure1}

Note that it is common *not* to specify the file extension ``.pdf``.
The command ``\includegraphics`` has various options to scale and
rotate the figure.  For instance, to scale the same figure to 50%,
use:

.. code-block::

  \includegraphics[scale=0.5]{figs/figure1}

To scale such that the width of the figure becomes 5 cm:

.. code-block::

  \includegraphics[width=5cm]{figs/figure1}

Instead, one can specify the required height with ``height``.

Here is an example that scales a figure to 200% and rotates it by
45 degrees counter-clockwise.  Note that the scale argument should be
given *before* the ``angle`` argument.

.. code-block::

  \includegraphics[scale=2,angle=45]{figs/figure1}

Let's stress once again that these commands are the standard commands
for including PDF figures in a LaTeX document.  Ipe files neither
require nor support any special treatment.  If you want to know more
about the LaTeX packages for including graphics and producing
colour, check the :file:`grfguide.tex` document that is probably
somewhere in your TeX installation.

Bounding boxes
^^^^^^^^^^^^^^

There is a slight complication here: Each page of a PDF document can
carry several *bounding boxes*, such as the *MediaBox* (which
indicates the paper size), the *CropBox* (which indicates how the
paper will by cut), or the *ArtBox* (which indicates the extent of the
actual contents of the page).  Ipe automatically saves, for each page,
the paper size in the *MediaBox*, and a bounding box for the drawing
in the *ArtBox*.  Ipe also puts the bounding box in the *CropBox*
unless this has been turned off by the stylesheet.

Now, when including a PDF figure, Pdflatex will (by default) first
look at the CropBox, and, if that is not set, fall back on the
MediaBox.  It does not inspect the ArtBox, and so it is important that
you use the correct stylesheet for the kind of figure you are
making---with cropping for figures to be included, without cropping
for presentations (as otherwise Acrobat Reader will not display full
pages---Acrobat Reader actually crops each page to the CropBox).

If you have a recent version of Pdflatex (1.40 or higher), you can
actually ask Pdflatex to inspect the ArtBox by saying ``\pdfpagebox5``
in your Latex file's preamble.

Classic LaTeX and EPS
^^^^^^^^^^^^^^^^^^^^^

If you are still using the "original" Latex, which compiles documents
to DVI format, you need figures in Encapsulated Postscript (EPS)
format (the "Encapsulated" means that there is only a single
Postscript page and that it contains a bounding box of the figure).
Some publishers may also require that you submit figures in EPS
format, rather than in PDF.

Ipe allows you to export your figure in EPS format, either from the
Ipe program (*File* menu, *Export as EPS*), or by using the command
line tool :ref:`iperender <iperender>` with the ``-eps`` option.
Remember to keep a copy of your original Ipe figure!  Ipe cannot read
the exported EPS figure, you will not be able to edit them any
further.

Including EPS figures works exactly like for PDF figures, using
``\includegraphics``.  In fact you can save all your figures in both
EPS and PDF format, so that you can run both Latex and Pdflatex on
your document---when including figures, Latex will look for the EPS
variant, while Pdflatex will look for the PDF variant. (Here it comes
in handy that you didn't specify the file extension in the
``\includegraphics`` command.)

It would be cumbersome to have to export to EPS every time you modify
and save an Ipe figure in PDF format.  What you should do instead is
to write a shell script or batch file that calls :ref:`iperender
<iperender>` to export to EPS.


All figures in one Ipe document
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On the other hand, if you *only* use Pdflatex, you might opt to
exploit a feature of Pdflatex: You can keep all the figures for a
document in a single, multi-page Ipe document, with one figure per
page.  You can then include the figures one by one into your document
by using the ``page`` argument of ``\includegraphics``.

For example, to include page 3 from the PDF file :file:`figures.pdf`
containing several figures, you could use

.. code-block::

  \includegraphics[page=3]{figures}

It's a bit annoying that one has to refer to the page by its page
number.  Ipe comes with a useful script that will allow you to use
:ref:`symbolic names <multiple>` instead.

