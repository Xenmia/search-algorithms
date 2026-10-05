#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>  // pentru sortare
#include <functional>
#include <sstream>
#include <cmath>      // pentru log2

using namespace std;

// Contor global pentru numarul de comparatii efectuate în faza de cautare
long long comparisons = 0;

// Structura pentru a retine un produs din fisierul CSV
struct Product {
    string id;
    string name;
    double price;
};

// Functie pentru citirea produselor din fisier CSV
vector<Product> loadProducts(const string& filename) {
    vector<Product> products;   // pornim cu o lista goala
    ifstream file(filename);    // încercam sa deschidem fisierul


    if (!file.is_open()) {
        cout << "ERROR: Nu s-a putut deschide fisierul " << filename << "\n";
        return products;  // returnam lista goala, speram ca programul se descurca
    }

    string line;
    // Sarim peste prima linie (header: "id,name,price")
    getline(file, line);

    // Citim fisierul linie cu linie pana la final
    while (getline(file, line)) {
        stringstream ss(line);  // transformam linia în stream pentru a o separa cu getline
        string id, name, price_str;

        // Separare dupa virgula (format CSV)
        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, price_str, ',');

        Product p;
        p.id = id;
        p.name = name;
        p.price = stod(price_str);

        products.push_back(p);  // adaugam în vector
    }

    file.close();  // închidem fisierul (buna practica)
    return products;
}

// ---------- FUNCTII DE CAUTARE ----------

// 1. Cautare secventiala – parcurgem toate produsele pana gasim numele
const Product* sequentialSearch(const vector<Product>& products, const string& name) {
    for (const Product& p : products) {
        comparisons++;               // o comparatie: p.name == name ?
        if (p.name == name) {
            return &p;  // gasit, returnam adresa
        }
    }
    return nullptr;  // nu a fost gasit
}

// 2. Cautare binara – functioneaza pe un vector sortat dupa nume
const Product* binarySearch(const vector<Product>& products, const string& name) {
    if (products.empty()) return nullptr;

    // Cream un vector de pointeri pentru a nu copia produsele
    vector<const Product*> arr;
    arr.reserve(products.size());

    for (const Product& p : products) {
        arr.push_back(&p);
    }

    // Sortam dupa nume
    sort(arr.begin(), arr.end(), [](const Product* a, const Product* b) {
        return a->name < b->name;
    });

    int left = 0;
    int right = arr.size() - 1;

    // Cautare binara clasica
    while (left <= right) {

        int mid = left + (right - left) / 2;

        // comparatie ==
        comparisons++;
        if (arr[mid]->name == name) {
            return arr[mid];
        }

		// comparatie <
        if (arr[mid]->name < name) {
            left = mid + 1;
        }
        else {
            right = mid - 1;
        }
    }

    return nullptr;
}

