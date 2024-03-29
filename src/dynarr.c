/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "dynarr.h"

struct dynarr
{
	char
		* items,
		* indicesUsed;
	int
		capacity,
		used;
	size_t
		size;
};

struct dyn_iterator
{
	Dynarr da;
	char
		* indices;
	int
		checkedIndex,
		finalIndex;
};

static bool dynarr_index_used (const Dynarr da, int index);
static void dynarr_set_index (Dynarr da, int index);
static void dynarr_unset_index (Dynarr da, int index);
static int dynarr_index_first (const Dynarr da);
static int dynarr_index_final (const Dynarr da);

static void dynarr_resize (Dynarr da, int newSize);

static void dynIterator_iterate (DynIterator it);

static bool dynarr_index_used (const Dynarr da, int index)
{
	int
		j = index / CHAR_BIT,
		k = index % CHAR_BIT;
	if (j > da->capacity / CHAR_BIT) // overrun
		return false;
	return (da->indicesUsed[j] & (0x01 << k))
		? true
		: false;
}

static void dynarr_set_index (Dynarr da, int index)
{
	int
		j = index / CHAR_BIT,
		k = index % CHAR_BIT;
	if (j > da->capacity / CHAR_BIT) // overrun
		return;
	da->indicesUsed[j] |= (0x01 << k);
}

static void dynarr_unset_index (Dynarr da, int index)
{
	int
		j = index / CHAR_BIT,
		k = index % CHAR_BIT;
	da->indicesUsed[j] &= ~(0x01 << k);
}

static int dynarr_index_first (const Dynarr da)
{
	int
		j = 0,
		k;
	assert (da != NULL);
	while (j <= da->capacity / CHAR_BIT)
	{
		if (!da->indicesUsed[j])
		{
			j++;
			continue;
		}
		k = 0;
		while (k < CHAR_BIT)
		{
			if (da->indicesUsed[j] & (0x01 << k))
				return j * CHAR_BIT + k;
			k++;
		}
		return -2;
	}
	return -1;
}

static int dynarr_index_final (const Dynarr da)
{
	int
		j,
		k;
	assert (da != NULL);
	j = da->capacity / CHAR_BIT;
	while (j >= 0)
	{
		if (!da->indicesUsed[j])
		{
			j--;
			continue;
		}
		k = CHAR_BIT - 1;
		while (k >= 0)
		{
			if (da->indicesUsed[j] & (0x01 << k))
				return j * CHAR_BIT + k;
			k--;
		}
		return -2;
	}
	return -1;
}

static void dynarr_resize (Dynarr da, int newSize)
{
	int
		final;
	char
		* items = NULL,
		* indicesUsed = NULL;
	final = dynarr_index_final (da);
	if (final < 0)
		final = 0;
	while (da->capacity <= newSize)
	{
		da->capacity *= 2;
		items = realloc (da->items, da->capacity * da->size + 1);
		if (items == NULL)
			exit (ENOMEM);
		da->items = items;
		memset (da->items + da->capacity * da->size, '\0', da->size);

		if (final / CHAR_BIT < da->capacity / CHAR_BIT)
		{
			indicesUsed = calloc (1, da->capacity / CHAR_BIT + 1);
			if (indicesUsed == NULL)
				exit (ENOMEM);
			if (final)
				memcpy (indicesUsed, da->indicesUsed, final / CHAR_BIT + 1);
			free (da->indicesUsed);
			da->indicesUsed = indicesUsed;
		}
		da->items = items;
	}
}

Dynarr dynarr_create (int indices, size_t size)
{
	Dynarr
		da = NULL;
	int
		ind;
	if (indices <= 0 || size <= 0)
		return NULL;
	ind = indices / CHAR_BIT + 1;
	da = malloc (sizeof (struct dynarr));
	if (da == NULL)
		exit (ENOMEM);
	da->items = calloc (indices + 1, size);
	if (da->items == NULL)
		exit (ENOMEM);
	da->indicesUsed = malloc (ind);
	if (da->indicesUsed == NULL)
		exit (ENOMEM);
	da->capacity = indices;
	memset (da->indicesUsed, '\0', ind);
	da->used = 0;
	da->size = size;
	return da;
}

void dynarr_destroy (Dynarr da)
{
	if (da == NULL)
	{
		WARNING ("Tried to destroy a NULL array");
		return;
	}
	free (da->items);
	free (da->indicesUsed);
	free (da);
}

int dynarr_assign (Dynarr da, int index,  ...)
{
	va_list
		ap;
	char
		* datum;
	va_start (ap, index);
	datum = va_arg (ap, char *);
	va_end (ap);
	if (index >= da->capacity)
		dynarr_resize (da, index);
	memcpy (da->items + index * da->size, &datum, da->size);
	if (!dynarr_index_used (da, index))
	{
		da->used++;
		dynarr_set_index (da, index);
	}
	return index;
}

