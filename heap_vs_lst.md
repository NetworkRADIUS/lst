# About the "heap vs. lst" plot

FreeRADIUS has a binary heap type. The leftmost skeleton tree tests
are a superset of the binary heap tests, and one in particular, the
"cycle" test, seemed a good candidate for comparing the run times of
binary heap and the LST.

This repository includes [lst_tests.c](lst_tests.c) as test code, and
an example of how to use it; `lst_cycle()` is the test in question. It
is parametrized by the `cycle size`, the number of randomly-generated
values inserted into the austructure. The test has three parts:

- `insert`: insert the values into the structure.
- `extract`: repeatedly pop the "minimum" value from the structure
  until half of the values are removed.
- `swap`: make a pass over the array of values. Those still in the
  structure are extracted; those removed in the second part are
  reinserted. (The extraction honors what ordering is present, but
  doesn't impose additional order, and if a pivot is removed, that's
  less ordering to check.)

We ran the heap and LST versions of the test using powers of two from
2^10 to 2^24 as cycle sizes, and kept the times for each section. The
plot shows the heap time divided by the LST time for each section and
each cycle size.  That is, higher numbers show the LSTs being faster
than heaps.

The insert section favors LSTs, because the "buckets" of an LST are
only ordered to the extent that there are pivots that bound
them. During the initial run of inserts, there are no pivots, and
pointers to inserted elements just get added at the end, save for any
copying done by `realloc()` when an LST is expanded. A binary heap is
"heapified" with each insert.

Ordering happens in two cases: first, for a peek or a pop
(`ExtractMin()` and `FindMin()` in the paper), and second, for inserts
when pivots are present, so that a new element must be placed in the
appropriate bucket.  Peeks and pops thus do some of the work binary
heaps do in heapification, so the extract portion is least
advantageous to the LST.

The final stage, "swap", rather surprisingly shows the most advantage
over binary heaps. Insertions must honor any pivots present, but they
will tend to go away over time with extractions, and during this stage
the LST will never be expanded, avoiding possible large block copies.
