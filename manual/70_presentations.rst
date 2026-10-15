Presentations
=============

An Ipe presentation is an Ipe document with several pages ("slides")
that is presented using a video projector.

The following sections explain a number of Ipe features that are
useful for making such presentations.


PDF or HTML/JS?
---------------

You have two choices for presenting Ipe presentations: PDF or HTML/JS

The classic approach is to save the Ipe document in PDF format, and
use any PDF viewer to present it.  One such viewer is the presentation
tool IpePresenter that comes with Ipe.  It shows the current slide in
one window (which you can make full screen on the external display),
while showing the current slide, the next slide, notes for the current
page, as well as a timer on your own display.  IpePresenter not only
works for Ipe presentations, but also for presentations made with
beamer.  It easily fits on and runs from a USB-stick, and does not
require Latex on the computer where you give the presentation.

The alternative approach is to save the Ipe document as an HTML/JS
presentation.  This produces a single HTML-file, which you can open in
any browser.  The HTML-file uses reveal.js and the animation library
GSAP to implement navigation between slides, transitions, and
animations.  Note that in the current version this HTML-presentation
will not work offline, as it downloads the Javascript-libraries when
you open it.  This presentation format supports various kinds of
animations, see below.


Presentation stylesheets
------------------------

A presentation *must* use a dedicated stylesheet.  Presentations must
use much larger fonts than what is normal for a figure.  Don't try to
make use of the ``LARGE`` and ``huge`` textsizes, but use a stylesheet
that properly defines ``normal`` to be a large textsize.

Ipe comes with a style sheet `presentation` that can be used for
presentations.  To create an Ipe presentation, open "Style sheets"
(Shift+Ctrl+S), select `presentation` and press "Add".  Then select
`basic` and press "Del".  (`presentation` is meant to be used
*instead* of `basic`, not in addition to it.)

Note that :file:`presentation` has a rather simple look.  For a more
sophisticated presentation style sheet, have a look at `Jens' webpage
<https://algo.uni-rostock.de/software/#c2508443>`_.

You will notice that in a presentation, only part of the "paper" is
used for the contents.  This smaller area is called the *frame*.  The
*Insert text box* function, for instance, creates text objects that
fill exactly the width of the frame.

You can set a title for each page using :menuselection:`Page --> Edit
title & sections`.  It will be shown in a uniform style (defined by
the style sheet) outside the frame.


.. _views:   

Views
-----

When making a presentation, one would often like to present a page
incrementally. For instance, I would first like to show a polygon,
then add its triangulation, and finally color the vertices.  *Views*
make it possible to do this nicely.

An Ipe document consists of several pages, each of which can consist
of an arbitrary number of *views*.  When saving as PDF, each view
generates a separate PDF page (if you only look at the result in a PDF
viewer, you cannot tell whether two pages are actually two views of
the same Ipe page or two different Ipe pages).  When saving as a
HTML/JS presentation, the views of a single page become a *vertical*
*stack* of slides in the deck.

An Ipe page consists of a number of objects, a number of layers, and a
number of views.  Each object belongs to exactly one layer.  A layer
can be shown by any number of views---a view is really just a list of
layers to be presented. In addition, a view keeps a record of the
current active layer---this makes it easy to move around your views
and edit them.

Views can also give a different meaning to symbolic attributes, so
that objects that appear in blue on one view will be red on the next
one.  One can also change the dash pattern and pen width, and can even
replace one symbol by a different one.


Example
^^^^^^^

To return to our polygon triangulation example, let's create an empty
page.  We draw a polygon into the default layer *alpha*.  Now use
:menuselection:`Views --> New layer, new view`, and draw the
triangulation into the new layer *beta*.  Note that the function not
only created a new layer, but also a second view showing both *alpha*
and *beta*.  Try moving back and forth between the two views (using
the :kbd:`PageUp` and :kbd:`PageDown` keys). You'll see changes in the
layer list on the left: in view 1, layer *alpha* is selected and
active, in view 2, both layers are selected and *beta* is active.
Create a third layer and view, and mark the vertices.

Save in PDF format, and voila, you have a lovely little presentation.
The result is available `here <https://github.com/otfried/ipe/raw/master/examples/polygon.pdf>`_.


Text boxes
^^^^^^^^^^

