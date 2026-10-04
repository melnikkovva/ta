#include "Utils.h"
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

std::string Trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::string ReadPath(const std::string& prompt)
{
    std::cout << prompt;
    std::string path;
    if (!std::getline(std::cin, path))
    {
        throw std::runtime_error("Input interrupted");
    }
    path = Trim(path);
    if (path.empty())
    {
        throw std::runtime_error("The path cannot be empty.");
    }
    return path;
}

void ValidateMealy(const MealyMachine& mealy)
{
    std::set<std::pair<std::string, std::string>> seen;
    for (const auto& t : mealy.transitions)
    {
        if (!seen.insert({t.fromState, t.input}).second)
        {
            throw std::invalid_argument("The automaton is non‑deterministic.");
        }
    }
}

void ValidateMoore(const MooreMachine& moore)
{
    if (moore.states.find(moore.startState) == moore.states.end())
    {
        throw std::invalid_argument("Initial state " + moore.startState + " not described in the states section");
    }

    std::set<std::pair<std::string, std::string>> seen;
    for (const auto& t : moore.transitions)
    {
        if (moore.states.find(t.fromState) == moore.states.end())
        {
            throw std::invalid_argument("Transition from an unspecified state " + t.fromState);
        }
        if (moore.states.find(t.toState) == moore.states.end())
        {
            throw std::invalid_argument("Transition to an unspecified state " + t.toState);
        }
        if (!seen.insert({t.fromState, t.input}).second)
        {
            throw std::invalid_argument("The automaton is non‑deterministic.");
        }
    }
}

Automaton ParseAutomaton(std::istream& input)
{
    std::string line;
    std::string type;
    std::string startState;
    std::string section;
    size_t lineNumber = 0;

    MealyMachine mealy;
    MooreMachine moore;

    auto fail = [&](const std::string& message)
    {
        throw std::invalid_argument("Строка " + std::to_string(lineNumber) + ": " + message);
    };

    while (std::getline(input, line))
    {
        ++lineNumber;
        line = Trim(line);
        if (line.empty())
        {
            continue;
        }

        if (line.rfind("type:", 0) == 0)
        {
            type = Trim(line.substr(5));
            if (type != "mealy" && type != "moore")
            {
                fail("unknown type of automaton: \"" + type + "\"");
            }
            continue;
        }
        if (line.rfind("start:", 0) == 0)
        {
            startState = Trim(line.substr(6));
            if (startState.empty())
            {
                fail("The initial state is not specified.");
            }
            continue;
        }
        if (line == "states:")
        {
            section = "states";
            continue;
        }
        if (line == "transitions:")
        {
            section = "transitions";
            continue;
        }

        if (type.empty())
        {
            fail("The type of the automaton (type:) must be specified before the states and transitions.");
        }

        std::istringstream ss(line);
        std::string extra;

        if (type == "mealy")
        {
            if (section != "transitions")
            {
                fail("For the Mealy machine, only the transitions section is expected.");
            }
            std::string from, to, in, slash, out;
            if (!(ss >> from >> to >> in >> slash >> out) || slash != "/" || (ss >> extra))
            {
                fail("Invalid transition format, expected: <from> <to> <input> / <output>");
            }
            mealy.transitions.push_back({from, to, in, out});
        }
        else if (section == "states")
        {
            std::string alias, pipe, name, slash, out;
            if (!(ss >> alias >> pipe >> name >> slash >> out) || pipe != "|" || slash != "/" || (ss >> extra))
            {
                fail("Invalid state format, expected: <alias> | <name> / <output>");
            }
            if (!moore.states.emplace(alias, MooreState{alias, name, out}).second)
            {
                fail("state " + alias + " described again");
            }
        }
        else if (section == "transitions")
        {
            std::string from, to, in;
            if (!(ss >> from >> to >> in) || (ss >> extra))
            {
                fail("Invalid transition format, expected: <from> <to> <input>");
            }
            moore.transitions.push_back({from, to, in});
        }
        else
        {
            fail("A string outside the states/transitions sections.");
        }
    }

    if (type.empty())
    {
        throw std::invalid_argument("The file does not specify the type of machine");
    }
    if (startState.empty())
    {
        throw std::invalid_argument("The file does not specify the initial state");
    }

    if (type == "mealy")
    {
        mealy.startState = startState;
        ValidateMealy(mealy);
        return mealy;
    }

    moore.startState = startState;
    ValidateMoore(moore);
    return moore;
}

struct AutomatonWriter
{
    std::ostream& out;

    void operator()(const MealyMachine& machine) const
    {
        out << "type: mealy\n";
        out << "start: " << machine.startState << "\n\n";
        out << "transitions:\n";
        for (const auto& t : machine.transitions)
        {
            out << t.fromState << " " << t.toState << " " << t.input << " / " << t.output << "\n";
        }
    }

    void operator()(const MooreMachine& machine) const
    {
        out << "type: moore\n";
        out << "start: " << machine.startState << "\n\n";
        out << "states:\n";
        for (const auto& [alias, state] : machine.states)
        {
            out << alias << " | " << state.name << " / " << state.output << "\n";
        }
        out << "\ntransitions:\n";
        for (const auto& t : machine.transitions)
        {
            out << t.fromState << " " << t.toState << " " << t.input << "\n";
        }
    }
};

void WriteAutomaton(std::ostream& output, const Automaton& automaton)
{
    std::visit(AutomatonWriter{output}, automaton);
}