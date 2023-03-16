/**
 * @file string.h
 * @author Matteo Loporchio
 * @brief Utility functions for working with strings
 * @version 0.1
 * @date 2023-03-16
 * 
 * This file contains functions that can be used
 * for performing basic operations on strings.
 * 
 * @copyright Copyright (c) 2023 Matteo Loporchio
 * 
 */

#ifndef STRING_H
#define STRING_H

#include <string>
#include <vector>

using namespace std;

/**
 * @brief Splits a string according to a given delimiter
 * 
 * @param s string to be tokenized
 * @param delimiter the character used as separator
 * @return vector<string> a sequence of (possibly empty) string tokens
 */
static inline vector<string> split(const string &s, const char delimiter) {
    vector<string> output;
    size_t start = 0, end = s.find_first_of(delimiter);
    while (end <= string::npos) {
	    output.emplace_back(s.substr(start, end-start));
	    if (end == string::npos) break;
    	start = end+1;
    	end = s.find_first_of(delimiter, start);
    }
    return output;
}

#endif