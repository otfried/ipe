.. index:: canvas 

General Concepts
================

After you start up Ipe, you will see a window with a large gray area
containing a white rectangle.  This area, the *canvas*, is the
drawing area where you will create your figures.  The white rectangle
is your *sheet of paper*, the first page of your document.  (While
Ipe doesn't stop you from drawing outside the paper, such documents
generally do not print very well.)

At the top of the window, above the canvas, you find two toolbars: one
for snapping modes, grid size and angular resolution; and another one
to select the current mode.

.. index:: layers

On the left hand side of the canvas you find an area where you can
select object properties such as stroke and fill color, pen width,
path properties, text size, and mark size.  Below it is a list of the
*layers* of the current page.

All user interface elements have tool tips - if you move the mouse
over them and wait a few seconds, a short explanation will appear.

The mode toolbar allows you to set the current **Ipe mode**.  Roughly
speaking, the mode determines what the left mouse button will do when
you click it in the figure.  The leftmost five buttons select modes
for selecting and transforming objects, the remaining buttons select
modes for creating new objects.

Pressing the right mouse button pops up the object context menu in any
mode.

In this chapter we will discuss the general concepts of Ipe.
Understanding these properly will be essential if you want to get the
most out of Ipe.

.. index:: object, order of objects, hidden objects, front (putting objects in), foreground (putting objects in), background (putting objects in)

.. _order-of-objects:

Order of objects
----------------

An Ipe drawing is a sequence of geometric objects. The order of the
objects is important---wherever two objects overlap, the object which
comes first in Ipe's sequence will hide the other ones. When new
objects are created, they are added in *front* of all other
objects. However, you can change the position of an object by putting
it in front or in the back, using :menuselection:`Edit --> Front` and
:menuselection:`Edit --> Back`.

.. index:: selection, primary selection, secondary selection

The current selection
---------------------

Whenever you call an Ipe function, you have to specify which objects
the function should operate on. This is done by *selecting*
objects.  The selected objects (the *selection*) consists of two
parts: the *primary* selection consists of exactly one object (of
course, this object could be a group). All additional selected objects
form the *secondary* selection. Some functions (like the context
menu) operate only on the primary selection, while others treat
primary and secondary selections differently (the align functions, for
instance, align the secondary selections with respect to the primary
selection.)

The selection is shown by outlining the selected object in color. Note
that the primary selection is shown with a slightly different look.

.. index:: selecting objects

The primary and secondary selections can be set in selection mode.
Clicking the left mouse button close to an object makes that object
the primary selection and deselects all other objects. If you keep the
:kbd:`Shift` key pressed while clicking with the left mouse key, the
object closest to the mouse will be added to or deleted from the
current selection.  You can also drag a rectangle with the
mouse---when you release the mouse button, all objects inside the
rectangle will be selected.  With the :kbd:`Shift` key, the selection
status of all objects inside the rectangle will be switched.

To make it easier to select objects that are below or close to other
objects, it is convenient to understand exactly how selecting objects
works.  In fact, when you press the mouse button, a list of all
objects is computed that are sufficiently close to the mouse position
(the exact distance is set as the ``select_distance`` in
:file:`prefs.lua`).  This list is then sorted by increasing distance
from the mouse and by increasing depth in the drawing.  If
:kbd:`Shift` was not pressed, the current selection is now
cleared. Then the first object in the list is presented.  Now, while
still keeping the mouse button pressed, you can use the :kbd:`Space`
key to step through the list of objects near the mouse in order of
increasing depth and distance.  When you release the right mouse
button, the object is selected (or deselected).

When measuring the distance from the mouse position to objects, Ipe
considers the boundary of objects only.  So to select a filled object,
don't just click somewhere in its interior, but close to its boundary.

.. index:: select all function, select all layer function

Another way to select objects is using the *Select all* function
from the *Edit* menu. It selects all objects on the page.
Similarly, the *Select all in layer* function in the *Layer*
menu selects all objects in the active layer.

.. index:: moving objects, rotating objects, translating objects, scaling objects, resizing objects, stretching objects, shearing objects

Moving and scaling objects
--------------------------

There are four modes for transforming objects: *translate*, *stretch*,
*rotate*, and *shear*.  If you hold the shift key while pressing the
left mouse button, the stretch function keeps the aspect ratio of the
objects (an operation we call *scaling*), and the translate function
is restricted to horizontal and vertical translations.

Normally, the transformation functions work on the current selection.
However, to make it more convenient to move around many different
objects, there is an exception: When the mouse button is pressed while
there is no current selection, then the object closest to the cursor
is moved, rotated, scaled, or sheared.

By default, the *rotate* function rotates around the center of the
bounding box for the selected objects.  This behavior can be
overridden by specifying an :ref:`axis system <angular-snapping>`. If
an axis system is set, then the origin is used as the center.

