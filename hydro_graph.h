/**
 * ============================================================================
 *  HYDRO-SYNC GRAPH & ROUTING SYSTEM
 *  File: hydro_graph.h
 *  Purpose: Adjacency list graph implementation for the municipal water grid.
 *           Includes BFS, DFS, and Dijkstra's Shortest Path algorithms.
 *  Targets: SDG 6 (Clean Water) & SDG 11 (Sustainable Cities)
 * ============================================================================
 */

#ifndef HYDRO_GRAPH_H
#define HYDRO_GRAPH_H

#include "hydro_core.h"
#include <limits.h>

/* -------------------------------------------------------------------------- */
/*                            GRAPH DATA STRUCTURES                           */
/* -------------------------------------------------------------------------- */

// Represents a directed pipe connection between two municipal zones
typedef struct PipeEdge {
    int dest_zone_id;
    int flow_capacity;       // Max volume of water transfer
    int pipe_health;         // 0-100 scale (deteriorates over time)
    int travel_time_mins;    // Edge weight for Dijkstra's dispatch routing
    struct PipeEdge* next;
} PipeEdge;

// Represents a municipal zone vertex in the graph
typedef struct ZoneVertex {
    CityZone data;
    PipeEdge* head;          // Pointer to adjacency list
} ZoneVertex;

// The main Graph structure representing the entire city grid
typedef struct CityGraph {
    int total_zones;
    ZoneVertex vertices[SYS_MAX_ZONES];
} CityGraph;

/* -------------------------------------------------------------------------- */
/*                            INITIALIZATION & SETUP                          */
/* -------------------------------------------------------------------------- */

/**
 * Initializes the city graph, setting all zones to an inactive/empty state.
 */
void init_city_graph(CityGraph* graph, int num_zones) {
    if (num_zones > SYS_MAX_ZONES) num_zones = SYS_MAX_ZONES;
    graph->total_zones = num_zones;
    
    for (int i = 0; i < SYS_MAX_ZONES; i++) {
        graph->vertices[i].data.zone_id = i;
        sprintf(graph->vertices[i].data.zone_name, "Sector-%02d", i);
        graph->vertices[i].data.water_quality_index = 100;
        graph->vertices[i].data.infrastructure_health = 100;
        graph->vertices[i].data.status = STAT_ONLINE;
        graph->vertices[i].head = NULL;
    }
}

/**
 * Adds a bidirectional pipe connection between two zones (Undirected Graph).
 */
void add_pipe_connection(CityGraph* graph, int src, int dest, int capacity, int travel_time) {
    if (src >= graph->total_zones || dest >= graph->total_zones) {
        printf("%s[ERROR] Invalid zone IDs for connection.%s\n", UI_FG_RED, UI_RESET);
        return;
    }

    // Edge from src to dest
    PipeEdge* new_edge1 = (PipeEdge*)malloc(sizeof(PipeEdge));
    new_edge1->dest_zone_id = dest;
    new_edge1->flow_capacity = capacity;
    new_edge1->pipe_health = 100;
    new_edge1->travel_time_mins = travel_time;
    new_edge1->next = graph->vertices[src].head;
    graph->vertices[src].head = new_edge1;

    // Edge from dest to src (Bidirectional flow)
    PipeEdge* new_edge2 = (PipeEdge*)malloc(sizeof(PipeEdge));
    new_edge2->dest_zone_id = src;
    new_edge2->flow_capacity = capacity;
    new_edge2->pipe_health = 100;
    new_edge2->travel_time_mins = travel_time;
    new_edge2->next = graph->vertices[dest].head;
    graph->vertices[dest].head = new_edge2;
}

/* -------------------------------------------------------------------------- */
/*                         SLA 3: SEARCHING ALGORITHMS                        */
/* -------------------------------------------------------------------------- */

/**
 * Breadth-First Search (BFS): Scans the network layer by layer starting from
 * a specific zone to detect widespread infrastructure failures.
 */
void bfs_network_scan(CityGraph* graph, int start_zone) {
    bool visited[SYS_MAX_ZONES] = {false};
    int queue[SYS_MAX_ZONES];
    int front = 0, rear = 0;

    printf("\n%s--- [SYS] INITIATING BFS NETWORK SCAN FROM ZONE %d ---%s\n", UI_FG_CYAN, start_zone, UI_RESET);
    
    visited[start_zone] = true;
    queue[rear++] = start_zone;

    while (front < rear) {
        int current_zone = queue[front++];
        printf("Scanning %s... ", graph->vertices[current_zone].data.zone_name);
        
        if (graph->vertices[current_zone].data.infrastructure_health < 50) {
            printf("%s[WARNING] Structural integrity compromised.%s\n", UI_FG_YELLOW, UI_RESET);
        } else {
            printf("%s[OK]%s\n", UI_FG_GREEN, UI_RESET);
        }

        PipeEdge* temp = graph->vertices[current_zone].head;
        while (temp != NULL) {
            int adj_zone = temp->dest_zone_id;
            if (!visited[adj_zone]) {
                visited[adj_zone] = true;
                queue[rear++] = adj_zone;
            }
            temp = temp->next;
        }
    }
    printf("%s--- BFS SCAN COMPLETE ---%s\n", UI_FG_CYAN, UI_RESET);
}

