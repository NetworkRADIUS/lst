/** Tests for a Leftmost Skeleton Tree
 *
 * @file lst_tests.c
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * This counterintuitive #include gives these separately-compiled
 * tests access to elements of the opaque lst_t structure which are
 * needed for the tests.
 */
#include "lst.c"

typedef struct {
        int		data;
	lst_index_t	index;
	bool		visited;	/* Only used by iterator test */
}       heap_thing;

#if 0
static bool	lst_validate(lst_t *lst, bool show_items);
#endif

static bool lst_contains(lst_t *lst, void *data)
{
	int size = lst_num_elements(lst);

	for (int i = 0; i < size; i++) if (item(lst, i + lst->idx) == data) return true;

	return false;
}

static int8_t	heap_cmp(void const *one, void const *two)
{
	heap_thing const	*item1 = one, *item2 = two;

	return (item1->data > item2->data) - (item2->data > item1->data);
}

#define NVALUES	20
static void lst_test_basic(void)
{
	lst_t		*lst;
	heap_thing	values[NVALUES];

	srand((unsigned int)time(NULL));

	lst = lst_alloc(heap_cmp, heap_thing, index);
	if (lst == NULL) {
		fprintf(stderr, "lst_test_basic: failed to create LST\n");
		return;
	}

	for (int i = 0; i < NVALUES; i++) {
		values[i].data = i;
		values[i].index = 0;
	}

	/* shuffle values before insertion, so the heap has to work to give them back in order */
	for (int i = 0; i < NVALUES - 1; i++) {
		int	j = rand() % (NVALUES - i);
		int	temp = values[i].data;

		values[i].data = values[j].data;
		values[j].data = temp;
	}

	for (int i = 0; i < NVALUES; i++) lst_insert(lst, &values[i]);

	for (int i = 0; i < NVALUES; i++) {
		heap_thing	*value = lst_pop(lst);

		if (value == NULL) {
			fprintf(stderr, "lst_test_basic: pop failed, iteration %d\n", i);
			continue;
		}
		if (value->data != i) {
			fprintf(stderr, "lst_test_basic: pop yielded unexpected value, iteration %d\n", i);
		}
	}
	lst_free(lst);
}

#define LST_TEST_SIZE (4096)

static void lst_test(int skip)
{
	lst_t	*lst;
	int		i;
	heap_thing	*array;
	int		left;
	int		ret;

	srand((unsigned int)time(NULL));

	lst = lst_alloc(heap_cmp, heap_thing, index);
	if (lst == NULL) {
		fprintf(stderr, "lst_test(%d): failed to create LST\n", skip);
		return;
	}

	array = malloc(sizeof(heap_thing) * LST_TEST_SIZE);
	if (array == NULL) {
		lst_free(lst);
		fprintf(stderr, "lst_test(%d): failed to create array\n", skip);
		return;
	}

	/*
	 *	Initialise random values
	 */
	for (i = 0; i < LST_TEST_SIZE; i++) array[i].data = rand() % 65537;

	for (i = 0; i < LST_TEST_SIZE; i++) {
		ret = lst_insert(lst, &array[i]);
		if (ret < 0) {
			fprintf(stderr, "lst_test(%d): element %d insert failed\n", skip, i);
		} else if (!lst_contains(lst, &array[i])) {
			fprintf(stderr, "lst_test(%d): element %d inserted but not in LST", skip, i);
		}
	}

	for (int entry = 0; entry < LST_TEST_SIZE; entry += skip) {
		if (array[entry].index == -1) {
			fprintf(stderr, "lst_test(%d): element %d removed out of order\n", skip, entry);
		}
		ret = lst_extract(lst, &array[entry]);
		if (ret < 0) {
			fprintf(stderr, "lst_test(%d): element %d removal failed\n", skip, entry);
		} else {
			if (lst_contains(lst, &array[entry])) {
				fprintf(stderr, "lst_test(%d): element %d removed but still in LST\n", skip, entry);
			}
			if (array[entry].index != -1) {
				fprintf(stderr, "lst_test(%d): element %d removed but index not set to -1\n", skip, entry);
			}
		}
	}

	left = lst_num_elements(lst);
	for (i = 0; i < left; i++) {
		if (lst_pop(lst) == NULL) {
			fprintf(stderr, "lst_test(%d): pop failed, iteration %d; expected %d elements remaining\n",
				skip, i, left - i);
		}
	}

	if ((ret = lst_num_elements(lst)) != 0) {
		fprintf(stderr, "lst_test(%d): pops failed to empty LST; %d elements remaining\n", skip, ret);
	}

	lst_free(lst);
	free(array);
}

static void lst_test_skip_1(void)
{
	lst_test(1);
}

