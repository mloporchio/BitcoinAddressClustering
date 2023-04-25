/**
 * @file builder.cpp
 * @author Matteo Loporchio
 * @brief Implementation of the Bitcoin address clustering algorithm [1]
 * @version 0.2
 * @date 2023-04-25
 * 
 * This program implements the Bitcoin address clustering procedure 
 * by Di Francesco Maesa et al. described in [1]. 
 * Given a list of transactions, the procedure produces a partition 
 * (i.e., a clustering) of all Bitcoin addresses included in such transactions. 
 * The clustering is constructed in accordance with the multi-input heuristic.
 * 
 * More precisely, the program reads a list of Bitcoin transactions 
 * and constructs a graph, called auxiliary graph, defined as follows.
 * 
 * 1)   Nodes correspond to all addresses appearing in the transactions.
 *      Both input and output addresses are considered.
 * 2)   For each transaction T in the list, there is a path 
 *      among the nodes representing the input addresses of T. 
 * 
 * The input file contains a textual representation
 * of Bitcoin transactions. See https://zenodo.org/record/7696454#.ZBOmgy9abq0
 * for a more detailed description of the required input file format.
 * 
 * The output graph is represented as a binary file containing
 * a sequence of 32-bit signed integers (in big-endian format). 
 * The file has the following format.
 * 
 * 1)   the first 32 bits represent the number of nodes N;
 * 2)   the next 32 bits represent the number of edges M;
 * 3)   the remaining M pairs of 32-bit integers represent the edges
 *      of the graph. The first (resp. second) integer corresponds
 *      to the identifier of the source (resp. target) node of the edge.
 * 
 * References:
 * 
 * [1] Di Francesco Maesa, Damiano, Andrea Marino, and Laura Ricci. 
 * "Data-driven analysis of bitcoin properties: exploiting the users graph." 
 * International Journal of Data Science and Analytics 6 (2018): 63-80.
 * 
 * @copyright Copyright (c) 2023 Matteo Loporchio
 */

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <unordered_map>

using namespace std;
using namespace std::chrono;

/// @brief The node map represents a mapping from addresses to node identifiers
typedef unordered_map<int,int> node_map_t;

/// @brief The edge set contains a set of ordered pairs representing graph edges
typedef set<pair<int,int>> edge_set_t;

/**
 * @brief Checks if the node map contains an identifier for the given address
 * 
 * @param map reference to the node map
 * @param address address to be checked
 * @return the node identifier if the address is present, -1 otherwise
 */
inline int contains_node(node_map_t &map, int address) {
    node_map_t::iterator it = map.find(address);
    return ((it != map.end()) ? it->second : -1);
}

/**
 * @brief Processes the list of transaction inputs (represented as a semicolon-separated string)
 * 
 * @param inputs string containing all transaction inputs
 * @param nodes reference to the node map
 * @param edges reference to the edge set
 * @param id_count counter for used node identifiers
 */
void process_inputs(char *inputs, node_map_t &nodes, edge_set_t &edges, int *id_count) {
    // Extract all unique input addresses.
    set<int> input_addresses;
    char *ptr, *save_ptr, *input_str, *address_str;
    for (ptr = inputs; ; ptr = NULL) {
        if (!(input_str = strtok_r(ptr, ";", &save_ptr))) break;
        // The first field of the current input corresponds to the address.
        address_str = strtok(input_str, ",");
        int address = atoi(address_str);
        input_addresses.insert(address);
    }
    // Then we create a path through all unique input addresses.
    int prev_id = -1, curr_id;
    for (int curr_addr : input_addresses) {
        curr_id = contains_node(nodes, curr_addr);
        // If no node is associated with the address, then add one.
        if (curr_id == -1) {
            nodes[curr_addr] = *id_count;
            curr_id = *id_count;
            *id_count = *id_count + 1;
        }
        if (prev_id != -1) edges.insert(make_pair(prev_id, curr_id));
        prev_id = curr_id;
    }
}

/**
 * @brief Processes the list of transaction outputs (represented as a semicolon-separated string)
 * 
 * @param outputs string containing all transaction outputs
 * @param nodes reference to the node map
 * @param edges reference to the edge set
 * @param id_count counter for used node identifiers
 */
void process_outputs(char *outputs, node_map_t &nodes, edge_set_t &edges, int *id_count) {
    char *ptr, *save_ptr, *output_str, *address_str;
    for (ptr = outputs; ; ptr = NULL) {
        if (!(output_str = strtok_r(ptr, ";", &save_ptr))) break;
        // The first field of the output corresponds to the address.
        address_str = strtok(output_str, ",");
        int address = atoi(address_str);
        // Check if a node associated with the address exists.
        int id = contains_node(nodes, address);
        // If this is not the case, add a new node to the graph
        // and associate it with the address.
        if (id == -1) {
            nodes[address] = *id_count;
            *id_count = *id_count + 1;
        }
    }
}

/**
 * @brief Processes a single line of the input file (i.e., a transaction)
 * 
 * @param line_buf buffer containing the line
 * @param line_size size of the line buffer
 * @param nodes reference to the node map
 * @param edges reference to the edge set
 * @param id_count counter for used node identifiers
 */
void process_line(char *line_buf, size_t line_size, node_map_t &nodes, edge_set_t &edges, int *id_count) {
    char *token = NULL;
    int token_count = 0;
    while ((token = strsep(&line_buf, ":"))) {
        if (token_count == 1) {
            // Processing inputs...
            if (token[0] != '\0') process_inputs(token, nodes, edges, id_count);
        }
        if (token_count == 2) {
            // Processing outputs...
            if (token[0] != '\0') process_outputs(token, nodes, edges, id_count);
        }
        token_count++;
    }
}

int main(int argc, char **argv) {
    // Check the input arguments.
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <inputFile> <outputFile>\n";
        return 1;
    }
    auto start = high_resolution_clock::now();
    // Open the input and output files.
    FILE *input_file = fopen(argv[1], "r");
    if (!input_file) {
        cerr << "Error: could not open input file!\n";
        return 1;
    }
    FILE *output_file = fopen(argv[2], "wb");
    if (!output_file) {
        cerr << "Error: could not open output file!\n";
        return 1;
    }
    // Build the graph by reading the input file line by line.
    node_map_t nodes;
    edge_set_t edges;
    int id_count = 0;
    char *line_buf = NULL;
    size_t line_size = 0;
    while (getline(&line_buf, &line_size, input_file) > 0) {
        process_line(line_buf, line_size, nodes, edges, &id_count);
    }
    // Write the graph to the output file.
    int num_nodes = nodes.size();
    int num_edges = edges.size();
    int buf[2];
    buf[0] = __builtin_bswap32(num_nodes);
    buf[1] = __builtin_bswap32(num_edges);
    fwrite(buf, sizeof(int), 2, output_file);
    for (auto &e : edges) {
        buf[0] = __builtin_bswap32(e.first);
        buf[1] = __builtin_bswap32(e.second);
        fwrite(buf, sizeof(int), 2, output_file);
    }
    // Close the input and output files.
    fclose(input_file);
    fclose(output_file);
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start);
    // Print statistics.
    cout << "Nodes:\t" << num_nodes << '\n' 
    << "Edges:\t" << num_edges << '\n'
    << "Time:\t" << duration.count() << " ns\n";
    return 0;
}
