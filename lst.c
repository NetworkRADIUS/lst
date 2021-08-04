/** Functions for a Leftmost Skeleton Tree
 *
 * @file lst.c
 *
 * @copyright 2021 Network RADIUS SARL (legal@networkradius.com)
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include "lst.h"

/*
 * Leftmost Skeleton Trees are defined in "Stronger Quickheaps" (Gonzalo Navarro,
 * Rodrigo Paredes, Patricio V. Poblete, and Peter Sanders) International Journal
 * of Foundations of Computer Science, November 2011. As the title suggests, it
 * is inspired by quickheaps, and indeed the underlying representation looks
 * like a quickheap.
 *
 * heap/priority queue operations are defined in the paper in terms of LST
 * operations.
 */

typedef int	stack_index_t;

typedef struct {
	stack_index_t	depth;
	stack_index_t	size;
	lst_index_t	*data;	/* array of indices of the pivots (also called roots) */
}	pivot_stack_t;

struct lst_s {
	lst_index_t	capacity;	//!< Number of elements that will fit
	lst_index_t	idx;		//!< Starting index, initially zero
	lst_index_t	num_elements;	//!< Number of elements in the LST
	size_t		offset;		//!< Offset of heap index in element structure.
	void		**p;		//!< Array of elements.
	pivot_stack_t	*s;		//!< Stack of pivots, always with depth >= 1.
	lst_cmp_t	cmp;		//!< Comparator function.
};

#define index_addr(_lst, _data) ((uint8_t *)(_data) + (_lst)->offset)
#define item_index(_lst, _data) (*(lst_index_t *)index_addr((_lst), (_data)))

#define is_equivalent(_lst, _index1, _index2)	(index_reduce((_lst), (_index1) - (_index2)) == 0)
#define item(_lst, _index)			((_lst)->p[index_reduce((_lst), (_index))])
#define index_reduce(_lst, _index)		((_index) & ((_lst)->capacity - 1))
#define pivot_item(_lst, _index)		item((_lst), stack_item((_lst)->s, (_index)))

#define unlikely(_x)	__builtin_expect((_x), 0)

/*
 * The LST as defined in the paper has a fixed size set at creation.
 * Here, as with quickheaps, but we want to allow for expansion...
 * though given that, as the paper shows, the expected stack depth
 * is proportion to the log of the number of items in the LST, expanding
 * the pivot stack may be a rare event.
 */
#define INITIAL_CAPACITY	2048
#define INITIAL_STACK_CAPACITY	32

/*
 * The paper defines randomized priority queue operations appropriately for the
 * sum type definition the authors use for LSTs, which are used to implement the
 * RPQ operations. This code, however, deals with the internal representation,
 * including the root/pivot stack, which must change as the LST changes. Also, an
 * insertion or deletion may shift the position of any number of buckets or change
 * the number of buckets.
 *
 * So... for those operations, we will pass in the pointer to the LST, but
 * internally, we'll represent it and its subtrees with an (LST pointer, stack index)
 * pair. The index is that of the least pivot greater than or equal to all items in
 * the subtree (considering the "fictitious" pivot greater than anything, so (lst, 0)
 * represents the entire tree.
 *
 * The fictitious pivot at the bottom of the stack isn't actually in the array,
 * so don't try to refer to what's there.
 *
 * The index is visible for the size and length functions, since they need
 * to know the subtree they're working on.
 */

#define is_bucket(_lst, _stack_index) (lst_length((_lst), (_stack_index)) == 1)

/*
 * First, the canonical stack implementation, customized for LST usage:
 * 1. pop doesn't return a stack value, and even lets you discard multiple
 *    stack items at a time
 * 2. one can fetch and modify arbitrary stack items; when array elements must be
 *    moved to keep them contiguous, the pivot stack entries must change to match.
 */
static pivot_stack_t	*stack_alloc(void)
{
	pivot_stack_t	*s;

	s = calloc(sizeof(pivot_stack_t), 1);
	if (!s) return NULL;

	s->data = calloc(sizeof(lst_index_t), INITIAL_STACK_CAPACITY);
	if (!s->data) {
		free(s);
		return NULL;
	}
	s->depth = 0;
	s->size = INITIAL_STACK_CAPACITY;
	return s;
}

