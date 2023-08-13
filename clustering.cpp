/**
 * @file clustering.cpp
 * @author Matteo Loporchio
 * @brief Computes the connected components of the auxiliary graph
 * @version 1.0
 * @date 2023-08-13
 * 
 * This program reads the auxiliary graph from a binary file and analyzes it
 * by computing its connected components. The output of this program
 * is a CSV file containing one line for each node of the auxiliary graph.
 * Each line contains the node identifier and the identifier of its 
 * component, separated by a comma.
 * 
 * @copyright Copyright (c) 2023 Matteo Loporchio
 */

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <igraph.h>

using namespace std;
using namespace std::chrono;

/**
 * @brief Reads the auxiliary graph from a binary file
 * 
 * @param graph igraph data structure where the graph will be stored
 * @param input_file pointer to the (already opened) binary file
 * @param forced_nodes defines the number of nodes for the input graph. If zero, the number of nodes will be deduced from the input file.
 */
void read_graph_binary(igraph_t *graph, FILE *input_file, int forced_nodes) {
    int buf[2];
    // Read the number of nodes and edges from the binary file.
    int num_read = fread(buf, sizeof(int), 2, input_file);
    int num_nodes = ((forced_nodes == 0) ? __builtin_bswap32(buf[0]) : forced_nodes);
    int num_edges = __builtin_bswap32(buf[1]);
    // Initialize the graph.
    igraph_empty(graph, (igraph_integer_t) num_nodes, IGRAPH_UNDIRECTED);
    // Read edges from the input file and add them to the graph.
    igraph_vector_int_t edges;
    igraph_vector_int_init(&edges, 2 * num_edges);
    igraph_integer_t i = 0;
    while ((num_read = fread(buf, sizeof(int), 2, input_file)) > 0) {
        VECTOR(edges)[i] = (igraph_integer_t) (__builtin_bswap32(buf[0]));
        VECTOR(edges)[i+1] = (igraph_integer_t) (__builtin_bswap32(buf[1]));
        i += 2;
    }
    igraph_add_edges(graph, &edges, NULL);
    igraph_vector_int_destroy(&edges);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <input_file> <output_file> [<num_addresses>]\n";
        return 1;
    }
    auto start = high_resolution_clock::now();
    
    // 
    int num_addresses = ((argc >= 4) ? atoi(argv[3]) : 0);

    // Open the input and output files.
    FILE *input_file = fopen(argv[1], "r");
    if (!input_file) {
        cerr << "Error: could not open input file!\n";
        return 1;
    }
        
    FILE *output_file = fopen(argv[2], "w");
    if (!output_file) {
        cerr << "Error: could not open output file!\n";
        return 1;
    }

    // Load the graph from the corresponding file.
    igraph_t graph;
    read_graph_binary(&graph, input_file, num_addresses);
    //igraph_read_graph_edgelist(&graph, input_file, num_addresses, 0);
    fclose(input_file);

    // 
    igraph_integer_t num_nodes = igraph_vcount(&graph);
    igraph_integer_t num_edges = igraph_ecount(&graph);

    // Compute the weakly connected components of the graph.
    igraph_integer_t num_cc;
    igraph_vector_int_t comp_map;
    //igraph_vector_int_t wcc_sizes;
    igraph_vector_int_init(&comp_map, num_nodes);
    //igraph_vector_int_init(&wcc_sizes, total_nodes);
    igraph_connected_components(&graph, &comp_map, NULL, &num_cc, IGRAPH_WEAK);

    // 
    fprintf(output_file, "node_id,comp_id\n");
    for (int i = 0; i < num_nodes; i++) {
        int comp_id = VECTOR(comp_map)[i];
        fprintf(output_file, "%d,%d\n", i, comp_id);
    }
    fclose(output_file);

    // Free the memory occupied by the graph.
    igraph_destroy(&graph);
    igraph_vector_int_destroy(&comp_map);
    
    auto end = high_resolution_clock::now();
    auto elapsed = duration_cast<nanoseconds>(end - start);

    // Print information about the program execution. 
    // Specifically, we print the following values:
    // (1) number of nodes;
    // (2) number of edges;
    // (3) number of weakly connected components.
    cout << num_nodes << '\t' << num_edges << '\t' << (int) num_cc << '\t' << elapsed.count() << '\n';
    return 0;

}