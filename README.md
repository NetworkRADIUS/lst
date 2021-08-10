# Leftmost Skeleton Trees

Leftmost Skeleton Trees are defined in the paper "Stronger Quickheaps"
by Gonzalo Navarro, Rodrigo Paredes, Patricio V. Poblete, and Peter
Sanders. in the International Journal of Foundations of Computer
Science, November 2011.

http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.186.5910

https://users.dcc.uchile.cl/~gnavarro/ps/ijfcs11.pdf

As the title suggests, it is inspired by quickheaps, and indeed the
underlying representation looks like a quickheap.

The heap/priority queue operations are defined in the paper in terms
of LST operations.

## Short Summary

The Left-most Skeleton Tree is a heap which defers ordering the
entries.  In contrast, priority heaps always keep the entire heap
ordered.  The benefit is that in many use cases, the LSTs can avoid
some work entirely, and thus be faster than heaps.

To some extent, LSTs can be seen as priority heaps which are divided
into sections by special `pivot` entries.  The heap is composed of
pivots, and sub-arrays which are associated with each pivot.  The
pivots are ordered as with heaps, but the sub-arrays are only ordered
with respect to the pivots.

For example, if a heap has entries `1...2^7`, the pivots could be `1`,
`4`, `8`, `16`, `32`, and `64`.  Each pivot has an associated array
which contains the elements which are between the current pivot, and
the next one.

On insert, the correct pivot is found, and the entry is appended to
the associated array.  On extract, a similar operation is performed.

If the pivot itself is modified, then more work needs to be done.

There are many details, of course.  We suggest reading the code and/or
the paper for more information.

## Why LSTs?

Performance tests show that LSTs can be significantly faster than
priority heaps.  In some cases, LSTs take only 20% of the time of a
priority heap, for the same set of operations.

There are no cases where LSTs are slower than priority heaps.  In
general, LSTs take half of the time of a priority heap, for similar
operations.  Where the operations are insert-heavy, LSTs are
significantly faster.

We are moving to LSTs in [FreeRADIUS](http://freeradius.org) for all
internal heaps and queues.

## License

The code is copyright Network RADIUS SARL, and is made available under
the [3-clause BSD license](LICENSE).

The code can be made available under another license, with generous
terms.  Please email sales@networkradius.com with a query.
