/**
 * ============================================================================
 *  HYDRO-SYNC CRITICAL TRIAGE (PRIORITY QUEUE)
 *  File: hydro_pq.h
 *  Purpose: Dynamically sorted linked list for handling critical municipal 
 *           emergencies based on severity metrics (1 = Highest, 5 = Lowest).
 *  Targets: SDG 6 (Clean Water) & SDG 11 (Sustainable Cities)
 * ============================================================================
 */

#ifndef HYDRO_PQ_H
#define HYDRO_PQ_H

#include "hydro_core.h"

/* -------------------------------------------------------------------------- */
/*                        PRIORITY QUEUE DATA STRUCTURES                      */
/* -------------------------------------------------------------------------- */

// Represents a high-stakes critical emergency
typedef struct CriticalEmergencyNode {
    unsigned long alert_id;
    int target_zone_id;
    char description[128];
    SeverityLevel severity;           // 1 to 5 scaling
    unsigned long tick_reported;
    int hazmat_cost;                  // Premium treasury cost for hazardous dispatch
    struct CriticalEmergencyNode* next;
} CriticalEmergencyNode;

// Priority Queue Header
typedef struct PriorityQueue {
    CriticalEmergencyNode* head;
    int active_critical_threats;
} PriorityQueue;

/* -------------------------------------------------------------------------- */
/*                            INITIALIZATION & SETUP                          */
/* -------------------------------------------------------------------------- */

/**
 * Initializes the Priority Queue for the city's crisis response network.
 */
void init_priority_queue(PriorityQueue* pq) {
    pq->head = NULL;
    pq->active_critical_threats = 0;
    printf("%s[SYS] Critical Threat Triage (Priority Queue) Online.%s\n", UI_FG_CYAN, UI_RESET);
}

/* -------------------------------------------------------------------------- */
/*                              CORE PQ OPERATIONS                            */
/* -------------------------------------------------------------------------- */

/**
 * Enqueue Priority: Inserts an emergency into the list sorted by severity.
 * O(N) time complexity due to list traversal[cite: 2]. Lower integer = Higher priority.
 */
void enqueue_critical_alert(PriorityQueue* pq, GameState* state, int zone_id, 
                            const char* desc, SeverityLevel severity) {
                            
    CriticalEmergencyNode* new_alert = (CriticalEmergencyNode*)malloc(sizeof(CriticalEmergencyNode));
    if (!new_alert) {
        printf("%s[FATAL] Priority Queue memory allocation failed.%s\n", UI_BG_RED, UI_RESET);
        return;
    }

    new_alert->alert_id = 900000 + (rand() % 90000); // 9xxxxx ID block for criticals
    new_alert->target_zone_id = zone_id;
    strncpy(new_alert->description, desc, 127);
    new_alert->description[127] = '\0';
    new_alert->severity = severity;
    new_alert->tick_reported = state->current_time.total_ticks;
    
    // Higher severity costs exponentially more to clean up
    new_alert->hazmat_cost = 500 + ((6 - severity) * 200) + (rand() % 100);
    new_alert->next = NULL;

    // Edge Case: Handle head-node replacement for maximum-severity alerts[cite: 2]
    if (pq->head == NULL || pq->head->severity > severity) {
        new_alert->next = pq->head;
        pq->head = new_alert;
    } else {
        // Traverse to find the correct sorted insertion point
        CriticalEmergencyNode* current = pq->head;
        while (current->next != NULL && current->next->severity <= severity) {
            current = current->next;
        }
        new_alert->next = current->next;
        current->next = new_alert;
    }

    pq->active_critical_threats++;

    printf("\n%s%s[CRITICAL ALERT LOGGED] Zone %d | SEV-%d: %s%s\n", 
           UI_BG_RED, UI_FG_WHITE, zone_id, severity, desc, UI_RESET);
           
    // Immediate approval hit for a catastrophic event occurring
    if (severity == SEV_CATASTROPHIC) {
        state->city_approval_rating -= 5;
        printf("%s[NEWS FLASH] Catastrophic failure in Zone %d! Approval Rating tanks.%s\n", 
               UI_FG_MAGENTA, zone_id, UI_RESET);
    }
}

/**
 * Dequeue Priority: Resolves the highest severity issue instantly (head of the list).
 * O(1) time complexity, ensuring critical issues bypass minor ones instantly[cite: 2].
 */
