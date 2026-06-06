#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h> 
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h> 
#include "rstack.h"


typedef struct rstack_node rstack_node_t;

// Stack node representing either a value or a sub-stack pointer.
struct rstack_node {
    uint64_t val;
    bool is_stack; 
    rstack_t *child_stack; // sub-stack pointer
    rstack_node_t *next; // Next node in line
};

struct rstack {
    rstack_node_t *top; // Top node in current stack
    bool is_held_by_user; // Becomes false when rstack_delete is called
    bool is_visited; // Used for DFS and cycle detecting
    rstack_t *global_next; // Pointer to the next stack in global registry
};

 // Global registry of all stacks in use.
 // Every created stack is linked here until it is deleted.
static rstack_t *all_stack = nullptr;


// Resets the is_visited flag for all stacks in global registry.
static void clear_visited() {
    rstack_t *s_reset = all_stack;
    while (s_reset != nullptr) {
        s_reset->is_visited = false;
        s_reset = s_reset->global_next;
    }
}

// Packs flags and values into a result structure.
static result_t wrap_result(bool flag, uint64_t value) {
    result_t res;
    res.flag = flag;
    res.value = value;
    return res;
}

// Full node destruction without activating cleanup.
static void pop_node_without_running_cleanup(rstack_t *rs) {
    if (rs == nullptr || rs->top == nullptr) {
        return;
    }
    rstack_node_t *old_top = rs->top;
    rs->top = old_top->next;
    free(old_top);
}

// Full stack destruction without activating cleanup.
static void delete_stack_without_running_cleanup(rstack_t *rs) {
    if (rs == nullptr) {
        return;
    }

    while (rs->top != nullptr) {
        pop_node_without_running_cleanup(rs);
    }
    free(rs);
}

// Marks stacks that are still in use to prevent them
// from being removed during cleanup.
static void mark_reachable_dfs(rstack_t *rs) {
    if (rs == nullptr || rs->is_visited) {
        return;
    }

    rs->is_visited = true;

    rstack_node_t *curr = rs->top;
    while (curr != nullptr) {
        if (curr->is_stack) {
            mark_reachable_dfs(curr->child_stack);
        }
        curr = curr->next;
    }
}

// Identifies and deallocates stacks not reachable by user.
static void delete_unreachable_stacks() {
    clear_visited();

    // Mark all stacks reachable from stacks currently held by user.
    rstack_t *s_mark = all_stack;
    while (s_mark != nullptr) {
        if (s_mark->is_held_by_user) {
            mark_reachable_dfs(s_mark);
        }
        s_mark = s_mark->global_next;
    }

    // Free stacks that was not marked and 
    // remove them from global registry.
    rstack_t *curr = all_stack;
    rstack_t *prev = nullptr;
    while (curr != nullptr) {
        if (!curr->is_visited) {
            rstack_t *to_delete = curr;
            rstack_t *next_in_line = curr->global_next;

            if (prev == nullptr) {
                all_stack = next_in_line;
            } else {
                prev->global_next = next_in_line;
            }
            delete_stack_without_running_cleanup(to_delete);
            curr = next_in_line;
        } else {
            prev = curr;
            curr = curr->global_next;
        }
    }
}

// Initializes a new empty stack and add it to global list.
rstack_t* rstack_new() {
    rstack_t *rs = malloc(sizeof(rstack_t));
    if (rs == nullptr) {
        errno = ENOMEM;
        return nullptr;
    }

    rs->top = nullptr;
    rs->is_held_by_user = true;
    rs->is_visited = false;
    rs->global_next = all_stack;
    all_stack = rs;
    return rs;
}

// Marks the stack as no longer used by the user and 
// cleans up unreachable memory.
void rstack_delete(rstack_t *rs) {
    if (rs == nullptr) {
        return;
    }
    
    rs->is_held_by_user = false;
    delete_unreachable_stacks();
}

// Removes the top node and triggers cleanup.
void rstack_pop(rstack_t *rs) {
    if (rs == nullptr || rs->top == nullptr) {
        return;
    }

    pop_node_without_running_cleanup(rs);
    delete_unreachable_stacks();
}

// Pushes a 64-bit unsigned int onto the top of the stack.
int rstack_push_value(rstack_t *rs, uint64_t value) {
    if (rs == nullptr) { 
        errno = EINVAL;
        return -1;
    }

    rstack_node_t *new_node = malloc(sizeof(rstack_node_t));
    if (new_node == nullptr) {
        errno = ENOMEM;
        return -1;
    }

    new_node->val = value;
    new_node->is_stack = false;
    new_node->child_stack = nullptr;
    new_node->next = rs->top;
    rs->top = new_node;

    return 0;
}

// Pushes a reference of the stack rs2 onto the top of the stack (rs1).
int rstack_push_rstack(rstack_t *rs1, rstack_t *rs2) {
    if (rs1 == nullptr || rs2 == nullptr) { 
        errno = EINVAL;
        return -1;
    }

    rstack_node_t *new_node = malloc(sizeof(rstack_node_t));
    if (new_node == nullptr) {
        errno = ENOMEM;
        return -1;
    }
    new_node->val = 0;
    new_node->is_stack = true;
    new_node->child_stack = rs2;
    new_node->next = rs1->top;
    rs1->top = new_node;

    return 0;
}