static void lst_test_skip_2(void)
{
	lst_test(2);
}

static void lst_test_skip_10(void)
{
	lst_test(10);
}


static void lst_stress_realloc(void)
{
	lst_t		*lst;
	heap_thing	*array;

	srand((unsigned int)time(NULL));

	lst = lst_alloc(heap_cmp, heap_thing, index);
	if (lst == NULL) {
		fprintf(stderr, "lst_stress_realloc(): failed to create LST\n");
		return;
	}

	array = calloc(2 * INITIAL_CAPACITY, sizeof(heap_thing));
	if (array == NULL) {
		lst_free(lst);
		fprintf(stderr, "lst_stress_realloc(): failed to create array\n");
		return;
	}

	/*
	 *	Initialise random values
	 */
	for (int i = 0; i < 2 * INITIAL_CAPACITY; i++) array[i].data = rand() % 65537;

	/* Add the first INITIAL_CAPACITY values to lst and to hp */
	for (int i = 0; i < INITIAL_CAPACITY; i++) {
		if (lst_insert(lst, &array[i]) < 0) {
			fprintf(stderr, "lst_stress_realloc(): partial fill insert failed, iteration %d\n", i);
		}
	}

	/* Pop INITIAL_CAPACITY / 2 values from each */
	for (int i = 0; i < INITIAL_CAPACITY / 2; i++) {
		if (lst_pop(lst) == NULL) {
			fprintf(stderr, "lst_stress_realloc(): first stage pop failed, iteration %d\n", i);
		}
	}

	/*
	 * Add the second INITIAL_CAPACITY values to lst and to hp.
	 * This should force lst to move entries to maintain adjacency,
	 * which is what we're testing here.
	 */
	for (int i = INITIAL_CAPACITY; i < 2 * INITIAL_CAPACITY; i++) {
		if (lst_insert(lst, &array[i]) < 0) {
			fprintf(stderr, "lst_stress_realloc(): final fill insert failed, iteration %d\n", i);
		}
	}

	/* pop the remaining 3 * INITIAL_CAPACITY / 2 values from each */
	for (int i = 0; i < 3 * INITIAL_CAPACITY / 2; i++) {
		if (lst_pop(lst) == NULL) {
			fprintf(stderr, "lst_stress_realloc(): final stage pop failed, iteration %d\n", i);
		}
	}

	if (lst_num_elements(lst) != 0) {
		fprintf(stderr, "lst_stress_realloc(): lst should be empty, but has %d elements\n", lst_num_elements(lst));
	}

	lst_free(lst);
	free(array);
}

#define BURN_IN_OPS	(10000000)

static void lst_burn_in(void)
{
	lst_t		*lst = NULL;
	heap_thing	*array = NULL;
	int		insert_count = 0;
	int		element_count = 0;

	srand((unsigned int)time(NULL));

	array = calloc(BURN_IN_OPS, sizeof(heap_thing));
	if (array == NULL) {
		fprintf(stderr, "lst_burn_in(): failed to create array\n");
		return;
	}
	for (int i = 0; i < BURN_IN_OPS; i++) array[i].data = rand() % 65537;

	lst = lst_alloc(heap_cmp, heap_thing, index);
	if (lst == NULL) {
		fprintf(stderr, "lst_burn_in(): failed to create lst\n");
		return;
	}

	for (int i = 0; i < BURN_IN_OPS; i++) {
		if (lst_num_elements(lst) == 0) {
		insert:
			if (lst_insert(lst, &array[insert_count]) < 0) {
				fprintf(stderr, "lst_burn_in(): insert %d failed\n", insert_count + 1);
			}
			insert_count++;
			element_count++;
		} else {
			switch (rand() % 3) {
				case 0: /* insert */
					goto insert;

				case 1: /* pop */
					if (lst_pop(lst) == NULL) {
						fprintf(stderr, "lst_burn_in(): pop failed\n");
					}
					element_count--;
					break;
				case 2: /* peek */
					if (lst_peek(lst) == NULL) {
						fprintf(stderr, "lst_burn_in(): peek failed\n");
					}
					break;
			}
		}
	}

	lst_free(lst);
	free(array);
}

#define LST_CYCLE_SIZE (1600000)

