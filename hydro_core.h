/**
 * ============================================================================
 *  HYDRO-SYNC CORE ARCHITECTURE 
 *  File: hydro_core.h
 *  Purpose: Global definitions, ANSI UI codes, core structures, and simulation
 *           state management for the Hydro-Sync terminal application.
 *  Targets: SDG 6 (Clean Water) & SDG 11 (Sustainable Cities)
 * ============================================================================
 */

#ifndef HYDRO_CORE_H
#define HYDRO_CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

/* -------------------------------------------------------------------------- */
/*                            SYSTEM CONFIGURATION                            */
/* -------------------------------------------------------------------------- */
#define SYS_MAX_ZONES         50      // Expanded for Graph operations
#define SYS_HASH_SIZE         100     // Expanded for Sensor Hashing
#define SYS_MAX_INVENTORY     500     // Warehouse capacity
#define SYS_TICK_RATE_MS      500000  // Half-second UI refresh intervals
#define SYS_CRITICAL_HP       25      // Health threshold for UI warnings

/* -------------------------------------------------------------------------- */
/*                            ANSI TERMINAL UI CODES                          */
/* -------------------------------------------------------------------------- */
#define UI_RESET       "\033[0m"
#define UI_BOLD        "\033[1m"
#define UI_DIM         "\033[2m"
#define UI_UNDERLINE   "\033[4m"

#define UI_FG_RED      "\033[31m"
#define UI_FG_GREEN    "\033[32m"
#define UI_FG_YELLOW   "\033[33m"
#define UI_FG_BLUE     "\033[34m"
#define UI_FG_MAGENTA  "\033[35m"
#define UI_FG_CYAN     "\033[36m"
#define UI_FG_WHITE    "\033[37m"

#define UI_BG_RED      "\033[41m"
#define UI_BG_BLUE     "\033[44m"
#define UI_BG_BLACK    "\033[40m"

#define UI_CLEAR       "\033[2J\033[H" // Clear screen and move cursor to 0,0

/* -------------------------------------------------------------------------- */
/*                              ENUMERATIONS                                  */
/* -------------------------------------------------------------------------- */
typedef enum {
    SEV_LOW = 5,
    SEV_MODERATE = 4,
    SEV_HIGH = 3,
    SEV_CRITICAL = 2,
    SEV_CATASTROPHIC = 1
} SeverityLevel;

typedef enum {
    STAT_ONLINE,
    STAT_OFFLINE,
    STAT_MAINTENANCE,
    STAT_COMPROMISED
} NodeStatus;

typedef enum {
    EVENT_SUPPLY_DROP,
    EVENT_LEAK_DETECTED,
    EVENT_SPILL_DETECTED,
    EVENT_SENSOR_FAILURE,
    EVENT_ZONE_LOCKDOWN
} EventType;

/* -------------------------------------------------------------------------- */
/*                           COMPLEX DATA STRUCTURES                          */
/* -------------------------------------------------------------------------- */

// Represents a real-time timestamp in the simulation
typedef struct {
    int day;
    int hour;
    int minute;
    unsigned long total_ticks;
} SimTime;

// Comprehensive tracking of a city zone (Used in Graph & BST)
typedef struct {
    int zone_id;
    char zone_name[64];
    int population;
    int water_quality_index;      // 0-100 scale
    int infrastructure_health;    // 0-100 scale
    NodeStatus status;
    float contamination_spread;   // Risk metric for graph traversal
} CityZone;

// Advanced Sensor Node for Hash Table Lookups
typedef struct {
    unsigned long sensor_id;
    int parent_zone_id;
    float flow_rate;
    float ph_level;
    bool is_active;
    char last_calibrated[32];
} SensorData;

// Global Player & City State Tracker
typedef struct {
    char mayor_name[64];
    int score_xp;
    int city_approval_rating;     // Impacts game over conditions
    int global_water_reserves;
    int treasury_funds;
    SimTime current_time;
    bool is_simulation_active;
    
    // Performance Metrics
    int total_emergencies_resolved;
    int total_supplies_deployed;
    int zones_scanned;
} GameState;

/* -------------------------------------------------------------------------- */
/*                          CORE PROTOTYPES & MACROS                          */
/* -------------------------------------------------------------------------- */

// Advanced Logging Macros
#define LOG_INFO(msg)    printf("%s[INFO] %s%s\n", UI_FG_CYAN, msg, UI_RESET)
#define LOG_SUCCESS(msg) printf("%s[SUCCESS] %s%s\n", UI_FG_GREEN, msg, UI_RESET)
#define LOG_WARN(msg)    printf("%s[WARNING] %s%s\n", UI_FG_YELLOW, msg, UI_RESET)
#define LOG_CRIT(msg)    printf("%s%s[CRITICAL] %s%s\n", UI_BG_RED, UI_FG_WHITE, msg, UI_RESET)

// Global State Declaration (Extern) to be instantiated in main.c
extern GameState global_state;

// Utility functions
void init_game_state(const char* player_name);
void advance_simulation_tick();
void render_ui_header();
void delay_ms(int milliseconds);

#endif // HYDRO_CORE_H