#pragma once
/** Structures and prototypes for leftmost skeleton trees (LSTs)
 *
 * @file lst.h
 *
 * @copyright 2021  Network RADIUS SARL (legal@networkradius.com)
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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct lst_s	lst_t;

/*
 * The type of LST indexes.
 * The type passed to lst_alloc() in _type must be the type of a structure with a
 * member of type lst_index_t. That member's name must be passed as the _field argument.
 */
typedef int		lst_index_t;

typedef lst_index_t	lst_iter_t;

/*
 *  Return a negative number to make a "precede" b.
 *  Return a positive number to make a "follow" b.
 */
typedef int8_t (*lst_cmp_t)(void const *a, void const *b);

/** Create an LST
 *
 * @param[in] _cmp		Comparator used to compare elements.
 * @param[in] _type		Of elements.
 * @param[in] _field		to store LST indexes in.
 */
#define lst_alloc(_cmp, _type, _field)	_lst_alloc((_cmp), (size_t)(offsetof(_type, _field)))

lst_t *_lst_alloc(lst_cmp_t cmp, size_t offset) __attribute__((nonnull));

/** Free an LST
 *
 * @param[in] lst 		to be freed along with its underlying data
 */
void	lst_free(lst_t *lst) __attribute__((nonnull));

void 	*lst_peek(lst_t *lst) __attribute__((nonnull));

void 	*lst_pop(lst_t *lst) __attribute__((nonnull));

int 	lst_insert(lst_t *lst, void *data) __attribute__((nonnull));

/** Remove an element from an LST
 *
 * @param[in] lst		the LST to remove an element from
 * @param[in] data		the element to remove
 * @return
 *	- 0 if removal succeeds
 * 	- -1 if removal fails
 */
int	lst_extract(lst_t *lst, void *data) __attribute__((nonnull));

lst_index_t	lst_num_elements(lst_t *lst) __attribute__((nonnull));

/** Iterate over entries in LST
 *
 * @note If the LST is modified, the iterator should be considered invalidated.
 *
 * @param[in] lst	to iterate over.
 * @param[in] iter	Pointer to an iterator struct, used to maintain
 *			state between calls.
 * @return
 *	- User data.
 *	- NULL if at the end of the list.
 */
void		*lst_iter_init(lst_t *lst, lst_iter_t *iter);

/** Get the next entry in an LST
 *
 * @note If the LST is modified, the iterator should be considered invalidated.
 *
 * @param[in] lst	to iterate over.
 * @param[in] iter	Pointer to an iterator struct, used to maintain
 *			state between calls.
 * @return
 *	- User data.
 *	- NULL if at the end of the list.
 */
void		*lst_iter_next(lst_t *lst, lst_iter_t *iter);

#ifdef __cplusplus
}
#endif