static void lst_cycle(void)
{
	lst_t		*lst;
	heap_thing	*array;
	int		to_remove;
	int 		inserted, removed;

	srand((unsigned int)time(NULL));

	lst = lst_alloc(heap_cmp, heap_thing, index);
	if (lst == NULL) {
		fprintf(stderr, "lst_cycle(): failed to create lst\n");
		return;
	}

	array = calloc(LST_CYCLE_SIZE, sizeof(heap_thing));
	if (array == NULL) {
		lst_free(lst);
		fprintf(stderr, "lst_cycle(): failed to create array\n");
		return;
	}

	/*
	 *	Initialise random values
	 */
	for (int i = 0; i < LST_CYCLE_SIZE; i++) array[i].data = rand() % 65537;

	/*
	 *	Insert them into the LST
	 */
	for (int i = 0; i < LST_CYCLE_SIZE; i++) {
		if (lst_insert(lst, &array[i]) < 0) {
			fprintf(stderr, "lst_cycle(), insert section: insert failed, iteration %d\n", i + 1);
		}
	}
	if (lst_num_elements(lst) != LST_CYCLE_SIZE) {
		fprintf(stderr, "lst_cycle(), insert section: not all %d values inserted\n", LST_CYCLE_SIZE);
	}

	/*
	 *	Remove half the elements from the LST
	 */
	to_remove = lst_num_elements(lst) / 2;
	for (int i = 0; i < to_remove; i++) {
		if (lst_pop(lst) == NULL) {
			fprintf(stderr, "lst_cycle(), extract section: extract %d failed; expected %d elements remaining\n",
				i + 1, to_remove - i);
		}
	}

	/*
	 *	Now swap the inserted and removed set creating churn
	 */
	inserted = 0;
	removed = 0;

	for (int i = 0; i < LST_CYCLE_SIZE; i++) {
		if (array[i].index == -1) {
			if (lst_insert(lst, &array[i]) < 0) {
				fprintf(stderr, "lst_cycle(), swap section: element %d insert failed\n", i);
			}
			inserted++;
		} else {
			if (lst_extract(lst, &array[i]) < 0) {
				fprintf(stderr, "lst_cycle(), swap section: element %d extract failed\n", i);
			}
			removed++;
		}
	}

	if (removed != (LST_CYCLE_SIZE - to_remove)) {
		fprintf(stderr, "lst_cycle(): expected to remove %d, actually removed %d\n", LST_CYCLE_SIZE - to_remove, removed);
	};

	if (inserted != to_remove) {
		fprintf(stderr, "lst_cycle(): expected to insert %d, actually inserted %d\n", to_remove, inserted);
	}

	lst_free(lst);
	free(array);
}

static void lst_iter(void)
{
	lst_t	*lst;
	lst_iter_t	iter;
	heap_thing	values[NVALUES], *data;

	lst = lst_alloc(heap_cmp, heap_thing, index);
	if (lst == NULL) {
		fprintf(stderr, "lst_iter(): failed to create lst\n");
		return;
	}

	for (int i = 0; i < NVALUES; i++) {
		values[i].data = i;
		values[i].index = 0;
		values[i].visited = false;
	}

	for (int i = 0; i < NVALUES - 1; i++) {
		int	j = rand() % (NVALUES - i);
		int	temp = values[i].data;

		values[i].data = values[j].data;
		values[j].data = temp;
	}

	for (int i = 0; i < NVALUES; i++) {
		if (lst_insert(lst, &values[i]) < 0) {
			fprintf(stderr, "lst_iter(): insert #%d failed\n", i + 1);
		}
	}

	data = lst_iter_init(lst, &iter);

	for (int i = 0; i < NVALUES; i++, data = lst_iter_next(lst, &iter)) {
		if (data == NULL) {
			fprintf(stderr, "lst_iter(): unexpected end at %d\n", i);
		} else {
			if (data->visited) {
				fprintf(stderr, "lst_iter(): visited element more than once\n");
			}
			if (data->index < 0) {
				fprintf(stderr, "lst_iter(): visited element not in LST\n");
			}
			data->visited = true;
		}
	}

	if (data != NULL) {
		fprintf(stderr, "lst_iter(): iterator didn't terminate as expected\n");
	}
	lst_free(lst);
}

