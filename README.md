# BitcoinAddressClustering

This repository contains a C++ implementation of the Bitcoin address clustering procedure described in [1]. 
The repository also comprises a tool for analyzing the resulting graph.

Given a list of transactions, the procedure produces a partition (i.e., a clustering) of all Bitcoin addresses included in such transactions. 
The clustering is constructed in accordance with the multi-input heuristic.

More precisely, the program reads a list of Bitcoin transactions 
and constructs a graph, called auxiliary graph, defined as follows.

1. Nodes correspond to all addresses appearing in the transactions. Both input and output addresses are considered.
2. For each transaction _T_ in the list, there is a path among the nodes representing the input addresses of _T_. 

The input file contains a textual representation of Bitcoin transactions. In <a href="https://zenodo.org/record/7696454#.ZBOmgy9abq0">this repository</a> you can find a more detailed description of the required input file format.

The output graph is represented as a binary file containing a sequence of 32-bit signed integers (in little-endian format). 
The file has the following format:

1. the first 32 bits represent the number of nodes _N_;
2. the next 32 bits represent the number of edges _M_;
3. the remaining _M_ pairs of 32-bit integers represent the edges of the graph. More precisely, the first (resp. second) integer corresponds to the identifier of the source (resp. target) node of the edge.

## References

[1] Di Francesco Maesa, Damiano, Andrea Marino, and Laura Ricci. "Data-driven analysis of bitcoin properties: exploiting the users graph."
_International Journal of Data Science and Analytics_ 6 (2018): 63-80.
