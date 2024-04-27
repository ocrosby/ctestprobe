#include <stdlib.h>

#include "../include/list.h"

// Function to create a new node with given data
node_t *create_node(void *data) {
    node_t *new_node;
    size_t size;

    // Allocate memory for the new node
    size = sizeof(node_t);
    new_node = (node_t *)malloc(size);

    // Check if memory allocation was successful
    if (new_node == NULL) {
        // Memory allocation failed
        // Print an error message and exit the program
        fprintf(stderr, "Error: Unable to allocate memory for new node.\n");
        exit(1);
    }

    new_node->data = data;
    new_node->next = NULL;

    return new_node;
}

// Function to insert a node at the end of the linked list
void insert_node(node_t **head, void *data) {
    node_t *new_node, *current;

    // Create a new node
    new_node = create_node(data);

    // If the list is empty, set the new node as the head
    if (*head == NULL) {
        *head = new_node;
        return;
    }

    // Traverse to the end of the list
    current = *head;
    while (current->next != NULL) {
        current = current->next;
    } // end while

    // Insert the new node at the end
    current->next = new_node;
}

// Function to delete a node from the linked list by data.
void delete_node(node_t **head, void *data, deallocator_t dealloc) {
    node_t *current, *prev;

    current = *head;
    prev = NULL;

    // Traverse the list to find the node with the given data
    while (current != NULL) {
        if (current->data == data) {
            if (prev == NULL) {
                // The node to be deleted is the head
                *head = current->next;
            } else {
                // The node to be deleted is not the head
                prev->next = current->next;
            }

            // Use the provided deallocator function to free the data
            if (dealloc != NULL) {
                if (current->data != NULL) {
                    dealloc(current->data);
                }
            }

            // Free the memory allocated for the node
            free(current);

            return;
        }

        prev = current;
        current = current->next;
    }
}

// Function to delete the entire linked list
void delete_list(node_t **head, deallocator_t dealloc) {
    // Base case: list is empty
    if (*head == NULL) {
        return;
    }

    // Recursively delete the rest of the list
    delete_list(&((*head)->next), dealloc);

    // Use the provided deallocator function to free the data
    if (dealloc != NULL) {
        if ((*head)->data != NULL) {
            dealloc((*head)->data);
        }
    }

    // After the recursive call returns, free the current node
    free(*head);

    // Set the head to NULL
    *head = NULL;
}

// Function to print the linked list
void print_list(FILE *fp, node_t *head, printer_t print) {
    node_t *current;

    current = head;
    while (current != NULL) {
        // Use the provided printer function to print the data
        if (print != NULL) {
            print(fp, current->data);
        } else {
            fprintf(fp, "%p", current->data);
        }
        current = current->next;
    } // end while

    fprintf(fp, "NULL\n");
}

// Path: src/list.c