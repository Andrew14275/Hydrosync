/**
 * ============================================================================
 *  HYDRO-SYNC ADVANCED SEARCH ALGORITHMS
 *  File: hydro_search.h
 *  Purpose: BFS, DFS, and Binary Search implementations for grid scanning, 
 *           contamination tracing, and log retrieval.
 *  Targets: SDG 11 (Sustainable Cities) & SDG 6 (Clean Water)
 * ============================================================================
 */

#ifndef HYDRO_SEARCH_H
#define HYDRO_SEARCH_H

#include "hydro_core.h"
#include "hydro_graph.h"

/* -------------------------------------------------------------------------- */
/*                          1. BREADTH-FIRST SEARCH (BFS)                     */
/* -------------------------------------------------------------------------- */

/**
 * Executes a radial BFS scan from a designated command center (start_zone).
 * Used to ping surrounding zones layer by layer to assess immediate structural damage.
 */
int execute_radial_bfs_scan(CityGraph* graph, int start_zone, GameState* state) {
    if (start_zone < 0 || start_zone >= graph->total_zones) {
        printf("%s[ERROR] Invalid Zone ID for BFS Scan.%s\n", UI_FG_RED, UI_RESET);
        return 0;
    }

    bool visited[SYS_MAX_ZONES] = {false};
    int queue[SYS_MAX_ZONES];
    int front = 0, rear = 0;
    int vulnerable_zones_found = 0;

    printf("\n%s=== RADIAL BFS INFRASTRUCTURE SCAN (ZONE %d) ===%s\n", UI_FG_CYAN, start_zone, UI_RESET);
    
    // Enqueue starting node
    visited[start_zone] = true;
    queue[rear++] = start_zone;

    // Expend treasury funds for the radar scan
    state->treasury_funds -= 150; 

    while (front < rear) {
        int current = queue[front++];
        CityZone* zone_data = &graph->vertices[current].data;

        // Animate the scan delay slightly for gamified effect
        printf("Scanning %-10s [ID: %02d] ... ", zone_data->zone_name, current);
        delay_ms(100); 

        if (zone_data->infrastructure_health < 50) {
            printf("%sSTRUCTURAL COMPROMISE DETECTED (%d%%)%s\n", 
                   UI_FG_RED, zone_data->infrastructure_health, UI_RESET);
            vulnerable_zones_found++;
        } else if (zone_data->infrastructure_health < 80) {
            printf("%sWEAR DETECTED (%d%%)%s\n", 
                   UI_FG_YELLOW, zone_data->infrastructure_health, UI_RESET);
        } else {
            printf("%sOPTIMAL%s\n", UI_FG_GREEN, UI_RESET);
        }

        // Traverse adjacency list for BFS expansion
        PipeEdge* temp = graph->vertices[current].head;
        while (temp != NULL) {
            int adj = temp->dest_zone_id;
            if (!visited[adj]) {
                visited[adj] = true;
                queue[rear++] = adj;
            }
            temp = temp->next;
        }
    }
    
    printf("%sScan Complete. Found %d vulnerable zones.%s\n\n", 
           UI_BOLD, vulnerable_zones_found, UI_RESET);
           
    return vulnerable_zones_found;
}

/* -------------------------------------------------------------------------- */
/*                           2. DEPTH-FIRST SEARCH (DFS)                      */
/* -------------------------------------------------------------------------- */

/**
 * Recursive DFS utility to aggressively trace interconnected pipes.
 */
void _dfs_contamination_trace(CityGraph* graph, int current, bool visited[], int* spread_count) {
    visited[current] = true;
    (*spread_count)++;
    
    CityZone* zone_data = &graph->vertices[current].data;
    
    // Contamination damages the water quality of every zone it touches
    zone_data->water_quality_index -= 15;
    if (zone_data->water_quality_index < 0) zone_data->water_quality_index = 0;

    printf("%s   -> [TRACE] Contamination spread to: %s (WQI dropped to %d)%s\n", 
           UI_FG_MAGENTA, zone_data->zone_name, zone_data->water_quality_index, UI_RESET);

    delay_ms(150);

    PipeEdge* temp = graph->vertices[current].head;
    while (temp != NULL) {
        int adj = temp->dest_zone_id;
        if (!visited[adj]) {
            _dfs_contamination_trace(graph, adj, visited, spread_count);
        }
        temp = temp->next;
    }
}

/**
 * Initiates a DFS traversal when a catastrophic chemical spill occurs.
 * This simulates the rapid spread of toxins through deep pipe networks.
 */
void trigger_dfs_spill_event(CityGraph* graph, int origin_zone, GameState* state) {
    if (origin_zone < 0 || origin_zone >= graph->total_zones) return;

    bool visited[SYS_MAX_ZONES] = {false};
    int spread_count = 0;

    printf("\n%s%s=== CATASTROPHIC SPILL DETECTED AT ZONE %d ===%s\n", 
           UI_BG_RED, UI_FG_WHITE, origin_zone, UI_RESET);
    printf("%s[SYS] Initiating DFS to trace toxic spread vectors...%s\n", UI_FG_RED, UI_RESET);

    _dfs_contamination_trace(graph, origin_zone, visited, &spread_count);

    printf("%s\n[IMPACT] Toxin reached %d interconnected zones.%s\n", 
           UI_BOLD, spread_count, UI_RESET);
           
    // Heavy penalty for a deep spill
    state->city_approval_rating -= (spread_count * 2); 
}

/* -------------------------------------------------------------------------- */
/*                           3. BINARY SEARCH (ARRAYS)                        */
/* -------------------------------------------------------------------------- */

// Represents a historical log entry (requires a sorted array for Binary Search)
typedef struct {
    unsigned long timestamp_tick;
    char event_log[64];
} MaintenanceLog;

/**
 * Standard O(log N) Binary Search over a chronologically sorted array of logs.
 * Used by the player to audit past maintenance actions at a specific simulation tick.
 */
int binary_search_logs(MaintenanceLog logs[], int left, int right, unsigned long target_tick) {
    while (left <= right) {
        int mid = left + (right - left) / 2;

        if (logs[mid].timestamp_tick == target_tick) {
            return mid; // Found exactly at this tick
        }
        if (logs[mid].timestamp_tick < target_tick) {
            left = mid + 1; // Search right half
        } else {
            right = mid - 1; // Search left half
        }
    }
    return -1; // Log not found
}

/**
 * Gamified wrapper for the Binary Search function.
 */
void audit_maintenance_records(MaintenanceLog logs[], int total_logs, unsigned long target_tick) {
    printf("\n%s[AUDIT] Running O(log N) Binary Search for Tick #%lu...%s\n", 
           UI_FG_BLUE, target_tick, UI_RESET);
           
    int result_index = binary_search_logs(logs, 0, total_logs - 1, target_tick);
    
    if (result_index != -1) {
        printf("%s[FOUND] Match at Index %d:%s %s\n", 
               UI_FG_GREEN, result_index, UI_RESET, logs[result_index].event_log);
    } else {
        printf("%s[NOT FOUND] No maintenance records exist for Tick #%lu.%s\n", 
               UI_FG_YELLOW, target_tick, UI_RESET);
    }
}

#endif // HYDRO_SEARCH_H