// Performs a DFS to find the first available value,
// skipping cycles and empty stacks.
static result_t rstack_front_dfs(rstack_t *rs) {
    if(rs == nullptr || rs->is_visited) {
        return wrap_result(false, 0);
    }

    rs->is_visited = true;

    if (rs->top == nullptr) {
        return wrap_result(false, 0);
    }
    
    rstack_node_t *curr_node = rs->top;
    while (curr_node != nullptr) {
        if (!curr_node->is_stack) {
            return wrap_result(true, curr_node->val);
        } else {
            // Returns immediately if a value is found in the substack.
            result_t res = rstack_front_dfs(curr_node->child_stack);
            if (res.flag) {
                return res;
            }
        }
        curr_node = curr_node->next;
    }
    return wrap_result(false, 0);
}

// Returns the first value found in the stack.
result_t rstack_front(rstack_t *rs) {
    if (rs == nullptr) {
        return wrap_result(false, 0);
    }
    clear_visited();
    return rstack_front_dfs(rs);
}

// Checks if the stack does not contain values.
bool rstack_empty(rstack_t *rs) {
    result_t res = rstack_front(rs);
    return !res.flag;
}

// Creates a stack from a file containing whitespace-separated integers.
rstack_t* rstack_read(char const *path) {
    if (path == nullptr) {
        errno = EINVAL;
        return nullptr;
    }

    FILE *f = fopen(path, "r");
    if (f == nullptr) {
        // errno is set by fopen
        return nullptr;
    }

    // Allocate manually to keep the stack hidden from the global list
    // until the parsing is fully successful.
    rstack_t *rs = malloc(sizeof(rstack_t));
    if (rs == nullptr) {
        fclose(f);
        errno = ENOMEM;
        return nullptr;
    }
    rs->top = nullptr;
    rs->is_held_by_user = true;
    rs->is_visited = false;
    
    int c = fgetc(f);
    while (c != EOF) {
        if (isspace((unsigned char)c)) {
            c = fgetc(f);
            continue;
        }

        if (!isdigit((unsigned char)c)) {
            delete_stack_without_running_cleanup(rs);
            fclose(f);
            errno = EINVAL;
            return nullptr;
        }
        
        uint64_t val = 0;
        // String to integer conversion.
        while (c != EOF && isdigit((unsigned char)c)) {
            int digit = c - '0';
            // Boundary check to prevent uint64 overflow.
            if (val > (UINT64_MAX - (uint64_t)digit) / 10) {
                delete_stack_without_running_cleanup(rs);
                fclose(f);
                errno = ERANGE;
                return nullptr;
            }
            val = val * 10 + digit;
            c = fgetc(f);
        }

        if (rstack_push_value(rs, val) != 0) {
            delete_stack_without_running_cleanup(rs);
            fclose(f);
            errno = ENOMEM;
            return nullptr;
        }
    }
    if (fclose(f) != 0) {
        delete_stack_without_running_cleanup(rs);
        return nullptr;
    }
    
    // Registration with the global list after successful file read.
    rs->global_next = all_stack;
    all_stack = rs;
    return rs;
}

// Performs a bottom-up DFS to write stack values to the file.
// Returns 
// 1 when all values where successfully written without cycles
// 0 when we found cycle
// -1 when error occurred.
static int rstack_write_dfs(FILE *f, rstack_node_t *node) {
    if (node == nullptr) {
        return 1;
    }

    int next_res = rstack_write_dfs(f, node->next);
    if (next_res != 1) {
        return next_res;
    }

    if (node->is_stack) {
        rstack_t *child = node->child_stack;
        if (child != nullptr) {
            if (child->is_visited) {
                return 0;
            }

            // Mark as visited during substack traversal, then clear.
            child->is_visited = true;
            int child_res = rstack_write_dfs(f, child->top);
            child->is_visited = false;
            if (child_res != 1) {
                return child_res;
            }
        }
    } else {
        if (fprintf(f, "%" PRIu64 "\n", node->val) < 0) {
            // errno is set by fprintf.
            return -1;
        }
    }
    return 1;
}

// Writes all reachable values to a file in a bottom-up order.
int rstack_write(char const *path, rstack_t *rs) {
    if (path == nullptr || rs == nullptr) {
        errno = EINVAL;
        return -1;
    }

    FILE *f = fopen(path, "w");
    if (f == nullptr) {
        // errno is set by fopen.
        return -1;
    }
    clear_visited();

    // Protect the root from self-referencing cycle.
    rs->is_visited = true;
    int result = rstack_write_dfs(f, rs->top);
    if (fclose(f) != 0) {
        return -1;
    }

    if (result == -1) {
        return -1;
    }
    return 0; 
}

