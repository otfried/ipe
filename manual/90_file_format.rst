.. |bezier| replace:: Bézier

The Ipe file format
===================

Ipe can store documents in two different formats.  One of them is
standard PDF, which can be read by any application capable of opening
PDF files.  (Ipe embeds its own information inside PDF files.  The way
this is done is not documented here, and may change between releases
of Ipe.)

The second Ipe file format is a pure XML implementation.  Files stored
in this format can be parsed with any XML-aware application, and you
can create XML files for Ipe from your own applications.

A DTD for the Ipe format is available as `ipe.dtd
<https://github.com/otfried/ipe/raw/master/doc/ipe.dtd>`_. For
instance, you can use this to validate an Ipe document using

.. code-block::

  xmllint --dtdvalid ipe.dtd --noout file.ipe

or

.. code-block::

  xmlstarlet val -d ipe.dtd file.ipe


The tags understood by Ipe are described informally in this section.

Ipe does not use a full XML-parser, in particular it does not
understand namespaces and doesn't allow certain elements that have no
contents to be expanded (you cannot, for instance, write
``<info></info>`` instead of ``<info />``).

Tags in the XML file can carry attributes other than the ones
documented here.  Ipe ignores all attributes it doesn't understand, and
they will be lost if the document is saved again from Ipe.  Ipe will
complain about any XML elements not described here, with the exception
that you can use elements whose name starts with ``x-`` freely to add
your own information inside an Ipe file.

An Ipe XML file must contain exactly one ``<ipe>`` element, while
an Ipe stylesheet file must contain exactly one ``<ipestyle>``
element (both types of files are allowed to start with an
``<?xml>`` tag, which is simply ignored by Ipe).  An Ipe file may
also contain a ``<!DOCTYPE>`` tag.
 
All elements are documented below.

The ``<ipe>`` element
---------------------

*Attributes*

``version`` (required)

  The value (a number, e.g. 70103 for Ipelib 7.1.3) indicates the
  earliest Ipelib version that can interpret the document. Ipe will
  refuse to load documents that require a version larger than its own,
  and may refuse to load documents that are too old (and which will
  have to be converted using a separate program).
  
``creator`` (optional)

  indicates the program that created the file.  This information is
  not interpreted by Ipe at all.

*Contents*

#. An ``<info>`` element (optional),
#. a ``<preamble>`` element (optional),
#. a series of ``<bitmap>`` and ``<ipestyle>`` elements (optional),
#. a series of ``page`` elements.

The ``<ipestyle>`` elements form a *cascade*, with the *last*
``<ipestyle>`` element becoming the *top-level* style sheet.  When
symbolic names are looked up, the style sheets are checked from top to
bottom.  Ipe always appends the built-in standard style sheet at the
bottom of the stack.


The ``<info>`` element
^^^^^^^^^^^^^^^^^^^^^^

*Attributes*

``title`` (optional)
  document title,

``author`` (optional)
  document author,
  
``subject`` (optional)
  document subject,

``keywords`` (optional)
  document keywords,

``language`` (optional)
  the document language, to be used for spell checking.  E.g. `de_DE`
  for German spell checking.  Currently has an effect only on Linux
  when spell checking is compiled in.

``pagemode`` (optional)
  the only value understood by Ipe is
  ``fullscreen``, which causes the document to be opened in full
  screen mode in PDF readers.

``created`` (optional)
  creation time in PDF format, e.g. ``D:20030127204100``.
  
``modified`` (optional)
  modification time in PDF format,
  
``numberpages`` (optional)
  if the value is ``yes``, then Ipe will save PDF documents with
  visible page numbers on each page.
  
``sequentialtext`` (optional)
  if the value is ``yes``, then Ipe will typeset all text objects one
  by one, even if they are identical, in the order in which they
  appear in the document.
  
``tex`` (optional)
  determines the TeX-engine used to translate your text.  The
  possible values are ``pdftex``, ``xetex``, and ``luatex``. 

This element must be empty.


The ``<preamble>`` element
^^^^^^^^^^^^^^^^^^^^^^^^^^

The contents of this element is LaTeX source code, to be used as
the LaTeX preamble when running LaTeX to process the text
objects in the document. It should *not* contain a
``\documentclass`` command, but can contain ``\usepackage``
commands and macro definitions.

