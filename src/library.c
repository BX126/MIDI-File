#include "library.h"

#include <stdlib.h>
//#include <malloc.h>
#include <assert.h>
#include <ftw.h>
#include <stdio.h>
#include <string.h>

#define DONE (0)
#define DEPTH (6)

tree_node_t *g_song_library = NULL;

int fn(const char *input_path, const struct stat *stat_struct, int flag);

/* find_parent_pointer will return the parent pointer
 * of the tree node with the same song same as
 * the input given name
 */

tree_node_t **find_parent_pointer(tree_node_t **input_tree,
                                  const char *input_song_name) {

  /* found and return */

  if (strcmp((*input_tree)->song_name, input_song_name) == 0) {
    return input_tree;
  }

  /* go left */

  if (strcmp((*input_tree)->song_name, input_song_name) > 0) {
    if ((*input_tree)->left_child != NULL) {
      return find_parent_pointer(&((*input_tree)->left_child),
                                 input_song_name);
    }
  }

  /* go right */

  if (strcmp((*input_tree)->song_name, input_song_name) < 0) {
    if ((*input_tree)->right_child != NULL) {
      return find_parent_pointer(&((*input_tree)->right_child),
                                 input_song_name);
    }
  }
  return NULL;
} /* find_parent_pointer() */

/* tree_insert function will insert the given node into
 * the given tree
 */

int tree_insert(tree_node_t **input_tree, tree_node_t *insert_node) {

  /* if the root is the position to insert */

  if ((*input_tree) == NULL) {
    (*input_tree) = insert_node;
    return INSERT_SUCCESS;
  }

  /* if the node we need to insert already in the tree, return error */

  if (strcmp((*input_tree)->song_name, insert_node->song_name) == 0) {
    return DUPLICATE_SONG;
  }
  if (strcmp((*input_tree)->song_name, insert_node->song_name) > 0) {
    if ((*input_tree)->left_child == NULL) {
      (*input_tree)->left_child = insert_node;
      return INSERT_SUCCESS;
    } else {
      return tree_insert(&((*input_tree)->left_child), insert_node);
    }
  }
  if (strcmp((*input_tree)->song_name, insert_node->song_name) < 0) {
    if ((*input_tree)->right_child == NULL) {
      (*input_tree)->right_child = insert_node;
      return INSERT_SUCCESS;
    } else {
      return tree_insert(&((*input_tree)->right_child), insert_node);
    }
  }
  return INSERT_SUCCESS;
} /* tree_insert() */

/* remove_song_from_tree will remove the tree node
 * with the given song name
 */

int remove_song_from_tree(tree_node_t **input_tree,
                          const char *input_song_name) {

  bool is_root = false;
  tree_node_t *current_node = *input_tree;
  tree_node_t *left_node = current_node->left_child;
  tree_node_t *right_node = current_node->right_child;

  /* if the root is the node we need to remove */

  if (strcmp(current_node->song_name, input_song_name) == 0) {

    /* has no child */

    if ((left_node == NULL) && (right_node == NULL)) {
      free_node(current_node);
      current_node = NULL;
      return DELETE_SUCCESS;
    }

    /* has at least one child */

    if ((left_node == NULL) || (right_node == NULL)) {

      if (left_node != NULL) {

        /* create new root */

        *input_tree = left_node;
        is_root = true;
      }

      if (right_node != NULL) {

        /* if the new root exist, insert the right child in the right */

        if (is_root) {
          tree_insert(input_tree, right_node);
          free_node(current_node);
          current_node = NULL;
          return DELETE_SUCCESS;
        } else {
          *input_tree = right_node;
          free_node(current_node);
          current_node = NULL;
          return DELETE_SUCCESS;
        }
      }
    }
  }

  /* Traverse the tree to find the node we need to remove */

  while (current_node != NULL) {
    if (strcmp(current_node->song_name, input_song_name) > 0) {
      current_node = current_node->left_child;
    } else if (strcmp(current_node->song_name, input_song_name) < 0) {
      current_node = current_node->right_child;
    } else if (strcmp(current_node->song_name, input_song_name) == 0) {
      left_node = current_node->left_child;
      right_node = current_node->right_child;
      tree_node_t **parent_node = (find_parent_pointer(input_tree,
                                                       input_song_name));
      free_node(current_node);
      current_node = NULL;
      *parent_node = NULL;
      if (left_node != NULL) {
        tree_insert(input_tree, left_node);
      }
      if (right_node != NULL) {
        tree_insert(input_tree, right_node);
      }
      return DELETE_SUCCESS;
    }
  }
  return SONG_NOT_FOUND;
} /* remove_song_from_tree() */

/* Define free_node will free the given node
 * and it's memory
 */

