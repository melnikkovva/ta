#!/bin/bash
# Использование: ./run_all.sh regexes.txt
# Для каждой строки файла:
#   regular -> determinator -> minimizer
# После КАЖДОГО этапа результат отдаётся в machine и сохраняется картинка.
#
# Результаты запуска N лежат в $RESULTS_DIR/N/:
#   regex.txt, nfa.txt, dfa.txt, min.txt   - файлы этапов
#   nfa.png,   dfa.png, min.png            - картинки этапов
#   *.log                                  - вывод программ
# Копии картинок: $RESULTS_DIR/N_nfa.png, N_dfa.png, N_min.png

REGULAR="$HOME/Desktop/ta/lab4/build/regular"
DETERMINATOR="$HOME/Desktop/ta/lab3/build/determinator"
MINIMIZER="$HOME/Desktop/ta/lab1_1/build/minimizer"

RUN_DIR="/home/maria-melnikova/Downloads/Telegram Desktop/run"
MACHINE="$RUN_DIR/machine"
MACHINE_INPUT="$RUN_DIR/dist/input.txt"
PNG="$RUN_DIR/Moore_Machine.png"

RESULTS_DIR="$HOME/Desktop/ta/results"
STAGE_TIMEOUT=90      # сек на каждую программу
MACHINE_TIMEOUT=180   # сек на ожидание картинки

if [ $# -ne 1 ] || [ ! -f "$1" ]; then
    echo "Использование: $0 regexes.txt"
    exit 1
fi

mkdir -p "$RESULTS_DIR"
mkdir -p "$(dirname "$MACHINE_INPUT")"

# run_stage <exe> <лог> <ввод1> [ввод2 ...]
# Запускает программу из её папки и подаёт на stdin строки ввода (как при ручном вводе).
run_stage() {
    local exe="$1" log="$2"
    shift 2
    ( cd "$(dirname "$exe")" && printf '%s\n' "$@" | timeout "$STAGE_TIMEOUT" "$exe" ) >"$log" 2>&1
}

# Ждёт появления PNG и пока его размер перестанет меняться
wait_for_png() {
    local pid="$1" last=-1 size i
    for (( i = 0; i < MACHINE_TIMEOUT * 2; i++ )); do
        if [ -f "$PNG" ]; then
            size=$(stat -c %s "$PNG")
            if [ "$size" -gt 0 ] && [ "$size" -eq "$last" ]; then
                return 0
            fi
            last=$size
        elif ! kill -0 "$pid" 2>/dev/null; then
            return 1   # процесс завершился, а картинки нет
        fi
        sleep 0.5
    done
    return 1
}

# render <файл этапа> <папка запуска> <имя этапа> <номер запуска>
# Кладёт файл в dist/input.txt, запускает machine, переносит картинку.
render() {
    local src="$1" W="$2" name="$3" n="$4" pid

    cp "$src" "$MACHINE_INPUT"
    rm -f "$PNG"                         # чтобы не взять картинку от прошлого запуска
    ( cd "$RUN_DIR" && exec "$MACHINE" ) >"$W/machine_$name.log" 2>&1 </dev/null &
    pid=$!

    local ok=1
    if wait_for_png "$pid"; then
        mv "$PNG" "$W/$name.png"
        cp "$W/$name.png" "$RESULTS_DIR/${n}_$name.png"
        echo "  $name: OK -> $RESULTS_DIR/${n}_$name.png"
        ok=0
    else
        echo "  $name: ОШИБКА, картинка не появилась (см. $W/machine_$name.log)"
    fi

    # если machine всё ещё работает - завершаем
    if kill -0 "$pid" 2>/dev/null; then
        kill "$pid" 2>/dev/null
        sleep 0.5
        kill -9 "$pid" 2>/dev/null
    fi
    wait "$pid" 2>/dev/null
    return $ok
}

n=0
while IFS= read -r line || [ -n "$line" ]; do
    line="${line%$'\r'}"                 # на случай Windows-переводов строк
    [ -z "${line//[[:space:]]/}" ] && continue
    n=$((n + 1))
    echo "[$n] $line"

    W="$RESULTS_DIR/$n"
    rm -rf "$W"
    mkdir -p "$W"
    printf '%s\n' "$line" > "$W/regex.txt"

    # 1. regular: строка 1 - регулярка, строка 2 - файл результата
    run_stage "$REGULAR" "$W/regular.log" "$line" "$W/nfa.txt"
    if [ ! -s "$W/nfa.txt" ]; then echo "  ОШИБКА: regular (см. $W/regular.log)"; continue; fi
    render "$W/nfa.txt" "$W" nfa "$n"

    # 2. determinator: файл для чтения, файл для записи
    run_stage "$DETERMINATOR" "$W/determinator.log" "$W/nfa.txt" "$W/dfa.txt"
    if [ ! -s "$W/dfa.txt" ]; then echo "  ОШИБКА: determinator (см. $W/determinator.log)"; continue; fi
    render "$W/dfa.txt" "$W" dfa "$n"

    # 3. minimizer: файл для чтения, файл для записи
    run_stage "$MINIMIZER" "$W/minimizer.log" "$W/dfa.txt" "$W/min.txt"
    if [ ! -s "$W/min.txt" ]; then echo "  ОШИБКА: minimizer (см. $W/minimizer.log)"; continue; fi
    render "$W/min.txt" "$W" min "$n"
done < "$1"