// 3. Cautare Fibonacci
//    Functioneaza pe un vector sortat (dupa nume) si foloseste numere Fibonacci.
const Product* fibonacciSearch(const vector<Product>& products, const string& name) {
    if (products.empty()) return nullptr;

    // Avem nevoie de produse sortate dupa nume.
    // În loc sa copiez produsele, creez un vector de pointeri.
    vector<const Product*> arr;
    arr.reserve(products.size()); // rezervam memorie pentru eficienta
    for (const Product& p : products) {
        arr.push_back(&p);
    }

    // Sortam pointerii dupa nume folosind o functie lambda
    // Sintaxa lambda înca e confuza, dar functioneaza
    sort(arr.begin(), arr.end(), [](const Product* a, const Product* b) {
        return a->name < b->name;
        });

    int n = arr.size();  // numarul de elemente

    // Initializare numere Fibonacci
    int fibM_minus_2 = 0;          // Fibonacci (m-2)
    int fibM_minus_1 = 1;          // Fibonacci (m-1)
    int fibM = fibM_minus_2 + fibM_minus_1;  // Fibonacci m

    // Gasim cel mai mic numar Fibonacci >= n
    while (fibM < n) {
        fibM_minus_2 = fibM_minus_1;
        fibM_minus_1 = fibM;
        fibM = fibM_minus_2 + fibM_minus_1;
    }

    int offset = -1;  // pozitia de început (marcheaza zona eliminata)

    // Bucla principala
    while (fibM > 1) {
        // Alegem un index de comparare
        int i = min(offset + fibM_minus_2, n - 1);

        // Daca numele este mai mic decat ce cautam
        if (arr[i]->name < name) {
            fibM = fibM_minus_1;
            fibM_minus_1 = fibM_minus_2;
            fibM_minus_2 = fibM - fibM_minus_1;
            offset = i;
        }
        // Daca numele este mai mare
        else if (arr[i]->name > name) {
            fibM = fibM_minus_2;
            fibM_minus_1 = fibM_minus_1 - fibM_minus_2;
            fibM_minus_2 = fibM - fibM_minus_1;
        }
        // Gasit
        else {
            comparisons++;             // < (fals) si > (fals) – deci egal
            return arr[i];
        }
        comparisons++;
    }

    // Ultima verificare posibila
    if (fibM_minus_1 && offset + 1 < n && arr[offset + 1]->name == name) {
        comparisons++;               // comparatia finala
        return arr[offset + 1];
    }

    return nullptr;
}

// 4. Cautare prin interpolare – încearca sa „ghiceasca” pozitia (ca în dictionar)
const Product* interpolationSearch(const vector<Product>& products, const string& name) {
    if (products.empty()) return nullptr;

    // Construim din nou un vector sortat de pointeri
    vector<const Product*> arr;
    arr.reserve(products.size());
    for (const Product& p : products) {
        arr.push_back(&p);
    }

    sort(arr.begin(), arr.end(), [](const Product* a, const Product* b) {
        return a->name < b->name;
        });

    // Transformam numele în numar pentru interpolare
    auto nameToKey = [](const string& s) -> unsigned long long {
        unsigned long long key = 0;
        int chars = s.size() < 8 ? s.size() : 8;
        for (int i = 0; i < chars; i++) {
            key = (key << 8) + (unsigned char)s[i];  // deplasare cu 8 biti
        }
        return key;
        };

    int n = arr.size();

    // Precalculam cheile
    vector<unsigned long long> keys;
    keys.reserve(n);

    for (const Product* p : arr) {
        keys.push_back(nameToKey(p->name));
    }

    unsigned long long targetKey = nameToKey(name);

    int low = 0;
    int high = n - 1;

    while (low <= high && targetKey >= keys[low] && targetKey <= keys[high]) {

        // Daca toate valorile sunt egale, cautam liniar
        if (keys[low] == keys[high]) {
            for (int i = low; i <= high; i++) {
                comparisons++;               // comparatie nume
                if (arr[i]->name == name) return arr[i];
            }
            break;
        }

        // Formula de interpolare
        unsigned long long numerator = (targetKey - keys[low]) * (high - low);
        unsigned long long denominator = keys[high] - keys[low];
        int pos = low + (int)(numerator / denominator);

        // Asiguram limitele
        if (pos < low) pos = low;
        if (pos > high) pos = high;

        comparisons++;                       // comparatie nume
        if (arr[pos]->name == name) return arr[pos];

        if (arr[pos]->name < name) {
            low = pos + 1;
        }
        else {
            high = pos - 1;
        }
    }

    // Ultima cautare liniara (desi în principiu nu ar mai fi necesara)
    for (int i = low; i <= high; i++) {
        if (i >= 0 && i < n) {
            comparisons++;                   // comparatie nume
            if (arr[i]->name == name) return arr[i];
        }
    }

    return nullptr;
}