If the preamble starts with `%&`, then the first line of the preamble
will be placed at the **top** of the LaTeX source that Ipe produces to
process text objects.


The ``<bitmap>`` element
^^^^^^^^^^^^^^^^^^^^^^^^

Each ``<bitmap>`` element defines a bitmap to be used by
``<image>`` objects. 

Attributes


``id`` (required)
  the value must be an integer that will define the bitmap throughout
  the Ipe document,
  
``width`` (required)
  integer width in pixels,

``height`` (required)
  integer height in pixels,

``ColorSpace`` (optional)
  possible values are ``DeviceGray``, ``DeviceGrayAlpha``, ``DeviceRGB`` (default value), and
  ``DeviceRGBAlpha``. The suffix ``Alpha`` indicates the presence of
  an alpha channel. 

``ColorKey`` (optional)
  an RGB color in hexadecimal, indicating the
  transparent color (not supported for JPEG images and for images with
  alpha channel),

``length`` (required unless there is no filter and no alpha channel)
  the number of bytes of image data,

``Filter`` (optional)
  possible values are ``FlateDecode`` or
  ``DCTDecode`` to indicate a compressed image (the latter is used for
  JPEG images),

``encoding`` (optional)
  possible value is ``base64`` to indicate
  that the image data is base64-encoded (not in hexadecimal),

``alphaLength`` (optional)
  indicates that the alpha channel is
  provided separately.


The contents of the ``<bitmap>`` element is the image data, either
base64-encoded or in hexadecimal format.  White space between bytes is
ignored.  If no filter is specified, pixels are stored row by row.

If ``alphaLength`` present, then the alpha channel follows the
image data.  If the data is deflated, image data and alpha channel are
deflated separately.  If no ``alphaLength`` is present, then the
alpha component is part of each pixel before the color components.

Bitmaps use 8-bit color and alpha components.  Bitmaps with color maps
or with a different number of bits per component are not supported,
and such support is not planned. (The *Insert image* function
does allow you to insert arbitrary image formats, but they are stored
as 8-bit per component images.  Since the data is compressed, this
does not seriously increase the image data size.)

The ``<page>`` element
----------------------

*Attributes*

``title`` (optional)
  title of this page (displayed at a fixed
  location in a format specified by the style sheet),

``section`` (optional)
  Title of document section starting with this
  page. If the attribute is not present, this page continues the
  section of the previous page.  If the attribute is present, but its
  value is an empty string, then the contents of the ``title``
  attribute is used instead.

``subsection`` (optional)
  Title of document subsection starting
  with this page. If the attribute is not present, this page continues
  the subsection of the previous page.  If the attribute is present,
  but its value is an empty string, then the contents of the
  ``title`` attribute is used instead.

``marked`` (optional)
  The page is marked for printing unless the
  value of this attribute is ``no``.

*Contents*

#. An optional ``<notes>`` element,
#. a possibly empty sequence of ``<layer>`` elements,
#. a possibly empty sequence of ``<view>`` elements,
#. a possibly empty sequence of Ipe object elements.

If a page contains no layer element, Ipe automatically adds a default
layer named ``alpha``, visible and editable. 

If a page contains no view element, a single view where all layers are
visible is assumed.


The ``<notes>`` element
^^^^^^^^^^^^^^^^^^^^^^^

This element has no attributes.  Its contents is plain text,
containing notes for this page.


The ``<layer>`` element
^^^^^^^^^^^^^^^^^^^^^^^

*Attributes*

``name`` (required)
  Name of the layer.  It must not contain white
  space.

``edit`` (optional)
  The value should be ``yes`` or
  ``no`` and indicates whether the user can select and modify the
  contents of the layer in the Ipe user interface (of course the user
  can always modify the setting of the attribute).

``snap`` (optional)
  The value should be ``never``,
  ``visible``, or ``always``, and indicates whether snapping to
  this layer is enabled.  The default is ``visible``.

``data`` (optional)
  A free-use string associated with the
  layer. Ipe makes no use of this string, except for layers it creates
  itself for group edits.

The layer element must be empty.


