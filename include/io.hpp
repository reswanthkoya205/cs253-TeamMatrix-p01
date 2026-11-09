#pragma once
// Input and output. Owner: Architect. Implemented in app/.
// Formats are specified in INTERFACE.md (Makefile-like file and the evaluation format).
#include <iostream>
#include <string>
#include <vector>
#include "graph.hpp"
#include "result.hpp"

// Makefile-like input. Returns false and fills errorMessage on malformed input.
// names (optional) receives the id -> target-name mapping.
bool readGraph(std::istream& in, Graph& g, std::string& errorMessage,
               std::vector<std::string>* names = nullptr);

// Plain format: "n m", durations line, then m lines "u v". Used by generators and tests.
void writeGraph(std::ostream& out, const Graph& g);
bool readGraphPlain(std::istream& in, Graph& g, std::string& errorMessage);

// Canonical text form of a Result, for diffing and the hidden-test output format.
std::string serializeResult(const Result& r);

// Human-readable output (names is optional).
void printTopologicalOrder(const Result& r, std::ostream& os = std::cout,
                           const std::vector<std::string>* names = nullptr);
void printCycle(const Result& r, std::ostream& os = std::cout,
                const std::vector<std::string>* names = nullptr);
void printSchedule(const Result& r, std::ostream& os = std::cout);
void printSlack(const Result& r, std::ostream& os = std::cout);
void printCriticalPath(const Result& r, std::ostream& os = std::cout,
                       const std::vector<std::string>* names = nullptr);
void printResult(const Result& r, std::ostream& os = std::cout,
                 const std::vector<std::string>* names = nullptr);
