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

## Why LSTs?

Performance tests show that LSTs can be significantly faster than
priority heaps.  In some cases, LSTs take only 20% of the time of a
priority heap, for the same set of operations.

There are no cases where LSTs are slower than priority heaps.

We are using LSTs in [FreeRADIUS](http://freeradius.org) for all
internal heaps and queues.

## License

The code is copyright Network RADIUS SARL, and is made available under
the [3-clause BSD license](LICENSE).

The code can be made available under another license, with generous
terms.  Please email sales@networkradius.com with a query.
