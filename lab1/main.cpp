#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream> 
#include <map>
#include <set>
#include <variant>

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
                std::string from, to, in, slash, out;
                if (ss >> from >> to >> in >> slash >> out && slash == "/") 
                {
                    mealy.transitions.push_back({from, to, in, out});
                }
            }
        } 
        else if (type == "moore") 
        {
            if (currentSection == "states") 
            {
                std::string alias, pipe, name, slash, out;
                if (ss >> alias >> pipe >> name >> slash >> out && pipe == "|" && slash == "/") 
                {
                    moore.states[alias] = {alias, name, out};
                }
            } 
            else if (currentSection == "transitions") 
            {
                std::string from, to, in;
                if (ss >> from >> to >> in) 
                {
                    moore.transitions.push_back({from, to, in});
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

MooreMachine ConvertMealyToMoore(const MealyMachine& mealy) 
{
    MooreMachine moore;

    for (const auto& t : mealy.transitions) 
    {
        std::string targetAlias = t.toState + "_" + t.output;
        
        if (moore.states.find(targetAlias) == moore.states.end()) 
        {
            moore.states[targetAlias] = {targetAlias, targetAlias, t.output};
        }
    }

    for (const auto& [sourceAlias, sourceState] : moore.states) 
    {
        std::string origFrom = sourceAlias.substr(0, sourceAlias.find('_'));

        for (const auto& t : mealy.transitions) 
        {
            if (t.fromState == origFrom) 
            {
                std::string targetAlias = t.toState + "_" + t.output;
                moore.transitions.push_back({sourceAlias, targetAlias, t.input});
            }
        }
    }

    return moore;
}

MealyMachine ConvertMooreToMealy(const MooreMachine& moore) 
{
    MealyMachine mealy;
    mealy.startState = moore.startState;

    for (const auto& t : moore.transitions) 
    {
        std::string output = "";
        auto it = moore.states.find(t.toState);
        if (it != moore.states.end()) 
        {
            output = it->second.output;
        }

        mealy.transitions.push_back({t.fromState, t.toState, t.input, output});
    }

    return mealy;
}

struct AutomatonConverter 
{
    Automaton operator()(const MealyMachine& mealy) const 
    {
        return ConvertMealyToMoore(mealy);
    }

    Automaton operator()(const MooreMachine& moore) const 
    {
        return ConvertMooreToMealy(moore);
    }
};

struct AutomatonWriter 
{
    std::ostream& outputStream;

    void operator()(const MealyMachine& machine) const 
    {
        outputStream << "type: mealy\n";
        outputStream << "start: " << machine.startState << "\n\n";
        outputStream << "transitions:\n";
        for (const auto& t : machine.transitions) 
        {
            outputStream << t.fromState << " " << t.toState << " " 
                         << t.input << " / " << t.output << "\n";
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
        for (const auto& t : machine.transitions) 
        {
            outputStream << t.fromState << " " << t.toState << " " << t.input << "\n";
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
    Automaton convertedAutomaton = std::visit(AutomatonConverter{}, originalAutomaton);

    std::string outputFilePath;
    std::cout << "Введите имя файла для записи результата: ";
    std::cin >> outputFilePath;

    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) 
    {
        std::cerr << "Ошибка: Не удалось открыть файл для записи \"" << outputFilePath << "\"\n";
    }

    std::visit(AutomatonWriter{outputFile}, convertedAutomaton);
    std::cout << "Автомат успешно сохранен в файл \"" << outputFilePath << "\".\n";
}