In presentations, one often has slides with mostly text.  The
:ref:`textbox <text-objects>` object is convenient for this, as one
doesn't need to use the mouse to create it.  To create a slide where
several text items appear one by one, one only needs to press F10 to
create a textbox, then :kbd:`Shift+Ctrl+I` to make a new view, F10
again for the next textbox, and so on.  Finally, one moves the
textboxes vertically for the most pleasing effect (*Shift+Alt+Left
Mouse* does a constrained vertical translation, or *Shift+Left Mouse*
in *Translate* mode).


Animations
----------

If you save your presentation in HTML/JS format, then you can use
animations for the transitions between different views of the same
page.

The basic idea is very simple: To animate an Ipe object, you give it a
*name* by setting the `animate` attribute in the context menu.  When
two consecutive views contain an object with the same `animate` name,
then the object will be animated during the transition from the first
view to the second.

For path objects, the animation will completely "morph" the object
from the first view into the second.  A triangle can smoothly turn
into rectangle or even a circle, colors blend, opacity changes, line
width grows or shrinks.

For all other objects, such as groups, text, images, or references,
the animation is limited to changing the transformation matrix of the
object.  So the object will smoothly move from its original position
to the new position, rotating and scaling along the way.

If the object is a single open path, you can set the `animate` name to
start with `draw:`, for instance `draw:my-blue-path`.  On the first
view that includes the object, it will appear as if drawn, from
starting point to end point.  (You can do that for other path objects
as well, but the resulting animation is less predictable.)

The most involved animation is to make an object (any object, be it
path, group, or text) move along a given path.  For this to happen,
you give the object an `animate` name, for instance `myobject`.  Then
you draw a *guide* *path*: a second object consisting of a single open
path, which you name `slide:myobject` or `path:myobject`.  When
entering a view that contains the object *and* a corresponding guide
path, it will follow the guide path to its target position.  With
`slide:`, it will only translate, with `path:`, it will also rotate as
the guide path turns.  Note that it doesn't matter if the previous
view contained an object with the same `animate` name - the guide path
animation has priority.

So you can have an object with name `myobject` in several consecutive
views.  In transitions without a guide path it will morph or
transform, in transitions with guide path it will follow the guide
path.

The guide path rotation is always around the center of the bounding
box of the object (in its target position).  You will therefore obtain
the best results if you draw the path such that it ends in that point
(that is, in the center of the bounding box of the object in its
target position).  Let the path start in the corresponding point of
the initial position, and make sure that the tangent directions in the
initial and final position are the same (from the object's
perspective).  So, for instance, if the object itself is oriented
identically in the start and final position, then the tangent
directions should also be the same.



Bookmarks
---------

You can set a section title and a subsection title for each page of an
Ipe document.  Theses titles will be shown in the *bookmarks list*
(right-click on a toolbar to make it visible).  Double-clicking a
title brings you directly to its page, making navigation of long
documents much easier.  The titles are also exported to PDF, and are
visible in the bookmarks view of PDF viewers.

You can also use the section title of a page to refer to this page in
:program:`ipetoipe` and :program:`iperender`.


Excluding a page from the presentation
--------------------------------------

If you add a layer with name ``NOPDF`` to a page (it does not need to
be visible in any view of the page), then the page is not included in
the presentation.  So while Ipe will show the page normally, viewing
the document with a PDF reader (including IpePresenter) or as a
HTML/JS presentation will not show that page.


Tricks with views
-----------------

Changing symbolic values inside a view
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Imagine you have an object that is shown in dark orange on your
current page, but on one specific view you want to highlight the
object in purple.

To achieve this, go to the view, and open *Edit view* from the
*Views* menu.  In the attribute map field, write this line:

.. code-block::

   color:darkorange=purple;

You will notice that all objects on the page that had the
``darkorange`` color now show in purple.

You can similarly map other attribute values, for instance:

.. code-block::

   color:navy=red;
   color:darkorange=purple;
   symbolsize:large=tiny;
   pen:ultrafat=normal;
   dashstyle:dotted=dashed;
   symbol:mark/disk(sx)=mark/box(sx);

The attribute kinds you are allowed to modify are "pen", "symbolsize",
"arrowsize", "opacity", "color", "dashstyle", and "symbol".  Both the
original and the new attribute must be symbolic attributes defined in
your style sheet.

It's probably not a good idea to remap common attributes in your
document---this may quickly become confusing.  A better approach would
be to create dedicated attribute values like ``emphasis``, that
map to some standard value in the stylesheet, and which you can then
redefine in the views where you want to emphasize the object.


