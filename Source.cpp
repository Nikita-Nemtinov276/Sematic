#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype>

using namespace std;

struct TreeNode {
    string value;
    string type; // Type of the token, e.g., [WordsKey], [Id], etc.
    vector<TreeNode*> children;

    TreeNode(const string& val, const string& typ = "") : value(val), type(typ) {}

    ~TreeNode() {
        for (TreeNode* child : children) {
            delete child;
        }
    }

    void print(int depth = 0) const {
        for (int i = 0; i < depth; ++i) {
            cout << "  ";
        }
        cout << value;
        if (!type.empty()) {
            cout << "  [" << type << "]";
        }
        cout << endl;
        for (const TreeNode* child : children) {
            child->print(depth + 1);
        }
    }
};

class Parser {
public:
    explicit Parser(const string& input) : input(input), pos(0), root(nullptr) {}

    ~Parser() {
        delete root;
    }

    void parse() {
        root = parseProcedure();
        if (pos < input.size()) {
            error("Unexpected input at the end");
        }
    }

    void printParseTree() const {
        if (root) {
            root->print();
        }
        else {
            cout << "No parse tree available." << endl;
        }
    }

private:
    string input;
    size_t pos;
    TreeNode* root;

    void skipWhitespace() {
        while (pos < input.size() && isspace(input[pos])) {
            pos++;
        }
    }

    void match(const string& token) {
        skipWhitespace();
        if (input.substr(pos, token.size()) == token) {
            pos += token.size();
        }
        else {
            error("Expected '" + token + "'");
        }
    }

    string parseId() {
        skipWhitespace();
        size_t start = pos;

        // Считываем символы до первого разделителя или пробела
        while (pos < input.size() && !isspace(input[pos]) && input[pos] != ';' &&
            input[pos] != ',' && input[pos] != ':' && input[pos] != '+' &&
            input[pos] != '-' && input[pos] != '=' && input[pos] != '(' &&
            input[pos] != ')') {
            pos++;
        }

        string lexeme = input.substr(start, pos - start);

        // Проверяем, что лексема состоит только из букв
        for (char ch : lexeme) {
            if (!isalpha(ch)) {
                error("Invalid identifier: '" + lexeme + "' must consist of letters only");
            }
        }

        return lexeme;
    }





    string parseNumber() {
        skipWhitespace();
        size_t start = pos;

        // Считываем лексему до первого разделителя или пробела
        while (pos < input.size() && !isspace(input[pos]) && input[pos] != ';' &&
            input[pos] != ',' && input[pos] != ':' && input[pos] != '+' &&
            input[pos] != '-' && input[pos] != '=' && input[pos] != '(' &&
            input[pos] != ')') {
            pos++;
        }

        string lexeme = input.substr(start, pos - start);

        // Проверяем, состоит ли лексема только из цифр
        for (char ch : lexeme) {
            if (!isdigit(ch)) {
                error("Invalid number: '" + lexeme + "' contains invalid characters");
            }
        }

        return lexeme;
    }



    string parseStringConst() {
        skipWhitespace();
        if (pos < input.size() && input[pos] == '"') {
            size_t start = ++pos; // Пропускаем начальную кавычку
            string result;

            while (pos < input.size() && input[pos] != '"') {
                char ch = input[pos];
                // Проверяем, является ли символ допустимым (цифры или буквы)
                if (isdigit(ch) || isalpha(ch)) {
                    result += ch;
                    pos++;
                }
                else {
                    error("Invalid character '" + string(1, ch) + "' in string constant");
                }
            }

            // Проверяем, закрывается ли строковая константа
            if (pos < input.size() && input[pos] == '"') {
                pos++; // Пропускаем закрывающую кавычку
                return result;
            }
            else {
                error("Unterminated string constant");
            }
        }
        else {
            error("Expected string constant");
        }
        return "";
    }


    void error(const string& message) {
        cerr << "Error at position " << pos << ": " << message << endl;
        exit(EXIT_FAILURE);
    }

    TreeNode* parseProcedure() {
        TreeNode* node = new TreeNode("Program", "Program");
        match("procedure");
        node->children.push_back(new TreeNode("procedure", "WordsKey"));
        node->children.push_back(new TreeNode(parseId(), "Id"));
        match(";");
        match("begin");
        node->children.push_back(new TreeNode("begin", "WordsKey"));
        node->children.push_back(parseDescriptions());
        node->children.push_back(parseOperators());
        match("end");
        node->children.push_back(new TreeNode("end", "WordsKey"));
        return node;
    }

    TreeNode* parseDescriptions() {
        TreeNode* node = new TreeNode("Descriptions");
        skipWhitespace();

        if (input.substr(pos, 3) == "var") {
            match("var");
            node->children.push_back(new TreeNode("var", "WordsKey"));
            node->children.push_back(parseDescrList());  // Обрабатываем список переменных
        }
        return node;
    }

    TreeNode* parseDescrList() {
        TreeNode* node = new TreeNode("DescrList");

        // Парсим первый элемент
        node->children.push_back(parseDescr());

        skipWhitespace();

        // Проверяем, есть ли еще объявления после var (например, var y: char;)
        while (pos < input.size() && input.substr(pos, 3) == "var") {
            match("var");  // Пропускаем "var"
            node->children.push_back(new TreeNode("var", "WordsKey"));
            node->children.push_back(parseDescr());  // Парсим еще одно описание переменной
        }

        return node;
    }

