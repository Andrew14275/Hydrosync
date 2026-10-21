/**
 * ============================================================================
 *  HYDRO-SYNC WAREHOUSE INVENTORY (STACK)
 *  File: hydro_stack.h
 *  Purpose: LIFO stack implementation for managing municipal infrastructure 
 *           supplies, batch allocations, and supply degradation.
 *  Targets: SDG 11 (Sustainable Cities & Communities)
 * ============================================================================
 */

#ifndef HYDRO_STACK_H
#define HYDRO_STACK_H

#include "hydro_core.h"

/* -------------------------------------------------------------------------- */
/*                           STACK DATA STRUCTURES                            */
/* -------------------------------------------------------------------------- */

// Category classification for warehouse inventory
typedef enum {
    CAT_PIPING,          // Heavy PVC, steel conduits, joints
    CAT_FILTRATION,      // Industrial bio-filters, reverse osmosis units
    CAT_CHEMICAL,        // Neutralization agents, chlorine tablets
    CAT_SENSORS          // Smart leak detectors, pH telemetry probes
} SupplyCategory;

// Detailed Supply Item Node for the LIFO Stack
typedef struct SupplyNode {
    unsigned long batch_id;
    char item_name[64];
    SupplyCategory category;
    int quantity_units;
    int durability_rating;    // 0-100% condition (degrades over time)
    int treasury_cost;        // Unit value in municipal funds
    int xp_reward;            // Experience gained upon deployment
    struct SupplyNode* next;
} SupplyNode;

// Warehouse Stack Header Container
typedef struct WarehouseStack {
    SupplyNode* top;
    int total_batches;
    int total_unit_count;
    int total_asset_value;
} WarehouseStack;

/* -------------------------------------------------------------------------- */
/*                            INITIALIZATION & SETUP                          */
/* -------------------------------------------------------------------------- */

/**
 * Initializes the municipal warehouse stack header.
 */
void init_warehouse_stack(WarehouseStack* stack) {
    stack->top = NULL;
    stack->total_batches = 0;
    stack->total_unit_count = 0;
    stack->total_asset_value = 0;
    printf("%s[SYS] Warehouse LIFO Inventory Stack Initialized.%s\n", UI_FG_CYAN, UI_RESET);
}

/* -------------------------------------------------------------------------- */
/*                              CORE STACK OPERATIONS                         */
/* -------------------------------------------------------------------------- */

/**
 * Push Operation: Receives a new batch shipment into the warehouse stack.
 * Places the new inventory batch onto the top of the stack.
 */
void push_supply_batch(WarehouseStack* stack, const char* name, SupplyCategory cat, 
                       int quantity, int unit_cost, int xp_val) {
                       
    if (stack->total_batches >= SYS_MAX_INVENTORY) {
        printf("%s[WAREHOUSE OVERFLOW] Storage capacity reached! Shipment rejected.%s\n", 
               UI_FG_RED, UI_RESET);
        return;
    }

    SupplyNode* new_batch = (SupplyNode*)malloc(sizeof(SupplyNode));
    if (!new_batch) {
        printf("%s[FATAL] Stack memory allocation failed.%s\n", UI_BG_RED, UI_RESET);
        return;
    }

    // Assign random batch tracking ID
    new_batch->batch_id = 100000 + (rand() % 900000);
    strncpy(new_batch->item_name, name, 63);
    new_batch->item_name[63] = '\0';
    new_batch->category = cat;
    new_batch->quantity_units = quantity;
    new_batch->durability_rating = 100; // New stock starts at 100% condition
    new_batch->treasury_cost = unit_cost;
    new_batch->xp_reward = xp_val;

    // LIFO Insertion
    new_batch->next = stack->top;
    stack->top = new_batch;

    // Update aggregate stats
    stack->total_batches++;
    stack->total_unit_count += quantity;
    stack->total_asset_value += (quantity * unit_cost);

    printf("%s[WAREHOUSE] Received Batch #%lu: %s (%d units) added to top of Stack.%s\n", 
           UI_FG_GREEN, new_batch->batch_id, name, quantity, UI_RESET);
}

/**
 * Pop Operation: Deploys the most recently received supply batch from the stack top.
 * Deducts units, adds XP to player score, and frees node if fully depleted.
 */
