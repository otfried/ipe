.. index:: snapping
.. index:: grid snapping
.. index:: aligning objects
.. index:: magnetic objects

Snapping
========

One of the nice features of Ipe is the possibility of having the mouse
*snap* to other objects during entry or moving. Certain features on
the canvas become "magnetic", and it is very easy to align objects
to each other, to place new objects properly with respect to the
present objects and so on.

Snapping comes in three flavors: *grid snapping*, *context
snapping*, and *angular snapping*.

In general, you turn a snapping mode on by pressing one of the buttons
in the *Snap* toolbar, or selecting the equivalent functions in
the Snap menu. The buttons are independent, you can turn them on and
off independently. (The snapping modes, however, are not
independent. See below for the precise interaction.)  The keyboard
shortcuts are rather convenient since you will want to toggle snapping
modes on and off while in the middle of creating or editing some
object.

.. index:: secondary cursor, Fifi

Whenever one of the snapping modes is enabled, you will see a little
cross near the cursor position. This is the secondary cursor *Fifi*
[#]_. Fifi marks the position the mouse is snapped to.

.. [#] Fifi is called after the dog in the ``rogue`` computer game
       installed on most Unix systems in the 1980's, because it also
       keeps running around your feet.

.. _grid-snapping:
.. index:: grid
.. index:: grid snapping
.. index:: aligning objects
.. index:: grid size

Grid snapping
-------------

*Grid snapping* is easy to explain. It simply means that the mouse
position is rounded to the nearest grid point. Grid points are points
whose coordinates are integer multiples of the *grid size*, which can
be set in the box in the *Snap* field. You have a choice from a set of
possible grid sizes. The units are Postscript/PDF points (in LaTeX
called ``bp``), which are equal to 1/72 of an inch.

You can ask Ipe to show the grid points by selecting the function
:menuselection:`View --> Grid visible`. The same function turns it off
again.

.. _context-snapping:
.. index:: context snapping

Context snapping
----------------

When *context snapping* is enabled, certain features of the
objects of your current drawing become magnetic. There are three
buttons to enable three different features of your objects: vertices,
the boundary, and intersection points.

When the mouse is too far away from the nearest interesting feature,
the mouse position will not be "snapped".  The snapping distance can
be changed by setting *Snapping distance* value in the preference
dialog.  If you use a high setting, you will need to toggle snapping
on and off during drawing. Some people prefer to set snapping on once
and for all, and to set the snap distance to a very small value like 3
or 4.

The features that you can make "magnetic" are the following:

.. index:: vertex snapping
.. index:: snapping to vertices

vertices
  are vertices of polygonal objects, control points of
  multiplicity three of splines, centers of circles and ellipses,
  centers and end points of circular arcs, and mark positions.

.. index:: snapping to boundaries  
.. index:: boundary snapping

boundaries
  are the object boundaries of polygonal objects,
  splines and splinegons, circles and ellipses, and circular arcs.

.. index:: snapping to intersection points  
.. index:: intersection snapping

intersections
  are the intersection points between the
  boundaries of path objects.

.. 
  index:: edit mode
  index:: snapping to object during editing
  \cindex[self snapping]{``self'' snapping}

  When you are creating or editing a polygonal or spline object, you
  sometimes want to be able to snap to the already present vertices or
  control points of the object you are working on. This can be
  achieved by pressing the button *self* in the *Snap* field. Note
  that this only concerns vertices (and, unlike normal vertex
  snapping, the control points of a spline are now magnetic as well),
  not the boundary curve or intersection points.

.. _custom-grid-snapping:
.. index:: custom grid
.. index:: grid snapping

Custom grid snapping
--------------------

Sometimes you need a special grid, for instance a triangular grid, or
a grid for making perspective drawings.  Since Ipe cannot offer every
possible grid under the sun, it instead offers you snapping to custom
grids.

To use custom grid snapping, create a layer with name *GRID*, and
draw your grid in this layer.  You can then snap to the intersection
points between the objects in the *GRID* layer.  It does not
matter if the layer is visible or not.

The *grid maker* ipelet offers a few ready-made grids to be used as
custom grids.


.. _angular-snapping:
.. index:: angular snapping
.. index:: snap angle
.. index:: axis system
.. index:: origin, of axis system
.. index:: base direction, of axis system

Angular snapping
----------------

When *angular snapping* is enabled, the mouse position is restricted
to lie on a set of lines through the *origin* of your current *axis
system*. The lines are the lines whose angle with the *base direction*
is an integer multiple of the snap angle. The snap angle can be set in
the second box in the Snap toolbar. The values are indicated in
degrees.  So, for a snapping angle of :math:`45^{\circ}`, we get the
snap lines indicated in the figure below. (In the figure the base
direction---indicated with the arrow---is assumed horizontal.)

.. image:: images/snaplines.*
   :align: center

For a snap angle of 180 degrees, snapping is to a single line through
the current origin.

.. index:: fix-point of scale, stretch, rotate, and shear 

In order to use angular snapping, it is important to set the axis
system correctly. To set the origin, move the mouse to the correct
position, and press the :kbd:`F1`-key. Note that angular snapping is
*disabled* while setting the origin. This way you can set a new
origin for angular snapping without leaving the mode first.  Once the
origin has been set, the base direction is set by moving to a point on
the desired base line, and pressing the :kbd:`F2`-key. Again, angular
snapping is disabled.  Together, origin and base direction determine
the current *axis system*. Remember that the origin is also used
as the fix-point of scale, stretch, and rotate operations, if it is
set.

You can hide the current axis system by pressing :kbd:`Ctrl+F1`. This
also turns off angular snapping, but preserves origin and orientation of
the axes.  To reset the orientation (such that the :math:`x`-axis is
horizontal), use :kbd:`Ctrl+F2`.

You can set origin and base direction at the same time by pressing
:kbd:`F3` when the mouse is very near (or snapped to) an edge of a
polygonal object.  The origin is set to an endpoint of the edge, and
the base direction is aligned with it. This is useful to make objects
parallel to a given edge.

.. index:: automatic angular snapping

For drawing rectilinear or c-oriented polygons, the origin should be
set to the previous vertex at every step. This can be done by pressing
:kbd:`F1` every time you click the left mouse button, but that would
not be very convenient. Therefore, Ipe offers a second angular snap
mode, called *automatic angular snapping*. This mode uses an
independent origin, which is automatically set every time you add a
vertex when creating a polygonal object. Note that while the origin is
independent of the origin set by :kbd:`F1`, the base direction and the
snap angle used by automatic angular snapping is the same as for
angular snapping.  Hence, you can align the axis system with some edge
of your drawing using :kbd:`F3`, and then use automatic angular
snapping to draw a new object that is parallel or orthogonal to this
edge.

This snapping mode has another advantage: It remains silent and
ineffective until you start creating a polygonal object. So, even with
automatic angular snapping already turned on, you can still freely
place the first point of a polygon, and then the remaining vertices
will be properly aligned to make a :math:`c`-oriented polygon.

The automatic angular snapping mode is never active for any
non-polygonal object.  In particular, to *move* an object in a
prescribed direction, you have to use normal angular snapping.

A final note: Many things that can be done with angular snapping can
also be done by drawing auxiliary lines and using context snapping. It
is mostly a matter of taste and exercise to figure out which mode
suits you best.

.. _snap interaction:

Interaction of the snapping modes
---------------------------------

Not all the snapping modes can be active at the same time, even if all
buttons are pressed. Here we have a close look at the possible
interactions, and the priorities of snapping.

The two angular snapping modes restrict the possible mouse positions
to a *one-dimensional* subspace of the canvas. Therefore, they are
incompatible with the modes that try to snap to a zero-dimensional
subspace, namely vertex snapping, intersection snapping, and grid
snapping. Consequently, when one of the angular snapping modes is
*on*, vertex snapping, intersection snapping, and grid snapping
are ineffective.

On the other hand, it is reasonable to snap to boundaries while in an
angular snapping mode, and this function is actually implemented
correctly. When both angular and boundary snapping are *on*, Ipe
will compute intersections between the snap lines with the boundaries
of your objects, and whenever the mouse position *on* the snap
line comes close enough to an intersection, the mouse is snapped to
that intersection.
        
The two angular snapping modes themselves can also coexist in the same
fashion. If both angular and automatic angular snapping are enabled,
Ipe computes the intersection point between the snap lines defined by
the two origins and snaps there. It the snap lines are parallel or
coincide, automatic angular snapping is used.

When no angular snapping mode is active, Ipe has three
priorities. First, Ipe checks whether the closest vertex or
intersection point is close enough. If that is not the case, the
closest boundary edge is determined. If even that is too far away, Ipe
uses grid snapping (assuming all these modes are enabled).

Note that this can actually mean that snapping is *not* to the
*closest* point on an object. Especially for intersections of two
straight edges, the closest point can never be the intersection point,
as in the figure below!

.. image:: images/intersection.*
   :align: center

.. _snapping-examples:

Examples
--------

It takes some time and practice to feel fully at ease with the different
snapping modes, especially angular snapping. Here are some examples
showing what can be done with angular snapping.

.. index:: vertical extensions
	   
Example 1:
""""""""""

We are given segments :math:`s_1`, :math:`s_2`, and :math:`e`, and we
want to add the dashed vertical extensions through :math:`p` and
:math:`q`.

.. image:: images/example1.*
   :align: center

#. set :kbd:`F4` and :kbd:`F5` snapping on, go into *line* mode, and
   reset axis orientation with :kbd:`Ctrl+F2`,
#. go near :math:`p`, press :kbd:`F1` and :kbd:`F8` to set origin and
   to turn on angular snap.
#. go near :math:`p'`, click left, and extend segment to :math:`s_1`.
#. go near :math:`q`, press :kbd:`F1` to reset origin, and draw second
   extension in the same way.


Example 2:
""""""""""

We are given the polygon :math:`C`, and we want to draw the bracket
:math:`b`, indicating its vertical extension.

.. image:: images/example2.*
   :align: center


#. set :kbd:`F4` and :kbd:`F9` snapping on, go into *line* mode, reset
   axis orientation with :kbd:`Ctrl+F2`, set snap angle to
   :math:`90^{\circ}`.
#. go near :math:`p`, press :kbd:`F1` and :kbd:`F8` to set origin and
   angular snapping
#. go to :math:`x`, click left, extend segment to :math:`y`, click left
#. now we want to have :math:`z` on a horizontal line through
   :math:`q`: go near :math:`q`, and press :kbd:`F1` and :kbd:`F8` to
   reset origin and to turn on angular snapping. Now both angular
   snapping modes are on, the snap lines intersect in :math:`z`.
   \item click left at :math:`z`, goto :math:`x` and press :kbd:`F1`,
   goto :math:`t` and finish bracket.


Example 3:
""""""""""

We want to draw the following "skyline". The only problem is to get
:math:`q` horizontally aligned with :math:`p`.

.. image:: images/example3.*
   :align: center

#. draw the baseline using automatic angular snapping to get it
   horizontal.
#. place :math:`p` with boundary snapping, draw the rectilinear curve
   up to :math:`r` with automatic angular snapping in
   :math:`90^{\circ}` mode.
#. now go to :math:`p` and press :kbd:`F1` and :kbd:`F8`. The snap
   lines intersect in :math:`q`. Click there, turn off angular
   snapping with :kbd:`Shift-F2`, and finish curve.  The last point is
   placed with boundary snapping.


.. index:: tangent (drawing a)

Example 4:
""""""""""

We want to draw a line through :math:`p`, tangent to :math:`C` in
:math:`q`.

.. image:: images/example4.*
   :align: center

#. with vertex snapping on, put origin at :math:`p` with :kbd:`F1`
#. go to :math:`q` and press :kbd:`F2`. This puts the base direction from
   :math:`p` to :math:`q`.
#. set angular snapping with :kbd:`F8` and draw line.


Example 5:
""""""""""

We want to draw the following "windmill".  The angle of the sector and
between sectors should be :math:`30^{\circ}`.

.. image:: images/example5.*
   :align: center

#. set vertex snapping, snap angle to :math:`30^{\circ}`,
   reset axis orientation with :kbd:`Ctrl+F2`,
#. with automatic angular snapping, draw a horizontal segment :math:`pq`.
#. go to :math:`p`, place origin and turn on angular snapping with :kbd:`F1` and :kbd:`F8`,
#. duplicate segment with :kbd:`d`, go to :math:`q` and pick up
   :math:`q` for rotation (with :kbd:`Alt` and the right mouse button,
   or by switching to rotate mode).  Rotate until segment falls on the
   next snap line.
#. turn off angular snapping with :kbd:`F8`. Choose arc mode, variant
   *center & two points*,
#. go to :math:`p`, click for center. Go to :math:`q`, click for first
   endpoint of arc, and at :math:`r` for the second endpoint. Select all,
   and group.
#. turn angular snapping on again. Duplicate sector, and rotate by
   :math:`60^{\circ}`, using angular snapping.
#. duplicate and rotate four more times.


.. index:: c-oriented polygon

Example 6:
""""""""""

We want to draw a :math:`c`-oriented polygon, where the angles between
successive segments are multiples of :math:`30^{\circ}`.
The automatic angular snapping mode makes this pretty easy, but there is a
little catch: How do we place the ultimate vertex such that it is at the
same time properly aligned to the penultimate and to the very first vertex?

.. image:: images/example6.*
   :align: center

#. set snap angle to :math:`30^{\circ}`,
   and turn on automatic angular snapping.
#. click first vertex :math:`p` and draw the polygon up to the penultimate
   vertex :math:`q`.
#. it remains to place :math:`r` such that it is in a legal position
   both with respect to :math:`q` and :math:`p`. The automatic angular
   snapping mode ensures the position with respect to :math:`q`. We
   will use angular snapping from :math:`p` to get it right: Go near
   :math:`p` and turn on vertex snapping. Press :kbd:`F1` to place the
   origin at :math:`p` and :kbd:`F8` to turn on angular snapping. Now
   it is trivial to place :math:`r`.
