/**
 * @file tdictionary.c
 *
 * @copyright
 * Please read the Exosite Copyright: @ref copyright
 *
 * @if Authors
 * @authors
 *   - Szilveszter Balogh (szilveszterbalogh@exosite.com)
 *   - Zoltan Ribi (zoltanribi@exosite.com)
 * @endif
 *
 * @brief
 * A tree based dictionary implementation
 **/

#include <string.h>
#include <stdio.h>
#include <lib/type.h>
#include <lib/error.h>
#include <lib/debug.h>
#include <lib/sf_malloc.h>
#include "lib/dictionary.h"

#define DEBUG_DICT
#undef DEBUG_DICT

#ifdef DEBUG_DICT
#define tree_log(message) debug_log(DEBUG_COMMON, message)
#else
#define tree_log(message)
#endif


/* a tree node */
struct node {
    struct node *left;
    struct node *right;
    struct node *parent;
    char *key;
    void *value;
};

struct dictionary_class {
    struct node *root;
};

static char *lib_strdup(const char *s) /* make a duplicate of s */
{
    char *p;

    p = (char *) malloc(strlen(s)+1); /* +1 for ’\0’ */
    if (p != NULL)
        strcpy(p, s);
    return p;
}

int32_t dictionary_new(struct dictionary_class **new_dict)
{
    int32_t error;
    struct dictionary_class *dict;

    error = sf_mem_alloc((void **)&dict, sizeof(*dict));
    if (error != ERR_SUCCESS)
        return error;

    sf_memfill(dict, 0, sizeof(*dict));


    *new_dict = dict;
    return ERR_SUCCESS;
}

static int32_t node_new(struct node **new_node, struct node *parent, const char *key, void *value)
{
    struct node *node;
    int32_t error;

    error = sf_mem_alloc((void **)&node, sizeof(struct node));
    if (error != ERR_SUCCESS)
        return error;

    node->key = lib_strdup(key);
    if (node->key == NULL)
        return ERR_NO_MEMORY;

    node->value = value;
    node->parent = parent;
    node->left = NULL;
    node->right = NULL;
    *new_node = node;

    return ERR_SUCCESS;
}

static int32_t insert(struct node **root, struct node *parent, const char *key, void *value)
{
    int32_t error;
    int32_t compare;

    if (!*root) {
        error = node_new(root, parent, key, value);
        if (error != ERR_SUCCESS)
            return error;
        return ERR_SUCCESS;
    }

    tree_log(("Insert %s\n", value));
    compare = strcmp(key, (*root)->key);

    /*Already added*/
    if (compare == 0)
        return ERR_DUPLICATE;

    if (compare < 0) {
        tree_log(("Insert left\n"));
        error = insert(&(*root)->left, *root, key, value);
    } else {
        tree_log(("Insert right\n"));
        error = insert(&(*root)->right, *root, key, value);
    }

    return error;
}


static struct node *lookup(struct node *tree, const char *key)
{
    int32_t compare;

    if (!tree)
        return NULL;

    compare = strcmp(key, tree->key);
    if (compare < 0)
        return lookup(tree->left, key);
    else if (compare > 0)
        return lookup(tree->right, key);
    else if (compare == 0)
        return tree;

    return NULL;
}

void *dictionary_lookup(struct dictionary_class *dict, const char *key)
{
    struct node *node;

    node = lookup(dict->root, key);
    if (node)
        return node->value;

    return NULL;
}

int32_t dictionary_add(struct dictionary_class *dict, const char *name, void *value)
{
    return insert(&dict->root, NULL, name, value);
}

int32_t dictionary_remove(struct dictionary_class *dict, char *name)
{
    (void)dict;
    (void)name;
    return 0;
}

#ifdef PREORDER
static void print_preorder(struct node *tree)
{
    if (tree) {
        printf("%s\n", tree->value);
        print_preorder(tree->left);
        print_preorder(tree->right);
    }
}
#else
static void print_inorder(struct node *tree)
{
    if (tree) {
        print_inorder(tree->left);
        tree_log(("%s\n", tree->key));
        print_inorder(tree->right);
    }
}
#endif

static struct node *get_begin(struct node *node)
{
    struct node *leftmost;

    if (!node)
        return NULL;

    if (node->left)
        leftmost = get_begin(node->left);
    else
        return node;

    return leftmost;
}

struct dict_iterator *dictionary_get_iterator(struct dictionary_class *dict)
{
    struct node *node = dict->root;

    return (struct dict_iterator *)get_begin(node);
}

struct dict_iterator *dictionary_iterate(struct dict_iterator *it)
{
    struct node *node = (struct node *)it;


    if (node) {
        tree_log(("%p %p %p\n", node, node->left, node->right));
        if (node->right) {
            node = node->right;
            while (node->left != NULL)
                node = node->left;
        } else {
            struct node *base;

            tree_log(("No right go up %p %p\n", node, node->parent));
            base = node->parent;
            while (base && node == base->right) {
                node = base;
                base = base->parent;
            }
            if (node->right != base)
                node = base;
            if (base == NULL)
                node = NULL;
        }
        tree_log(("node %p\n", node));
    }
    return (struct dict_iterator *)node;
}

void *dictionary_get_value(struct dict_iterator *it)
{
    struct node *node = (struct node *)it;

    if (it)
        return node->value;

    return NULL;
}

void dictionary_print(struct dictionary_class *dict)
{
    print_inorder(dict->root);
}

