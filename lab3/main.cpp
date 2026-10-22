
#include "Utils.h"
#include "Types.h"
#include <fstream>
#include <iostream>
#include <queue>
#include <set>

using CombinedVertex = std::set<std::string>;

CombinedVertex GetGluedVertex(CombinedVertex states, const std::vector<MooreTransition>& transitions)
{
    std::queue<std::string> q;
    for (const auto& s : states) 
    {
        q.push(s);
    }

    while (!q.empty()) 
    {
        std::string currentV = q.front();
        q.pop();

        for (const auto& t : transitions) 
        {
            if (t.fromState == currentV && t.input == "-") 
            {
                if (states.find(t.toState) == states.end()) 
                {
                    states.insert(t.toState);
                    q.push(t.toState);
                }
            }
        }
    }
    return states;
}

std::string GetAlias(const CombinedVertex& cv) 
{
    std::string alias = "";
    for (const auto& s : cv) 
    {
        alias += s;
    }
    return alias;
}

MooreMachine ConvertDFA(MooreMachine nfa)
{
    MooreMachine newAutomaton;
    std::queue<CombinedVertex> q; 
    CombinedVertex start = GetGluedVertex({nfa.startState}, nfa.transitions);
    std::string startAlias = GetAlias(start);
    newAutomaton.startState = startAlias;
    q.push(start);

    std::string defaultOutput = "";
    if (!start.empty()) 
    {
        auto it = nfa.states.find(*start.begin());
        if (it != nfa.states.end()) 
        {
            defaultOutput = it->second.output;
        }
    }

    newAutomaton.states[startAlias] = MooreState{startAlias, startAlias, defaultOutput};

    std::set<std::string> inputs;
    for (const auto& t : nfa.transitions) 
    {
        if (t.input != "-") 
        {
            inputs.insert(t.input);
        }
    }

    std::set<CombinedVertex> visited;
    visited.insert(start);

    while (!q.empty())
    {
        CombinedVertex v = q.front();
        q.pop();
        std::string currentAlias = GetAlias(v);

        for (const std::string& input : inputs) 
        {
            CombinedVertex nextStates;

            for (const std::string& stateStr : v) 
            {
                for (const auto& t : nfa.transitions) 
                {
                    if (t.fromState == stateStr && t.input == input) 
                    {
                        nextStates.insert(t.toState);
                    }
                }
            }

            CombinedVertex nextClosure = GetGluedVertex(nextStates, nfa.transitions);

            if (nextClosure.empty()) 
            {
                continue; 
            }

            std::string nextAlias = GetAlias(nextClosure);

            if (visited.find(nextClosure) == visited.end()) 
            {
                visited.insert(nextClosure);
                q.push(nextClosure);

                std::string output = "";
                auto it = nfa.states.find(*nextClosure.begin());
                if (it != nfa.states.end()) 
                {
                    output = it->second.output;
                }

                newAutomaton.states[nextAlias] = MooreState{nextAlias, nextAlias, output};
            }
            newAutomaton.transitions.push_back(MooreTransition{currentAlias, nextAlias, input});
        }
    }
    return newAutomaton;
}

int main()
{
    try
    {
        std::string inputPath = ReadPath("Введите путь к файлу для чтения: ");
        std::ifstream inputFile(inputPath);
        if (!inputFile.is_open())
        {
            throw std::runtime_error("Не удалось открыть файл \"" + inputPath + "\"");
        }

        Automaton automaton = ParseAutomaton(inputFile);

        Automaton result;
        if (std::holds_alternative<MooreMachine>(automaton))
        {
            result = ConvertDFA(std::get<MooreMachine>(automaton));
        }
        else
        {
            throw std::invalid_argument("Ожидался автомат Мура");
        }

        std::string outputPath = ReadPath("Введите имя файла для записи результата: ");
        std::ofstream outputFile(outputPath);
        if (!outputFile.is_open())
        {
            throw std::runtime_error("Не удалось открыть файл для записи \"" + outputPath + "\"");
        }

        WriteAutomaton(outputFile, result);
        std::cout << "Автомат успешно сохранен в файл \"" << outputPath << "\".\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
    return 0;
}