int dynarrInsertInPlace (Dynarr da, int index, ...)
{
	va_list
		ap;
	size_t
		i = index;
	char
		* datum,
		* replaced = NULL;
	va_start (ap, index);
	datum = va_arg (ap, char *);
	va_end (ap);
	if (index >= da->capacity)
		dynarr_resize (da, index);
	/* this passes the three tests i wrote for it, but I wouldn't trust it until it's been tested through use or got better test coverage
	 * - xph 2011 06 16
	 */
	while (dynarr_index_used (da, i))
	{
		replaced = *(void **)dynarr_at (da, index);
		*(void **)(da->items + da->size * index) = *(void **)(da->items + da->size * i);
		*(void **)(da->items + da->size * i) = replaced;
		i++;
	}
	// ihni; originally this was dynarr_assign (da, i, replaced); but replaced isn't the right value at this point since it's [i-1] and we want [i], so i don't know if replaced is the right value for the ifcheck at all (it isn't) or what
	//  - xph 2011 06 16
	if (replaced != NULL)
		dynarr_assign (da, i, *(void **)dynarr_at (da, index));
	dynarr_assign (da, index, datum);
	return index;
}

int dynarr_push (Dynarr da, ...)
{
	va_list
		ap;
	int
		index = dynarr_index_final (da) + 1;
	char
		* datum;
	va_start (ap, da);
	datum = va_arg (ap, char *);
	va_end (ap);
	if (index >= da->capacity)
		dynarr_resize (da, index);
	memcpy (da->items + index * da->size, &datum, da->size);
	da->used++;
	dynarr_set_index (da, index);
	return index;
}

char * dynarr_at (const Dynarr da, int index)
{
	if (index < 0 || !dynarr_index_used (da, index))
		return da->items + da->capacity * da->size;
	return da->items + index * da->size;
}

char * dynarr_pop (Dynarr da)
{
	int
		index = dynarr_index_final (da);
	if (index < 0)
		return da->items + da->capacity * da->size;
	dynarr_unset_index (da, index);
	da->used--;
	return da->items + index * da->size;
}

char * dynarr_front (Dynarr da)
{
	int
		index = dynarr_index_first (da);
	if (index < 0)
		return da->items + da->capacity * da->size;
	return da->items + index * da->size;
}

char * dynarr_back (Dynarr da)
{
	int
		index = dynarr_index_final (da);
	if (index < 0)
		return da->items + da->capacity * da->size;
	return da->items + index * da->size;
}


char * dynarr_dequeue (Dynarr da)
{
	int
		index = dynarr_index_first (da);
	if (index < 0)
		return da->items + da->capacity * da->size;
	dynarr_unset_index (da, index);
	da->used--;
	return da->items + index * da->size;
}



void dynarr_unset (Dynarr da, int index)
{
	if (!dynarr_index_used (da, index))
		return;
	memset (da->items + index * da->size, '\0', da->size);
	dynarr_unset_index (da, index);
	da->used--;
}

void dynarr_clear (Dynarr da)
{
	DynIterator
		it = dynIterator_create (da);
	while (!dynIterator_done (it))
		dynarr_unset (da, dynIterator_nextIndex (it));
	dynIterator_destroy (it);
}

void dynarr_wipe (Dynarr da, void free_func (void *))
{
	char
		* datum;
	while (!dynarr_isEmpty (da))
	{
		datum = *(char **)dynarr_pop (da);
		if (free_func != NULL)
			free_func (datum);
	}
}

/***
 * SEARCHING FUNCTIONS
 */

void dynarr_condense (Dynarr da)
{
	int
		i = 0,
		index;
	DynIterator
		it = dynIterator_create (da);
	while (!dynIterator_done (it))
	{
		index = dynIterator_nextIndex (it);
		if (index == i)
		{
			i++;
			continue;
		}
		memcpy (da->items + i * da->size, da->items + index * da->size, da->size);
		dynarr_unset_index (da, index);
		dynarr_set_index (da, i);
		i++;
	}
	dynIterator_destroy (it);
}

void dynarr_sort (Dynarr da, int (*sort) (const void *, const void *))
{
	if (da->used != dynarr_index_final (da) - 1)
		dynarr_condense (da);
	qsort (da->items, da->used, da->size, sort);
}

void dynarrSortFinal (Dynarr da, int (*sort) (const void *, const void *), int amount)
{
	size_t
		end,
		start,
		match;
	void
		** dataAddr,
		* data;
	if (da->used != dynarr_index_final (da) - 1)
		dynarr_condense (da);
	end = dynarr_index_final (da);
	start = end - amount;
	if (amount == 1)
	{
		dataAddr = (void **)dynarr_at (da, end);
		data = *dataAddr;
		match = 0;
		while (match < end && sort (dataAddr, dynarr_at (da, match)) > 0)
			match++;
		if (match == end)
			return;
		dynarr_pop (da);
		dynarrInsertInPlace (da, match, data);
		return;
	}
	// sort final [amount] items
	qsort (da->items + start * da->size, amount + 1, da->size, sort);
	// 'mergesort'
	match = 0;
	while (start <= end)
	{
		dataAddr = (void **)dynarr_at (da, start);
		data = *dataAddr;
		while (match < end && sort (dataAddr, dynarr_at (da, match)) > 0)
			match++;
		dynarr_unset (da, start);
		dynarrInsertInPlace (da, match, data);
		start++;
	}
}