The ``<view>`` element
^^^^^^^^^^^^^^^^^^^^^^

*Attributes*

``layers`` (required)
  The value must be a sequence of layer names
  defined in this page, separated by white space.

``active`` (required)
  The layer that is the active layer in this view.

``effect`` (optional)
  The symbolic name of a graphics effect to be
  used during the PDF page transition.  The effect must be defined in
  the style sheet.

``name`` (optional)
  The name of the view.

``marked`` (optional)
  The view is marked for printing if the value
  of this attribute is ``yes``.

The view element may be empty, or it may contain a sequence of
attribute mappings and layer transformations.

An attribute mapping is a ``<map>`` element with three attributes

``kind``, ``from``, and ``to``. The ``kind`` attribute must have one
of the following values:

.. code-block::

  pen, symbolsize, arrowsize, opacity, color, dashstyle, symbol

The attributes ``from`` and ``to`` must both be names of symbolic
attribute values.  The effect is that in this view, the symbolic
attribute ``from`` is replaced by the symbolic attribute ``to`` for
attributes of the given kind.

A layer transformation is a ``<transform>`` element. It must have two
attributes: ``layer`` is the name of a layer of the page, ``matrix``
is a transformation matrix.  In the view, all objects on that layer
are transformed with this matrix.


Ipe object elements
-------------------

Common attributes
^^^^^^^^^^^^^^^^^

``layer`` (optional)
  Only allowed on ``top-level`` objects, that
  is, objects directly inside a ``<page>`` element.  The value
  indicates into which layer the object goes.  If the attribute is
  missing, the object goes into the same layer as the preceding
  object.  If the first object has no layer attribute, it goes into
  the layer defined first in the page, or the default ``alpha`` layer.

``matrix`` (optional)
  A sequence of six real numbers,
  separated by white space, indicating a transformation matrix for all
  coordinates inside the element (including embedded elements if this is
  a ``<group>`` element).  A missing ``matrix`` attribute is
  interpreted as the identity matrix.

``pin`` (optional)
  Possible values are ``yes`` (object is fixed
  on the page), ``h`` (object is pinned horizontally, but can move
  in the vertical direction), and ``v`` (the opposite).  The
  default is no pinning.

``transformations`` (optional)
  This attribute determines how
  objects can be deformed by transformations. Possible values are
  *affine* (the default), *rigid*, and *translations*.


Color attribute values
^^^^^^^^^^^^^^^^^^^^^^

A color attribute value is either a symbolic name defined in one of
the style sheets of the document, one of the predefined names

``black`` or ``white``, a single real number between :math:`0` (black) and
:math:`1` (white) indicating a gray level, or three real numbers in the
range :math:`[0,1]` indicating the red, green, and blue component (in this
order), separated by white space.


Path construction operators
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Graphical shapes in Ipe are described using a series of ``path
construction operators`` with arguments. This generalizes the PDF path
construction syntax.

Each operator follows its arguments.  The operators are

``m`` (moveto) (1 point argument):
  begin new subpath.

``l`` (lineto) (1 point argument):
  add straight segment to subpath.

``c`` (cubic B-spline) (:math:`n` point arguments): 
  add a uniform cubic B-spline with :math:`n+1` control points (the current
  position plus the $n$ arguments).  If :math:`n = 3`, this is equivalent to
  a single cubic |bezier| spline, if :math:`n = 2` it is equivalent to a
  single quadratic |bezier| spline.

``q`` (deprecated) (2 point arguments):
  identical to 'c'.

``e`` (ellipse) (1 matrix argument):
  add a closed subpath
  consisting of an ellipse, the ellipse is the image of the unit
  circle under the transformation described by the matrix.

``a`` (arcto) (1 matrix argument, 1 point argument):
  add an elliptic arc, on the ellipse described by the matrix, from current
  position to given point.

``s`` (deprecated) (:math:`n` point arguments):
  add an "old style" uniform cubic B-spline as used by Ipe up to version 7.1.6.

``C`` (:math:`n` point arguments plus one tension argument):
  add a cardinal cubic spline through the given points and the given
  tension. (The definition of the tension in the literature
  varies.  Ipe's tangent is the factor by which the vector from
  previous to next point is multiplied to obtain the tangent vector at
  this point. So 0.5 corresponds to the Catmull-Rom spline.)

