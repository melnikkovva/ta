#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include "Utils.h"

using NextStateTable = std::map<std::string, std::map<std::string, std::string>>;

struct Table
{
    std::string startState;
    std::vector<std::string> states;  
    std::vector<std::string> inputs;  
    NextStateTable nextState;
    NextStateTable mealyOutput;                    
    std::map<std::string, std::string> mooreOutput; 
};

const std::string NO_TRANSITION = "<none>";

bool HasTransition(const Table& table, const std::string& state, const std::string& input)
{
    auto stateIt = table.nextState.find(state);
    if (stateIt == table.nextState.end())
    {
        return false;
    }
    return stateIt->second.count(input) > 0;
}

std::vector<std::string> FindReachableStates(const std::string& startState, const NextStateTable& nextState)
{
    std::vector<std::string> reachable = {startState};
    std::set<std::string> visited = {startState};

    for (size_t i = 0; i < reachable.size(); i++)
    {
        auto it = nextState.find(reachable[i]);
        if (it == nextState.end())
        {
            continue;
        }
        for (const auto& inputAndTarget : it->second)
        {
            const std::string& target = inputAndTarget.second;
            if (visited.count(target) == 0)
            {
                visited.insert(target);
                reachable.push_back(target);
            }
        }
    }
    return reachable;
}

std::vector<std::string> CollectInputs(const std::vector<std::string>& states, const NextStateTable& nextState)
{
    std::set<std::string> inputs;
    for (const std::string& state : states)
    {
        auto it = nextState.find(state);
        if (it == nextState.end())
        {
            continue;
        }
        for (const auto& inputAndTarget : it->second)
        {
            inputs.insert(inputAndTarget.first);
        }
    }
    return std::vector<std::string>(inputs.begin(), inputs.end());
}

Table MakeTable(const MooreMachine& moore)
{
    Table table;
    table.startState = moore.startState;

    for (const MooreTransition& transition : moore.transitions)
    {
        table.nextState[transition.fromState][transition.input] = transition.toState;
    }

    table.states = FindReachableStates(table.startState, table.nextState);
    table.inputs = CollectInputs(table.states, table.nextState);

    for (const std::string& state : table.states)
    {
        table.mooreOutput[state] = moore.states.at(state).output;
    }
    return table;
}

Table MakeTable(const MealyMachine& mealy)
{
    Table table;
    table.startState = mealy.startState;

    for (const MealyTransition& transition : mealy.transitions)
    {
        table.nextState[transition.fromState][transition.input] = transition.toState;
        table.mealyOutput[transition.fromState][transition.input] = transition.output;
    }

    table.states = FindReachableStates(table.startState, table.nextState);
    table.inputs = CollectInputs(table.states, table.nextState);
    return table;
}

std::map<std::string, int> GroupByKey(const std::vector<std::string>& states, const std::map<std::string, std::string>& keyOfState)
{
    std::map<std::string, int> classOfKey;
    std::map<std::string, int> classOfState;

    for (const std::string& state : states)
    {
        const std::string& key = keyOfState.at(state);
        if (classOfKey.count(key) == 0)
        {
            int newClassNumber = static_cast<int>(classOfKey.size());
            classOfKey[key] = newClassNumber;
        }
        classOfState[state] = classOfKey[key];
    }
    return classOfState;
}

int CountClasses(const std::map<std::string, int>& classOfState)
{
    std::set<int> classes;
    for (const auto& stateAndClass : classOfState)
    {
        classes.insert(stateAndClass.second);
    }
    return static_cast<int>(classes.size());
}

std::map<std::string, int> FirstSplitMoore(const Table& table)
{
    return GroupByKey(table.states, table.mooreOutput);
}

std::map<std::string, int> FirstSplitMealy(const Table& table)
{
    std::map<std::string, std::string> keyOfState;

    for (const std::string& state : table.states)
    {
        std::string key;
        for (const std::string& input : table.inputs)
        {
            if (HasTransition(table, state, input))
            {
                key += table.mealyOutput.at(state).at(input);
            }
            else
            {
                key += NO_TRANSITION;
            }
            key += "\n"; 
        }
        keyOfState[state] = key;
    }
    return GroupByKey(table.states, keyOfState);
}