static __attribute__((nonnull)) void stack_free(pivot_stack_t *s)
{
	free(s->data);
	free(s);
}

static bool stack_expand(pivot_stack_t *s)
{
	lst_index_t	*n;
	size_t		n_size = 2 * s->size;

	n = realloc(s->data, sizeof(lst_index_t) * n_size);
	if (unlikely(!n)) return false;

	s->size = n_size;
	s->data = n;
	return true;
}

static inline __attribute__((always_inline, nonnull)) int stack_push(pivot_stack_t *s, lst_index_t pivot)
{
	if (unlikely(s->depth == s->size && !stack_expand(s))) return -1;

	s->data[s->depth++] = pivot;
	return 0;
}

static inline __attribute__((always_inline, nonnull)) void stack_pop(pivot_stack_t *s, size_t n)
{
	s->depth -= n;
}

static inline __attribute__((always_inline, nonnull)) size_t stack_depth(pivot_stack_t *s)
{
	return s->depth;
}

static inline __attribute__((always_inline, nonnull)) lst_index_t stack_item(pivot_stack_t *s, stack_index_t index)
{
	return s->data[index];
}

static inline __attribute__((always_inline, nonnull)) void stack_set(pivot_stack_t *s, stack_index_t index,
								     lst_index_t new_value)
{
	s->data[index] = new_value;
}

lst_t *_lst_alloc(lst_cmp_t cmp, size_t offset)
{
	lst_t	*lst;

	lst = calloc(sizeof(lst_t), 1);
	if (!lst) return NULL;

	lst->capacity = INITIAL_CAPACITY;
	lst->p = calloc(sizeof(void *), lst->capacity);
	if (!lst->p) {
	cleanup:
		free(lst);
		return NULL;
	}

	lst->s = stack_alloc();
	if (!lst->s) goto cleanup;

	/* Initially the LST is empty and we start at the beginning of the array */
	stack_push(lst->s, 0);
	lst->idx = 0;

	lst->cmp = cmp;
	lst->offset = offset;

	return lst;
}

void lst_free(lst_t *lst)
{
	stack_free(lst->s);
	free(lst->p);
	free(lst);
}

/*
 * The length function for LSTs (how many buckets it contains)
 */
static inline __attribute__((always_inline, nonnull)) stack_index_t lst_length(lst_t *lst, stack_index_t stack_index)
{
	return stack_depth(lst->s) - stack_index;
}

/*
 * The size function for LSTs (number of items a (sub)tree contains)
 */
static __attribute__((nonnull)) lst_index_t lst_size(lst_t *lst, stack_index_t stack_index)
{
	lst_index_t	reduced_right, reduced_idx;

	if (stack_index == 0) return lst->num_elements;

	reduced_right = index_reduce(lst, stack_item(lst->s, stack_index));
	reduced_idx = index_reduce(lst, lst->idx);

	if (reduced_idx <= reduced_right) return reduced_right - reduced_idx;	/* No wraparound--easy. */

	return (lst->capacity - reduced_idx) + reduced_right;
}

/*
 * Flatten an LST, i.e. turn it into the base-case one bucket [sub]tree
 * NOTE: so doing leaves the passed stack_index valid--we just add
 * everything once in the left subtree to it.
 */
static inline __attribute__((always_inline, nonnull)) void lst_flatten(lst_t *lst, stack_index_t stack_index)
{
	stack_pop(lst->s, stack_depth(lst->s) - stack_index);
}

/*
 * Move data to a specific location in an LST's array.
 * The caller must have made sure the location is available and exists
 * in said array.
 */
static inline __attribute__((always_inline, nonnull)) void lst_move(lst_t *lst, lst_index_t location, void *data)
{
	item(lst, location) = data;
	item_index(lst, data) = index_reduce(lst, location);
}

/*
 * Add data to the bucket of a specified (sub)tree..
 */