``L`` (:math:`n` point arguments):
  add a clothoid spline as computed by the libspiro library by Raph Levien.
  When Ipe writes clothoid splines, it includes the control points of
  the computed Bezier approximation in the path description, separated
  from the defining points by a ``*``.

``u`` (closed spline) (:math:`n` point arguments):
  add a closed subpath consisting of a closed uniform B-spline with :math:`n` control
  points,

``h`` (closepath) (no arguments):
  close the current subpath.
  No more segments can be added to this subpath, so the next operator
  (if there is one) must start a new subpath.

A *point* argument is a pair of :math:`x` and :math:`y` coordinates.

A *matrix* argument is a sequence of 6 numbers :math:`a, b, c, d, s, t`.
They describe the affine transformation

  | :math:`x' = a x + c y + s`
  | :math:`y' = b x + d y + t`

Note that you write the matrix column by column!

Paths consisting of more than one closed loop are allowed.  A subpath
can consist of any mix of straight segments, elliptic arcs, and
B-splines.


The ``<group>`` element
^^^^^^^^^^^^^^^^^^^^^^^

The ``<group>`` element allows to group objects together, so that
they appear as one in the user interface.  

*Attributes*

``clip`` (optional)
  The value is a sequence of path construction
  operators, forming a clipping path for the objects inside the group.

``url`` (optional)
  The value is a link action (and the attribute
  name is somewhat of a misnomer, as actions do not need to be
  URLs---see the description of group objects).

``decoration`` (optional)
  The name of a decoration symbol.  The
  default is *normal*, meaning no decoration.


The contents of the ``<group>`` element is a series of Ipe object
elements.


The ``<image>`` element
^^^^^^^^^^^^^^^^^^^^^^^

*Attributes*

``bitmap`` (required)
  Value is an integer referring
  to a bitmap defined in a ``<bitmap>`` element in the document,

``rect`` (required)
  Four real coordinates separated by white space,
  in the order :math:`x_{1}, y_{1}, x_{2}, y_{2}`, indicating two
  opposite corners of the image in Ipe coordinates).

``opacity`` (optional)
  Opacity of the image.  This must be a
  symbolic name. The default is ``normal``, meaning fully opaque.


The image element is normally empty.  However, it is allowed to omit
the ``bitmap`` attribute.  In this case, the ``<image>`` must
carry all the attributes of the ``<bitmap>`` element, with the
exception of ``id``.  The element contents is then the bitmap data,
as described for ``<bitmap>``.


The ``<use>`` element
^^^^^^^^^^^^^^^^^^^^^

The ``<use>`` element refers to a symbol (an Ipe object) defined in
the style sheet.  The attributes ``stroke``, ``fill``, ``pen``, and
``size`` make sense only when the symbol accepts these parameters.


*Attributes*

``name`` (required)
  The name of a ``symbol`` defined in a style
  sheet of the document.

``pos`` (optional)
  Position of the symbol on the page (two real
  numbers, separated by white space).  This is the location of the
  origin of the symbol coordinate system.  The default is the origin.

``stroke`` (optional)
  A stroke color (used whereever the symbol
  uses the symbolic color ``sym-stroke``).  The default is black.

``fill`` (optional)
  A fill color (used whereever the symbol uses
  the symbolic color ``sym-fill``).  The default is white.

``pen`` (optional)
  A line width (used whereever the symbol uses
  the symbolic value ``sym-pen``).  The default is ``normal``.

``size`` (optional)
  The size of the symbol, either a symbolic size
  (of type ``symbol size``), or an absolute scaling factor.  The
  default is :math:`1.0`.

The ``<use>`` element must be empty.


The ``<text>`` element
^^^^^^^^^^^^^^^^^^^^^^

*Attributes*

``stroke`` (optional)
  The stroke color. If the attribute is
  missing, black will be used.

``type`` (optional)
  Possible values are *label* (the default)
  and *minipage*.

``size`` (optional)
  The font size---either a symbolic name defined
  in a style sheet, or a real number.  The default is ``normal``.

``pos`` (required)
  Two real numbers separated by white space,
  defining the position of the text on the paper.