bool pop_supply_item(WarehouseStack* stack, GameState* state, int units_needed) {
    if (stack->top == NULL) {
        printf("%s[WARNING] Warehouse stack is completely empty! Cannot deploy supplies.%s\n", 
               UI_FG_YELLOW, UI_RESET);
        state->city_approval_rating -= 5; // Penalty for stockout
        return false;
    }

    SupplyNode* current = stack->top;

    // Evaluate durability efficiency factor
    float efficiency = (float)current->durability_rating / 100.0f;
    int effective_yield = (int)(units_needed * efficiency);

    printf("%s[DEPLOYING] Using Batch #%lu (%s) | Durability: %d%%%s\n", 
           UI_FG_CYAN, current->batch_id, current->item_name, current->durability_rating, UI_RESET);

    if (current->quantity_units <= units_needed) {
        // Deplete whole top batch node
        int deployed_units = current->quantity_units;
        stack->total_unit_count -= deployed_units;
        stack->total_asset_value -= (deployed_units * current->treasury_cost);
        stack->total_batches--;

        state->score_xp += (current->xp_reward * deployed_units);
        state->total_supplies_deployed += deployed_units;

        stack->top = current->next;
        free(current);
        
        printf("%s[BATCH EXHAUSTED] Top stack node popped and freed.%s\n", UI_FG_YELLOW, UI_RESET);
    } else {
        // Partial removal from top batch
        current->quantity_units -= units_needed;
        stack->total_unit_count -= units_needed;
        stack->total_asset_value -= (units_needed * current->treasury_cost);

        state->score_xp += (current->xp_reward * units_needed);
        state->total_supplies_deployed += units_needed;

        printf("%s[PARTIAL DEPLOY] %d units remaining in top batch.#%lu%s\n", 
               UI_FG_GREEN, current->quantity_units, current->batch_id, UI_RESET);
    }

    return true;
}

/**
 * Peek Operation: Inspects the top item on the stack without removing it.
 */
void peek_top_supply(WarehouseStack* stack) {
    if (stack->top == NULL) {
        printf("%s[INVENTORY PEEK] Stack is empty.%s\n", UI_FG_YELLOW, UI_RESET);
        return;
    }

    SupplyNode* top = stack->top;
    printf("\n%s=== TOP OF WAREHOUSE STACK (PEEK) ===%s\n", UI_BOLD, UI_RESET);
    printf("Batch ID     : #%lu\n", top->batch_id);
    printf("Item Name    : %s\n", top->item_name);
    printf("Units Avail  : %d\n", top->quantity_units);
    printf("Condition    : %d%%\n", top->durability_rating);
    printf("Unit Cost    : $%d\n", top->treasury_cost);
    printf("======================================\n");
}

/* -------------------------------------------------------------------------- */
/*                        SIMULATION TICKS & MAINTENANCE                      */
/* -------------------------------------------------------------------------- */

/**
 * Degrades inventory condition across all stack nodes over time ticks.
 */
void degrade_inventory_condition(WarehouseStack* stack) {
    SupplyNode* current = stack->top;
    while (current != NULL) {
        if (current->durability_rating > 10) {
            current->durability_rating -= (rand() % 3 + 1); // 1-3% wear down
        }
        current = current->next;
    }
}

/**
 * Renders an itemized report of all inventory items in LIFO order (top to bottom).
 */
void display_warehouse_manifest(WarehouseStack* stack) {
    if (stack->top == NULL) {
        printf("%s[MANIFEST] Warehouse inventory is empty.%s\n", UI_FG_YELLOW, UI_RESET);
        return;
    }

    printf("\n%s====================================================%s\n", UI_BOLD, UI_RESET);
    printf("%s       HYDRO-SYNC WAREHOUSE MANIFEST (LIFO)         %s\n", UI_FG_CYAN, UI_RESET);
    printf("%s====================================================%s\n", UI_BOLD, UI_RESET);
    printf("Total Batches: %d | Total Units: %d | Total Value: $%d\n\n", 
           stack->total_batches, stack->total_unit_count, stack->total_asset_value);

    SupplyNode* curr = stack->top;
    int position = 1;
    while (curr != NULL) {
        printf("[%02d] Batch #%lu | %-24s | Units: %4d | Cond: %3d%%\n", 
               position++, curr->batch_id, curr->item_name, curr->quantity_units, curr->durability_rating);
        curr = curr->next;
    }
    printf("====================================================\n\n");
}

/* -------------------------------------------------------------------------- */
/*                               MEMORY CLEANUP                               */
/* -------------------------------------------------------------------------- */

/**
 * Frees every node in the inventory stack on system shutdown.
 */
void free_warehouse_stack(WarehouseStack* stack) {
    SupplyNode* current = stack->top;
    while (current != NULL) {
        SupplyNode* temp = current;
        current = current->next;
        free(temp);
    }
    stack->top = NULL;
    stack->total_batches = 0;
    stack->total_unit_count = 0;
    stack->total_asset_value = 0;
}

#endif // HYDRO_STACK_H