// 5. Cautare cu hash – tabel hash simplu cu dimensiune fixa
const Product* hashSearch(const vector<Product>& products, const string& name) {
    if (products.empty()) return nullptr;

    // Dimensiunea tabelului (numar prim)
    const int TABLE_SIZE = 101;

    // Fiecare pozitie este un vector (chaining)
    vector<vector<const Product*>> table(TABLE_SIZE);

    // Functie hash simpla: suma codurilor ASCII
    auto hash = [](const string& s) -> int {
        int sum = 0;
        for (char c : s) {
            sum += c;
        }
        return sum % TABLE_SIZE;
        };

    // Construim tabelul
    for (const Product& p : products) {
        int index = hash(p.name);
        table[index].push_back(&p);
    }

    // Cautare
    int index = hash(name);

    for (const Product* p : table[index]) {
        comparisons++;               // comparatie nume
        if (p->name == name) {
            return p;
        }
    }

    return nullptr;
}

// 6. Cautare în arbore - arbore binar folosind pointeri simpli
const Product* treeSearch(const vector<Product>& products, const string& name) {
    if (products.empty()) return nullptr;

    // Fiecare nod contine un pointer catre produs si doi pointeri catre copii
    struct Node {
        const Product* p;
        Node* left;
        Node* right;
        Node(const Product* prod) : p(prod), left(nullptr), right(nullptr) {}
    };

    Node* root = nullptr;   // radacina arborelui

    // Functie recursiva pentru inserare – construim arborele dupa nume
    function<void(Node*&, const Product*)> insert = [&](Node*& node, const Product* prod) {
        if (node == nullptr) {
            node = new Node(prod);   // cream un nou nod daca am ajuns la o pozitie libera
        }
        else if (prod->name < node->p->name) {
            insert(node->left, prod);   // stanga daca numele este mai mic
        }
        else {
            insert(node->right, prod);  // dreapta daca numele este mai mare sau egal (pentru simplitate, nu gestionam duplicatele)
        }
        };

    // Construim arborele cu toate produsele
    for (const Product& prod : products) {
        insert(root, &prod);
    }

    // Functie recursiva pentru cautare în arbore (se putea si iterativ, dar recursiv e mai clar)
    function<const Product* (Node*, const string&)> search = [&](Node* node, const string& target) -> const Product* {
        comparisons++;
    	if (node == nullptr) return nullptr;
        
        if (target == node->p->name) return node->p;
        
        if (target < node->p->name) return search(node->left, target);
        return search(node->right, target);
        };

    const Product* result = search(root, name);

    // Eliberam memoria alocata pentru noduri
    function<void(Node*)> freeTree = [&](Node* node) {
        if (node) {
            freeTree(node->left);
            freeTree(node->right);
            delete node;
        }
        };
    freeTree(root);

    return result;
}

// FUNCTII AJUTATOARE PENTRU LUNGIMEA TEORETICA 
double sequentialTheoretical(int n) {
    return (n + 1) / 2.0;
}

double binaryTheoretical(int n) {
    if (n <= 0) return 0;
    return log2(n);   // O(log n)
}

double fibonacciTheoretical(int n) {
    if (n <= 0) return 0;
    return log2(n);  // aproximare rezonabila
}

double interpolationTheoretical(int n) {
    if (n <= 1) return 1;
    return log2(log2(n));  // aproximare
}

double hashTheoretical(int n, int tableSize = 101) {
    if (n == 0) return 0;
    double alpha = static_cast<double>(n) / tableSize;
    return 1.0 + alpha / 2.0;  // medie pentru chaining
}

double treeTheoretical(int n) {
    if (n <= 0) return 0;
    return 1.39 * log2(n);  // aproximare pentru arbore binar construit aleator
}

// ---------- MENU ----------
void showMenu() {
    cout << "\n========== MENU ==========" << endl;
    cout << "1. Cautare secventiala" << endl;
    cout << "2. Cautare binara" << endl;
    cout << "3. Cautare Fibonacci" << endl;
    cout << "4. Cautare prin interpolare" << endl;
    cout << "5. Cautare hash" << endl;
    cout << "6. Cautare in arbore" << endl;
    cout << "0. Iesire" << endl;
    cout << "Alege o optiune: ";
}

