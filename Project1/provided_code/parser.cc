#include "parser.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <string>

using namespace std;

// Global data structures declared 'extern' in parser.h
std::vector<Statement*> executeStatements;
std::unordered_map<std::string,int> symbolTable;
int mem[1000] = {0};
int nextAvailable = 0;

// ---------- Parser Implementation ----------

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

Parser::Parser() {
    for (int i = 0; i < 7; i++) {
        tasks[i] = false;
    }
    nextAvailable = 0;
}


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

    // ---------- Task 3: detect uninitialized arguments ----------
    if (tasks[3]) {
        DetectUninitializedVars();
    }

    // ---------- Task 4: detect useless assignments ----------
    if (tasks[4]) {
        DetectUselessAssignments();
    }

    // ---------- Task 5: print polynomial degrees ----------
    if (tasks[5]) {
        for (auto &ph : polyHeaders) {
            cout << ph.name << ": " << ph.degree << endl;
        }
        exit(0);
    }

    // If tasks[2], tasks[6], etc. exist, handle them similarly.
}

void Parser::program() {
    tasks_section();
    poly_section();
    execute_section();
    inputs_section();
}

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

void Parser::poly_section() {
    expect(POLY);
    poly_decl_list();
}

void Parser::poly_decl_list() {
    poly_decl();
    Token t = lexer.peek(1);
    while (t.token_type == ID) {
        poly_decl();
        t = lexer.peek(1);
    }
    if (t.token_type != EXECUTE) syntax_error();
}

void Parser::poly_decl() {
    poly_header();
    expect(EQUAL);

    currentPolyParams = polyHeaders.back().paramNames;
    int d = parsePolyBody();
    polyHeaders.back().degree = d;
    currentPolyParams.clear();

    expect(SEMICOLON);
}

void Parser::poly_header() {
    Token nameToken = poly_name();
    PolyHeaderInfo info;
    info.name = nameToken.lexeme;
    info.line_no = nameToken.line_no;
    info.degree = 0;

    // Check duplicates
    for (auto &ph : polyHeaders) {
        if (ph.name == info.name) {
            duplicateLines.push_back(info.line_no);
            break;
        }
    }
    declaredPolynomials.insert(info.name);

    if (lexer.peek(1).token_type == LPAREN) {
        expect(LPAREN);
        info.paramNames = id_list();
        expect(RPAREN);
    } else {
        info.paramNames.push_back("x");
    }

    polyHeaders.push_back(info);
}

Token Parser::poly_name() {
    return expect(ID);
}

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

// ===================== Polynomial Body Parsing (Computes Degree) =====================

int Parser::parsePolyBody() {
    return parseTermList();
}