static void bucket_add(lst_t *lst, stack_index_t stack_index, void *data)
{
	lst_index_t	new_space;

	/*
	 * For each bucket to the right, starting from the top,
	 * make a space available at the top and move the bottom item
	 * into it. Since ordering within a bucket doesn't matter, we
	 * can do that, minimizing moving and index adjustment.
	 *
	 * The fictitious pivot doesn't correspond to an actual value,
	 * so we save pivot moving for the end of the loop.
	 */
	for (stack_index_t rindex = 0; rindex < stack_index; rindex++) {
		lst_index_t	prev_pivot_index = stack_item(lst->s, rindex + 1);
		bool		empty_bucket;

		new_space = stack_item(lst->s, rindex);
		empty_bucket = (new_space - prev_pivot_index) == 1;
		stack_set(lst->s, rindex, new_space + 1);

		if (!empty_bucket) lst_move(lst, new_space, item(lst, prev_pivot_index + 1));

		/* move the pivot up, leaving space for the next bucket */
		lst_move(lst, prev_pivot_index + 1, item(lst, prev_pivot_index));
	}

	/*
	 * If the bucket isn't the leftmost, the above loop has made space
	 * available where the pivot used to be.
	 * If it is the leftmost, the loop wasn't executed, but the fictitious
	 * pivot isn't there, which is just as good.
	 */
	new_space = stack_item(lst->s, stack_index);
	stack_set(lst->s, stack_index, new_space + 1);
	lst_move(lst, new_space, data);

	lst->num_elements++;
}

/*
 * Reduce pivot stack indices based on their difference from lst->idx,
 * and then reduce lst->idx.
 */
static void lst_indices_reduce(lst_t *lst)
{
	lst_index_t	reduced_idx = index_reduce(lst, lst->idx);
	stack_index_t	depth = stack_depth(lst->s);

	for (stack_index_t i = 0; i < depth; i++) {
		stack_set(lst->s, i, reduced_idx + stack_item(lst->s, i) - lst->idx);
	}
	lst->idx = reduced_idx;
}

/*
 * Make more space available in an LST.
 * The LST paper only mentions this option in passing, pointing out that it's O(n); the only
 * constructor in the paper lets you hand it an array of items to initially insert
 * in the LST, so elements will have to be removed to make room for more (though it's
 * easy to see how one could specify extra space).
 *
 * Were it not for the circular array optimization, it would be realloc() and done; either
 * it works or it doesn't. (That's still O(n), since it may require copying the data.)
 *
 * With the circular array optimization, if lst->idx refers to something other than the
 * beginning of the array, you have to move the elements preceding it to beginning of the
 * newly-available space so it's still contiguous, and keep pivot stack entries consistent
 * with the positions of the elements.
 */
static bool lst_expand(lst_t *lst)
{
	void 		**n;
	size_t		n_capacity = 2 * lst->capacity;
	lst_index_t	old_capacity = lst->capacity;

	n = realloc(lst->p, sizeof(void *) * n_capacity);
	if (unlikely(!n)) return false;

	lst->p = n;
	lst->capacity = n_capacity;

	lst_indices_reduce(lst);

	for (lst_index_t i = 0; i < lst->idx; i++) {
		void		*to_be_moved = item(lst, i);
		lst_index_t	new_index = item_index(lst, to_be_moved) + old_capacity;
		lst_move(lst, new_index, to_be_moved);
	}

	return true;
}

static inline __attribute__((always_inline, nonnull)) lst_index_t bucket_lwb(lst_t *lst, size_t stack_index)
{
	if (is_bucket(lst, stack_index)) return lst->idx;
	return stack_item(lst->s, stack_index + 1) + 1;
}

/*
 * Note: buckets can be empty,
 */
static inline __attribute__((always_inline, nonnull)) lst_index_t bucket_upb(lst_t *lst, size_t stack_index)
{
	return stack_item(lst->s, stack_index) - 1;
}

/*
 * Partition an LST
 * It's only called for trees that are a single nonempty bucket;
 * if it's a subtree, it is thus necessarily the leftmost.
 */
