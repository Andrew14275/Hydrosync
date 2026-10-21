/**
 * ============================================================================
 *  HYDRO-SYNC ENVIRONMENTAL TRACKING (BST)
 *  File: hydro_bst.h
 *  Purpose: Binary Search Tree for hierarchical Water Quality Index (WQI) 
 *           tracking across municipal zones.
 *  Targets: SDG 11 (Sustainable Cities) & SDG 6 (Clean Water)
 * ============================================================================
 */

#ifndef HYDRO_BST_H
#define HYDRO_BST_H

#include "hydro_core.h"

/* -------------------------------------------------------------------------- */
/*                            BST DATA STRUCTURES                             */
/* -------------------------------------------------------------------------- */

// Represents an environmental data node in the BST
typedef struct WQINode {
    int zone_id;
    int water_quality_index;       // 0-100 (100 = Pure, < 50 = Hazardous)
    int historical_low;            // Tracks the worst recorded state
    int historical_high;           // Tracks the best recorded state
    int update_count;              // Number of times this zone's data changed
    struct WQINode* left;
    struct WQINode* right;
} WQINode;

/* -------------------------------------------------------------------------- */
/*                            CORE BST OPERATIONS                             */
/* -------------------------------------------------------------------------- */

/**
 * Inserts a new zone into the BST or updates an existing zone's WQI.
 * Ordered strictly by zone_id.
 */
WQINode* update_zone_wqi(WQINode* root, int zone_id, int new_wqi) {
    if (root == NULL) {
        WQINode* new_node = (WQINode*)malloc(sizeof(WQINode));
        if (!new_node) {
            printf("%s[FATAL] BST Memory allocation failed.%s\n", UI_BG_RED, UI_RESET);
            return NULL;
        }
        new_node->zone_id = zone_id;
        new_node->water_quality_index = new_wqi;
        new_node->historical_low = new_wqi;
        new_node->historical_high = new_wqi;
        new_node->update_count = 1;
        new_node->left = new_node->right = NULL;
        return new_node;
    }

    if (zone_id < root->zone_id) {
        root->left = update_zone_wqi(root->left, zone_id, new_wqi);
    } else if (zone_id > root->zone_id) {
        root->right = update_zone_wqi(root->right, zone_id, new_wqi);
    } else {
        // Zone exists, update metrics
        root->water_quality_index = new_wqi;
        root->update_count++;
        if (new_wqi < root->historical_low) root->historical_low = new_wqi;
        if (new_wqi > root->historical_high) root->historical_high = new_wqi;
    }
    return root;
}

/**
 * Searches for a specific zone's environmental data in O(log N) time.
 */
WQINode* search_zone_wqi(WQINode* root, int zone_id) {
    if (root == NULL || root->zone_id == zone_id) {
        return root;
    }
    if (root->zone_id < zone_id) {
        return search_zone_wqi(root->right, zone_id);
    }
    return search_zone_wqi(root->left, zone_id);
}

/* -------------------------------------------------------------------------- */
/*                           ANALYTICAL TRAVERSALS                            */
/* -------------------------------------------------------------------------- */

/**
 * In-Order Traversal to display a sorted list of all monitored zones.
 * Color-codes the output based on safety thresholds.
 */
void display_wqi_report(WQINode* root) {
    if (root != NULL) {
        display_wqi_report(root->left);
        
        printf("Zone ID: %02d | WQI: ", root->zone_id);
        if (root->water_quality_index >= 80) {
            printf("%s%3d (SAFE)%s", UI_FG_GREEN, root->water_quality_index, UI_RESET);
        } else if (root->water_quality_index >= 50) {
            printf("%s%3d (WARN)%s", UI_FG_YELLOW, root->water_quality_index, UI_RESET);
        } else {
            printf("%s%3d (CRIT)%s", UI_FG_RED, root->water_quality_index, UI_RESET);
        }
        printf(" | Low: %3d | High: %3d | Updates: %d\n", 
               root->historical_low, root->historical_high, root->update_count);
        
        display_wqi_report(root->right);
    }
}

/**
 * Helper function to calculate the city-wide average WQI.
 */
void _sum_wqi(WQINode* root, int* total_wqi, int* count) {
    if (root != NULL) {
        _sum_wqi(root->left, total_wqi, count);
        *total_wqi += root->water_quality_index;
        (*count)++;
        _sum_wqi(root->right, total_wqi, count);
    }
}

/**
 * Returns the average Water Quality Index across all registered zones.
 * Impacts the global GameState approval rating.
 */
int get_city_average_wqi(WQINode* root) {
    if (root == NULL) return 100; // Default to perfect if no data
    
    int total_wqi = 0;
    int count = 0;
    _sum_wqi(root, &total_wqi, &count);
    
    return count == 0 ? 100 : (total_wqi / count);
}

/**
 * Helper function to find the zone with the lowest current WQI.
 */
void _find_worst_wqi(WQINode* root, WQINode** worst_node) {
    if (root != NULL) {
        if (*worst_node == NULL || root->water_quality_index < (*worst_node)->water_quality_index) {
            *worst_node = root;
        }
        _find_worst_wqi(root->left, worst_node);
        _find_worst_wqi(root->right, worst_node);
    }
}

/**
 * Identifies the most polluted zone currently tracked in the BST.
 */
WQINode* get_worst_polluted_zone(WQINode* root) {
    WQINode* worst = NULL;
    _find_worst_wqi(root, &worst);
    return worst;
}

/* -------------------------------------------------------------------------- */
/*                               MEMORY CLEANUP                               */
/* -------------------------------------------------------------------------- */

/**
 * Post-Order Traversal to safely free the entire BST from memory.
 */
void free_wqi_bst(WQINode* root) {
    if (root != NULL) {
        free_wqi_bst(root->left);
        free_wqi_bst(root->right);
        free(root);
    }
}

#endif // HYDRO_BST_H