int Parser::parseTermList() {
    int d = parseTerm();
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

int Parser::parseTerm() {
    Token t = lexer.peek(1);
    if (t.token_type == NUM) {
        lexer.GetToken(); // consume the coefficient
        TokenType t2 = lexer.peek(1).token_type;
        if (t2 == ID || t2 == LPAREN) {
            return parseMonomialList();
        } else {
            return 0; // just the coefficient => 0
        }
    } else if (t.token_type == ID || t.token_type == LPAREN) {
        // no numeric coefficient => 1 * ...
        return parseMonomialList();
    } else {
        syntax_error();
        return 0;
    }
}

int Parser::parseMonomialList() {
    // sum of degrees of each monomial
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

int Parser::parseMonomial() {
    int pdeg = parsePrimary();
    int deg = pdeg;
    if (lexer.peek(1).token_type == POWER) {
        lexer.GetToken(); // consume ^
        Token numTok = expect(NUM);
        int expVal = stoi(numTok.lexeme);
        deg = pdeg * expVal;
    }
    return deg;
}

int Parser::parsePrimary() {
    Token t = lexer.peek(1);
    if (t.token_type == ID) {
        Token varTok = lexer.GetToken(); // consume ID
        // Check if varTok is valid param
        if (!currentPolyParams.empty()) {
            bool found = false;
            for (auto &p : currentPolyParams) {
                if (p == varTok.lexeme) {found=true; break;}
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

// ===================== EXECUTE section =====================

void Parser::execute_section() {
    expect(EXECUTE);
    statement_list();
}

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
    Token t = lexer.peek(1);
    if (t.token_type == INPUT) {
        input_statement();
    }
    else if (t.token_type == OUTPUT) {
        output_statement();
    }
    else if (t.token_type == ID) {
        assign_statement();
    }
    else {
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

void Parser::argument(PolyEval* pe) {
    Token t = lexer.peek(1);
    if (t.token_type == ID) {
        Token t2 = lexer.peek(2);
        if (t2.token_type == LPAREN) {
            // nested poly call
            PolyEval* nested = new PolyEval;
            poly_evaluation(nested);
            pe->nestedArgs.push_back(nested);
        } else {
            // It's a simple ID argument. We must allocate so it shows up as a variable.
            Token idTok = expect(ID);
            allocateVariable(idTok.lexeme);  // <<--- ADD THIS
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

void Parser::inputs_section() {
    expect(INPUTS);
    inputnum_list();
}

void Parser::inputnum_list() {
    expect(NUM);
    while (lexer.peek(1).token_type == NUM) {
        expect(NUM);
    }
}

// ========== Memory and Execution Stubs ==========

int Parser::allocateVariable(const string &varName) {
    if (symbolTable.find(varName) == symbolTable.end()) {
        symbolTable[varName] = nextAvailable++;
    }
    return symbolTable[varName];
}

void Parser::execute_program() {
    // Not relevant unless you do Task 2
}

int Parser::evaluate_polynomial(PolyEval* /*polyEval*/) {
    // dummy
    return 42;
}

// ========== Task 3: Detect Uninitialized Variables ==========
// We produce "Warning Code 1: ..." if an argument in a polynomial evaluation
// is used but never assigned nor input prior to that usage.

void Parser::DetectUninitializedVars() 
{
    // Keep track of whether each variable location is "initialized"
    // by an earlier INPUT or ASSIGN statement.
    vector<bool> initialized(nextAvailable, false);
    vector<int> uninitLines;

    // We'll define a function to collect all variable locations used by a polyEval
    std::function<void(PolyEval*, std::vector<int>&)> collectUsedVars;
    collectUsedVars = [&](PolyEval* pe, std::vector<int>& used) {
        if (!pe) return;
        // For each arg that is ID, gather its location
        for (auto &arg : pe->args) {
            bool isNum = true;
            for (char c : arg) {
                if (c < '0' || c > '9') {
                    isNum = false;
                    break;
                }
            }
            if (!isNum) { // it's presumably a variable
                if (symbolTable.find(arg) != symbolTable.end()) {
                    used.push_back(symbolTable[arg]);
                }
            }
        }
        // Recurse into nested calls
        for (auto* nested : pe->nestedArgs) {
            collectUsedVars(nested, used);
        }
    };

    // Now do a forward pass over the statements
    for (Statement* st : executeStatements) {
        switch (st->type) {
            case STMT_INPUT:
            {
                // This "initializes" st->variable
                int loc = symbolTable[st->variable];
                initialized[loc] = true;
                break;
            }
            case STMT_OUTPUT:
            {
                // We do *not* treat OUTPUT X as a polynomial usage, so no checks here
                break;
            }
            case STMT_ASSIGN:
            {
                // 1) Collect the used variables in st->polyEval
                vector<int> usedVars;
                collectUsedVars(st->polyEval, usedVars);

                // 2) For each used var, if not initialized => record line number
                for (int vloc : usedVars) {
                    if (!initialized[vloc]) {
                        // If the statement uses multiple uninitialized, we add the line multiple times
                        uninitLines.push_back(st->line_no);
                    }
                }

                // 3) Now that we've used them, we define st->variable
                int lhsLoc = symbolTable[st->variable];
                initialized[lhsLoc] = true;
                break;
            }
        }
    }

    // Print if we have uninit lines
    if (!uninitLines.empty()) {
        sort(uninitLines.begin(), uninitLines.end());
        cout << "Warning Code 1:";
        for (int ln : uninitLines) {
            cout << " " << ln;
        }
        cout << endl;
    }
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
                // If redefinition of var => check if used in j's RHS
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
