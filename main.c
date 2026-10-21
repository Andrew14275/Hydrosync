/**
 * ============================================================================
 *  HYDRO-SYNC: SUSTAINABLE CITY CONTROLLER
 *  File: main.c
 *  Purpose: Master execution loop, UI rendering, event spawning, and command
 *           integration for all 8 underlying data structures.
 *  Targets: SDG 6 & SDG 11
 * Made by Smit and Gagan with Efforts
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

// Include all 8 architectural headers
#include "hydro_core.h"
#include "hydro_graph.h"
#include "hydro_hash.h"
#include "hydro_bst.h"
#include "hydro_stack.h"
#include "hydro_queue.h"
#include "hydro_pq.h"
#include "hydro_search.h"

/* -------------------------------------------------------------------------- */
/*                            GLOBAL INSTANTIATIONS                           */
/* -------------------------------------------------------------------------- */

GameState global_state;

CityGraph         city_map;
SensorNetworkHash sensor_network;
WQINode*          water_quality_bst = NULL;
WarehouseStack    warehouse;
EmergencyQueue    standard_hotline;
PriorityQueue     critical_triage;

MaintenanceLog    sys_logs[100];
int               total_sys_logs = 0;

/* -------------------------------------------------------------------------- */
/*                              UTILITY FUNCTIONS                             */
/* -------------------------------------------------------------------------- */

/**
 * Cross-platform millisecond delay for UI animations.
 */