void free_node(tree_node_t *input_node) {
  input_node->left_child = NULL;
  input_node->right_child = NULL;
  free(input_node->song->path);
  input_node->song->path = NULL;

  /* free all track in the song */

  while (input_node->song->track_list != NULL) {
    track_node_t *old_track = input_node->song->track_list;
    input_node->song->track_list = input_node->song->track_list->next_track;
    free_track_node(old_track);
    old_track = NULL;
  }
  free(input_node->song->track_list);
  input_node->song->track_list = NULL;
  free(input_node->song);
  input_node->song = NULL;
  free(input_node);
  input_node = NULL;
} /* free_node() */

/* Print_node will print the song name of given node
 * into the given file
 */

void print_node(tree_node_t *input_node, FILE *fp) {
  int print_len = fprintf(fp, "%s\n", input_node->song_name);
  assert(print_len != 0);
} /* print_node() */

/* traverse_pre_order will traverse the tree
 * in pre-order and call the function for each node
 */

void traverse_pre_order(tree_node_t *input_tree_pointer, void *input_data,
                        traversal_func_t func) {
  if ((input_tree_pointer == NULL) && (input_tree_pointer->left_child == NULL)
      && (input_tree_pointer->left_child == NULL)) {
    return;
  }
  func(input_tree_pointer, input_data);
  if (input_tree_pointer->left_child != NULL) {
    traverse_pre_order(input_tree_pointer->left_child, input_data, func);
  }
  if (input_tree_pointer->right_child != NULL) {
    traverse_pre_order(input_tree_pointer->right_child, input_data, func);
  }
} /* traverse_pre_order() */

/* traverse_pre_order will traverse the tree
 * in-order and call the function for each node
 */

void traverse_in_order(tree_node_t *input_tree_pointer, void *input_data,
                       traversal_func_t func) {
  if ((input_tree_pointer == NULL) && (input_tree_pointer->left_child == NULL)
      && (input_tree_pointer->left_child == NULL)) {
    return;
  }
  if (input_tree_pointer->left_child != NULL) {
    traverse_in_order(input_tree_pointer->left_child, input_data, func);
  }
  func(input_tree_pointer, input_data);
  if (input_tree_pointer->right_child != NULL) {
    traverse_in_order(input_tree_pointer->right_child, input_data, func);
  }
} /* traverse_in_order() */

/* traverse_pre_order will traverse the tree
 * in post-order and call the function for each node
 */

void traverse_post_order(tree_node_t *input_tree_pointer, void *input_data,
                         traversal_func_t func) {
  if ((input_tree_pointer == NULL) && (input_tree_pointer->left_child == NULL)
      && (input_tree_pointer->left_child == NULL)) {
    return;
  }
  if (input_tree_pointer->left_child != NULL) {
    traverse_post_order(input_tree_pointer->left_child, input_data, func);
  }
  if (input_tree_pointer->right_child != NULL) {
    traverse_post_order(input_tree_pointer->right_child, input_data, func);
  }
  func(input_tree_pointer, input_data);
} /* traverse_post_order() */

/* free_library will remove all tree node
 * in the given tree
 */

void free_library(tree_node_t *input_tree) {
  if (input_tree == NULL) {
    return;
  }
  tree_node_t *current_node = input_tree;

  /* go left to free left child */

  free_library(current_node->left_child);

  /* go right to free right child */

  free_library(current_node->right_child);
  free_node(input_tree);
} /* free_library() */

/* write_song_list will write the song list
 * of the given input tree
 */

void write_song_list(FILE *fp, tree_node_t *input_tree) {
  traverse_in_order(input_tree, fp, (void *)print_node);
} /* write_song_list() */

/* helper function for make library, it will add a tree node
 * to the library each time when it be called
 */

int fn(const char *input_path, const struct stat *stat_struct, int flag) {
  if ((flag == FTW_F)) {
    char *input_file_extension = strrchr(input_path, '.');
    if (input_file_extension != NULL) {
      if (strcmp(input_file_extension, ".mid") == 0) {
        tree_node_t *node = malloc(sizeof(tree_node_t));
        assert(node != NULL);
        node->song = parse_file(input_path);
        char* input_name = strrchr(node->song->path, '/');
        node->song_name = input_name + 1;
        node->left_child = NULL;
        node->right_child = NULL;
        int insert_return_val = tree_insert(&g_song_library, node);
        assert(insert_return_val != DUPLICATE_SONG);
      }
    }
  }
  return DONE;
} /* fn() */

/* make_library will generate the library of songs
 * with the given directory
 */

void make_library(const char *input_directory) {
  ftw(input_directory, fn, DEPTH);
} /* make_library() */