Controlling the PDF bounding box of a view
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Note that all views of a page receive the same bounding box,
containing all objects visible on some view, plus all objects in a
layer named ``BBOX`` (even if that layer is not visible).  This can be
used to force a larger bounding box without adding a white rectangle
or the like.

.. TODO: use a flag in view instead?
   
If you need independent bounding boxes for each view, create a layer
named ``VIEWBBOX``.  Any view in which this layer is visible will
receive a bounding box computed for the objects visible in this view
only.



Making your own presentation style sheet
----------------------------------------

Instead of adding a presentation style sheet from Ipe, you can also
create a new presentation from the command line by starting Ipe like
this:

.. code-block:: console
	
  ipe -sheet presentation

Let's have a look inside this presentation stylesheet, in case you
want to roll your own.

The style sheet enlarges all standard sizes by a factor 2.8.  Note the
use of the ``<textstretch>`` element to magnify text:

.. code-block::

   <textstretch name="normal" value="2.8"/>
   <textstretch name="large" value="2.8"/>

The text size you choose from the Ipe user interface (*large*, for
instance) is in fact used for *two* symbolic attributes, namely
``textsize`` (where *large* maps to ``\large``) and ``textstretch``
(where it maps to no stretch in the standard style sheet).  By setting
the text stretch, you can magnify fonts.

The ``<layout>`` element in this stylesheet redefines the
paper size to be of the correct proportions for a projector, and
defines the *frame*.  

The ``<titlestyle>`` element defines the style of the page *title*,
typically outside the frame.

The LaTeX-preamble defined in the ``<preamble>`` element
redefines the standard font shape to ``cmss`` (Computer Modern Sans
Serif).  Many people find sans-serif fonts easier to read on a screen.
In addition, it redefines the list environments to use less spacing,
and the text styles to not justify paragraphs (the ``<textstyle>``
elements). 

.. _pagenumberstyle:

If you want to number the pages of the presentation, you'll need to
use the ``<pagenumberstyle>`` element.  It defines color, size,
position, and alignment of the page number, and provides a template
for the page number text.  The template can use the following Latex
counters:

* ``ipePage`` for the current page number,
* ``ipeView`` for the current view number,
* ``ipePages`` for the total number of pages,
* ``ipeViews`` for the number of views of the current page,

and the special macro ``\ipeNumber{x}{y}`` that evaluates to ``x``
when the page has only one view, and to ``y`` if there is more than
one view.

If the template is empty, this has the same effect as the following
definition (this is also the default definition):

.. code-block:: latex

   \ipeNumber{\arabic{ipePage}}{\arabic{ipePage} - \arabic{ipeView}}

The following example definition indicates the views of a single page
with letters A, B, C, ...:

.. code-block:: xml

   <pagenumberstyle pos="300 100" size="Huge" color="navy"
   halign="center">\ipeNumber{\arabic{ipePage}}{\arabic{ipePage} 
   \Alph{ipeView}}</pagenumberstyle>

The following definition uses roman numerals for the pages and does
not indicate different views at all:

.. code-block:: xml

   <pagenumberstyle pos="580 20" size="Huge" color="purple"
   halign="right">\roman{ipePage}</pagenumberstyle>


And this shows both the current page and the total number of pages:

.. code-block:: xml

   <pagenumberstyle pos="20 820" size="Huge" color="0.5 0 0" 
   valign="top">\arabic{ipePage}/\arabic{ipePages}</pagenumberstyle>

   
.. _gradient-patterns:

Gradient patterns
-----------------

Gradient patterns allow to shade objects with continuously changing
colors.  This is often used for backgrounds, or to achieve the
illusion of three-dimensional spheres or cylinders.  

The intended use of gradients is to allow the creation of attractive
symbols inside the style sheet, for backgrounds, as bullets in item
lists (see next section), or simply to define attractive glassy-ball
symbols and the like that can be used through the *Use symbol*
ipelet.

The Ipe user interface does not offer any way of creating or editing
gradients.  If your stylesheet defines a gradient, then it is possible
to fill a path object with this gradient, but getting the gradient
coordinate system right is not trivial.  (The trick is to draw the
path object at gradient coordinates, and translate/rotate it to the
final location afterwards.)

The definition of a linear (axial) gradient looks like this:

.. code-block:: xml

  <gradient name="linear" type="axial" extend="yes" coords="75 0 325 0">
    <stop offset="0.05" color="1 0.4 0"/>
    <stop offset="0.95" color="1 1 0.4"/>
  </gradient>

