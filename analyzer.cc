/**
 * @file analyzer.cc
 * @author Matteo Loporchio
 * @brief Computes the connected components of the auxiliary graph
 * @version 0.1
 * @date 2023-03-16
 * 
 * This program reads the auxiliary graph from a file and analyzes it
 * by computing its connected components. The output of this program
 * is a CSV file containing one line for each node of the auxiliary graph.
 * Each line contains the node identifier and the identifier of the 
 * component the node belongs to, separated by a comma.
 * 
 * @copyright Copyright (c) 2023 Matteo Loporchio
 */

#include <chrono>
#include <fstream>
#include <iostream>
#include <lemon/connectivity.h>
#include <lemon/list_graph.h>

using namespace lemon;
using namespace std;
using namespace std::chrono;

int main(int argc, char **argv) {
    if (argc < 3) {
        cerr << "Usage: analyzer <inputFile> <outputFile>" << endl;
        return 1;
    }
    // Open input and output files.
    ifstream input_file(argv[1], ios::binary);
    ofstream output_file(argv[2], ios::out);
    if (!input_file) {
        cerr << "Error: could not open input file!" << endl;
        return 1;
    }
    if (!output_file) {
        cerr << "Error: could not open output file!" << endl;
        return 1;
    }
    auto start = high_resolution_clock::now();
    // Read graph from input file.
    // First, read the number of nodes and the number of edges.
    int int_pair[2];
    input_file.read((char*) &int_pair, 2*sizeof(int));
    int num_nodes = int_pair[0], num_edges = int_pair[1];
    // Reserve memory for nodes and edges and add new nodes.
    ListGraph graph;
    graph.reserveNode(num_nodes);
    graph.reserveEdge(num_edges);
    for (int i = 0; i < num_nodes; i++) graph.addNode();
    // Read edges and add them to the graph.
    while (input_file.read((char*) &int_pair, 2*sizeof(int))) {
        int u_node_id = int_pair[0], v_node_id = int_pair[1];
        ListGraph::Node u_node = graph.nodeFromId(u_node_id), 
        v_node = graph.nodeFromId(v_node_id);
        graph.addEdge(u_node, v_node);
    }
    input_file.close();
    // Compute the connected components of the graph.
    ListGraph::NodeMap<int> comp_map(graph);
    int num_comp = connectedComponents(graph, comp_map);
    // For each node, write a line containing the node identifier
    // and the identifier of the component it belongs to.
    for (ListGraph::NodeIt n(graph); n != INVALID; ++n) {
        int node_id = graph.id(n), comp_id = comp_map[n];
        output_file << node_id << ',' << comp_id << '\n';
    }
    output_file.close();
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start);
    // Print some basic information.
    cout << "Nodes:\t\t" << countNodes(graph) << endl 
    << "Edges:\t\t" << countEdges(graph) << endl
    << "Components:\t" << num_comp << endl
    << "Time:\t\t" << duration.count() << " ns" << endl;
    return 0;
}