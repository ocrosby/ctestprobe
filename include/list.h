#ifndef LIST_H
#define LIST_H

#include <stdio.h>

typedef void (*deallocator_t)(void*);
typedef void (*printer_t)(FILE*, void*);

// Define the structure for a node in the linked list
typedef struct Node {
    void *data;
    struct Node *next;
} node_t;

// Function to create a new node with given data
node_t *create_node(void *data);

// Function to insert a node at the end of the linked list
void insert_node(node_t **head, void *data);

// Function to delete a node from the linked list by data.
void delete_node(node_t **head, void *data, deallocator_t dealloc);

// Function to delete all nodes from the linked list
void delete_list(node_t **head, deallocator_t dealloc);

// Function to print the linked list
void print_list(FILE *fp, node_t *head, printer_t print);

#endif // LIST_H
