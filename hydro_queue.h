/**
 * ============================================================================
 *  HYDRO-SYNC STANDARD EMERGENCY HOTLINE (QUEUE)
 *  File: hydro_queue.h
 *  Purpose: FIFO Queue implementation for managing routine sanitation and 
 *           infrastructure repair tickets. Features ticket aging/escalation.
 *  Targets: SDG 6 (Clean Water & Sanitation)
 * ============================================================================
 */

#ifndef HYDRO_QUEUE_H
#define HYDRO_QUEUE_H

#include "hydro_core.h"

/* -------------------------------------------------------------------------- */
/*                            QUEUE DATA STRUCTURES                           */
/* -------------------------------------------------------------------------- */

// Represents a standard sanitation emergency ticket (FIFO Node)
typedef struct StandardEmergencyNode {
    unsigned long ticket_id;
    int zone_id;
    char description[128];
    unsigned long tick_reported;      // Simulation time when ticket was created
    int estimated_repair_cost;
    struct StandardEmergencyNode* next;
} StandardEmergencyNode;

// Standard Emergency Hotline Queue Header
typedef struct EmergencyQueue {
    StandardEmergencyNode* front;
    StandardEmergencyNode* rear;
    int total_pending_tickets;
    int resolved_count_session;
} EmergencyQueue;

/* -------------------------------------------------------------------------- */
/*                            INITIALIZATION & SETUP                          */
/* -------------------------------------------------------------------------- */

/**
 * Initializes the municipal standard emergency queue.
 */
void init_standard_queue(EmergencyQueue* q) {
    q->front = NULL;
    q->rear = NULL;
    q->total_pending_tickets = 0;
    q->resolved_count_session = 0;
    printf("%s[SYS] Standard Emergency Hotline (FIFO) Online.%s\n", UI_FG_CYAN, UI_RESET);
}

/* -------------------------------------------------------------------------- */
/*                              CORE QUEUE OPERATIONS                         */
/* -------------------------------------------------------------------------- */

/**
 * Enqueue Operation: Logs a new standard emergency ticket at the rear of the queue.
 * Ensures strict First-In-First-Out handling to maintain equitable service (SDG 6).
 */
void enqueue_standard_issue(EmergencyQueue* q, GameState* state, int target_zone, const char* desc) {
    StandardEmergencyNode* new_ticket = (StandardEmergencyNode*)malloc(sizeof(StandardEmergencyNode));
    if (!new_ticket) {
        printf("%s[FATAL] Queue memory allocation failed.%s\n", UI_BG_RED, UI_RESET);
        return;
    }

    new_ticket->ticket_id = 500000 + (rand() % 90000);
    new_ticket->zone_id = target_zone;
    strncpy(new_ticket->description, desc, 127);
    new_ticket->description[127] = '\0';
    new_ticket->tick_reported = state->current_time.total_ticks;
    new_ticket->estimated_repair_cost = 50 + (rand() % 150);
    new_ticket->next = NULL;

    // FIFO Insertion logic
    if (q->rear == NULL) {
        q->front = q->rear = new_ticket;
    } else {
        q->rear->next = new_ticket;
        q->rear = new_ticket;
    }

    q->total_pending_tickets++;

    printf("%s[HOTLINE] Ticket #%lu Logged | Zone %d | Issue: %s%s\n", 
           UI_FG_YELLOW, new_ticket->ticket_id, target_zone, desc, UI_RESET);
}

/**
 * Dequeue Operation: Dispatches a maintenance team to the oldest pending issue (front).
 * Consumes treasury funds but boosts the player's XP and city approval rating.
 */