#if 0
static bool lst_validate(lst_t *lst, bool show_items)
{
	lst_index_t	fake_pivot_index, reduced_fake_pivot_index, reduced_end;
	int		depth = stack_depth(lst->s);
	int		bucket_size_sum;
	bool		pivots_in_order = true;
	bool		pivot_indices_in_order = true;
	bool		is_valid = true;

	/*
	 * There has to be at least the fictitious pivot.
	 */
	if (depth < 1) {
		fprintf(stderr, "lst_validate(): LST pivot stack empty\n");
		return false;
	}

	/*
	 * Modulo circularity, idx + the number of elements should be the index
	 * of the fictitious pivot.
	 */
	fake_pivot_index = stack_item(lst->s, 0);
	reduced_fake_pivot_index = index_reduce(lst, fake_pivot_index);
	reduced_end = index_reduce(lst, lst->idx + lst->num_elements);
	if (reduced_fake_pivot_index != reduced_end) {
		fprintf(stderr, "lst_validate(): fictitious pivot inconsistent with idx and number of elements");
		is_valid = false;
	}

	/*
	 * Bucket sizes must make sense.
	 */
	if (lst->num_elements) {
		bucket_size_sum = 0;

		for (int stack_index = 0; stack_index < depth; stack_index++)  {
			lst_index_t bucket_size = bucket_upb(lst, stack_index) - bucket_lwb(lst, stack_index) + 1;
			if (bucket_size > lst->num_elements) {
				fprintf(stderr, "bucket %d size %d is invalid\n", stack_index, bucket_size);
				is_valid = false;
			}
			bucket_size_sum += bucket_size;
		}

		if (bucket_size_sum + depth - 1 != lst->num_elements) {
			fprintf(stderr, "total bucket size inconsistent with number of elements\n");
			is_valid = false;
		}
	}

	/*d
	 * No elements should be NULL.
	 */
	for (lst_index_t i = 0; i < lst->num_elements; i++) {
		if (!item(lst, lst->idx + i)) {
			fprintf(stderr, "null element at %d\n", lst->idx + i);
			is_valid = false;
		}
	}

	/*
	 * There's nothing more to check for a one-bucket tree.
	 */
	if (is_bucket(lst, 0)) return is_valid;

	/*
	 * Otherwise, first, pivots from left to right (aside from the fictitious
	 * one) should be in ascending order.
	 */
	for (int stack_index = 1; stack_index + 1 < depth; stack_index++) {
		heap_thing	*current_pivot = pivot_item(lst, stack_index);
		heap_thing	*next_pivot = pivot_item(lst, stack_index + 1);

		if (current_pivot && next_pivot && lst->cmp(current_pivot, next_pivot) < 0) pivots_in_order = false;
	}
	if (!pivots_in_order) {
		fprintf(stderr, "pivots not in ascending order\n");
		is_valid = false;
	}

	/*
	 * Next, all non-fictitious pivots must correspond to non-null elements of the array.
	 */
	for (int stack_index = 1; stack_index < depth; stack_index++) {
		if (!pivot_item(lst, stack_index)) {
			fprintf(stderr, "pivot #%d refers to NULL", stack_index);
			is_valid = false;
		}
	}

	/*
	 * Next, the stacked pivot indices should decrease as you ascend from
	 * the bottom of the pivot stack. Here we *do* include the fictitious
	 * pivot; we're just comparing indices.
	 */
	for (int stack_index = 0; stack_index + 1 < depth; stack_index++) {
		lst_index_t current_pivot_index = stack_item(lst->s, stack_index);
		lst_index_t previous_pivot_index = stack_item(lst->s, stack_index + 1);


		if (previous_pivot_index >= current_pivot_index) pivot_indices_in_order = false;
	}

	if (!pivot_indices_in_order) {
		fprintf(stderr, "pivot indices not in order\n");
		is_valid = false;
	}

	/*
	 * Finally...
	 * values in buckets shouldn't "follow" the pivot to the immediate right (if it exists)
	 * and shouldn't "precede" the pivot to the immediate left (if it exists)
	 *
	 * todo: this will find pivot ordering issues as well; get rid of that ultimately,
	 * since pivot-pivot ordering errors are caught above.
	 */
	for (int stack_index = 0; stack_index < depth; stack_index++) {
		lst_index_t	lwb, upb, pivot_index;
		void		*pivot, *element;

		if (stack_index > 0) {
			lwb = (stack_index + 1 == depth) ? lst->idx : stack_item(lst->s, stack_index + 1);
			pivot_index = upb = stack_item(lst->s, stack_index);
			pivot = item(lst, pivot_index);
			for (lst_index_t index = lwb; index < upb; index++) {
				element = item(lst, index);
				if (element && pivot && lst->cmp(element, pivot) > 0) {
					fprintf(stderr, "element at %d > pivot at %d\n", index, pivot_index);
					is_valid = false;
				}
			}
		}
		if (stack_index + 1 < depth) {
			upb = stack_item(lst->s, stack_index);
			lwb = pivot_index = stack_item(lst->s, stack_index + 1);
			pivot = item(lst, pivot_index);
			for (lst_index_t index = lwb; index < upb; index++) {
				element = item(lst, index);
				if (element && pivot && lst->cmp(pivot, element) > 0) {
					fprintf(stderr,  "element at %d < pivot at %d\n", index, pivot_index);
					is_valid = false;
				}
			}
		}
	}

	return is_valid;
}
#endif

int main(int argc, char **argv)
{
	lst_test_basic();
	lst_test_skip_1();
	lst_test_skip_2();
	lst_test_skip_10();
	lst_stress_realloc();
	lst_burn_in();
	lst_cycle();
	lst_iter();

	return EXIT_SUCCESS;
}