The *scale*, *stretch*, and *shear* functions use a corner of the
bounding box for the selected objects as the fix-point of the
transformation. Again, if an axis system is set, the origin of the
system is used instead.  In this case, the *shear* function will use
your *x*-axis as the fixed axis of the shear operation (rather than a
horizontal line).

It is often convenient to rotate or scale around a vertex of an
object.  This is easy to achieve by setting the origin of the axis
system to that vertex, using the *Snap to vertex* :ref:`function
<context-snapping>` for setting the axis system.

.. index:: color, stroke color, fill color, void color, interior of objects, filled objects, text object type, mark object type

Stroke and fill colors
----------------------

Path objects can have two different colors, one for the boundary and
one for the interior of the object. The Postscript/PDF terms *stroke*
and *fill* are used to denote these two colors.  Stroke and fill color
can be selected independently in the *Properties* window.  Imagine
preparing a drawing by hand, using a pen and black ink. What Ipe draws
in its *stroke* color is what you would stroke in black ink with your
pen. Probably you would not use your pen to fill objects, but you
would use a brush, and maybe even a different kind of paint like water
color. Well, the *fill* color is Ipe's "brush."

When you create a path object, you'll have to tell Ipe whether you
want it stroked, filled, or both.  This is set in the *Path
properties* field.  Clicking near the right end of the field will
cycle through the three modes *stroked*, *stroked & filled*, and
*filled*. You can also use the context menu of the path properties
field.

.. index:: arrow color, mark color

Text objects and arrows only use the stroke color, even for the filled
arrows. You would also use a pen for these details, not the brush.

The mark shapes *disk* and *square* also use only the stroke
color. You can make bicolored marks using the mark shapes *fdisk*
and *fsquare*.

.. index:: pen, line style, solid line style, dashed line style, dotted line style, line width, fat lines, thin lines, arrow size

Pen, dash style, arrows, and tiling patterns
--------------------------------------------

The *path properties* field is used to set all properties of path
objects except for the pen width, which is set using the selector just
above the path properties field.  The dash-dot pattern (solid line,
dashed, dotted etc.)  effect for the boundaries of *path objects*,
such as polygons and polygonal lines, splines, circles and ellipses,
rectangles and circular arcs. It does not effect text or marks.

Line width is given in Postscript/PDF points (1/72 inch). A good value is
something around 0.4 or 0.6.

By clicking near the ends of the segment shown in the path properties
field, you can toggle the front and rear arrows. Only polygonal lines,
splines, and circular arcs can have arrows.

If you draw a single line segment with arrows and set it to *filled
only*, then the arrows will be drawn using the fill color (instead of
the stroke color), and the segment is not drawn at all.  This is
sometimes useful to place arrows that do not appear at the end of a
curve.

Various shapes and sizes of arrows are available through the context
menu in the path properties field.  You can add other shapes and sizes
using a stylesheet.

The arrow shapes *arc* and *farc* are special.  When the final segment
of a path object is a circular arc, then these arcs take on a curved
shape that depends on the radius of the arc.  They are designed to
look right even for arcs with rather small radius.

The arrow shapes whose name starts with *mid* are also special:
Those arrows are not drawn at the endpoint of a curve, but at its
midpoint.  (This is currently only implemented for polylines, that is
curves that do not contain circular arcs or splines.)

.. index:: tiling pattern

A tiling pattern allows you to hatch a path object instead of filling
it with a solid color.  Only path objects can be filled with a tiling
pattern.  The pattern defines the slope, thickness, and density of the
hatching lines, their color is taken from the object's fill color.
You can select a tiling pattern using the context menu in the path
properties field.  You can define your own tiling patterns in the
documents stylesheet.

Transparency
------------

Ipe supports a simple model of transparency.  You can set the opacity
of path objects, text objects, and images: an opacity of 1.0 means a
fully opaque object, while 0.5 would mean that the object is
half-transparent.  All opacity values you wish to use in a document
must be defined in its stylesheet.

.. index:: color, line width, dash style


.. _symbolic-and-absolute-attributes:

Symbolic and absolute attributes
--------------------------------

Attributes such as color, line width, pen, mark size, or text size,
can be either absolute (a number, or a set of numbers specifying a
color) or symbolic (a name).  Symbolic attributes must be translated
to absolute values by a :doc:`stylesheet <60_stylesheets>` for
rendering.

One purpose of stylesheets is to be able to reuse figures from
articles in presentations.  Obviously, the figure has to use much
larger fonts, markers, arrows, and fatter lines in the presentation.
If the original figure used symbolic attributes, this can be achieved
by simply swapping the stylesheet for another one.

The Ipe user interface is tuned for using symbolic attribute values.
You can use absolute colors, pen width, text size, and mark size by
clicking the button to the left of the selector for the symbolic
names. 
 
