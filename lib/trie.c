/*
 * trie.c
 *      Trie implementation in C99
 *
 * IDENTIFICATION
 *	    contrib/mipt-asj/lib/trie.c
 */

#include "trie.h"

struct trieptr {
    struct trie *trie;
    int c;
};

struct trie {
    void *data;
    short nchildren, size;
    struct trieptr children[];
};


/* Mini stack library for non-recursive traversal. */

struct stack_node {
    void *data;
    int i;
};

struct stack {
    struct stack_node *stack;
    size_t fill, size;
};

static inline int
stack_init(struct stack *s)
{
    s->size = 256;
    s->fill = 0;
    s->stack = palloc(s->size * sizeof(s->stack));
    return s->stack == NULL ? -1 : 0;
}

static inline void
stack_free(struct stack *s)
{
    pfree(s->stack);
    s->stack = NULL;
}

static inline int
stack_grow(struct stack *s)
{
    struct stack_node *resize = repalloc(s->stack, s->size * 2 * sizeof(s->stack));
    if (resize == NULL) {
        stack_free(s);
        return -1;
    }
    s->size *= 2;
    s->stack = resize;
    return 0;
}

static inline int
stack_push(struct stack *s, struct trie *trie)
{
    if (s->fill == s->size)
        if (stack_grow(s) != 0)
            return -1;
    s->stack[s->fill++] = (struct stack_node){trie, 0};
    return 0;
}

static inline struct trie *
stack_pop(struct stack *s)
{
    return s->stack[--s->fill].data;
}

static inline struct stack_node *
stack_peek(struct stack *s)
{
    return &s->stack[s->fill - 1];
}


/* Constructor and destructor. */

struct trie *
trie_create(void)
{
    /* Root never needs to be resized. */
    struct trie *root = palloc(sizeof(*root) + sizeof(struct trieptr) * 255);
    if (root == NULL)
        return NULL;
    root->size = 255;
    root->nchildren = 0;
    root->data = NULL;
    return root;
}

int
trie_free(struct trie *trie)
{
    struct stack stack, *s = &stack;
    if (stack_init(s) != 0)
        return 1;
    stack_push(s, trie); // first push always successful
    while (s->fill > 0) {
        struct stack_node *node = stack_peek(s);
        if (node->i < ((struct trie*)node->data)->nchildren) {
            int i = node->i++;
            if (stack_push(s, ((struct trie*)node->data)->children[i].trie) != 0)
                return 1;
        } else {
            pfree(stack_pop(s));
        }
    }
    stack_free(s);
    return 0;
}


/* Core search functions. */

static size_t
binary_search(struct trie *self, struct trie **child,
              struct trieptr **ptr, const char *key)
{
    size_t i = 0;
    int found = 1;
    *ptr = NULL;
    while (found && key[i] != '\0') {
        int first = 0;
        int last = self->nchildren - 1;
        int middle;
        found = 0;
        while (first <= last) {
            middle = (first + last) / 2;
            struct trieptr *p = &self->children[middle];
            if (p->c < key[i]) {
                first = middle + 1;
            } else if (p->c == key[i]) {
                self = p->trie;
                *ptr = p;
                found = 1;
                i++;
                break;
            } else {
                last = middle - 1;
            }
        }
    }
    *child = self;
    return i;
}

void *
trie_search(const struct trie *self, const char *key)
{
    struct trie *child;
    struct trieptr *parent;
    size_t depth = binary_search((struct trie *)self, &child, &parent, key);
    return key[depth] == '\0' ? child->data : NULL;
}


/* Insertion functions */

static struct trie *
grow(struct trie *self) {
    int size = self->size * 2;
    if (size > 255)
        size = 255;
    size_t children_size = sizeof(struct trieptr) * size;
    struct trie *resized = repalloc(self, sizeof(*self) + children_size);
    if (resized == NULL)
        return NULL;
    resized->size = size;
    return resized;
}

static int
ptr_cmp(const void *a, const void *b)
{
    return ((struct trieptr *)a)->c - ((struct trieptr *)b)->c;
}

static struct trie *
add(struct trie *self, int c, struct trie *child)
{
    if (self->size == self->nchildren) {
        self = grow(self);
        if (self == NULL)
            return NULL;
    }
    int i = self->nchildren++;
    self->children[i].c = c;
    self->children[i].trie = child;
    qsort(self->children, self->nchildren, sizeof(self->children[0]), ptr_cmp);
    return self;
}

static struct trie *
create(void)
{
    int size = 1;
    struct trie *trie = palloc(sizeof(*trie) + sizeof(struct trieptr) * size);
    if (trie == NULL)
        return NULL;
    trie->size = size;
    trie->nchildren = 0;
    trie->data = NULL;
    return trie;
}

int
trie_insert(struct trie *trie, const char *key, void *data)
{
    struct trie *last;
    struct trieptr *parent;
    size_t depth = binary_search(trie, &last, &parent, key);
    while (key[depth] != '\0') {
        struct trie *subtrie = create();
        if (subtrie == NULL)
            return 1;
        struct trie *added = add(last, key[depth], subtrie);
        if (added == NULL) {
            pfree(subtrie);
            return 1;
        }
        if (parent != NULL) {
            parent->trie = added;
            parent = NULL;
        }
        last = subtrie;
        depth++;
    }
    last->data = data;
    return 0;
}


/* Subsequences search */

static int
trie_search_subsequences_step(const struct trie *trie, const char *key, int key_start_pos, char ***container_ptr) {
    if (*container_ptr != NULL) {
        return -1;
    }
    int container_length = 0;

    if (trie->nchildren == 0) {
        // this is a leaf, return
        char** container = palloc(sizeof(*container));
        *container_ptr = container;
        container[0] = palloc(strlen(trie->data) + 1);
        strcpy(container[0], (char*)trie->data);
        return 1;
    }

    // iterate over all characters
    for (int i = key_start_pos; i < strlen(key); i++) {
        int first = 0;
        int last = trie->nchildren - 1;
        int middle;
        // binary search
        while (first <= last) {
            middle = (first + last) / 2;
            const struct trieptr *p = &trie->children[middle];
            if (p->c < key[i]) {
                first = middle + 1;
            }
            else if (p->c == key[i]) {
                // character found; run recursively and copy result to container
                char** temp = NULL;
                int temp_length = trie_search_subsequences_step(p->trie, key, i + 1, &temp);
                if (temp_length != 0) {
                    *container_ptr = *container_ptr == NULL ?
                        palloc(sizeof(**container_ptr) * temp_length) :
                        repalloc(*container_ptr, sizeof(**container_ptr) * (container_length + temp_length));
                    for (int j = 0; j < temp_length; j++) {
                        (*container_ptr)[container_length + j] = palloc(strlen(temp[j]) + 1);
                        strcpy((*container_ptr)[container_length + j], temp[j]);
                        pfree(temp[j]);
                    }
                    pfree(temp);
                    container_length += temp_length;
                }
                break;
            }
            else {
                last = middle - 1;
            }
        }
    }

    return container_length;
}

int
trie_search_subsequences(const struct trie *trie, const char *key, char ***container) {
    return trie_search_subsequences_step(trie, key, 0, container);
}
