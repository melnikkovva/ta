#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>
#include "Types.h"

const std::string EPSILON = "e";

struct TemporaryAutomaton
{
    std::string startState;
    std::string endState;
    std::map<std::string, MooreState> states;
    std::vector<MooreTransition> transitions;
};

std::string CreateState(TemporaryAutomaton& automaton, int& stateCounter)
{
    std::string alias = "S" + std::to_string(stateCounter);
    std::string name = "q" + std::to_string(stateCounter);
    ++stateCounter;
    automaton.states[alias] = MooreState{ alias, name, "0" };
    return alias;
}

void AddTransition(TemporaryAutomaton& automaton, const std::string& from, const std::string& to, const std::string& input)
{
    automaton.transitions.push_back(MooreTransition{ from, to, input });
}

void Merge(TemporaryAutomaton& automaton, const TemporaryAutomaton& ta)
{
    automaton.states.insert(ta.states.begin(), ta.states.end());
    automaton.transitions.insert(automaton.transitions.end(),
        ta.transitions.begin(), ta.transitions.end());
}

std::vector<std::string> SplitTopLevel(const std::string& expr, char separator)
{
    std::vector<std::string> parts;
    std::string current;
    int depth = 0;

    for (char c : expr)
    {
        if (c == '(')
        {
            ++depth;
        }
        else if (c == ')')
        {
            --depth;
            if (depth < 0)
            {
                throw std::runtime_error("Incorrect regular expression");
            }
        }

        if (c == separator && depth == 0)
        {
            parts.push_back(current);
            current.clear();
        }
        else
        {
            current += c;
        }
    }

    if (depth != 0)
    {
        throw std::runtime_error("Incorrect regular expression");
    }

    parts.push_back(current);
    return parts;
}

std::vector<std::string> SplitConcatenation(const std::string& expr)
{
    std::vector<std::string> units;
    size_t i = 0;

    while (i < expr.size())
    {
        std::string unit;
        char c = expr[i];

        if (c == '(')
        {
            int depth = 0;
            size_t j = i;
            for (; j < expr.size(); ++j)
            {
                if (expr[j] == '(') ++depth;
                else if (expr[j] == ')') --depth;
                if (depth == 0) break;
            }
            if (j >= expr.size())
            {
                throw std::runtime_error("Incorrect regular expression");
            }
            unit = expr.substr(i, j - i + 1);
            i = j + 1;
        }
        else if (c == ')' || c == '*' || c == '|')
        {
            throw std::runtime_error("Incorrect regular expression");
        }
        else
        {
            unit = std::string(1, c);
            ++i;
        }

        while (i < expr.size() && expr[i] == '*')
        {
            unit += '*';
            ++i;
        }

        units.push_back(unit);
    }

    return units;
}

TemporaryAutomaton BuildTemporary(const std::string& expr, int& stateCounter)
{
    TemporaryAutomaton result;

    if (expr.empty() || expr == "ε")
    {
        std::string q0 = CreateState(result, stateCounter);
        std::string q1 = CreateState(result, stateCounter);
        AddTransition(result, q0, q1, EPSILON);
        result.startState = q0;
        result.endState = q1;
        return result;
    }

    std::vector<std::string> alternatives = SplitTopLevel(expr, '|');
    if (alternatives.size() > 1)
    {
        std::string newStart = CreateState(result, stateCounter);
        std::string newEnd = CreateState(result, stateCounter);

        for (const std::string& part : alternatives)
        {
            TemporaryAutomaton sub = BuildTemporary(part, stateCounter);
            Merge(result, sub);
            AddTransition(result, newStart, sub.startState, EPSILON);
            AddTransition(result, sub.endState, newEnd, EPSILON);
        }

        result.startState = newStart;
        result.endState = newEnd;
        return result;
    }

    std::vector<std::string> units = SplitConcatenation(expr);
    if (units.size() > 1)
    {
        TemporaryAutomaton first = BuildTemporary(units[0], stateCounter);
        Merge(result, first);
        result.startState = first.startState;
        result.endState = first.endState;

        for (size_t i = 1; i < units.size(); ++i)
        {
            TemporaryAutomaton next = BuildTemporary(units[i], stateCounter);
            Merge(result, next);
            AddTransition(result, result.endState, next.startState, EPSILON);
            result.endState = next.endState;
        }
        return result;
    }

    const std::string& unit = units[0];

    if (unit.back() == '*')
    {
        std::string inner = unit.substr(0, unit.size() - 1);
        TemporaryAutomaton sub = BuildTemporary(inner, stateCounter);
        Merge(result, sub);

        std::string newStart = CreateState(result, stateCounter);
        std::string newEnd = CreateState(result, stateCounter);

        AddTransition(result, newStart, sub.startState, EPSILON); 
        AddTransition(result, sub.endState, newEnd, EPSILON);     
        AddTransition(result, sub.endState, sub.startState, EPSILON);
        AddTransition(result, newStart, newEnd, EPSILON);

        result.startState = newStart;
        result.endState = newEnd;
        return result;
    }

    if (unit.front() == '(' && unit.back() == ')')
    {
        return BuildTemporary(unit.substr(1, unit.size() - 2), stateCounter);
    }

    if (unit.size() == 1)
    {
        std::string q0 = CreateState(result, stateCounter);
        std::string q1 = CreateState(result, stateCounter);
        AddTransition(result, q0, unit, q1 == "" ? q1 : q1);
        result.transitions.back() = MooreTransition{ q0, q1, unit };
        result.startState = q0;
        result.endState = q1;
        return result;
    }

    throw std::runtime_error("Incorrect regular expression");
}