bool resolve_standard_issue(EmergencyQueue* q, GameState* state) {
    if (q->front == NULL) {
        printf("%s[DISPATCH] No active standard emergencies. The city is secure.%s\n", 
               UI_FG_GREEN, UI_RESET);
        return false;
    }

    StandardEmergencyNode* target = q->front;

    // Treasury Check
    if (state->treasury_funds < target->estimated_repair_cost) {
        printf("%s[FUNDS DEPLETED] Cannot afford repair for Ticket #%lu (Cost: $%d).%s\n", 
               UI_FG_RED, target->ticket_id, target->estimated_repair_cost, UI_RESET);
        return false;
    }

    // Process Resolution
    state->treasury_funds -= target->estimated_repair_cost;
    state->score_xp += 25; // Standard XP reward
    state->city_approval_rating += 2; // Improve public opinion
    state->total_emergencies_resolved++;

    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL; // Queue is now completely empty
    }

    printf("%s[DISPATCH SUCCESS] Resolved Ticket #%lu (Zone %d).%s\n", 
           UI_FG_GREEN, target->ticket_id, target->zone_id, UI_RESET);
    printf("   -> Cost: $%d | Reward: +25 XP | Issue: %s\n", 
           target->estimated_repair_cost, target->description);

    q->total_pending_tickets--;
    q->resolved_count_session++;
    
    free(target);
    return true;
}

/* -------------------------------------------------------------------------- */
/*                        SIMULATION TICKS & ESCALATION                       */
/* -------------------------------------------------------------------------- */

/**
 * Scans the queue to penalize the player if tickets have been ignored for too long.
 * Simulates real-world citizen frustration when infrastructure is neglected.
 */
void process_queue_aging(EmergencyQueue* q, GameState* state) {
    if (q->front == NULL) return;

    StandardEmergencyNode* current = q->front;
    int stale_count = 0;

    while (current != NULL) {
        unsigned long age = state->current_time.total_ticks - current->tick_reported;
        
        // If a ticket has been sitting for more than 10 ticks, penalize
        if (age > 10) {
            stale_count++;
            if (age % 5 == 0) { // Periodic penalty hit
                state->city_approval_rating -= 1;
                printf("%s[CITIZEN COMPLAINT] Ticket #%lu (Zone %d) has been ignored too long! (-1 Approval)%s\n", 
                       UI_FG_MAGENTA, current->ticket_id, current->zone_id, UI_RESET);
            }
        }
        current = current->next;
    }

    if (stale_count > 3) {
        printf("%s[WARNING] Multiple aging tickets! City approval is bleeding.%s\n", UI_BG_RED, UI_RESET);
    }
}

/**
 * Renders the current state of the standard emergency queue.
 */
void display_hotline_queue(EmergencyQueue* q, GameState* state) {
    if (q->front == NULL) {
        printf("%s[HOTLINE] The standard emergency queue is currently empty.%s\n", UI_FG_GREEN, UI_RESET);
        return;
    }

    printf("\n%s=== STANDARD EMERGENCY HOTLINE (FIFO) ===%s\n", UI_BOLD, UI_RESET);
    printf("Pending Tickets: %d\n\n", q->total_pending_tickets);

    StandardEmergencyNode* curr = q->front;
    int pos = 1;
    while (curr != NULL) {
        unsigned long age = state->current_time.total_ticks - curr->tick_reported;
        printf("[%02d] TKT-#%lu | Zone: %02d | Age: %03lu ticks | Cost: $%4d | %s\n", 
               pos++, curr->ticket_id, curr->zone_id, age, curr->estimated_repair_cost, curr->description);
        curr = curr->next;
    }
    printf("=========================================\n\n");
}

/* -------------------------------------------------------------------------- */
/*                               MEMORY CLEANUP                               */
/* -------------------------------------------------------------------------- */

/**
 * Safely clears all pending nodes to prevent memory leaks on exit.
 */
void free_standard_queue(EmergencyQueue* q) {
    StandardEmergencyNode* current = q->front;
    while (current != NULL) {
        StandardEmergencyNode* temp = current;
        current = current->next;
        free(temp);
    }
    q->front = q->rear = NULL;
    q->total_pending_tickets = 0;
}

#endif // HYDRO_QUEUE_H