bool resolve_critical_alert(PriorityQueue* pq, GameState* state) {
    if (pq->head == NULL) {
        printf("%s[DISPATCH] No critical threats active. The city is safe.%s\n", UI_FG_GREEN, UI_RESET);
        return false;
    }

    CriticalEmergencyNode* target = pq->head;

    // Treasury Check (Game Over trigger if city goes bankrupt on a crisis)
    if (state->treasury_funds < target->hazmat_cost) {
        printf("%s[BANKRUPTCY IMMINENT] Cannot afford HAZMAT response for ALERT #%lu (Cost: $%d).%s\n", 
               UI_BG_RED, target->alert_id, target->hazmat_cost, UI_RESET);
        return false;
    }

    // Process High-Stakes Resolution
    state->treasury_funds -= target->hazmat_cost;
    
    // Massive XP scaling based on severity
    int xp_reward = 100 * (6 - target->severity); 
    state->score_xp += xp_reward; 
    
    state->city_approval_rating += (6 - target->severity); // Earn back public trust
    state->total_emergencies_resolved++;

    // Safe head pointer removal[cite: 2]
    pq->head = pq->head->next;

    printf("\n%s[HAZMAT DISPATCH SUCCESS] Contained Level %d Threat in Zone %d.%s\n", 
           UI_FG_GREEN, target->severity, target->target_zone_id, UI_RESET);
    printf("   -> Incident: %s\n", target->description);
    printf("   -> Cost: $%d | Reward: +%d XP\n", target->hazmat_cost, xp_reward);

    pq->active_critical_threats--;
    free(target);
    return true;
}

/* -------------------------------------------------------------------------- */
/*                        SIMULATION TICKS & ESCALATION                       */
/* -------------------------------------------------------------------------- */

/**
 * Severely penalizes the player for allowing critical threats to linger.
 * High severity threats will drain the city's health/approval rapidly.
 */
void process_critical_aging(PriorityQueue* pq, GameState* state) {
    if (pq->head == NULL) return;

    CriticalEmergencyNode* current = pq->head;

    while (current != NULL) {
        unsigned long age = state->current_time.total_ticks - current->tick_reported;
        
        // Severity 1 & 2 damage the city every single tick they exist
        if (current->severity <= SEV_CRITICAL) {
            state->city_approval_rating -= 1;
            if (age % 3 == 0) {
                printf("%s[CRISIS ESCALATING] Level %d threat in Zone %d is causing severe damage! (-1 Approval)%s\n", 
                       UI_BG_RED, current->severity, current->target_zone_id, UI_RESET);
            }
        } else if (age > 5) {
            // Lower severities penalize if left alone too long
            state->city_approval_rating -= 1;
        }
        
        current = current->next;
    }
}

/**
 * Displays the current Priority Queue, sorted highest threat to lowest.
 */
void display_critical_threats(PriorityQueue* pq, GameState* state) {
    if (pq->head == NULL) {
        printf("%s[TRIAGE] Zero critical threats detected.%s\n", UI_FG_GREEN, UI_RESET);
        return;
    }

    printf("\n%s%s=== CRITICAL EMERGENCY TRIAGE (PRIORITY QUEUE) ===%s\n", UI_BG_RED, UI_FG_WHITE, UI_RESET);
    printf("Active Threats: %d\n\n", pq->active_critical_threats);

    CriticalEmergencyNode* curr = pq->head;
    while (curr != NULL) {
        unsigned long age = state->current_time.total_ticks - curr->tick_reported;
        
        // Dynamic UI color based on Severity
        const char* color = UI_FG_YELLOW;
        if (curr->severity == SEV_CATASTROPHIC) color = UI_BG_RED;
        else if (curr->severity == SEV_CRITICAL) color = UI_FG_RED;
        else if (curr->severity == SEV_HIGH) color = UI_FG_MAGENTA;

        printf("%s[SEV-%d] ALERT #%lu | Zone %02d | Age: %03lu | Cost: $%4d | %s%s\n", 
               color, curr->severity, curr->alert_id, curr->target_zone_id, age, 
               curr->hazmat_cost, curr->description, UI_RESET);
               
        curr = curr->next;
    }
    printf("======================================================\n\n");
}

/* -------------------------------------------------------------------------- */
/*                               MEMORY CLEANUP                               */
/* -------------------------------------------------------------------------- */

/**
 * Frees all pending critical emergency nodes on shutdown.
 */
void free_priority_queue(PriorityQueue* pq) {
    CriticalEmergencyNode* current = pq->head;
    while (current != NULL) {
        CriticalEmergencyNode* temp = current;
        current = current->next;
        free(temp);
    }
    pq->head = NULL;
    pq->active_critical_threats = 0;
}

#endif // HYDRO_PQ_H