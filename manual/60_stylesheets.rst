Stylesheets
===========

The symbolic attributes appearing in an Ipe document are translated to
absolute values for rendering by a *stylesheet* that is attached to
the document.  Documents can have multiple *cascaded* stylesheets, the
sheets form a stack, and symbols are looked up from top to bottom.  At
the bottom of any stylesheet cascade is always the minimal *standard*
style sheet, which is built into Ipe.

When you create a new empty document, it automatically gets a copy of
this standard style sheet (which does little more than define the
*normal* attribute for each kind of attribute).  In addition, Ipe
inserts a predefined list of stylesheets.  The list of these
stylesheets can be :ref:`customized <customize>` using an ipelet,
using Ipe's command line options, or an environment variable.  By
default, a new document gets the stylesheet *basic* that comes with
Ipe.

The stylesheet dialog (in :menuselection:`Edit --> Stylesheets`)
allows you to inspect the cascade of stylesheets associated with your
document, to add and remove stylesheets, and to change their order.
You can also save individual stylesheets.

The stylesheets of your document also determine the symbolic choices
you have in the Ipe user interface.  If you feel that Ipe does not
offer you the right choice of colors, pen widths, etc., you are ready
to make your own style sheet!


Make your own stylesheet!
-------------------------

So you are ready to roll your own stylesheet, to have the colors and
pen widths you've always wanted?  Here you go:

First, decide on a name for your stylesheet.  In this section, let's
pick the name *personal*.

Open an Ipe document (or make a new one and save it in a file), then
start up your favorite text editor and create the file
:file:`personal.isy`.  The file must be in the **same folder** as your
Ipe document!

Enter the following contents into the file, and save it:

.. code-block:: xml

   <ipestyle name="personal">
   <color name="yellowgreen" value="0.604 0.804 0.196"/>
   </ipestyle>

(The ``name`` attribute in the first line must match the filename,
without the extension ``.isy``.)

In Ipe, use :menuselection:`Edit --> Style sheets` to bring up the
stylesheet dialog.  Press the :guilabel:`Add` button, and select
your file :file:`personal.isy`.  You'll see *personal* appear at the
top of the list of stylesheets.  Click :guilabel:`Ok` to confirm
adding the stylesheet.

You will notice that a new color named ``yellowgreen`` is now
available in the dropdown for stroke and fill color.
Congratulations---you made your first stylesheet!

You can now add colors, pen widths, and sizes for symbols (markers),
arrows, and the grids, by imitating the following examples:

.. code-block:: xml

   <pen name="light" value="0.7"/>
   <symbolsize name="giant" value="20"/>
   <arrowsize name="small" value="6"/>
   <gridsize name="10 pts" value="10"/>
   <anglesize name="20 deg" value="20"/>

Each line contains the symbolic ``name``, and an absolute numeric
value in ``value``.  The name must start with a letter (grid size
and angle size are exceptions).

To add a definition, update the file :file:`personal.isy` in your text
editor, then use :menuselection:`Edit --> Update style sheets`. This
will cause Ipe to read the file again, and to replace the copy of the
stylesheet inside your document with this newest version.

This way, you can quickly test out new definitions by editing the
stylesheet in your text editor, and pressing :kbd:`Ctrl+Shift+U` in
Ipe to try out the new definitions.

You may wonder how to get your favorite colors right, so here is a
little trick: draw a small box in Ipe, then press the absolute stroke
color button (the top-left button in the *Properties* panel).  It will
allow you to select a color using a graphical user interface.  Once
you have found the right color, apply it to the box (by selecting
``<absolute>`` in the stroke color selector), then right-click on the
box and select :guilabel:`Edit as XML`. A dialog will appear showing
the current definition of the box in Ipe's internal XML format, like
this:

.. code-block:: xml

   <path stroke="0.561 0.349 0.008" pen="ultrafat">
   64 816 m
   64 800 l
   80 800 l
   80 816 l
   h
   </path>

