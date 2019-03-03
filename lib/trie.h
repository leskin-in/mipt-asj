#ifndef TRIE_H
#define TRIE_H

/*
 * trie.h
 *      Trie implementation in C99
 *
 * IDENTIFICATION
 *	    contrib/mipt-asj/lib/trie.h
 *
 * Adapted for PostgreSQL and mipt-asj. Changes include:
 *  * Use palloc() and pfree() calls
 *  * Add version of search() to check LCS in trie
 *
 *
 * This trie associates an arbitrary void* pointer with a UTF-8,
 * NUL-terminated C string key. All lookups are O(n), n being the
 * length of the string. Strings are stored sorted, so visitor
 * functions visit keys in lexicographical order. The visitor can also
 * be used to visit keys by a string prefix. An empty prefix ""
 * matches all keys (the prefix argument should never be NULL).
 *
 * Internally it uses flexible array members, which is why C99 is
 * required, and is why the initialization function returns a pointer
 * rather than fills a provided struct.
 *
 * Except for trie_free(), memory is never freed by the trie, even
 * when entries are "removed" by associating a NULL pointer.
 *
 * @see http://en.wikipedia.org/wiki/Trie
 *
 * (c) Cristopher Wellons, https://github.com/skeeto/trie
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "postgres.h"


struct trie;

/**
 * @return a freshly allocated trie, NULL on allocation error
 */
struct trie *trie_create(void);

/**
 * Destroys a trie created by trie_create().
 *
 * @return 0 on success
 */
int trie_free(struct trie *);

/**
 * Finds for the data associated with KEY.
 *
 * @return the previously inserted data
 */
void *trie_search(const struct trie *, const char *key);

/**
 * Insert or replace DATA associated with KEY. Inserting NULL is the
 * equivalent of unassociating that key, though no memory will be
 * released.
 *
 * @return 0 on success
 *
 * For purposes of mipt-asj, data must be equal to key
 */
int trie_insert(struct trie *, const char *key, void *data);

/**
 * Find all subsequences of given key that exist in trie and put them into container.
 *
 * Container will be allocated automatically using palloc.
 *
 * @return number of subsequences found.
 */
int trie_search_subsequences(const struct trie *, const char *key, char ***container);

#endif