If used like this:

.. code-block:: xml

  <path stroke="0" fill="1" gradient="linear" pen="3">
    50 50 m 350 50 l 350 150 l 50 150 l h
  </path>

it will look like this:

.. image:: images/lingrad1.*
   :align: center

A radial gradient looks like this:

.. code-block:: xml

  <gradient name="radial" type="radial" extend="yes"
  	    coords="200 100 0 200 100 150">
    <stop offset="0" color="1 0 0"/>
    <stop offset="0.5" color="0 0 1"/>
    <stop offset="1" color="1 0 0"/>
  </gradient>

It will look like this:

.. image:: images/radgrad1.*
   :align: center

A common use of radial gradients is to define glassy balls like this:

.. code-block:: xml

  <gradient name="ball" type="radial" coords="-4 10 2 0 0 18">
    <stop offset="0" color="1 1 1"/>
    <stop offset="1" color="0 0 1"/>
  </gradient>

Note that the gradient is centered at *0 0*, so it needs to be
moved to the location where it is used:

.. code-block:: xml

  <path matrix="3 0 0 3 100 100" fill="1" gradient="ball">
    18 0 0 18 0 0 e
  </path>

.. image:: images/radgrad2.*
   :align: center

Normally, you would define a symbol looking like a glassy ball in your
style sheet:

.. code-block:: xml

  <ipestyle>
    <gradient name="ball" type="radial" coords="-4 10 2 0 0 18">
      <stop offset="0" color="1 1 1"/>
      <stop offset="1" color="0 0 1"/>
    </gradient>
    <symbol name="ball(x)" transformations="translations">
      <path fill="1" gradient="ball"> 18 0 0 18 0 0 e </path>
    </symbol>
  </ipestyle>

The glassy ball can then be used in the document using the *Use
symbol* ipelet.  Note that ``transformations="translations"`` ensures
that stretching your drawing does not change the glassy ball.  Adding
the ``(x)`` suffix to the symbol name allows you to resize the glassy
ball by changing the symbol size from the properties (the same
selector used to change the mark size).

For the precise syntax of the gradient definition see :ref:`here
<gradient-element>`.  The easiest method of creating gradients,
though, is to use an SVG editor such as :program:`Inkscape` and to
convert the SVG gradient to Ipe format using :program:`svgtoipe`.


Prettier bullet points
----------------------

.. see gradient2.ipe for examples

Presentations often make use of ``itemize`` environments.  You can
make these prettier in a number of ways:

You can color your bullets:

.. code-block:: xml

   <preamble>
   \def\labelitemi{\LARGE\textcolor{red}{$\bullet$}}
   </preamble>


Enumeration numbers could be put in a colored box:

.. code-block:: xml

   <preamble>
   \newcommand{\labelenumi}{\fbox{\Roman{enumi}}}
   </preamble>

You could use the Dingbats font for nice enumerations, for instance
by putting ``\usepackage{pifont}`` in your preamble, and then having
text objects with ``\begin{dinglist}{43}`` or ``\begin{dingautolist}{172}``
(or use 182, 192, 202 for various circled numbers).

You can mark items as *good* and *bad* using these "bullets":

  | *Bad item*:  ``\textcolor{red}{\ding{55}}``
  | *Good item*: ``\textcolor{green}{\ding{52}}``


.. _filming:

Filming from the canvas
-----------------------

Some Ipe users make presentation videos directly from Ipe, as follows:

* Make Ipe full-screen (Zoom menu);
* Turn off the grid (Zoom menu);
* Enable pretty display (Zoom menu);
* Mac: Turn off the Snap toolbar and possibly the mode toolbar (Zoom menu);
* Linux: Turn off or rearrange toolbars and panels to taste;
* Windows: Turn off or rearrange toolbars by setting ``prefs.win_toolbar_order``;
* Set up your video software to film the canvas area of Ipe (where your document is shown).

You can now go through your presentation, using the *Laser pointer*
mode to point to items on your slide, and using ink mode to write
annotations directly onto the slides.

For this to work well, it is essential that the width of the canvas
remains fixed while you step through your document.  Try it before you
start filming!  You can change the contents of the "View" and "Page"
buttons by changing the preferences ``prefs.view_button_prefix`` and
``prefs.page_button_prefix``.  Furthermore, on MacOS, you can force the
canvas width to remain fixed by making ``prefs.osx_properties_width``
large enough.