``width`` (required for minipage objects, optional for label objects)
  The width of the object in points.

``height`` (optional)
  The total height of the object in points.

``depth`` (optional)
  The depth of the object in points.

``valign`` (optional)
  Possible values are *top* (default
  for a minipage object), *bottom* (default for a label object),
  *center*, and *baseline*.

``halign`` (optional)
  Possible values are *left*,
  *right*, and *center*. *left* is the default.  This
  determines the position of the reference point with respect to the
  text box.

``style`` (optional)
  Selects a LaTeX ``style`` to be used for
  formatting the text, and must be a symbolic name defined in a style
  sheet.  There are separate definitions for minipages and for labels.
  For minipages, the standard style sheet defines the styles
  ``normal``, ``center``, ``itemize``, and ``item``.  If the attribute
  is not present, the ``normal`` style is applied.

``opacity`` (optional)
  Opacity of the element.  This must be a
  symbolic name. The default is ``normal``, meaning fully opaque.

The dimensions are recomputed by Ipe when running LaTeX, with the
exception of ``width`` for minipage objects whose width is fixed.

The contents of the ``<text>`` element must be a legal LaTeX fragment
that can be interpreted by LaTeX inside ``\hbox``, possibly using the
macros or packages defined in the preamble.


The ``<path>`` element
^^^^^^^^^^^^^^^^^^^^^^

*Attributes*

``stroke`` (optional)
  The stroke color. If the attribute is
  missing, the shape will not be stroked.

``fill`` (optional)
  The fill color.  If the attribute is missing,
  the shape will not be filled.

``dash`` (optional)
  Either a symbolic name defined in a style
  sheet, or a dash pattern in PDF format, such as ``[3 1] 0`` for
  ``three pixels on, one off, starting with the first pixel``.
  If the attribute is missing, a solid line is drawn.

``pen`` (optional)
  The line width, either symbolic (defined in
  a style sheet), or as a single real number.  The default value is
  ``normal``. 

``cap`` (optional)
  The *line cap* setting of PDF as an
  integer.  If the argument is missing, the setting from the style
  sheet is used.

``join`` (optional)
  The *line join* setting of PDF as an
  integer. If the argument is missing, the setting from the style
  sheet is used.

``fillrule`` (optional)
  Possible values are ``wind`` and
  ``eofill``, selecting one of two algorithms for determining
  whether a point lies inside a filled object. If the argument is
  missing, the setting from the style sheet is used.

``arrow`` (optional)
  The value consists of a symbolic name, say
  ``triangle`` for an arrow type (a symbol with name
  ``arrow/triangle(spx)``), followed by a slash and the size of the arrow.
  The size is either a symbolic name (of type ``arrowsize``) defined
  in a style sheet, or a real number.  If the attribute is missing, no
  arrow is drawn.

``rarrow`` (optional)
  Same for an arrow in the reverse direction
  (at the beginning of the first subpath).

``opacity`` (optional)
  Opacity of the element.  This must be a
  symbolic name. The default is ``normal``, meaning fully opaque.

``stroke-opacity`` (optional)
  Opacity of the stroked part of the
  element.  This must be a symbolic name. The default is to use the
  opacity attribute.

``tiling`` (optional)
  A tiling pattern to be used to fill the
  element.  The default is not to tile the element.  If the element is
  not filled, then the tiling pattern is ignored.

``gradient`` (optional)
  A gradient pattern to be used to fill the
  element.  If the element is not filled, then the gradient pattern is
  ignored.  If ``gradient`` is set, then ``tiling`` is ignored.


The contents of the ``<path>`` element is a sequence of path
construction operators. The entire shape will be stroked and/or filled
with a single stroke and fill operation.

.. _ipestyle:

The ``<ipestyle>`` element
--------------------------

*Attributes*

``name`` (optional)
  The name serves to identify the style sheet
  informally, and can be used to automatically update the style sheet
  from a file with the matching name.

The contents of the ``<ipestyle>`` element is a series of style
definition elements, in no particular order.  These elements are
described below.


The ``<symbol>`` element
^^^^^^^^^^^^^^^^^^^^^^^^

*Attributes*

