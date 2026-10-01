#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream> 
#include <map>
#include <set>
#include <variant>
#include <algorithm>

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

using MooreTransitionTable = std::map<std::string, std::map<std::string, std::string>>;
using MealyTransitionTable = std::map<std::string, std::map<std::string, std::pair<std::string, std::string>>>;

std::string Trim(const std::string& str) 
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) 
    {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

Automaton ParseAutomaton(std::istream& input) 
{
    std::string line;
    std::string type = "";
    std::string startState = "";
    
    MealyMachine mealy;
    MooreMachine moore;
    std::string currentSection = "";

    while (std::getline(input, line)) 
    {
        line = Trim(line);
        if (line.empty()) 
        {
            continue;
        }

        if (line.rfind("type:", 0) == 0) 
        {
            type = Trim(line.substr(5));
            continue;
        }
        if (line.rfind("start:", 0) == 0) 
        {
            startState = Trim(line.substr(6));
            if (type == "mealy") 
            {
                mealy.startState = startState;
            }
            else if (type == "moore") 
            {
                moore.startState = startState;
            }
            continue;
        }

        if (line == "states:") 
        {
            currentSection = "states";
            continue;
        }
        if (line == "transitions:") 
        {
            currentSection = "transitions";
            continue;
        }

        std::stringstream ss(line);
        if (type == "mealy") 
        {
            if (currentSection == "transitions") 
            {
                std::string from, to, inSymbol, slash, outSymbol;
                if (ss >> from >> to >> inSymbol >> slash >> outSymbol && slash == "/") 
                {
                    mealy.transitions.push_back({from, to, inSymbol, outSymbol});
                }
            }
        } 
        else if (type == "moore") 
        {
            if (currentSection == "states") 
            {
                std::string alias, pipe, name, slash, outSymbol;
                if (ss >> alias >> pipe >> name >> slash >> outSymbol && pipe == "|" && slash == "/") 
                {
                    moore.states[alias] = {alias, name, outSymbol};
                }
            } 
            else if (currentSection == "transitions") 
            {
                std::string from, to, inSymbol;
                if (ss >> from >> to >> inSymbol) 
                {
                    moore.transitions.push_back({from, to, inSymbol});
                }
            }
        }
    }

    if (type == "mealy") 
    {
        return mealy;
    }
    return moore;
}

MooreTransitionTable BuildTransitionTableForMoore(const MooreMachine& moore, std::set<std::string>& inputAlphabet, std::vector<std::string>& statesList) 
{
    std::set<std::string> allStates;
    for (const auto& [alias, _] : moore.states) 
    {
        allStates.insert(alias);
    }
    
    MooreTransitionTable table;
    for (const auto& transition : moore.transitions) 
    {
        inputAlphabet.insert(transition.input);
        allStates.insert(transition.fromState);
        allStates.insert(transition.toState);
        table[transition.fromState][transition.input] = transition.toState;
    }
    statesList.assign(allStates.begin(), allStates.end());
    return table;
}

std::map<std::string, int> BuildInitialClassesForMoore(const MooreMachine& moore, const std::vector<std::string>& statesList) 
{
    std::map<std::string, int> stateToClassId;
    std::map<std::string, int> outputToClassId;
    int classCount = 0;

    for (const auto& stateName : statesList) 
    {
        auto it = moore.states.find(stateName);
        std::string outputSymbol = (it != moore.states.end()) ? it->second.output : "none";

        if (outputToClassId.find(outputSymbol) == outputToClassId.end()) 
        {
            outputToClassId[outputSymbol] = classCount++;
        }
        stateToClassId[stateName] = outputToClassId[outputSymbol];
    }
    return stateToClassId;
}

std::map<std::string, int> RefineClassesForMoore(const std::vector<std::string>& statesList, const std::set<std::string>& inputAlphabet, const MooreTransitionTable& table, std::map<std::string, int> stateToClassId) 
{
    bool splitOccurred = true;
    while (splitOccurred) 
    {
        splitOccurred = false;
        std::map<std::string, int> nextStateToClassId;
        std::map<std::pair<int, std::vector<int>>, int> signatureToClassId;
        int nextClassCount = 0;

        for (const auto& stateName : statesList) 
        {
            std::vector<int> targetClassIds;
            for (const auto& inputSymbol : inputAlphabet) 
            {
                auto it = table.find(stateName);
                std::string targetState = (it != table.end() && it->second.count(inputSymbol)) ? it->second.at(inputSymbol) : "";
                targetClassIds.push_back(stateToClassId[targetState]);
            }

            auto signature = std::make_pair(stateToClassId[stateName], targetClassIds);
            if (signatureToClassId.find(signature) == signatureToClassId.end()) 
            {
                signatureToClassId[signature] = nextClassCount++;
            }
            nextStateToClassId[stateName] = signatureToClassId[signature];
        }

        if (nextClassCount > signatureToClassId.size()) 
        {
            splitOccurred = true;
            stateToClassId = nextStateToClassId;
        }
    }
    return stateToClassId;
}

MooreMachine ConstructMooreMachine(const MooreMachine& original, const std::vector<std::string>& statesList, const std::set<std::string>& inputAlphabet, const MooreTransitionTable& table, const std::map<std::string, int>& stateToClassId) 
{
    MooreMachine minMoore;
    std::map<int, std::string> classIdToNewAlias;
    std::map<int, std::string> classIdToRepresentative;

    for (const auto& stateName : statesList) 
    {
        int equivalenceClassId = stateToClassId.at(stateName);
        if (classIdToNewAlias.find(equivalenceClassId) == classIdToNewAlias.end()) 
        {
            classIdToNewAlias[equivalenceClassId] = std::to_string(equivalenceClassId);
            classIdToRepresentative[equivalenceClassId] = stateName;
        }
    }

    minMoore.startState = classIdToNewAlias[stateToClassId.at(original.startState)];

    for (const auto& [equivalenceClassId, newAlias] : classIdToNewAlias) 
    {
        std::string representativeState = classIdToRepresentative[equivalenceClassId];
        auto it = original.states.find(representativeState);
        std::string outputSymbol = (it != original.states.end()) ? it->second.output : "none";
        
        minMoore.states[newAlias] = {newAlias, newAlias, outputSymbol};

        for (const auto& inputSymbol : inputAlphabet) 
        {
            auto tableIt = table.find(representativeState);
            if (tableIt != table.end() && tableIt->second.count(inputSymbol)) 
            {
                std::string targetState = tableIt->second.at(inputSymbol);
                if (!targetState.empty()) 
                {
                    minMoore.transitions.push_back({newAlias, classIdToNewAlias[stateToClassId.at(targetState)], inputSymbol});
                }
            }
        }
    }
    return minMoore;
}

MooreMachine MinimizeMoore(const MooreMachine& moore) 
{
    std::set<std::string> inputAlphabet;
    std::vector<std::string> statesList;

    auto transitionTable = BuildTransitionTableForMoore(moore, inputAlphabet, statesList);
    auto initialClasses = BuildInitialClassesForMoore(moore, statesList);
    auto refinedClasses = RefineClassesForMoore(statesList, inputAlphabet, transitionTable, initialClasses);

    return ConstructMooreMachine(moore, statesList, inputAlphabet, transitionTable, refinedClasses);
}

MealyTransitionTable BuildTransitionTableForMealy(const MealyMachine& mealy, std::set<std::string>& inputAlphabet, std::vector<std::string>& statesList) 
{
    std::set<std::string> allStates;
    MealyTransitionTable table;

    for (const auto& transition : mealy.transitions) 
    {
        inputAlphabet.insert(transition.input);
        allStates.insert(transition.fromState);
        allStates.insert(transition.toState);
        table[transition.fromState][transition.input] = {transition.toState, transition.output};
    }
    statesList.assign(allStates.begin(), allStates.end());
    return table;
}

std::map<std::string, int> BuildInitialClassesForMealy(const std::vector<std::string>& statesList, const std::set<std::string>& inputAlphabet, const MealyTransitionTable& table) 
{
    std::map<std::string, int> stateToClassId;
    std::map<std::vector<std::string>, int> outputsSignatureToClassId;
    int classCount = 0;

    for (const auto& stateName : statesList) 
    {
        std::vector<std::string> outputsSignature;
        for (const auto& inputSymbol : inputAlphabet) 
        {
            auto it = table.find(stateName);
            std::string outputSymbol = (it != table.end() && it->second.count(inputSymbol)) ? it->second.at(inputSymbol).second : "";
            outputsSignature.push_back(outputSymbol);
        }

        if (outputsSignatureToClassId.find(outputsSignature) == outputsSignatureToClassId.end()) 
        {
            outputsSignatureToClassId[outputsSignature] = classCount++;
        }
        stateToClassId[stateName] = outputsSignatureToClassId[outputsSignature];
    }
    return stateToClassId;
}

std::map<std::string, int> RefineClassesForMealy(const std::vector<std::string>& statesList, const std::set<std::string>& inputAlphabet, const MealyTransitionTable& table, std::map<std::string, int> stateToClassId) 
{
    bool splitOccurred = true;
    while (splitOccurred) 
    {
        splitOccurred = false;
        std::map<std::string, int> nextStateToClassId;
        std::map<std::pair<int, std::vector<int>>, int> signatureToClassId;
        int nextClassCount = 0;

        for (const auto& stateName : statesList) 
        {
            std::vector<int> targetClassIds;
            for (const auto& inputSymbol : inputAlphabet) 
            {
                auto it = table.find(stateName);
                std::string targetState = (it != table.end() && it->second.count(inputSymbol)) ? it->second.at(inputSymbol).first : "";
                targetClassIds.push_back(stateToClassId[targetState]);
            }

            auto signature = std::make_pair(stateToClassId[stateName], targetClassIds);
            if (signatureToClassId.find(signature) == signatureToClassId.end()) 
            {
                signatureToClassId[signature] = nextClassCount++;
            }
            nextStateToClassId[stateName] = signatureToClassId[signature];
        }

        if (nextClassCount > signatureToClassId.size()) 
        {
            splitOccurred = true;
            stateToClassId = nextStateToClassId;
        }
    }
    return stateToClassId;
}

MealyMachine ConstructMealyMachine(const MealyMachine& original, const std::vector<std::string>& statesList, const std::set<std::string>& inputAlphabet, const MealyTransitionTable& table, const std::map<std::string, int>& stateToClassId) 
{
    MealyMachine minMealy;
    std::map<int, std::string> classIdToNewAlias;
    std::map<int, std::string> classIdToRepresentative;

    for (const auto& stateName : statesList) 
    {
        int equivalenceClassId = stateToClassId.at(stateName);
        if (classIdToNewAlias.find(equivalenceClassId) == classIdToNewAlias.end()) 
        {
            classIdToNewAlias[equivalenceClassId] = std::to_string(equivalenceClassId);
            classIdToRepresentative[equivalenceClassId] = stateName;
        }
    }

    minMealy.startState = classIdToNewAlias[stateToClassId.at(original.startState)];

    for (const auto& [equivalenceClassId, newAlias] : classIdToNewAlias) 
    {
        std::string representativeState = classIdToRepresentative[equivalenceClassId];
        for (const auto& inputSymbol : inputAlphabet) 
        {
            auto tableIt = table.find(representativeState);
            if (tableIt != table.end() && tableIt->second.count(inputSymbol)) 
            {
                auto [targetState, outputSymbol] = tableIt->second.at(inputSymbol);
                if (!targetState.empty()) 
                {
                    minMealy.transitions.push_back({newAlias, classIdToNewAlias[stateToClassId.at(targetState)], inputSymbol, outputSymbol});
                }
            }
        }
    }
    return minMealy;
}

MealyMachine MinimizeMealy(const MealyMachine& mealy) 
{
    std::set<std::string> inputAlphabet;
    std::vector<std::string> statesList;

    auto transitionTable = BuildTransitionTableForMealy(mealy, inputAlphabet, statesList);
    auto initialClasses = BuildInitialClassesForMealy(statesList, inputAlphabet, transitionTable);
    auto refinedClasses = RefineClassesForMealy(statesList, inputAlphabet, transitionTable, initialClasses);

    return ConstructMealyMachine(mealy, statesList, inputAlphabet, transitionTable, refinedClasses);
}

Automaton Minimize(const MealyMachine& mealy) 
{
    return MinimizeMealy(mealy);
}

Automaton Minimize(const MooreMachine& moore) 
{
    return MinimizeMoore(moore);
}

Automaton MinimizeAutomaton(const Automaton& automaton) 
{
    return std::visit([](const auto& machine) -> Automaton {
        return Minimize(machine);
    }, automaton);
}

struct AutomatonWriter 
{
    std::ostream& outputStream;

    void operator()(const MealyMachine& machine) const 
    {
        outputStream << "type: mealy\n";
        outputStream << "start: " << machine.startState << "\n\n";
        outputStream << "transitions:\n";
        for (const auto& transition : machine.transitions) 
        {
            outputStream << transition.fromState << " " << transition.toState << " " << transition.input << " / " << transition.output << "\n";
        }
    }

    void operator()(const MooreMachine& machine) const 
    {
        outputStream << "type: moore\n";
        outputStream << "start: " << machine.startState << "\n\n";
        outputStream << "states:\n";
        for (const auto& [alias, state] : machine.states) 
        {
            outputStream << alias << " | " << state.name << " / " << state.output << "\n";
        }
        outputStream << "\ntransitions:\n";
        for (const auto& transition : machine.transitions) 
        {
            outputStream << transition.fromState << " " << transition.toState << " " << transition.input << "\n";
        }
    }
};

int main() 
{
    std::string inputFilePath;
    std::cout << "Введите путь к файлу для чтения: ";
    std::cin >> inputFilePath;

    std::ifstream inputFile(inputFilePath);
    if (!inputFile.is_open()) 
    {
        std::cerr << "Ошибка: Не удалось открыть файл \"" << inputFilePath << "\"\n";
    }

    Automaton originalAutomaton = ParseAutomaton(inputFile);
    inputFile.close();

    Automaton minimizedAutomaton = MinimizeAutomaton(originalAutomaton);

    std::string outputFilePath;
    std::cout << "Введите имя файла для записи результата минимизации: ";
    std::cin >> outputFilePath;

    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) 
    {
        std::cerr << "Ошибка: Не удалось открыть файл для записи \"" << outputFilePath << "\"\n";
    }

    std::visit(AutomatonWriter{outputFile}, minimizedAutomaton);
    outputFile.close();

    std::cout << "Автомат успешно сохранен в файл \"" << outputFilePath << "\".\n";
}