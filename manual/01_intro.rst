Welcome to the wonderful world of Ipe!
======================================

Preparing figures for a scientific article is a time-consuming
process.  If you are using the LaTeX document preparation system in
an environment where you can include PDF figures, then the extensible
drawing editor Ipe may be able to help you in the task.  Ipe allows
you to prepare and edit drawings containing a variety of basic
geometry primitives like lines, splines, polygons, circles etc.

Ipe also allows you to add text to your drawings, and unlike most
other drawing programs, Ipe treats these text object as LaTeX
text. This has the advantage that all usual LaTeX commands can be used
within the drawing, which makes the inclusion of mathematical formulae
(or even simple labels like :math:`q_i`) much simpler.  Ipe processes
your LaTeX source and includes the PDF rendering produced by LaTeX in
the figure.

In addition, Ipe offers you some editing functions that can usually
only be found in professional drawing programs or CAD systems. For
instance, it incorporates a context sensitive snapping mechanism,
which allows you to draw objects meeting in a point, having parallel
edges, objects aligned on intersection points of other objects,
rectilinear and *c*-oriented objects and the like. Whenever one of the
snapping modes is enabled, Ipe shows you *Fifi*, a secondary cursor,
which keeps track of the current aligning.

One of the nicest features of Ipe is the fact that it is
*extensible*. You can write your own functions, so-called
*ipelets*.  Once registered with Ipe by adding them to your
ipelet path, you can use those functions like Ipe's own editing
functions. (In fact, some of the functions in the standard Ipe
distribution are actually implemented as ipelets.)  Ipelets can be
written in Lua, an easy-to-learn interpreted language that is embedded
into Ipe, or also in C++.  Among others, there is an ipelet to compute
Voronoi diagrams.

Making a presentation is another task that requires drawing figures.
You can use Ipe to prepare presentations in PDF format.  Ipe offers
many features to make attractive presentations.

Ipe tries to be self-explanatory. There is online help available, and
most commands tell you about options, shortcuts, or errors.
Nevertheless, it would probably be wise to read at least a few
sections of this manual.  The chapter on general concepts and the
chapter explaining the snapping functions would be a useful read. If
you want to use Ipe to prepare presentations, you should also read the
:doc:`70_presentations` section.