/**
 * Depth-First Search (DFS): Traces the potential spread of a chemical spill
 * or contamination deep into connected water lines.
 */
void dfs_util(CityGraph* graph, int current_zone, bool visited[]) {
    visited[current_zone] = true;
    printf("%s[TRACE] Contamination reached: %s%s\n", UI_FG_RED, graph->vertices[current_zone].data.zone_name, UI_RESET);
    
    // Simulate contamination damage
    graph->vertices[current_zone].data.water_quality_index -= 20;

    PipeEdge* temp = graph->vertices[current_zone].head;
    while (temp != NULL) {
        int adj_zone = temp->dest_zone_id;
        if (!visited[adj_zone]) {
            dfs_util(graph, adj_zone, visited);
        }
        temp = temp->next;
    }
}

void dfs_contamination_trace(CityGraph* graph, int start_zone) {
    bool visited[SYS_MAX_ZONES] = {false};
    printf("\n%s--- [SYS] ALERT: TRACING CONTAMINATION FROM ZONE %d (DFS) ---%s\n", UI_BG_RED, start_zone, UI_RESET);
    dfs_util(graph, start_zone, visited);
    printf("%s--- DFS TRACE COMPLETE ---%s\n", UI_FG_RED, UI_RESET);
}

/* -------------------------------------------------------------------------- */
/*                 SLA 3 ADVANCED: DIJKSTRA'S SHORTEST PATH                   */
/* -------------------------------------------------------------------------- */

/**
 * Calculates the fastest route for a maintenance team to reach a critical emergency.
 * Uses travel_time_mins as edge weights.
 */
void dijkstra_dispatch_route(CityGraph* graph, int start_zone, int dest_zone) {
    int distances[SYS_MAX_ZONES];
    bool spt_set[SYS_MAX_ZONES]; // Shortest Path Tree Set
    int parent[SYS_MAX_ZONES];

    // Initialize all distances as infinite and spt_set as false
    for (int i = 0; i < graph->total_zones; i++) {
        distances[i] = INT_MAX;
        spt_set[i] = false;
        parent[i] = -1;
    }

    distances[start_zone] = 0;

    // Find shortest path for all vertices
    for (int count = 0; count < graph->total_zones - 1; count++) {
        // Pick the minimum distance vertex from the set of vertices not yet processed
        int min = INT_MAX, min_index = -1;
        for (int v = 0; v < graph->total_zones; v++) {
            if (spt_set[v] == false && distances[v] <= min) {
                min = distances[v];
                min_index = v;
            }
        }

        int u = min_index;
        if (u == -1) break; // Disconnected graph safeguard
        spt_set[u] = true;

        // Update distance value of the adjacent vertices
        PipeEdge* temp = graph->vertices[u].head;
        while (temp != NULL) {
            int v = temp->dest_zone_id;
            if (!spt_set[v] && distances[u] != INT_MAX && 
                distances[u] + temp->travel_time_mins < distances[v]) {
                distances[v] = distances[u] + temp->travel_time_mins;
                parent[v] = u;
            }
            temp = temp->next;
        }
    }

    // Print the optimal routing
    printf("\n%s--- OPTIMAL DISPATCH ROUTE (DIJKSTRA) ---%s\n", UI_FG_BLUE, UI_RESET);
    if (distances[dest_zone] == INT_MAX) {
        printf("%s[ERROR] No valid route to %s. Zone isolated.%s\n", UI_FG_RED, graph->vertices[dest_zone].data.zone_name, UI_RESET);
        return;
    }

    printf("ETA to %s: %d minutes.\n", graph->vertices[dest_zone].data.zone_name, distances[dest_zone]);
    printf("Navigation Path: ");
    
    // Backtrack to print path
    int path[SYS_MAX_ZONES];
    int path_idx = 0;
    int curr = dest_zone;
    while (curr != -1) {
        path[path_idx++] = curr;
        curr = parent[curr];
    }
    
    for (int i = path_idx - 1; i >= 0; i--) {
        printf("%d", path[i]);
        if (i != 0) printf(" -> ");
    }
    printf("\n");
}

/* -------------------------------------------------------------------------- */
/*                               MEMORY CLEANUP                               */
/* -------------------------------------------------------------------------- */

void free_city_graph(CityGraph* graph) {
    for (int i = 0; i < graph->total_zones; i++) {
        PipeEdge* current = graph->vertices[i].head;
        while (current != NULL) {
            PipeEdge* temp = current;
            current = current->next;
            free(temp);
        }
        graph->vertices[i].head = NULL;
    }
}

#endif // HYDRO_GRAPH_H