std::map<std::string, int> SplitUntilStable(const Table& table, std::map<std::string, int> classOfState)
{
    int classCount = CountClasses(classOfState);

    while (true)
    {
        std::map<std::string, std::string> keyOfState;
        for (const std::string& state : table.states)
        {
            std::string key = std::to_string(classOfState[state]);
            for (const std::string& input : table.inputs)
            {
                key += ",";
                if (HasTransition(table, state, input))
                {
                    std::string target = table.nextState.at(state).at(input);
                    key += std::to_string(classOfState[target]);
                }
                else
                {
                    key += "-1";
                }
            }
            keyOfState[state] = key;
        }

        classOfState = GroupByKey(table.states, keyOfState);

        int newClassCount = CountClasses(classOfState);
        if (newClassCount == classCount)
        {
            break; 
        }
        classCount = newClassCount;
    }
    return classOfState;
}

struct NewStates
{
    std::map<int, std::string> nameOfClass;           
    std::map<int, std::string> representativeOfClass; 
};

NewStates NameNewStates(const Table& table, const std::map<std::string, int>& classOfState, const std::string& prefix)
{
    std::vector<std::string> order = {table.startState};
    for (const std::string& state : table.states)
    {
        if (state != table.startState)
        {
            order.push_back(state);
        }
    }

    NewStates result;
    for (const std::string& state : order)
    {
        int stateClass = classOfState.at(state);
        if (result.nameOfClass.count(stateClass) == 0)
        {
            result.nameOfClass[stateClass] = prefix + std::to_string(result.nameOfClass.size());
            result.representativeOfClass[stateClass] = state;
        }
    }
    return result;
}

MooreMachine MinimizeMoore(const MooreMachine& moore)
{
    Table table = MakeTable(moore);
    std::map<std::string, int> classOfState = SplitUntilStable(table, FirstSplitMoore(table));
    NewStates newStates = NameNewStates(table, classOfState, "Q");

    MooreMachine result;
    result.startState = newStates.nameOfClass[classOfState[table.startState]];

    for (const auto& classAndName : newStates.nameOfClass)
    {
        int stateClass = classAndName.first;
        const std::string& newName = classAndName.second;
        const std::string& oldState = newStates.representativeOfClass[stateClass];

        result.states[newName] = {newName, newName, table.mooreOutput.at(oldState)};

        for (const std::string& input : table.inputs)
        {
            if (!HasTransition(table, oldState, input))
            {
                continue;
            }
            std::string oldTarget = table.nextState.at(oldState).at(input);
            std::string newTarget = newStates.nameOfClass[classOfState[oldTarget]];
            result.transitions.push_back({newName, newTarget, input});
        }
    }
    return result;
}

MealyMachine MinimizeMealy(const MealyMachine& mealy)
{
    Table table = MakeTable(mealy);
    std::map<std::string, int> classOfState = SplitUntilStable(table, FirstSplitMealy(table));
    NewStates newStates = NameNewStates(table, classOfState, "S");

    MealyMachine result;
    result.startState = newStates.nameOfClass[classOfState[table.startState]];

    for (const auto& classAndName : newStates.nameOfClass)
    {
        int stateClass = classAndName.first;
        const std::string& newName = classAndName.second;
        const std::string& oldState = newStates.representativeOfClass[stateClass];

        for (const std::string& input : table.inputs)
        {
            if (!HasTransition(table, oldState, input))
            {
                continue;
            }
            std::string oldTarget = table.nextState.at(oldState).at(input);
            std::string newTarget = newStates.nameOfClass[classOfState[oldTarget]];
            std::string output = table.mealyOutput.at(oldState).at(input);
            result.transitions.push_back({newName, newTarget, input, output});
        }
    }
    return result;
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
        if (std::holds_alternative<MealyMachine>(automaton))
        {
            result = MinimizeMealy(std::get<MealyMachine>(automaton));
        }
        else
        {
            result = MinimizeMoore(std::get<MooreMachine>(automaton));
        }

        std::string outputPath = ReadPath("Введите имя файла для записи результата минимизации: ");
        std::ofstream outputFile(outputPath);
        if (!outputFile.is_open())
        {
            throw std::runtime_error("Не удалось открыть файл для записи \"" + outputPath + "\"");
        }

        WriteAutomaton(outputFile, result);
        std::cout << "Автомат успешно сохранен в файл \"" << outputPath << "\".\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << "Ошибка: " << error.what() << "\n";
        return 1;
    }

    return 0;
}