MooreMachine BuildNFA(const std::string& regExpr)
{
    int stateCounter = 0;
    TemporaryAutomaton temp = BuildTemporary(regExpr, stateCounter);
    temp.states[temp.endState].output = "1";

    std::map<std::string, std::vector<std::string>> adjacency;
    for (const MooreTransition& tr : temp.transitions)
    {
        adjacency[tr.fromState].push_back(tr.toState);
    }

    std::map<std::string, std::string> renamed;
    std::vector<std::string> order;
    std::queue<std::string> queue;
    queue.push(temp.startState);
    renamed[temp.startState] = "S0";
    order.push_back(temp.startState);

    while (!queue.empty())
    {
        std::string current = queue.front();
        queue.pop();
        for (const std::string& next : adjacency[current])
        {
            if (renamed.find(next) == renamed.end())
            {
                renamed[next] = "S" + std::to_string(renamed.size());
                order.push_back(next);
                queue.push(next);
            }
        }
    }

    MooreMachine machine;
    machine.startState = renamed[temp.startState];

    for (size_t i = 0; i < order.size(); ++i)
    {
        const MooreState& old = temp.states[order[i]];
        std::string alias = renamed[order[i]];
        machine.states[alias] = MooreState{ alias, "q" + std::to_string(i), old.output };
    }

    for (const MooreTransition& tr : temp.transitions)
    {
        machine.transitions.push_back(
            MooreTransition{ renamed[tr.fromState], renamed[tr.toState], tr.input });
    }

    return machine;
}

bool AliasLess(const std::string& a, const std::string& b)
{
    if (a.size() != b.size()) return a.size() < b.size();
    return a < b;
}

void ReadRegularExpression(std::string& regularExpr)
{
    std::getline(std::cin, regularExpr);
}

void WriteToFile(const Automaton& automaton, const std::string& path)
{
    if (!std::holds_alternative<MooreMachine>(automaton))
    {
        throw std::runtime_error("Work only with Moore machine");
    }
    const MooreMachine& machine = std::get<MooreMachine>(automaton);

    std::ofstream out(path);
    if (!out.is_open())
    {
        throw std::runtime_error("Failed to open file: " + path);
    }

    out << "type: moore\n";
    out << "start: " << machine.startState << "\n\n";

    std::vector<MooreState> states;
    for (const auto& [alias, state] : machine.states)
    {
        states.push_back(state);
    }
    std::sort(states.begin(), states.end(),
        [](const MooreState& a, const MooreState& b) { return AliasLess(a.alias, b.alias); });

    out << "states:\n";
    for (const MooreState& s : states)
    {
        out << s.alias << " | " << s.name << " / " << s.output << "\n";
    }

    std::vector<MooreTransition> transitions = machine.transitions;
    std::stable_sort(transitions.begin(), transitions.end(),
        [](const MooreTransition& a, const MooreTransition& b) {
            return AliasLess(a.fromState, b.fromState);
        });

    out << "\ntransitions:\n";
    for (const MooreTransition& t : transitions)
    {
        out << t.fromState << " " << t.toState << " " << t.input << "\n";
    }
}

int main()
{
    std::string regularExpr;
    std::string path;

    try
    {
        ReadRegularExpression(regularExpr);
        std::getline(std::cin, path);
        if (path.empty())
        {
            path = "output.txt";
        }

        Automaton automaton = BuildNFA(regularExpr);
        WriteToFile(automaton, path);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}