You can now copy the color definition (in this case ``"0.561 0.349
0.008"``, a nice shade of brown) to your stylesheet.

You can find inspiration for more colors in the :file:`colors.isy`
stylesheet in Ipe's :file:`styles` folder. It defines all the colors
of the X11 color database---you could make a selection of these for
your own use.

There is much more you can do with stylesheets.  Have a look in the
stylesheets that come with Ipe for some inspiration, or keep reading
this chapter and the :doc:`next <70_presentations>`.  The ultimate
reference is, of course, the :ref:`description of the stylesheet file
format <ipestyle>`.


Stylesheet theory
-----------------

When a stylesheet is "added" to an Ipe document, the contents of the
stylesheet file is copied into the Ipe document.  Subsequent
modification of the stylesheet file has no effect on the Ipe document.
The right way to modify your stylesheet is to either "add" it again,
and then to delete the *old* copy from your stylesheet cascade (the
one further down in the list), or to use the *Update stylesheets*
function in the *Edit* menu.  This function assumes that the
stylesheet file is in the same directory as the document and that the
filename coincides with the name of the stylesheet (plus the extension
``.isy``).

Removing or replacing a stylesheet can cause some of the symbolic
attributes in your document to become undefined.  This is not a
disaster---Ipe will simply use some default value for any undefined
symbolic attribute.  To allow you to diagnose the problem, Ipe will
show a warning listing all undefined symbolic attributes.

We discuss a few stylesheet topics in this chapter.  Other stylesheet
definitions that are (mostly) meant for PDF presentations are
discussed in the next chapter.


Symbols
-------

Style sheets can also contain *symbols*, such as marks and
arrows, background patterns, or logos.  These are named Ipe objects
that can be referenced by the document. If your document's stylesheets
define a symbol named *Background*, it will be displayed
automatically on all pages. (If a layer named ``BACKGROUND`` is
present on a page, it suppresses the ``Background`` symbol for that
page.  It does not matter if the layer itself is visible or not.) You
can create and use symbols using the *Symbols* ipelet.  Here is a
(silly) example of a style sheet that defines such a background:

.. literalinclude:: samples/background.isy
   :language: xml		    

Note the use of the ``xform`` attribute---it ensures that the
background is embedded only once into PDF document.  This can make a
huge difference if your background is a complicated object.

Symbols can be parameterized with a stroke color, fill color, pen
size, and symbol size.  This means that the actual value of these
attributes is only set when the symbol is used in the document (not in
the symbol definition).  The name of a parameterized symbol must end
with a pair of parentheses containing some of the letters ``s``
(stroke), ``f`` (fill), ``p`` (pen), ``x`` (symbol size), in this
order.  The symbol definition can then use the special attribute
values ``sym-stroke``, ``sym-fill``, and ``sym-pen``.  A
resizable symbol is automatically magnified by the symbol size set in
the symbol reference.

A symbol can define several snap positions for the symbol
object. These positions are then active in vertex snap mode.  Symbols
with snap positions are also presented differently in the current
selection (the entire symbol is outlined, like a group, rather than
just showing a cross at the symbol location), and you can select such
symbols by clicking near any of the snap positions.

You can also use a stylesheet to define additional mark shapes, arrow
shapes, or tiling patterns.


Decorations
-----------

A *decoration* is a symbol that can be used to decorate a group
object.  Its name must start with the string *decoration/*, and
it should contain either a path object or a group of path objects.

Ipe resizes these path objects so that they fit nicely around the
bounding box of the group object being decorated.  For this to work
correctly, the decoration object must be drawn such that it decorates
the rectangle with corners at :math:`(100, 100)` and :math:`(300, 200)`. 

To make a decoration symbol, follow these steps:

#. Draw a rectangle. Select *Edit as XML* from its context
   menu, and change the coordinates to look as follows:

   .. code-block::

     <path stroke="black" fill="lightblue">
     100 100 m
     300 100 l
     300 200 l
     100 200 l
     h
     </path>

#. Draw your decoration so that it fits this rectangle.  You should draw only path objects.
#. Delete the rectangle from the first step.
#. If the decoration consists of more than one object, group them all together.
#. Save this object as a symbol whose name starts with *decoration/*
   (including the slash).  You can do this either using *create new
   symbol* in the *symbols* ipelet, or by selecting *Edit as XML* from
   the object's context menu and copying the code into a stylesheet
   open in your text editor.

For inspiration, have a look at the decoration symbols in the
stylesheet :file:`decorations.isy` that comes with Ipe.


More about stylesheets
----------------------

Paper size
""""""""""

The style sheet is also responsible for determining the paper and
frame size.  Ipe's default paper size is the ISO standard A4.  If you
wish to use letter size paper instead, include this style sheet:

.. code-block::

   <ipestyle name="letterpaper">
     <layout paper="612 792" origin="0 0" frame="612 792"/>
   </ipestyle>


Latex preamble.
"""""""""""""""

Stylesheets can also define a piece of LaTeX-preamble for your
document.  When your text objects are processed by LaTeX, the
preamble used consists of the pieces on the style sheet cascade, from
bottom to top, followed by the preamble set for the document itself.

Note that when putting LaTeX code in your style sheet, you have to
escape the characters that are special in XML.  For instance, for
``<`` you would have to write ``&lt;``.

