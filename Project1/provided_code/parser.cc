#include "parser.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <string>

using namespace std;

// We define the global data structures declared 'extern' in parser.h:
std::vector<Statement*> executeStatements;
std::unordered_map<std::string,int> symbolTable;
int mem[1000] = {0};
int nextAvailable = 0;

// ---------- Parser Implementation ----------

// syntax_error() and expect() are standard.
void Parser::syntax_error() {
    cout << "SYNTAX ERROR !!!!!&%!!" << endl;
    exit(1);
}

Token Parser::expect(TokenType expected_type) {
    Token token = lexer.GetToken();
    if (token.token_type != expected_type) {
        syntax_error();
    }
    return token;
}

// Constructor
Parser::Parser() {
    for (int i = 0; i < 7; i++) {
        tasks[i] = false;
    }
    nextAvailable = 0;
}

// Debug function
void Parser::ConsumeAllInput() {
    Token token;
    int i = 1;
    token = lexer.peek(i);
    token.Print();
    while (token.token_type != END_OF_FILE) {
        i++;
        token = lexer.peek(i);
        token.Print();
    }
    token = lexer.GetToken();
    token.Print();
    while (token.token_type != END_OF_FILE) {
        token = lexer.GetToken();
        token.Print();
    }
}

// The main parse flow
void Parser::input() {
    program();
    expect(END_OF_FILE);

    // ---------- Check any semantic errors from Task 1 ----------
    if (!duplicateLines.empty()) {
        sort(duplicateLines.begin(), duplicateLines.end());
        cout << "Semantic Error Code 1:";
        for (int ln : duplicateLines) {
            cout << " " << ln;
        }
        cout << endl;
        exit(1);
    }
    if (!invalidMonomialLines.empty()) {
        sort(invalidMonomialLines.begin(), invalidMonomialLines.end());
        cout << "Semantic Error Code 2:";
        for (int ln : invalidMonomialLines) {
            cout << " " << ln;
        }
        cout << endl;
        exit(1);
    }
    if (!undefinedPolyUseLines.empty()) {
        sort(undefinedPolyUseLines.begin(), undefinedPolyUseLines.end());
        cout << "Semantic Error Code 3:";
        for (int ln : undefinedPolyUseLines) {
            cout << " " << ln;
        }
        cout << endl;
        exit(1);
    }
    if (!wrongArgCountLines.empty()) {
        sort(wrongArgCountLines.begin(), wrongArgCountLines.end());
        cout << "Semantic Error Code 4:";
        for (int ln : wrongArgCountLines) {
            cout << " " << ln;
        }
        cout << endl;
        exit(1);
    }

    // ---------- Task 4: detect useless assignments ----------
    if (tasks[4]) {
        DetectUselessAssignments();
    }

    // ---------- Task 5: print polynomial degrees ----------
    if (tasks[5]) {
        // Print each polynomial's name and computed degree
        for (auto &ph : polyHeaders) {
            cout << ph.name << ": " << ph.degree << endl;
        }
        exit(0);
    }

    // (If you had other tasks like 2 or 3, you'd do them here.)
}

// program → tasks_section poly_section execute_section inputs_section
void Parser::program() {
    tasks_section();
    poly_section();
    execute_section();
    inputs_section();
}

// tasks_section → TASKS num_list
void Parser::tasks_section() {
    expect(TASKS);
    tasknum_list();
}

// num_list → NUM | NUM num_list
void Parser::tasknum_list() {
    Token t = expect(NUM);
    int task_num = stoi(t.lexeme);
    if (task_num < 1 || task_num > 6) syntax_error();
    tasks[task_num] = true;

    while (lexer.peek(1).token_type == NUM) {
        t = expect(NUM);
        task_num = stoi(t.lexeme);
        if (task_num < 1 || task_num > 6) syntax_error();
        tasks[task_num] = true;
    }
}

// poly_section → POLY poly_decl_list
void Parser::poly_section() {
    expect(POLY);
    poly_decl_list();
}

// poly_decl_list → poly_decl | poly_decl poly_decl_list
void Parser::poly_decl_list() {
    poly_decl();
    Token t = lexer.peek(1);
    while (t.token_type == ID) {
        poly_decl();
        t = lexer.peek(1);
    }
    if (t.token_type != EXECUTE) syntax_error();
}