char * dynarr_search (const Dynarr da, int (*search) (const void *, const void *), ...)
{
	va_list
		ap;
	char
		* key,
		* match;
	if (da->used != dynarr_index_final (da) + 1)
	{
		return dynarr_at (da, -1);
	}
	va_start (ap, search);
	key = va_arg (ap, char *);
	va_end (ap);
	match = bsearch (&key, da->items, da->used, da->size, search);
	if (match == NULL)
	{
		return dynarr_at (da, -1);
	}
	return match;
}

void dynarr_map (Dynarr da, void map_func (void *))
{
	DynIterator
		it = dynIterator_create (da);
	void
		* val;
	while (!dynIterator_done (it))
	{
		val = *(void **)dynIterator_next (it);
		map_func (val);
	}
	dynIterator_destroy (it);
}

void dynarr_remove_condense (Dynarr da, ...)
{
	va_list
		ap;
	int
		index;
	char
		* datum;
	va_start (ap, da);
	datum = va_arg (ap, char *);
	va_end (ap);
	index = in_dynarr (da, datum);
	if (index < 0)
		return;
	dynarr_unset (da, index);
	dynarr_condense (da);
}

int in_dynarr (const Dynarr da, ...)
{
	va_list
		ap;
	char
		* datum,
		* match;
	int
		index = -1;
	DynIterator
		it = dynIterator_create (da);
	va_start (ap, da);
	datum = va_arg (ap, char *);
	va_end (ap);
	while (!dynIterator_done (it))
	{
		match = dynIterator_next (it);
		if (memcmp (match, &datum, da->size) == 0)
		{
			index = dynIterator_lastIndex (it);
			break;
		}
	}
	dynIterator_destroy (it);
	return index;
}

int dynarr_size (const Dynarr da)
{
	assert (da != NULL);
	return da->used;
}

int dynarr_capacity (const Dynarr da)
{
	assert (da != NULL);
	return da->capacity;
}

bool dynarr_isEmpty (const Dynarr da)
{
	if (da == NULL)
		return true;
	return da->used == 0
		? true
		: false;
}

/***
 * ITERATORS
 */

DynIterator dynIterator_create (const Dynarr da)
{
	DynIterator
		it;
	if (da == NULL)
	{
		ERROR ("Canoot create iterator: array is NULL");
		return NULL;
	}
	it = malloc (sizeof (struct dyn_iterator));
	it->da = da;
	it->indices = NULL;
	it->checkedIndex = -1;
	it->finalIndex = -1;
	return it;
}

void dynIterator_destroy (DynIterator it)
{
	if (it->indices != NULL)
		free (it->indices);
	free (it);
}

static void dynIterator_iterate (DynIterator it)
{
	int
		indcap,
		j,
		k;
	if (it->indices == NULL)
	{
		indcap = it->da->capacity / CHAR_BIT + 1;
		it->indices = malloc (indcap);
		memcpy (it->indices, it->da->indicesUsed, indcap);
		it->checkedIndex = -1;
		it->finalIndex = dynarr_index_final (it->da);
	}
	it->checkedIndex++;
	j = it->checkedIndex / CHAR_BIT;
	k = it->checkedIndex % CHAR_BIT;
	if (it->checkedIndex > it->finalIndex)
		return;
	while (!(it->indices[j] & (0x01 << k)))
	{
		it->checkedIndex++;
		j = it->checkedIndex / CHAR_BIT;
		k = it->checkedIndex % CHAR_BIT;
		if (it->checkedIndex > it->finalIndex)
			return;
	}
	return;
}

char * dynIterator_next (DynIterator it)
{
	dynIterator_iterate (it);
	if (it->checkedIndex < 0 || it->checkedIndex > it->finalIndex)
		return dynarr_at (it->da, -1);
	return dynarr_at (it->da, it->checkedIndex);
}

int dynIterator_nextIndex (DynIterator it)
{
	dynIterator_iterate (it);
	if (it->checkedIndex < 0 || it->checkedIndex > it->finalIndex)
		return -1;
	return it->checkedIndex;
}

int dynIterator_lastIndex (const DynIterator it)
{
	return it->checkedIndex;
}

void dynIterator_reset (DynIterator it)
{
	if (it->indices != NULL)
	{
		free (it->indices);
		it->indices = NULL;
		it->checkedIndex = -1;
		it->finalIndex = -1;
	}
}

bool dynIterator_done (const DynIterator it)
{
	assert (it != NULL);
	return (it->indices != NULL || dynarr_size (it->da) == 0) && it->checkedIndex >= it->finalIndex;
}
