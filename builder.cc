/**
 * @file builder.cc
 * @author Matteo Loporchio
 * @brief Implementation of the Bitcoin address clustering algorithm [1]
 * @version 0.1
 * @date 2023-03-16
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
 * a sequence of 32-bit signed integers (in little-endian format). 
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
#include <fstream>
#include <iostream>
#include <lemon/list_graph.h>
#include <lemon/maps.h>
#include <set>
#include <unordered_map>
#include <utility>
#include "string.h"

using namespace std;
using namespace std::chrono;
using namespace lemon;

/// @brief Represents a map from addresses to node identifiers
typedef unordered_map<int,int> AddrToIdMap;

/**
 * @brief Checks if the map contains a node identifier for the given address
 * 
 * @param map address to node identifier map
 * @param key address
 * @return int the node identifier if the address is present, -1 otherwise
 */
inline int contains(AddrToIdMap &map, int key) {
    AddrToIdMap::iterator it = map.find(key);
    return ((it != map.end()) ? it->second : -1);
}

/**
 * @brief Adds a new (undirected) edge to the auxiliary graph, 
 * provided that it does not already exist
 * 
 * @param graph auxiliary graph
 * @param u_node_id identifier of the first node
 * @param v_node_id identifier of the second node
 */
inline void add_edge(ListGraph &graph, int u_node_id, int v_node_id) {
    ListGraph::Node u_node = graph.nodeFromId(u_node_id), 
    v_node = graph.nodeFromId(v_node_id);
    if (findEdge(graph, u_node, v_node) != INVALID) return;
    graph.addEdge(u_node, v_node);
}

/**
 * @brief Processes the inputs of a given transaction by creating a path among
 * all input addresses
 * 
 * @param graph auxiliary graph
 * @param addr_to_id map from addresses to node identifiers
 * @param inputs string representing all transaction inputs
 */
void process_inputs(ListGraph &graph, AddrToIdMap &addr_to_id, const string &inputs) {
    // Collect all unique input addresses.
    set<int> input_addresses;
    vector<string> tx_inputs = split(inputs, ';');
    for (const string &input : tx_inputs) {
        vector<string> input_parts = split(input, ',');
        int addr = stoi(input_parts[0]);
        input_addresses.insert(addr);
    }
    // Then we create a path through all unique input addresses.
    int prev_node_id = -1, curr_node_id;
    for (int curr_addr : input_addresses) {
        curr_node_id = contains(addr_to_id, curr_addr);
        // If no node is associated with the address, then add one.
        if (curr_node_id == -1) {
            curr_node_id = graph.maxNodeId() + 1;
            addr_to_id[curr_addr] = curr_node_id;
            graph.addNode();
        }
        if (prev_node_id != -1) add_edge(graph, prev_node_id, curr_node_id);
        prev_node_id = curr_node_id;
    }
}

/**
 * @brief Processes the outputs of a given transaction
 * 
 * @param graph auxiliary graph
 * @param addr_to_id map from addresses to node identifiers
 * @param outputs string representing all transaction outputs
 */
void process_outputs(ListGraph &graph, AddrToIdMap &addr_to_id, const string &outputs) {
    vector<string> tx_outputs = split(outputs, ';');
    for (const string &output : tx_outputs) {
        // Extract the address from the output.
        vector<string> output_parts = split(output, ',');
        int addr = stoi(output_parts[0]);
        // Check if a node associated with the address exists.
        int node_id = contains(addr_to_id, addr);
        // If this is not the case, add a new node to the graph
        // and associate it with the address.
        if (node_id == -1) {
            addr_to_id[addr] = graph.maxNodeId() + 1;
            graph.addNode();
        }
    }
}

int main(int argc, char **argv) {
    // Check the input arguments.
    if (argc < 3) {
        cerr << "Usage: builder <inputFile> <outputFile>" << endl;
        return 1;
    }
    auto start = high_resolution_clock::now();
    // Open the input and output files.
    ifstream input_file(argv[1], ios::in);
    if (!input_file) {
        cerr << "Error: could not open input file!" << endl;
        return 1;
    }
    ofstream output_file(argv[2], ios::out | ios::binary);
    if (!output_file) {
        cerr << "Error: could not open output file!" << endl;
        return 1;
    }
    // Build the graph by reading the input file line by line.
    ListGraph graph;
    AddrToIdMap addr_to_id;
    string line;
    while (getline(input_file, line)) {
        vector<string> parts = split(line, ':');
        string info = parts[0], inputs = parts[1], outputs = parts[2];
        if (inputs != "") process_inputs(graph, addr_to_id, inputs);
        if (outputs != "") process_outputs(graph, addr_to_id, outputs);
    }
    input_file.close();
    // At this point, we can write results to the output file.
    // First, we write the total number of nodes (i.e., the total number 
    // of unique addresses) and then the total number of edges.
    int num_nodes = countNodes(graph), num_edges = countEdges(graph);
    int int_pair[2];
    int_pair[0] = num_nodes;
    int_pair[1] = num_edges;
    output_file.write((char*) &int_pair, 2*sizeof(int));
    // Then we write the list of edges.
    for (ListGraph::EdgeIt e(graph); e != INVALID; ++e) {
        ListGraph::Node u_node = graph.u(e), v_node = graph.v(e);
        int_pair[0] = graph.id(u_node);
        int_pair[1] = graph.id(v_node);
        output_file.write((char*) &int_pair, 2*sizeof(int));
    }
    output_file.close();
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start);
    // Print statistics.
    cout << "Nodes:\t" << num_nodes << endl << "Edges:\t" << num_edges << endl
    << "Time:\t" << duration.count() << " ns" << endl;
    return 0;
}

