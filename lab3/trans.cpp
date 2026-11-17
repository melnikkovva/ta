#include <bits/stdc++.h>
using namespace std;

static string trim(const string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static vector<string> tokens(const string& s) {
    istringstream is(s);
    vector<string> r;
    string t;
    while (is >> t) r.push_back(t);
    return r;
}

static bool isEps(const string& t) { return t == "eps" || t == "ε"; }

int main(int argc, char** argv) {
    string inPath  = (argc > 1) ? argv[1] : "input.txt";
    string outPath = (argc > 2) ? argv[2] : "";

    ifstream fin(inPath);
    if (!fin) {
        cerr << "Не удалось открыть входной файл: " << inPath << "\n";
        return 1;
    }
    istream& in = fin;

    ofstream fout;
    if (!outPath.empty()) {
        fout.open(outPath);
        if (!fout) {
            cerr << "Не удалось открыть выходной файл: " << outPath << "\n";
            return 1;
        }
    }
    ostream& out = outPath.empty() ? cout : fout;

    string type = "right", startName;
    vector<pair<string, vector<string>>> rules;

    string line;
    while (getline(in, line)) {
        size_t c = line.find('#');
        if (c != string::npos) line = line.substr(0, c);
        line = trim(line);
        if (line.empty() || line == "rules:") continue;

        if (line.rfind("type:", 0) == 0) { type = trim(line.substr(5)); continue; }
        if (line.rfind("start:", 0) == 0) { startName = trim(line.substr(6)); continue; }

        size_t arrow = line.find("->");
        if (arrow == string::npos) continue;
        string lhs = trim(line.substr(0, arrow));
        string rhs = line.substr(arrow + 2);
        stringstream ss(rhs);
        string alt;
        while (getline(ss, alt, '|')) rules.push_back({lhs, tokens(alt)});
    }

    if (type != "left" && type != "right") {
        cerr << "type должен быть left или right\n";
        return 1;
    }
    bool left = (type == "left");

    map<string, int> nt;
    vector<string> names;
    for (auto& r : rules)
        if (!nt.count(r.first)) { nt[r.first] = names.size(); names.push_back(r.first); }
    if (!nt.count(startName)) {
        cerr << "Стартовый символ не найден среди нетерминалов\n";
        return 1;
    }
    int n = names.size();
    int EXTRA = n;  
    names.push_back(left ? "I" : "F");
    int total = n + 1;

    vector<map<string, set<int>>> delta(total);
    vector<set<int>> eps(total);
    vector<bool> acc(total, false);

    for (auto& r : rules) {
        int A = nt[r.first];
        const auto& b = r.second;
        if (b.size() == 1 && isEps(b[0])) {
            if (!left) acc[A] = true;          
            else eps[EXTRA].insert(A);        
        } else if (b.size() == 1) {
            if (nt.count(b[0])) {
                cerr << "Недопустимое правило (цепное): " << r.first << "\n";
                return 1;
            }
            if (!left) delta[A][b[0]].insert(EXTRA);  
            else       delta[EXTRA][b[0]].insert(A); 
        } else if (b.size() == 2) {
            if (!left) {
                if (nt.count(b[0]) || !nt.count(b[1])) {
                    cerr << "Правило не праволинейное: " << r.first << "\n";
                    return 1;
                }
                delta[A][b[0]].insert(nt[b[1]]);       
            } else {
                if (!nt.count(b[0]) || nt.count(b[1])) {
                    cerr << "Правило не леволинейное: " << r.first << "\n";
                    return 1;
                }
                delta[nt[b[0]]][b[1]].insert(A);    
            }
        } else {
            cerr << "Недопустимая длина правой части у " << r.first << "\n";
            return 1;
        }
    }

    int nfaStart;
    if (!left) { nfaStart = nt[startName]; acc[EXTRA] = true; }
    else       { nfaStart = EXTRA;         acc[nt[startName]] = true; }

    vector<int> order = {nfaStart};
    for (int i = 0; i < total; i++) if (i != nfaStart) order.push_back(i);
    vector<int> pos(total);
    for (int i = 0; i < total; i++) pos[order[i]] = i;

    out << "type: moore\n";
    out << "start: S0\n\n";
    out << "states:\n";
    for (int i = 0; i < total; i++) {
        int u = order[i];
        out << "S" << i << " | " << names[u] << " / " << (acc[u] ? 1 : 0) << "\n";
    }
    out << "\ntransitions:\n";
    for (int i = 0; i < total; i++) {
        int u = order[i];
        for (auto& [a, to] : delta[u])
            for (int v : to)
                out << "S" << i << " S" << pos[v] << " " << a << "\n";
        for (int v : eps[u])
            out << "S" << i << " S" << pos[v] << " eps\n";
    }

    return 0;
}