``name`` (required)
  The name identifies the symbol and must
  be unique in the style sheet.  For parameterized symbols, the name
  must end with the pattern ``(s?f?p?x?)``, where ``s`` stands for
  stroke, ``f`` for fill, ``p`` for pen, and ``x`` for size.

``transformations`` (optional)
  As for objects.

``xform`` (optional)
  If this attribute is set, a PDF XForm will be
  created for this symbol when saving or exporting to PDF. It implies
  ``transformations="translations"``, and will be ignored if 
  any of the symbol parameters (that is, stroke, fill, pen, or size)
  are used.  Setting this attribute will cause the PDF output to be
  significantly smaller for a complicated symbol that is used often
  (for instance, a complicated background used on every page).

``snap`` (optional)
  A list of space-separated coordinates for the
  snap positions of the symbol.


The contents of the ``<symbol>`` element is a single Ipe object.

The ``<preamble>`` element
^^^^^^^^^^^^^^^^^^^^^^^^^^

See the ``<preamble>`` elements inside ``<ipe>`` elements.

The ``<textstyle>`` element
^^^^^^^^^^^^^^^^^^^^^^^^^^^

*Attributes*

``name`` (required)
  The symbolic name (to be used in the ``style``
  attribute of ``<text>`` elements),

``begin`` (required)
  LaTeX code to be placed before the text of
  the object when it is formatted,

``end`` (required)
  LaTeX code to be placed after the text of
  the object when it is formatted.

``type`` (optional)
  Either ``label`` or ``minipage`` (the default).


The ``<layout>`` element
^^^^^^^^^^^^^^^^^^^^^^^^

It defines the layout of the frame on the paper and the paper size. 

*Attributes*

``paper`` (required)
  The size of the paper.

``origin`` (required)
  The lower left corner of the frame
  in the paper coordinate system.

``frame`` (required)
  The size of the frame.

``skip`` (optional)
  The default paragraph skip between textboxes.

``crop`` (optional)
  If the value of ``crop`` is ``yes``, Ipe
  will create a ``CropBox`` attribute when saving to PDF.  


The ``<titlestyle>`` element
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It defines the appearance of the page title on the page.

*Attributes*

``pos`` (required)
  The position of the title reference point in the
  frame coordinate system.

``color`` (required)
   The color of the title.

``size`` (required)
  The title font size (same as for ``<text>``
  elements). 

``halign`` (optional)
  The horizontal alignment (same as for ``<text>``
  elements). 

``valign`` (optional)
  The vertical alignment (same as for ``<text>``
  elements). 


The ``<pagenumberstyle>`` element
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It defines the appearance of page numbers on the page.  The contents
of the element is a :ref:`template for a text object
<pagenumberstyle>`.

*Attributes*

``pos`` (required)
  The position of the page number on the page.

``color`` (optional)
   The color of the page number.  The default is black.

``size`` (optional)
  The font size (same as for ``<text>``
  elements).  The default is ``normal``.

``halign`` (optional)
  The horizontal alignment (same as for ``<text>``
  elements). 

``valign`` (optional)
  The vertical alignment (same as for ``<text>``
  elements). 


The ``<textpad>`` element
^^^^^^^^^^^^^^^^^^^^^^^^^

It defines padding around text objects for the computation of bounding
boxes.  The four required attributes are ``left``,

``right``, ``top``, and  ``bottom``.


The ``<pathstyle>`` element
^^^^^^^^^^^^^^^^^^^^^^^^^^^

It defines the default setting for path objects.

*Attributes*

``cap`` (optional)
  Same as for ``<path>`` elements.

``join`` (optional)
  Same as for ``<path>`` elements.

``fillrule`` (optional)
  Same as for ``<path>`` elements.


The ``<opacity>`` element
^^^^^^^^^^^^^^^^^^^^^^^^^

The ``opacity`` element defines a possible opacity value (also
known as an alpha-value).  All opacity values used in a document must
be defined in the style sheet.

*Attributes*

``name`` (required)
  A symbolic name, to be used in the
  ``opacity`` attribute of a ``text`` or ``path`` element. 

``value`` (required)
  An absolute value for the opacity, between
  0.001 and 1.000. A value of 1.0 implies that the element is fully
  opaque.


