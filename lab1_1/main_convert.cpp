// Преобразование автомата Мили в автомат Мура и обратно.

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

#include "Utils.h"

// Пара (состояние Мили, выход перехода, которым в него пришли) = одно состояние Мура.
using StateAndOutput = std::pair<std::string, std::string>;

// Выход начального состояния Мура. Он не соответствует ни одному выходу Мили
// и на эквивалентность автоматов не влияет.
const std::string START_OUTPUT = "-";

MooreMachine ConvertMealyToMoore(const MealyMachine& mealy)
{
    MooreMachine moore;

    // Шаг 1. Собираем все пары (в какое состояние ведёт переход, какой у него выход).
    std::set<StateAndOutput> pairs;
    for (const MealyTransition& transition : mealy.transitions)
    {
        pairs.insert({transition.toState, transition.output});
    }

    // Начальное состояние Мура. Если в начальное состояние Мили ведут переходы,
    // берём одну из уже существующих пар (A, x): поведение у них одинаковое,
    // а лишнего состояния не появится. Если переходов в него нет, создаём пару с выходом "-".
    StateAndOutput startPair = {mealy.startState, START_OUTPUT};
    for (const StateAndOutput& pair : pairs)
    {
        if (pair.first == mealy.startState)
        {
            startPair = pair;
            break;
        }
    }
    pairs.insert(startPair);

    // Шаг 2. Каждой паре даём имя q0, q1, q2... и создаём состояние Мура.
    std::map<StateAndOutput, std::string> nameOfPair;
    int number = 0;
    for (const StateAndOutput& pair : pairs)
    {
        std::string alias = "q" + std::to_string(number);
        number++;

        nameOfPair[pair] = alias;
        moore.states[alias] = {alias, pair.first + "_" + pair.second, pair.second};
    }
    moore.startState = nameOfPair[startPair];

    // Шаг 3. Переходы. Состояние Мура (A, x) ведёт туда же, куда и исходное состояние A.
    for (const StateAndOutput& pair : pairs)
    {
        std::string fromName = nameOfPair[pair];

        for (const MealyTransition& transition : mealy.transitions)
        {
            if (transition.fromState != pair.first)
            {
                continue;
            }
            StateAndOutput targetPair = {transition.toState, transition.output};
            std::string toName = nameOfPair[targetPair];
            moore.transitions.push_back({fromName, toName, transition.input});
        }
    }

    return moore;
}

MealyMachine ConvertMooreToMealy(const MooreMachine& moore)
{
    MealyMachine mealy;
    mealy.startState = moore.startState;

    // Выход перехода = выход состояния, в которое переход ведёт.
    for (const MooreTransition& transition : moore.transitions)
    {
        std::string output = moore.states.at(transition.toState).output;
        mealy.transitions.push_back({transition.fromState, transition.toState, transition.input, output});
    }

    return mealy;
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
            result = ConvertMealyToMoore(std::get<MealyMachine>(automaton));
        }
        else
        {
            result = ConvertMooreToMealy(std::get<MooreMachine>(automaton));
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
    catch (const std::exception& error)
    {
        std::cerr << "Ошибка: " << error.what() << "\n";
        return 1;
    }

    return 0;
}