static void partition(lst_t *lst, stack_index_t stack_index)
{
	lst_index_t	low = bucket_lwb(lst, stack_index);
	lst_index_t	high = bucket_upb(lst, stack_index);
	lst_index_t	l, h;
	lst_index_t	pivot_index;
	void		*pivot;
	void		*temp;

	/*
	 * Hoare partition doesn't do the trivial case, so catch it here.
	 */
	if (is_equivalent(lst, low, high)) {
		stack_push(lst->s, low);
		return;
	}

	pivot_index = low + rand() % (high + 1 - low);
	pivot = item(lst, pivot_index);

	if (pivot_index != low) {
		lst_move(lst, pivot_index, item(lst, low));
		lst_move(lst, low, pivot);
	}

	/*
	 * Hoare partition; on the avaerage, it does a third the swaps of
	 * Lomuto.
	 */
	l = low - 1;
	h = high + 1;
	for (;;) {
		while (lst->cmp(item(lst, --h), pivot) > 0) ;
		while (lst->cmp(item(lst, ++l), pivot) < 0) ;
		if (l >= h) break;
		temp = item(lst, l);
		lst_move(lst, l, item(lst, h));
		lst_move(lst, h, temp);
	}

	/*
	 * Hoare partition doesn't guarantee the pivot sits at location h
	 * the way Lomuto does and LST needs, so first get its location...
	 */
	pivot_index = item_index(lst, pivot);
	if (pivot_index >= index_reduce(lst, low)) {
		pivot_index = low + pivot_index - index_reduce(lst, low);
	} else {
		pivot_index = high - (index_reduce(lst, high) - pivot_index);
	}

	/*
	 * ...and then move it if need be.
	 */
	if (pivot_index < h) {
		lst_move(lst, pivot_index, item(lst, h));
		lst_move(lst, h, pivot);
	}
	if (pivot_index > h) {
		h++;
		lst_move(lst, pivot_index, item(lst, h));
		lst_move(lst, h, pivot);
	}

	stack_push(lst->s, h);
}

/*
 * Delete an item from a bucket in an LST
 */
static void bucket_delete(lst_t *lst, stack_index_t stack_index, void *data)
{
	lst_index_t	location = item_index(lst, data);
	lst_index_t	top;

	if (is_equivalent(lst, location, lst->idx)) {
		lst->idx++;
		if (is_equivalent(lst, lst->idx, 0)) lst_indices_reduce(lst);
	} else {
		for (;;) {
			top = bucket_upb(lst, stack_index);
			if (!is_equivalent(lst, location, top)) lst_move(lst, location, item(lst, top));
			stack_set(lst->s, stack_index, top);
			if (stack_index == 0) break;
			lst_move(lst, top, item(lst, top + 1));
			stack_index--;
			location = top + 1;
		}
	}

	lst->num_elements--;
	item_index(lst, data) = -1;
}

/*
 * We precede each function that does the real work with a Pythonish
 * (but colon-free) version of the pseudocode from the paper.
 *
 * clang, in version 13, will have a way to force tail call optimization
 * with a "musttail" attribute. gcc has -f-foptimize-sibling-calls, but
 * it works only with -O[23s]. For now, -O2 will assure TCO. In its absence,
 * the recursion depth is bounded by the number of pivot stack entries, aka
 * the "length" of the LST, which has an expected value proportional to
 * log(number of nodes).
 */

/*
 * ExtractMin(LST T ) // assumes s(T ) > 0
 *	If T = bucket(B) Then
 *		Partition(T ) // O(|B|)
 *	Let T = tree(r, L, B )
 *	If s(L) = 0 Then
 *		Flatten T into bucket(B ) // O(1)
 *		Remove r from bucket B // O(1)
 *		Return r
 *	Else
 *		Return ExtractMin(L)
 */
static inline __attribute__((nonnull)) void *_lst_pop(lst_t *lst, stack_index_t stack_index)
{
	if (is_bucket(lst, stack_index)) partition(lst, stack_index);
	++stack_index;
	if (lst_size(lst, stack_index) == 0) {
		void	*min = pivot_item(lst, stack_index);

		lst_flatten(lst, stack_index);
		bucket_delete(lst, stack_index, min);
		return min;
	}
	return _lst_pop(lst, stack_index);
}

/*
 * FindMin(LST T ) // assumes s(T ) > 0
 * 	If T = bucket(B) Then
 * 		Partition(T ) // O(|B|)
 *	Let T = tree(r, L, B )
 *	If s(L) = 0 Then
 *		Return r
 *	Else
 *		Return FindMin(L)
 */
static inline __attribute__((nonnull)) void *_lst_peek(lst_t *lst, stack_index_t stack_index)
{
	if (is_bucket(lst, stack_index)) partition(lst, stack_index);
	++stack_index;
	if (lst_size(lst, stack_index) == 0) return pivot_item(lst, stack_index);
	return _lst_peek(lst, stack_index);
}