When creating an object, it takes its attributes from the current user
interface settings, so if you have selected an absolute value, it will
get an absolute attribute.  Absolute attributes have the advantage
that you are free to choose any value you wish, including picking
arbitrary colors using a color chooser.  In symbolic mode, you can
only use the choices provided by the current :doc:`stylesheet
<60_stylesheets>`.

The choices for symbolic attributes provided in the Ipe user interface
are taken from your stylesheet.  

.. _zoom-and-pan:
.. index:: zooming, resolution, panning, axis system

Zoom and pan
------------

You can zoom in and out the current drawing using a mouse wheel or the
zoom functions.  The minimum and maximum resolution can be
customized.  Ipe displays the current resolution at the bottom right
(behind the mouse coordinates).

Related are the functions *Normal size* (which sets the resolution to
72 pixels per inch), *Fit page* (which chooses the resolution so that
the current page fills the canvas), *Fit objects* (which chooses the
resolution such that the objects on the page fill the screen), and
*Fit selection* (which does the same for the selected objects only).
All of these are in the *Zoom* menu.

You can *pan* the drawing either with the mouse in *Pan* mode, or by
pressing the :kbd:`x` key (*here*) with the mouse anywhere on the canvas.
The drawing is then panned such that the cursor position is moved to
the center of the canvas. This shortcut has the advantage that it also
works while you are in the middle of any drawing operation. Since the
same holds for the *zoom in* and *zoom out* buttons and keys, you can
home in on any feature of your drawing *while* you are adding or
editing another object.

.. _groups:
.. index:: grouping objects, un-grouping objects
	 
Groups
------

It is often convenient to treat a collection of objects as a single
object.  This can be achieved by *grouping* objects. The result
is a geometric object, which can be moved, scaled, rotated etc. as a
whole.  To edit its parts or to move parts of it with respect to
others, however, you have to *un-group* the object, which
decomposes it into its component objects. To un-group a group object,
select it, bring up the object menu, and select the *Ungroup*
function.

Group objects can be elements of other groups, so you can create a
hierarchy of objects.

A second function of groups is that they allow you to add additional
information to an object or group of objects.  In particular, group
objects allow you to set a *clipping path* on part of your drawing, to
*create links* to external documents or websites, and to *decorate*
objects.  See :ref:`group objects <group-objects>` for details.

.. _layers:

Layers
------

A page of an Ipe document consists of one or more layers.  Each object
on the page belongs to a layer.  There is no relationship between
layers and the :ref:`back-to-front ordering <order-of-objects>` of
objects, so the layer is really just an attribute of the object.

The layers of the current page are displayed in the layer list, at the
bottom left of the Ipe window.  The checkmark to the left of the layer
name determines whether the layer is visible.  The layer marked with a
yellow background is the *active* layer. New
objects are always created in the active layer. You can change the
active layer by left-clicking on the layer name (on Windows,
double-click on the layer name).

By right-clicking on a layer name, you open the layer context menu
that allows you to change layer attributes, to rename layers, to
delete empty layers, and to change the ordering of layers in the layer
list (this ordering has no other significance).

A layer may be editable or locked.  Objects can be selected and
modified only if their layer is editable. Locked layer are displayed
in the layer list with a pink background.  You can lock and unlock
layers from the layer context menu, but note that the active layer
cannot be locked.

A layer may have snapping on or off---objects will behave magnetically
only if their layer has snapping on.  By default, snapping is on when
the layer is visible, but you can choose to turn it off entirely, or
to keep it on even when the layer is not visible.

Layers are also used to create pages that are displayed incrementally
in a PDF viewer.  Once you have distributed your objects over various
layers, you can create :ref:`views <views>`, which defines in what
order which layers of the page are shown.

.. index:: mouse buttons, scaling objects, stretching objects,
	   panning, moving objects, rotating objects, selecting objects

.. _mouse-shortcuts:

Mouse shortcuts
---------------

For the beginner, choosing a selection or transformation mode and
working with the left mouse button is easiest. Frequent Ipe users
don't mind to remember the following shortcuts, as they allow you to
perform selections and transformations without leaving the
current mode:

==========  ============================= =============
Modifiers   Left Mouse                    Right mouse
==========  ============================= =============
Plain       *mode-dependent*              context menu
Shift       *mode-dependent*              pan
Ctrl        select                        stretch
Ctrl+Shift  select non-destructively      scale
Alt         translate                     rotate      
Alt+Shift   translate horizontal/vertical rotate 
==========  ============================= =============

The middle mouse button always pans the canvas.  Without a
keyboard-modifier, the right mouse button brings up the object context
menu.

If you have to use Ipe with a two-button mouse, where you would
normally use the middle mouse button (for instance, to move a vertex
when editing a path object), you can hold the Shift-key and use the
right mouse button.

If you are not happy with these shortcuts, they :ref:`can be changed
easily <customize>`.
