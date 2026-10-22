#pragma once
#include <map>
#include <string>
#include <variant>
#include <vector>

struct MealyTransition
{
    std::string fromState;
    std::string toState;
    std::string input;
    std::string output;
};

struct MealyMachine
{
    std::string startState;
    std::vector<MealyTransition> transitions;
};

struct MooreState
{
    std::string alias;
    std::string name;
    std::string output;
};

struct MooreTransition
{
    std::string fromState;
    std::string toState;
    std::string input;
};

struct MooreMachine
{
    std::string startState;
    std::map<std::string, MooreState> states;
    std::vector<MooreTransition> transitions;
};

using Automaton = std::variant<MealyMachine, MooreMachine>;