void delay_ms(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

void init_game_state(const char* player_name) {
    strncpy(global_state.mayor_name, player_name, 63);
    global_state.score_xp = 0;
    global_state.city_approval_rating = 100;
    global_state.global_water_reserves = 50000;
    global_state.treasury_funds = 15000;
    global_state.current_time.day = 1;
    global_state.current_time.hour = 8;
    global_state.current_time.minute = 0;
    global_state.current_time.total_ticks = 0;
    global_state.is_simulation_active = true;
    
    global_state.total_emergencies_resolved = 0;
    global_state.total_supplies_deployed = 0;
    global_state.zones_scanned = 0;
}

/**
 * Creates the initial municipal grid, connecting zones and deploying sensors.
 */
void populate_initial_city() {
    init_city_graph(&city_map, 10); // Start with 10 zones
    
    // Build initial water routing grid
    add_pipe_connection(&city_map, 0, 1, 500, 5);
    add_pipe_connection(&city_map, 1, 2, 450, 8);
    add_pipe_connection(&city_map, 2, 3, 600, 12);
    add_pipe_connection(&city_map, 3, 4, 300, 4);
    add_pipe_connection(&city_map, 0, 5, 800, 15);
    add_pipe_connection(&city_map, 5, 6, 400, 7);
    add_pipe_connection(&city_map, 6, 7, 550, 9);
    add_pipe_connection(&city_map, 4, 8, 350, 11);
    add_pipe_connection(&city_map, 8, 9, 200, 6);
    add_pipe_connection(&city_map, 3, 7, 700, 14); // Cross-city main
    
    // Deploy IoT Sensors
    for (int i = 0; i < 15; i++) {
        unsigned long sid = 1000 + rand() % 8999;
        int zid = rand() % 10;
        register_sensor(&sensor_network, sid, zid, 7.0 + ((rand() % 20 - 10) / 10.0), 100.0 + (rand() % 50));
    }
    
    // Bootstrap WQI BST
    for (int i = 0; i < 10; i++) {
        water_quality_bst = update_zone_wqi(water_quality_bst, i, 90 + (rand() % 10));
    }

    // Add some initial stock
    push_supply_batch(&warehouse, "Standard PVC Pipes", CAT_PIPING, 50, 20, 5);
    push_supply_batch(&warehouse, "Bio-Filter Mesh", CAT_FILTRATION, 30, 45, 10);
    
    // Seed maintenance logs for binary search
    for (int i = 0; i < 20; i++) {
        sys_logs[i].timestamp_tick = i * 5;
        sprintf(sys_logs[i].event_log, "Routine network flush in sector %d.", rand() % 10);
        total_sys_logs++;
    }
}

/* -------------------------------------------------------------------------- */
/*                         DYNAMIC EVENT GENERATOR                            */
/* -------------------------------------------------------------------------- */

void spawn_random_event() {
    int chance = rand() % 100;
    int target_zone = rand() % city_map.total_zones;

    if (chance < 15) { // 15% chance of standard issue
        const char* issues[] = {"Minor Pipe Leak", "Clogged Drain", "Filter Replacement Needed", "Low Pressure Report"};
        enqueue_standard_issue(&standard_hotline, &global_state, target_zone, issues[rand() % 4]);
    } 
    else if (chance > 95) { // 5% chance of critical crisis
        const char* threats[] = {"Toxic Runoff", "Main Valve Explosion", "Biological Contamination", "Sewage Backflow"};
        enqueue_critical_alert(&critical_triage, &global_state, target_zone, threats[rand() % 4], (rand() % 3) + 1);
        
        // If it's a spill, trigger DFS spread trace
        if (chance > 98) {
            trigger_dfs_spill_event(&city_map, target_zone, &global_state);
        }
    }
}

void advance_simulation_tick() {
    global_state.current_time.total_ticks++;
    global_state.current_time.minute += 15;
    if (global_state.current_time.minute >= 60) {
        global_state.current_time.minute = 0;
        global_state.current_time.hour++;
        if (global_state.current_time.hour >= 24) {
            global_state.current_time.hour = 0;
            global_state.current_time.day++;
        }
    }

    // Process system degradations
    process_queue_aging(&standard_hotline, &global_state);
    process_critical_aging(&critical_triage, &global_state);
    if (global_state.current_time.total_ticks % 10 == 0) {
        degrade_inventory_condition(&warehouse);
    }

    spawn_random_event();

    // Check game over conditions
    if (global_state.city_approval_rating <= 0 || global_state.treasury_funds < 0) {
        global_state.is_simulation_active = false;
    }
}

/* -------------------------------------------------------------------------- */
/*                                UI RENDERING                                */
/* -------------------------------------------------------------------------- */

void render_ui_header() {
    printf("%s", UI_CLEAR);
    printf("%s%s", UI_BOLD, UI_FG_CYAN);
    printf("=================================================================================\n");
    printf(" || HYDRO-SYNC MUNICIPAL CONTROLLER v3.0 || Director: %s \n", global_state.mayor_name);
    printf("=================================================================================\n");
    printf("%s", UI_RESET);
    
    // Status Bar
    printf(" TIME: Day %02d | %02d:%02d | TICK: %04lu  ", 
           global_state.current_time.day, global_state.current_time.hour, 
           global_state.current_time.minute, global_state.current_time.total_ticks);
    
    printf("|| FUNDS: $%d  || XP: %d \n", global_state.treasury_funds, global_state.score_xp);
    
    // City Health Progress Bar
    printf(" APPROVAL: [");
    int bars = global_state.city_approval_rating / 5;
    for (int i = 0; i < 20; i++) {
        if (i < bars) {
            if (bars > 10) printf("%s#%s", UI_FG_GREEN, UI_RESET);
            else if (bars > 5) printf("%s#%s", UI_FG_YELLOW, UI_RESET);
            else printf("%s#%s", UI_FG_RED, UI_RESET);
        } else {
            printf(" ");
        }
    }
    printf("] %d%%\n", global_state.city_approval_rating);
    printf("---------------------------------------------------------------------------------\n");
    
    // Data Structure Overviews
    printf(" WAREHOUSE (Stack) : %d Batches | HOTLINE (Queue): %d Pending\n", 
           warehouse.total_batches, standard_hotline.total_pending_tickets);
    printf(" CRITICAL (PQ)     : %d Active  | SENSORS (Hash) : %d Online\n", 
           critical_triage.active_critical_threats, sensor_network.active_sensor_count);
    printf("=================================================================================\n\n");
}

/* -------------------------------------------------------------------------- */
/*                              MAIN EXECUTION                                */
/* -------------------------------------------------------------------------- */

int main() {
    srand((unsigned int)time(NULL));
    char input_buffer[100];
    int choice;

    // Initialization
    init_game_state("Gagan Gowda");
    init_sensor_network(&sensor_network);
    init_warehouse_stack(&warehouse);
    init_standard_queue(&standard_hotline);
    init_priority_queue(&critical_triage);
    populate_initial_city();

    // Main Game Loop
    while (global_state.is_simulation_active) {
        render_ui_header();

        printf("--- COMMAND DASHBOARD ---\n");
        printf("1. View Infrastructure Maps (BST & Graph)\n");
        printf("2. View Pending Emergencies (Queue & PQ)\n");
        printf("3. Dispatch Team to Standard Issue (Dequeue)\n");
        printf("4. Dispatch HAZMAT to Critical Threat (PQ Dequeue)\n");
        printf("5. Manage Warehouse Inventory (Stack Push/Pop)\n");
        printf("6. Execute Zone Radar Scan (BFS)\n");
        printf("7. Lookup Historical Log (Binary Search)\n");
        printf("8. Advance Time Tick (Skip Turn)\n");
        printf("9. Exit Simulation\n");
        printf("Select command (1-9): ");

        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) break;
        if (sscanf(input_buffer, "%d", &choice) != 1) continue;

        printf("\n");

        switch (choice) {
            case 1:
                printf("%s--- WATER QUALITY INDEX (BST) ---%s\n", UI_FG_CYAN, UI_RESET);
                display_wqi_report(water_quality_bst);
                printf("\nCity Average WQI: %d/100\n", get_city_average_wqi(water_quality_bst));
                break;
            case 2:
                display_hotline_queue(&standard_hotline, &global_state);
                display_critical_threats(&critical_triage, &global_state);
                break;
            case 3:
                resolve_standard_issue(&standard_hotline, &global_state);
                break;
            case 4:
                resolve_critical_alert(&critical_triage, &global_state);
                break;
            case 5:
                printf("1. Push (Order Supply) | 2. Pop (Deploy Supply) | 3. View Manifest: ");
                if (fgets(input_buffer, sizeof(input_buffer), stdin) && sscanf(input_buffer, "%d", &choice) == 1) {
                    if (choice == 1) push_supply_batch(&warehouse, "Heavy Duty Pump", CAT_PIPING, 20, 100, 15);
                    else if (choice == 2) pop_supply_item(&warehouse, &global_state, 5);
                    else if (choice == 3) display_warehouse_manifest(&warehouse);
                }
                break;
            case 6: {
                int z;
                printf("Enter Zone ID to scan (0-%d): ", city_map.total_zones - 1);
                if (fgets(input_buffer, sizeof(input_buffer), stdin) && sscanf(input_buffer, "%d", &z) == 1) {
                    execute_radial_bfs_scan(&city_map, z, &global_state);
                }
                break;
            }
            case 7: {
                unsigned long target_tick;
                printf("Enter target Tick ID to audit: ");
                if (fgets(input_buffer, sizeof(input_buffer), stdin) && sscanf(input_buffer, "%lu", &target_tick) == 1) {
                    audit_maintenance_records(sys_logs, total_sys_logs, target_tick);
                }
                break;
            }
            case 8:
                printf("%s[TIME] Fast forwarding 1 tick...%s\n", UI_FG_BLUE, UI_RESET);
                break;
            case 9:
                global_state.is_simulation_active = false;
                break;
            default:
                printf("%s[ERROR] Invalid command sequence.%s\n", UI_FG_RED, UI_RESET);
        }

        // Wait for user to read output before looping
        if (global_state.is_simulation_active) {
            printf("\nPress [ENTER] to continue...");
            fgets(input_buffer, sizeof(input_buffer), stdin);
            advance_simulation_tick();
        }
    }

    // Game Over Sequence
    printf("%s", UI_CLEAR);
    printf("%s%s", UI_BG_RED, UI_FG_WHITE);
    printf("====================================================\n");
    printf("               SIMULATION TERMINATED                \n");
    printf("====================================================\n");
    printf("%s\n", UI_RESET);
    printf("Final Score       : %d XP\n", global_state.score_xp);
    printf("Approval Rating   : %d%%\n", global_state.city_approval_rating);
    printf("Issues Resolved   : %d\n", global_state.total_emergencies_resolved);
    printf("Remaining Funds   : $%d\n\n", global_state.treasury_funds);

    // Secure Memory Cleanup
    free_city_graph(&city_map);
    free_sensor_network(&sensor_network);
    free_wqi_bst(water_quality_bst);
    free_warehouse_stack(&warehouse);
    free_standard_queue(&standard_hotline);
    free_priority_queue(&critical_triage);

    printf("Memory fully released. Shutting down Hydro-Sync.\n");
    return 0;
}