/*
 * Delete(LST T, x ∈ Z)
 *	If T = bucket(B) Then
 *		Remove x from bucket B // O(depth)
 *	Else
 *		Let T = tree(r, L, B′)
 *		If x < r Then
 *			Delete(L, x)
 *		Else If x > r Then
 *			Remove x from bucket B ′ // O(depth)
 *		Else
 *			Flatten T into bucket(B′′) // O(1)
 *			Remove x from bucket B′′ // O(depth)
 */
static inline __attribute__((nonnull)) void _lst_extract(lst_t *lst,  stack_index_t stack_index, void *data)
{
	int8_t	cmp;

	if (is_bucket(lst, stack_index)) {
		bucket_delete(lst, stack_index, data);
		return;
	}
	stack_index++;
	cmp = lst->cmp(data, pivot_item(lst, stack_index));
	if (cmp < 0) {
		_lst_extract(lst, stack_index, data);
	} else if (cmp > 0) {
		bucket_delete(lst, stack_index - 1, data);
	} else {
		lst_flatten(lst, stack_index);
		bucket_delete(lst, stack_index, data);
	}
}

/*
 * Insert(LST T, x ∈ Z)
 * 	If T = bucket(B) Then
 * 		Add x to bucket B // O(depth)
 *	Else
 *		Let T = tree(r, L, B)
 *		If random(s(T) + 1) != 1 Then
 *			If x < r Then
 *				Insert(L, x)
 *			Else
 *				Add x to bucket B // O(depth)
 *		Else
 *			Flatten T into bucket(B′) // O(1)
 *			Add x to bucket B′ // O(depth)
 */
static inline __attribute__((nonnull)) void _lst_insert(lst_t *lst, stack_index_t stack_index, void *data)
{
	if (is_bucket(lst, stack_index)) {
		bucket_add(lst, stack_index, data);
		return;
	}
	stack_index++;
	if (rand() % (lst_size(lst, stack_index) + 1) != 0) {
		if (lst->cmp(data, pivot_item(lst, stack_index)) < 0) {
			_lst_insert(lst, stack_index, data);
		} else {
			bucket_add(lst, stack_index - 1, data);
		}
	} else {
		lst_flatten(lst, stack_index);
		bucket_add(lst, stack_index, data);
	}
}

/*
 * We represent a (sub)tree with an (lst, stack index) pair, so
 * lst_pop(), lst_peek(), and lst_extract() are minimal
 * wrappers that
 *
 * (1) hide that representation from the user and preserve the interface
 * (2) check preconditions
 */

void *lst_pop(lst_t *lst)
{
	if (unlikely(lst->num_elements == 0)) return NULL;
	return _lst_pop(lst, 0);
}

void *lst_peek(lst_t *lst)
{
	if (unlikely(lst->num_elements == 0)) return NULL;
	return _lst_peek(lst, 0);
}

int lst_extract(lst_t *lst, void *data)
{
	if (unlikely(lst->num_elements == 0 || item_index(lst, data) < 0)) return -1;

	_lst_extract(lst, 0, data);
	return 1;
}

int lst_insert(lst_t *lst, void *data)
{
	lst_index_t	data_index;

	/*
	 * Expand if need be. Not in the paper, but we want the capability.
	 */
	if (unlikely(lst->num_elements == lst->capacity && !lst_expand(lst))) return -1;

	/*
	 * Don't insert something that looks like it's already in an LST.
	 */
	data_index = item_index(lst, data);
	if (unlikely(data_index > 0 ||
	    (data_index == 0 && lst->num_elements > 0 && lst->idx == 0 && item(lst, 0) == data))) {
		return -1;
	}

	_lst_insert(lst, 0, data);
	return 1;
}

lst_index_t lst_num_elements(lst_t *lst)
{
	return lst->num_elements;
}

void *lst_iter_init(lst_t *lst, lst_iter_t *iter)
{
	if (unlikely(!lst) || (lst->num_elements == 0)) return NULL;

	*iter = lst->idx;
	return item(lst, *iter);
}

void *lst_iter_next(lst_t *lst, lst_iter_t *iter)
{
	if (unlikely(!lst)) return NULL;

	if ((*iter + 1) >= stack_item(lst->s, 0)) return NULL;
	*iter += 1;

	return item(lst, *iter);
}