// poly_decl → poly_header EQUAL (poly_body) SEMICOLON
void Parser::poly_decl() {
    poly_header();
    expect(EQUAL);

    // Now parse the polynomial body, computing its degree in the process.
    // We'll store that degree in the last poly header we added.
    currentPolyParams = polyHeaders.back().paramNames;
    int d = parsePolyBody();  // parse the polynomial body & compute degree
    polyHeaders.back().degree = d;
    currentPolyParams.clear();

    expect(SEMICOLON);
}

// poly_header → poly_name [ LPAREN id_list RPAREN ]
void Parser::poly_header() {
    Token nameToken = poly_name();  // ID
    PolyHeaderInfo info;
    info.name = nameToken.lexeme;
    info.line_no = nameToken.line_no;
    info.degree = 0;

    // Check for duplicates:
    for (auto &ph : polyHeaders) {
        if (ph.name == info.name) {
            duplicateLines.push_back(info.line_no);
            break;
        }
    }

    declaredPolynomials.insert(info.name);

    // optional param list
    if (lexer.peek(1).token_type == LPAREN) {
        expect(LPAREN);
        info.paramNames = id_list();
        expect(RPAREN);
    } else {
        // default param is "x"
        info.paramNames.push_back("x");
    }

    polyHeaders.push_back(info);
}

// poly_name → ID
Token Parser::poly_name() {
    return expect(ID);
}

// id_list → ID | ID COMMA id_list
vector<string> Parser::id_list() {
    vector<string> names;
    Token t = expect(ID);
    names.push_back(t.lexeme);
    while (lexer.peek(1).token_type == COMMA) {
        expect(COMMA);
        Token t2 = expect(ID);
        names.push_back(t2.lexeme);
    }
    return names;
}

// ===================== Parsing polynomial body & computing degree =====================

// parsePolyBody -> parseTermList (and return that degree)
int Parser::parsePolyBody() {
    return parseTermList();
}

// parseTermList → (Term) ( (PLUS|MINUS) Term )* 
// The degree is the max of degrees among all terms in this list.
int Parser::parseTermList() {
    int d = parseTerm(); // parse first term
    while (true) {
        TokenType tt = lexer.peek(1).token_type;
        if (tt == PLUS || tt == MINUS) {
            lexer.GetToken(); // consume + or -
            int d2 = parseTerm();
            if (d2 > d) d = d2;
        } else {
            break;
        }
    }
    return d;
}

// parseTerm → 
//    (NUM) [ monomial_list ] 
//  | monomial_list
//  The degree is 0 if just a coefficient, otherwise the degree is monomial_list
int Parser::parseTerm() {
    Token t = lexer.peek(1);
    if (t.token_type == NUM) {
        // consume the coefficient
        Token coeffTok = lexer.GetToken(); // e.g. 3
        // if next token is ID or LPAREN => parse monomial_list => degree
        TokenType tt = lexer.peek(1).token_type;
        if (tt == ID || tt == LPAREN) {
            int mdeg = parseMonomialList();
            return mdeg; 
        } else {
            // just a coefficient => degree = 0
            return 0;
        }
    } else if (t.token_type == ID || t.token_type == LPAREN) {
        // no explicit coefficient => interpret coefficient=1
        return parseMonomialList();
    } else {
        syntax_error();
        return 0;
    }
}

// parseMonomialList → monomial ( monomial )*
/** The degree of a monomial_list is the sum of degrees of each monomial. **/
int Parser::parseMonomialList() {
    int total = 0;
    while (true) {
        TokenType tt = lexer.peek(1).token_type;
        if (tt != ID && tt != LPAREN) {
            break;
        }
        int mdeg = parseMonomial();
        total += mdeg;
    }
    return total;
}

// parseMonomial → primary [ exponent ]
// The degree is degree(primary) * exponent (exponent defaults to 1 if not present).
int Parser::parseMonomial() {
    int dprim = parsePrimary(); 
    int d = dprim;
    if (lexer.peek(1).token_type == POWER) {
        lexer.GetToken(); // consume ^
        Token n = expect(NUM);
        int expval = stoi(n.lexeme);
        d = dprim * expval;
    }
    return d;
}

