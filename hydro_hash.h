/**
 * ============================================================================
 *  HYDRO-SYNC SENSOR NETWORK (HASHING)
 *  File: hydro_hash.h
 *  Purpose: Hash table implementation with separate chaining for collision 
 *           resolution. Manages high-speed IoT sensor data lookups.
 *  Targets: SDG 6 (Clean Water) & SDG 11 (Sustainable Cities)
 * ============================================================================
 */

#ifndef HYDRO_HASH_H
#define HYDRO_HASH_H

#include "hydro_core.h"

/* -------------------------------------------------------------------------- */
/*                           HASH DATA STRUCTURES                             */
/* -------------------------------------------------------------------------- */

// Represents an individual IoT sensor deployed in the city
typedef struct SensorNode {
    unsigned long sensor_id;       // Unique identifier (Hash Key)
    int assigned_zone_id;          // Links back to the Graph/BST zone
    float current_ph_level;        // 0.0 to 14.0
    float flow_rate_gpm;           // Gallons per minute
    NodeStatus status;
    char last_maintenance[32];
    struct SensorNode* next;       // For separate chaining (collision handling)
} SensorNode;

// The Hash Table directory for the entire sensor network
typedef struct SensorNetworkHash {
    SensorNode* table[SYS_HASH_SIZE];
    int active_sensor_count;
} SensorNetworkHash;

/* -------------------------------------------------------------------------- */
/*                            INITIALIZATION & HASHING                        */
/* -------------------------------------------------------------------------- */

/**
 * Initializes the hash table array to NULL pointers.
 */
void init_sensor_network(SensorNetworkHash* network) {
    for (int i = 0; i < SYS_HASH_SIZE; i++) {
        network->table[i] = NULL;
    }
    network->active_sensor_count = 0;
    printf("%s[SYS] IoT Sensor Network Initialized. Hash Table Size: %d%s\n", 
           UI_FG_CYAN, SYS_HASH_SIZE, UI_RESET);
}

/**
 * Primary Hash Function using the Division Method.
 * Generates an array index based on the unique sensor ID.
 */
int hash_sensor_id(unsigned long sensor_id) {
    return sensor_id % SYS_HASH_SIZE;
}

/* -------------------------------------------------------------------------- */
/*                              CORE OPERATIONS                               */
/* -------------------------------------------------------------------------- */

/**
 * Registers a new sensor into the Hash Table.
 * Handles collisions by prepending to the linked list at the hashed index.
 */
void register_sensor(SensorNetworkHash* network, unsigned long sensor_id, 
                    int zone_id, float ph, float flow) {
    
    int index = hash_sensor_id(sensor_id);

    // Check for duplicate ID to prevent data corruption
    SensorNode* current = network->table[index];
    while (current != NULL) {
        if (current->sensor_id == sensor_id) {
            printf("%s[ERROR] Sensor ID %lu already exists in the network.%s\n", 
                   UI_FG_RED, sensor_id, UI_RESET);
            return;
        }
        current = current->next;
    }

    // Allocate and initialize new sensor node
    SensorNode* new_sensor = (SensorNode*)malloc(sizeof(SensorNode));
    if (!new_sensor) {
        printf("%s[FATAL] Memory allocation failed for new sensor.%s\n", UI_BG_RED, UI_RESET);
        return;
    }

    new_sensor->sensor_id = sensor_id;
    new_sensor->assigned_zone_id = zone_id;
    new_sensor->current_ph_level = ph;
    new_sensor->flow_rate_gpm = flow;
    new_sensor->status = STAT_ONLINE;
    strcpy(new_sensor->last_maintenance, "T-0 (Deployment)");
    
    // Insert via Separate Chaining (prepend to list)
    new_sensor->next = network->table[index];
    network->table[index] = new_sensor;
    network->active_sensor_count++;

    printf("%s[SENSOR ADDED] ID: %lu registered to Zone %d. (Hash Index: %d)%s\n", 
           UI_FG_GREEN, sensor_id, zone_id, index, UI_RESET);
}

/**
 * $O(1)$ Average Time Lookup for a specific sensor.
 * Returns a pointer to the sensor node if found, or NULL.
 */
SensorNode* lookup_sensor(SensorNetworkHash* network, unsigned long sensor_id) {
    int index = hash_sensor_id(sensor_id);
    SensorNode* current = network->table[index];

    // Traverse the chain in case of collisions
    while (current != NULL) {
        if (current->sensor_id == sensor_id) {
            return current;
        }
        current = current->next;
    }
    
    return NULL; // Sensor not found
}

/**
 * Diagnostic function to print sensor details nicely to the terminal UI.
 */
void display_sensor_diagnostics(SensorNetworkHash* network, unsigned long sensor_id) {
    SensorNode* target = lookup_sensor(network, sensor_id);
    
    if (target == NULL) {
        printf("%s[404] SENSOR NOT FOUND: No telemetry for ID %lu.%s\n", 
               UI_FG_YELLOW, sensor_id, UI_RESET);
        return;
    }

    printf("\n%s=== SENSOR %lu DIAGNOSTICS ===%s\n", UI_BOLD, sensor_id, UI_RESET);
    printf("Assigned Zone : %d\n", target->assigned_zone_id);
    
    // Color code pH levels (Normal is ~6.5 to 8.5)
    if (target->current_ph_level < 6.5 || target->current_ph_level > 8.5) {
        printf("pH Level      : %s%.2f (DANGER)%s\n", UI_FG_RED, target->current_ph_level, UI_RESET);
    } else {
        printf("pH Level      : %s%.2f (NORMAL)%s\n", UI_FG_GREEN, target->current_ph_level, UI_RESET);
    }
    
    printf("Flow Rate     : %.2f GPM\n", target->flow_rate_gpm);
    printf("Status        : %s\n", target->status == STAT_ONLINE ? "ONLINE" : "OFFLINE");
    printf("Last Service  : %s\n", target->last_maintenance);
    printf("==============================\n\n");
}

/* -------------------------------------------------------------------------- */
/*                               MEMORY CLEANUP                               */
/* -------------------------------------------------------------------------- */

/**
 * Safely frees the entire hash table to prevent memory leaks on exit.
 */
void free_sensor_network(SensorNetworkHash* network) {
    for (int i = 0; i < SYS_HASH_SIZE; i++) {
        SensorNode* current = network->table[i];
        while (current != NULL) {
            SensorNode* temp = current;
            current = current->next;
            free(temp);
        }
        network->table[i] = NULL;
    }
    network->active_sensor_count = 0;
}

#endif // HYDRO_HASH_H