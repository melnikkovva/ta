#pragma once

#include <iosfwd>
#include <string>

#include "Types.h"

std::string Trim(const std::string& str);
std::string ReadPath(const std::string& prompt);
Automaton ParseAutomaton(std::istream& input);
void WriteAutomaton(std::ostream& output, const Automaton& automaton);