// parsePrimary → ID | LPAREN TermList RPAREN
// If ID => degree=1. 
// If parenthesized => parseTermList => that is the degree.
int Parser::parsePrimary() {
    Token t = lexer.peek(1);
    if (t.token_type == ID) {
        // check if valid param
        Token varTok = lexer.GetToken(); // consume ID
        // semantic check if varTok is in currentPolyParams
        if (!currentPolyParams.empty()) {
            bool found = false;
            for (auto &p : currentPolyParams) {
                if (p == varTok.lexeme) { found = true; break; }
            }
            if (!found) {
                invalidMonomialLines.push_back(varTok.line_no);
            }
        }
        return 1;
    }
    else if (t.token_type == LPAREN) {
        lexer.GetToken(); // consume (
        int d = parseTermList();
        expect(RPAREN);
        return d;
    }
    else {
        syntax_error();
        return 0;
    }
}

// ===================== End polynomial body parse =====================

// ========== EXECUTE SECTION ==========

void Parser::execute_section() {
    expect(EXECUTE);
    statement_list();
}

// statement_list → statement+
void Parser::statement_list() {
    statement();
    while (true) {
        TokenType tt = lexer.peek(1).token_type;
        if (tt == INPUT || tt == OUTPUT || tt == ID) {
            statement();
        } else {
            break;
        }
    }
}

void Parser::statement() {
    Token nextT = lexer.peek(1);
    if (nextT.token_type == INPUT) {
        input_statement();
    } else if (nextT.token_type == OUTPUT) {
        output_statement();
    } else if (nextT.token_type == ID) {
        assign_statement();
    } else {
        syntax_error();
    }
}

void Parser::input_statement() {
    expect(INPUT);
    Token varTok = expect(ID);
    expect(SEMICOLON);

    allocateVariable(varTok.lexeme);

    Statement* s = new Statement;
    s->type = STMT_INPUT;
    s->variable = varTok.lexeme;
    s->line_no = varTok.line_no;
    s->polyEval = nullptr;
    executeStatements.push_back(s);
}

void Parser::output_statement() {
    expect(OUTPUT);
    Token varTok = expect(ID);
    expect(SEMICOLON);

    allocateVariable(varTok.lexeme);

    Statement* s = new Statement;
    s->type = STMT_OUTPUT;
    s->variable = varTok.lexeme;
    s->line_no = varTok.line_no;
    s->polyEval = nullptr;
    executeStatements.push_back(s);
}

void Parser::assign_statement() {
    Token lhsTok = expect(ID);
    allocateVariable(lhsTok.lexeme);
    expect(EQUAL);

    // Build a PolyEval for the RHS
    PolyEval* pe = new PolyEval;
    poly_evaluation(pe);

    expect(SEMICOLON);

    Statement* s = new Statement;
    s->type = STMT_ASSIGN;
    s->variable = lhsTok.lexeme;
    s->line_no = lhsTok.line_no;
    s->polyEval = pe;
    executeStatements.push_back(s);
}

// poly_evaluation -> poly_name ( argument_list )
void Parser::poly_evaluation(PolyEval* pe) {
    Token polyTok = poly_name();
    pe->polyName = polyTok.lexeme;
    if (declaredPolynomials.find(pe->polyName) == declaredPolynomials.end()) {
        undefinedPolyUseLines.push_back(polyTok.line_no);
    }
    expect(LPAREN);
    int c = argument_list(pe);
    expect(RPAREN);

    // check argument count
    int declaredCount = -1;
    for (auto &ph : polyHeaders) {
        if (ph.name == pe->polyName) {
            declaredCount = ph.paramNames.size();
            break;
        }
    }
    if (declaredCount != -1 && c != declaredCount) {
        wrongArgCountLines.push_back(polyTok.line_no);
    }
}

int Parser::argument_list(PolyEval* pe) {
    int count = 0;
    argument(pe);
    count++;
    while (lexer.peek(1).token_type == COMMA) {
        expect(COMMA);
        argument(pe);
        count++;
    }
    return count;
}

