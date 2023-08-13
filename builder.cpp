/**
 * @file builder.cpp
 * @author Matteo Loporchio
 * @brief Implementation of the Bitcoin address clustering algorithm [1]
 * @version 1.0
 * @date 2023-08-13
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

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <utility>
#include <vector>

using namespace std;
using namespace std::chrono;

/// @brief The edge list contains ordered pairs representing graph edges
typedef vector<pair<int,int>> edge_list_t;

/**
 * @brief Processes the list of transaction inputs (represented as a semicolon-separated string)
 * 
 * @param inputs string containing all transaction inputs
 * @param max_id maximum address identifier seen while parsing transactions
 * @param edges list of graph edges
 */
void process_inputs(char *inputs, int *max_id, edge_list_t &edges) {
    char *ptr = inputs, *save_ptr;
    // Obtain the first input address.
    char *input = strtok_r(ptr, ";", &save_ptr);
    int first_address = atoi(strtok(input, ","));
    if (first_address >= *max_id) *max_id = first_address;
    // Iterate trough all remaining inputs.
    int curr_address;
    while ((input = strtok_r(NULL, ";", &save_ptr))) {
        // Extract the current address.
        curr_address = atoi(strtok(input, ","));
        // Skip this address if it is equal to the first one.
        if (curr_address == first_address) continue;
        // Create the new edge.
        edges.push_back(minmax(first_address, curr_address));
        // Update the maximum identifier, if necessary.
        if (curr_address >= *max_id) *max_id = curr_address;
    }
}

/**
 * @brief Processes the list of transaction outputs (represented as a semicolon-separated string)
 * 
 * @param outputs string containing all transaction outputs
 * @param max_id maximum address identifier seen while parsing transactions
 * @param edges list of graph edges 
 */
void process_outputs(char *outputs, int *max_id, edge_list_t &edges) {
    char *ptr, *save_ptr, *output_str, *address_str;
    int address;
    for (ptr = outputs; ; ptr = NULL) {
        if (!(output_str = strtok_r(ptr, ";", &save_ptr))) break;
        // The first field of the output corresponds to the address.
        address_str = strtok(output_str, ",");
        address = atoi(address_str);
        // Update the maximum identifier, if necessary.
        if (address >= *max_id) *max_id = address;
    }
}

/**
 * @brief Processes a single line of the input file (i.e., a transaction)
 * 
 * @param line_buf buffer containing the line
 * @param max_id maximum address identifier seen while parsing transactions
 * @param edges list of graph edges
 */
void process_line(char *line_buf, int *max_id, edge_list_t &edges) {
    char *token = NULL;
    int token_count = 0;
    while ((token = strsep(&line_buf, ":"))) {
        if (token_count == 1) {
            // Processing inputs
            if (token[0] != '\0') process_inputs(token, max_id, edges);
        }
        if (token_count == 2) {
            // Processing outputs
            if (token[0] != '\0') process_outputs(token, max_id, edges);
        }
        token_count++;
    }
}

int main(int argc, char **argv) {
    // Check the input arguments.
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <input_file> <output_file>\n";
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

    // Read the input file line by line and build the graph.
    edge_list_t edges;
    int max_id = 0;
    char *line_buf = NULL;
    size_t line_size = 0;
    while (getline(&line_buf, &line_size, input_file) > 0) {
        process_line(line_buf, &max_id, edges);
    }

    // Sort the list of edges.
    sort(edges.begin(), edges.end());

    // First, write the list of edges to the graph file.
    int buf[2];
    int num_nodes = max_id + 1;
    int num_edges = 0;
    fseek(output_file, 8, SEEK_SET);
    for (int i = 0; i < edges.size(); i++) {
        if (i == 0 || edges[i] != edges[i-1]) {
            // Skip duplicates.
            buf[0] = __builtin_bswap32(edges[i].first);
            buf[1] = __builtin_bswap32(edges[i].second);
            fwrite(buf, sizeof(int), 2, output_file);
            num_edges++;
        }
    }
    // Then, write the number of nodes and edges at the beginning of the file.
    fseek(output_file, 0, SEEK_SET);
    buf[0] = __builtin_bswap32(num_nodes);
    buf[1] = __builtin_bswap32(num_edges);
    fwrite(buf, sizeof(int), 2, output_file);

    // Close the input and output files.
    fclose(input_file);
    fclose(output_file);

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start);

    // Print statistics.
    cout << num_nodes << '\t' << num_edges << '\t' << duration.count() << '\n';
    return 0;
}