// ---------- MAIN ----------
int main() {

    // Încarcam produsele
    vector<Product> products = loadProducts("products.csv");

    int n = products.size();
    cout << "Au fost incarcate " << n << " produse." << endl;

    int choice;

    do {
        showMenu();
        cin >> choice;

        string name;             // numele produsului
        const Product* result;   // rezultatul cautarii

        switch (choice) {

        case 1:
            cout << "Introdu numele produsului: ";
            getline(cin >> ws, name);

            comparisons = 0;
            result = sequentialSearch(products, name);

            cout << "\n--- REZULTAT ---\n";
            if (result != nullptr) {
                cout << "GASIT: " << result->id << " | "
                    << result->name << " | $" << result->price << endl;
            }
            else {
                cout << "NU A FOST GASIT" << endl;
            }
            cout << "Lungimea medie teoretica (aprox): " << sequentialTheoretical(n) << endl;
            cout << "Lungimea practica (comparatii): " << comparisons << endl;
            break;

        case 2:
            cout << "Introdu numele produsului: ";
            getline(cin >> ws, name);

            comparisons = 0;
            result = binarySearch(products, name);

            cout << "\n--- REZULTAT ---\n";
            if (result != nullptr) {
                cout << "GASIT: " << result->id << " | "
                    << result->name << " | $" << result->price << endl;
            }
            else {
                cout << "NU A FOST GASIT" << endl;
            }

            cout << "Lungimea medie teoretica (aprox): "
                << binaryTheoretical(n) << endl;

            cout << "Lungimea practica (comparatii): "
                << comparisons << endl;

            break;

        case 3:
            cout << "Introdu numele produsului: ";
            getline(cin >> ws, name);

            comparisons = 0;
            result = fibonacciSearch(products, name);

            cout << "\n--- REZULTAT ---\n";
            if (result != nullptr) {
                cout << "GASIT: " << result->id << " | "
                    << result->name << " | $" << result->price << endl;
            }
            else {
                cout << "NU A FOST GASIT" << endl;
            }
            cout << "Lungimea medie teoretica (aprox): " << fibonacciTheoretical(n) << endl;
            cout << "Lungimea practica (comparatii): " << comparisons << endl;
            break;

        case 4:
            cout << "Introdu numele produsului: ";
            getline(cin >> ws, name);

            comparisons = 0;
            result = interpolationSearch(products, name);

            cout << "\n--- REZULTAT ---\n";
            if (result != nullptr) {
                cout << "GASIT: " << result->id << " | "
                    << result->name << " | $" << result->price << endl;
            }
            else {
                cout << "NU A FOST GASIT" << endl;
            }
            cout << "Lungimea medie teoretica (aprox): " << interpolationTheoretical(n) << endl;
            cout << "Lungimea practica (comparatii): " << comparisons << endl;
            break;

        case 5:
            cout << "Introdu numele produsului: ";
            getline(cin >> ws, name);

            comparisons = 0;
            result = hashSearch(products, name);

            cout << "\n--- REZULTAT ---\n";
            if (result != nullptr) {
                cout << "GASIT: " << result->id << " | "
                    << result->name << " | $" << result->price << endl;
            }
            else {
                cout << "NU A FOST GASIT" << endl;
            }
            cout << "Lungimea medie teoretica (aprox): " << hashTheoretical(n) << endl;
            cout << "Lungimea practica (comparatii): " << comparisons << endl;
            break;

        case 6:
            cout << "Introdu numele produsului: ";
            getline(cin >> ws, name);

            comparisons = 0;
            result = treeSearch(products, name);

            cout << "\n--- REZULTAT ---\n";
            if (result != nullptr) {
                cout << "GASIT: " << result->id << " | "
                    << result->name << " | $" << result->price << endl;
            }
            else {
                cout << "NU A FOST GASIT" << endl;
            }
            cout << "Lungimea medie teoretica (aprox): " << treeTheoretical(n) << endl;
            cout << "Lungimea practica (comparatii): " << comparisons << endl;
            break;

        case 0:
            cout << "Program inchis. La revedere!" << endl;
            break;

        default:
            cout << "Optiune invalida – incearca din nou." << endl;
        }

    } while (choice != 0);

    return 0;
}