// argument -> ID | NUM | nested poly_evaluation
void Parser::argument(PolyEval* pe) {
    Token t = lexer.peek(1);
    if (t.token_type == ID) {
        Token t2 = lexer.peek(2);
        if (t2.token_type == LPAREN) {
            // nested call
            PolyEval* nested = new PolyEval;
            poly_evaluation(nested);
            pe->nestedArgs.push_back(nested);
        } else {
            Token idTok = expect(ID);
            pe->args.push_back(idTok.lexeme);
        }
    }
    else if (t.token_type == NUM) {
        Token numTok = expect(NUM);
        pe->args.push_back(numTok.lexeme);
    }
    else {
        syntax_error();
    }
}

// ========== inputs_section ==========

void Parser::inputs_section() {
    expect(INPUTS);
    inputnum_list();
}

// inputnum_list -> NUM+
void Parser::inputnum_list() {
    expect(NUM);
    while (lexer.peek(1).token_type == NUM) {
        expect(NUM);
    }
}

// ========== Memory and Execution ==========

int Parser::allocateVariable(const string &varName) {
    if (symbolTable.find(varName) == symbolTable.end()) {
        symbolTable[varName] = nextAvailable++;
    }
    return symbolTable[varName];
}

void Parser::execute_program() {
    // Not relevant unless Task 2 is implemented
}

int Parser::evaluate_polynomial(PolyEval* /*polyEval*/) {
    // dummy
    return 42;
}

// ========== Task 4: Useless Assignments ==========

void Parser::DetectUselessAssignments() {
    // We'll do a forward-scan approach as before.
    std::function<void(PolyEval*, vector<int>&)> collectUsedVars;
    collectUsedVars = [&](PolyEval* pe, vector<int>& usedVars) {
        if (!pe) return;
        // simple arguments
        for (auto &a : pe->args) {
            bool isNum = true;
            for (char c : a) {
                if (c < '0' || c > '9') {
                    isNum = false;
                    break;
                }
            }
            if (!isNum) {
                if (symbolTable.find(a) != symbolTable.end()) {
                    usedVars.push_back(symbolTable[a]);
                }
            }
        }
        // nested calls
        for (auto* nested : pe->nestedArgs) {
            collectUsedVars(nested, usedVars);
        }
    };

    vector<int> uselessLines;
    int n = executeStatements.size();
    for (int i = 0; i < n; i++) {
        if (executeStatements[i]->type == STMT_ASSIGN) {
            string var = executeStatements[i]->variable;
            int vloc = symbolTable[var];
            bool used = false;

            // Scan forward
            for (int j = i+1; j < n; j++) {
                // If redefinition of var => check if used in RHS
                if ((executeStatements[j]->type == STMT_ASSIGN ||
                     executeStatements[j]->type == STMT_INPUT)
                    && executeStatements[j]->variable == var) 
                {
                    if (executeStatements[j]->type == STMT_ASSIGN) {
                        // see if var is used in j's RHS
                        vector<int> usedVars;
                        collectUsedVars(executeStatements[j]->polyEval, usedVars);
                        bool found = false;
                        for (int loc : usedVars) {
                            if (loc == vloc) {found=true; break;}
                        }
                        if (found) { used = true; }
                    }
                    break; 
                }
                // If an OUTPUT uses var
                if (executeStatements[j]->type == STMT_OUTPUT &&
                    executeStatements[j]->variable == var) 
                {
                    used = true;
                    break;
                }
                // If another assignment's RHS uses var
                if (executeStatements[j]->type == STMT_ASSIGN) {
                    vector<int> usedVars;
                    collectUsedVars(executeStatements[j]->polyEval, usedVars);
                    bool found = false;
                    for (int loc : usedVars) {
                        if (loc == vloc) { found=true; break; }
                    }
                    if (found) { used=true; break; }
                }
            }

            if (!used) {
                uselessLines.push_back(executeStatements[i]->line_no);
            }
        }
    }

    if (!uselessLines.empty()) {
        sort(uselessLines.begin(), uselessLines.end());
        cout << "Warning Code 2:";
        for (int ln : uselessLines) {
            cout << " " << ln;
        }
        cout << endl;
    }
}

// ========== main() ==========

int main() {
    Parser parser;
    parser.input();
    return 0;
}