.. _gradient-element:

The ``<gradient>`` element
^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``gradient`` element defines a gradient pattern.

*Attributes of* ``<gradient>``

``name`` (required)
  The symbolic name (to be used in the ``gradient`` attribute of ``<path>`` elements).

``type`` (required)
  Possible values are ``axial`` and ``radial``.

``extend`` (optional)
  ``yes`` or ``no`` (the default).
  Indicates whether the gradient is extended beyond the boundaries.

``coords`` (required)
  For axial shading: the coordinates of the endpoints of the axis (in
  the order ``x1 y1 x2 y2``). For radial shading: the center and
  radius of both circles (in the order ``cx1 cy1 r1 cx2 cy2 r2``).

``matrix`` (optional)
  A transformation that transforms the gradient
  coordinate system into the coordinate system of the path object
  using the gradient. The default is the identity matrix.

The contents of the ``<gradient>`` element are ``<stop>``
elements defining the color stops of the gradient.  There must be at
least two stops.  Stops must be defined in increasing offset order.
It is not necessary that the first offset is 0.0 and the last one
is 1.0.

*Attributes of* ``<stop>``

``offset`` (required)
  Offset of the color stop (a number between
  0.0 and 1.0).

``color`` (required)
  Color at this color stop (three
  numbers). Symbolic names are not allowed.


The ``<tiling>`` element
^^^^^^^^^^^^^^^^^^^^^^^^

The ``tiling`` element defines a tiling pattern.  Only very simple
patterns that hatch the area with a line are supported.

*Attributes*

``name`` (required)
  The symbolic name (to be used in the ``tiling`` attribute of ``<path>`` elements).

``angle`` (required)
  Slope of the hatching line in degrees, between
  -90 and +90 degrees.

``width`` (required)
  Width of the hatching line.

``step`` (required)
  Distance from one hatching line to the next.

Here, ``width`` and ``step`` are measured in the *y*-direction if the
absolute value of ``angle`` is less than 45 degrees, and in the
*x*-direction otherwise.


The ``<effect>`` element
^^^^^^^^^^^^^^^^^^^^^^^^

The ``effect`` element defines a graphic effect to be used during a
PDF page transition.  Acrobat Reader supports these effects, but not
all PDF viewers do.

*Attributes*

``name`` (required)
  The symbolic name (to be used in the ``effect``
  attribute of ``<view>`` elements).

``duration`` (optional)
  Value must be a real number, indicating the
  duration of display in seconds.

``transition`` (optional)
  Value must be a real number, indicating the
  duration of the transition effect in seconds.

``effect`` (optional)
  a number indicated the desired effect.  The
  value must be an integer between 0 and 27 (see
  ``ipe::Effect::TEffect`` for the exact meaning).


Other style definition elements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The remaining style definition elements are:

``<color>``
  Defines a symbolic color.  The value must be an
  absolute color, that is either a single gray value (between 0 and
  1), or three components (red, green, blue) separated by space.
  
``<dashstyle>``
  Defines a symbolic dashstyle. The value must
  be a correct dashstyle description, e.g. ``[3 5 2 5] 0``.

``<pen>``
  Defines a symbolic pen width. The value is a single
  real number.

``<textsize>``
  Defines a symbolic text size.  The value is a
  piece of LaTeX source code selecting the desired font size.

``<textstretch>``
  Defines a symbolic text stretch factor. The symbolic name is shared
  with ``<textsize>`` elements. The value is a single real number.

``<symbolsize>``
  Defines a symbolic size for symbols. The value
  is a single real number, and indicates the scaling factor used for
  the symbol.

``<arrowsize>``
  Defines a symbolic size for arrows. The value
  is a single real number.

``<gridsize>``
  Defines a grid size.  The symbolic name cannot
  actually be used by objects in the document --- it is only used to
  fill the grid size selector in the user interface.

``<anglesize>``
  Defines an angular snap angle.  The symbolic
  name cannot actually be used by objects in the document --- it is
  only used to fill the angle selector in the user interface.

*Common attributes*

``name`` (required)
  A symbolic name, which must start with a letter "a" to "z" or "A" to "Z".

``value`` (required)
  A legal absolute value for the type of attribute.