    TreeNode* parseDescr() {
        TreeNode* node = new TreeNode("Descr");

        // Парсим список переменных
        node->children.push_back(parseVarList());

        // Пропускаем пробелы
        skipWhitespace();

        // Проверяем, что после имени переменной идет двоеточие ":"
        if (input[pos] == ':') {
            match(":"); // Ожидаем и потребляем двоеточие
            node->children.push_back(new TreeNode(":", "Symbols_of_Separating"));
        }
        else {
            cerr << "Unexpected character before ':' at position " << pos << ": " << input[pos] << endl;
            error("Expected ':' after variable declaration");
        }

        // Парсим тип
        node->children.push_back(new TreeNode(parseType(), "WordsKey"));

        // Ожидаем точку с запятой
        match(";");
        node->children.push_back(new TreeNode(";", "Symbols_of_Separating"));

        return node;
    }




    TreeNode* parseVarList() {
        TreeNode* node = new TreeNode("VarList");
        node->children.push_back(new TreeNode(parseId(), "Id"));
        skipWhitespace();
        while (pos < input.size() && input[pos] == ',') {
            match(",");
            node->children.push_back(new TreeNode(",", "Symbols_of_Separating"));
            node->children.push_back(new TreeNode(parseId(), "Id"));
        }
        return node;
    }

    string parseType() {
        skipWhitespace();
        if (input.substr(pos, 7) == "integer") {
            match("integer");
            return "integer";
        }
        else if (input.substr(pos, 4) == "char") {
            match("char");
            return "char";
        }
        else {
            error("Expected type 'integer' or 'char'");
            return "";
        }
    }

    TreeNode* parseOperators() {
        TreeNode* node = new TreeNode("Operators");
        do {
            node->children.push_back(parseOp());
            skipWhitespace();
        } while (pos < input.size() && input.substr(pos, 3) != "end");
        return node;
    }

    TreeNode* parseOp() {
        TreeNode* node = new TreeNode("Op");
        node->children.push_back(new TreeNode(parseId(), "Id"));
        match(":=");
        node->children.push_back(new TreeNode(":=", "Symbols_of_Operation"));
        skipWhitespace();

        // Проверка для строковых и числовых выражений
        if (input[pos] == '"') {
            node->children.push_back(new TreeNode(parseStringConst(), "Const"));
        }
        else if (isdigit(input[pos])) {
            node->children.push_back(new TreeNode(parseNumber(), "Const"));
        }
        else {
            node->children.push_back(parseNumExpr());
        }

        skipWhitespace();
        // Обработка арифметических операций
        if (pos < input.size() && (input[pos] == '+' || input[pos] == '-')) {
            node->children.push_back(new TreeNode(string(1, input[pos]), "Symbols_of_Operation"));
            pos++;
            skipWhitespace();
            if (input[pos] == '"') {
                node->children.push_back(new TreeNode(parseStringConst(), "Const"));
            }
            else if (isdigit(input[pos])) {
                node->children.push_back(new TreeNode(parseNumber(), "Const"));
            }
            else {
                node->children.push_back(parseNumExpr());
            }
        }

        match(";");
        node->children.push_back(new TreeNode(";", "Symbols_of_Separating"));
        return node;
    }



    TreeNode* parseNumExpr() {
        TreeNode* node = new TreeNode("Expr");
        node->children.push_back(parseSimpleNumExpr());
        skipWhitespace();
        while (pos < input.size() && (input[pos] == '+' || input[pos] == '-')) {
            node->children.push_back(new TreeNode(string(1, input[pos]), "Symbols_of_Operation"));
            pos++;
            node->children.push_back(parseSimpleNumExpr());
        }
        return node;
    }

    TreeNode* parseSimpleNumExpr() {
        TreeNode* node = new TreeNode("SimpleExpr");
        skipWhitespace();
        if (isalpha(input[pos])) {
            node->children.push_back(new TreeNode(parseId(), "Id"));
        }
        else if (isdigit(input[pos])) {
            node->children.push_back(new TreeNode(parseNumber(), "Const"));
        }
        else if (input[pos] == '(') {
            match("(");
            node->children.push_back(new TreeNode("(", "Symbols_of_Separating"));
            node->children.push_back(parseNumExpr());
            match(")");
            node->children.push_back(new TreeNode(")", "Symbols_of_Separating"));
        }
        else {
            error("Expected simple numerical expression");
        }
        return node;
    }
};

int main() {
    string filename;
    cout << "Enter filename: ";
    cin >> filename;

    ifstream file(filename);
    if (!file) {
        cerr << "Error: Unable to open file " << filename << endl;
        return 1;
    }

    string input((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    Parser parser(input);
    try {
        parser.parse();
        cout << "Parsing successful!" << endl;
        cout << "Parse Tree:" << endl;
        parser.printParseTree();
    }
    catch (const exception& e) {
        cerr << "Parsing failed: " << e.what() << endl;
    }

    return 0;
}
