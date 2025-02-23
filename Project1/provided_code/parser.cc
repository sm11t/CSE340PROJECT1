#include "parser.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <functional>


using namespace std;

// ---------------- Error Handling ----------------
void Parser::syntax_error()
{
    cout << "SYNTAX ERROR !!!!!&%!!" << endl;
    exit(1);
}

Token Parser::expect(TokenType expected_type)
{
    Token token = lexer.GetToken();
    if (token.token_type != expected_type)
    {
        syntax_error();
    }
    return token;
}

// -------------- Constructor --------------
Parser::Parser() {
    // Initialize tasks array
    for (int i = 0; i < 7; i++) {
        tasks[i] = false;
    }
    nextAvailable = 0; // We'll store new variables here
}

// ----------------- Helper to consume everything (for debugging) ------------------
void Parser::ConsumeAllInput()
{
    Token token;
    int i = 1;
    token = lexer.peek(i);
    token.Print();
    while (token.token_type != END_OF_FILE)
    {
        i = i + 1;
        token = lexer.peek(i);
        token.Print();
    }
    token = lexer.GetToken();
    token.Print();
    while (token.token_type != END_OF_FILE)
    {
        token = lexer.GetToken();
        token.Print();
    }
}

// ----------------- input() -----------------
//
// input → program END_OF_FILE
//
void Parser::input()
{
    program();
    expect(END_OF_FILE);

    // ---------- SEMANTIC ERROR CHECKS (TASK 1) ----------
    if (!duplicateLines.empty()) {
        sort(duplicateLines.begin(), duplicateLines.end());
        cout << "Semantic Error Code 1:";
        for (size_t i = 0; i < duplicateLines.size(); i++) {
            cout << " " << duplicateLines[i];
        }
        cout << endl;
        exit(1);
    }
    if (!invalidMonomialLines.empty()) {
        sort(invalidMonomialLines.begin(), invalidMonomialLines.end());
        cout << "Semantic Error Code 2:";
        for (size_t i = 0; i < invalidMonomialLines.size(); i++) {
            cout << " " << invalidMonomialLines[i];
        }
        cout << endl;
        exit(1);
    }
    if (!undefinedPolyUseLines.empty()) {
        sort(undefinedPolyUseLines.begin(), undefinedPolyUseLines.end());
        cout << "Semantic Error Code 3:";
        for (size_t i = 0; i < undefinedPolyUseLines.size(); i++) {
            cout << " " << undefinedPolyUseLines[i];
        }
        cout << endl;
        exit(1);
    }
    if (!wrongArgCountLines.empty()) {
        sort(wrongArgCountLines.begin(), wrongArgCountLines.end());
        cout << "Semantic Error Code 4:";
        for (size_t i = 0; i < wrongArgCountLines.size(); i++) {
            cout << " " << wrongArgCountLines[i];
        }
        cout << endl;
        exit(1);
    }

    // ------------------- Now do Task 4 if it is listed and no errors -------------------
    // Even if Task 1 wasn't explicitly listed, we've done the checks anyway;
    // but if tasks[4] == true, we should produce the useless-assignment warnings.
    if (tasks[4]) {
        DetectUselessAssignments();
    }

    // --------------- More tasks (e.g. Task 2, 3, 5, etc.) would go here ---------------
}

// ----------------- program() -----------------
//
// program → tasks_section poly_section execute_section inputs_section
//
void Parser::program()
{
    tasks_section();
    poly_section();
    execute_section();
    inputs_section();
}

// ----------------- tasks_section() -----------------
//
// tasks_section → TASKS num_list
//
void Parser::tasks_section()
{
    expect(TASKS);
    tasknum_list();
}

// num_list → NUM | NUM num_list
void Parser::tasknum_list() {
    Token t = expect(NUM);
    int task_num = stoi(t.lexeme);
    if (task_num < 1 || task_num > 6)
        syntax_error();
    tasks[task_num] = true;

    while (lexer.peek(1).token_type == NUM) {
        t = expect(NUM);
        task_num = stoi(t.lexeme);
        if (task_num < 1 || task_num > 6)
            syntax_error();
        tasks[task_num] = true;
    }
}

// ----------------- poly_section() -----------------
//
// poly_section → POLY poly_decl_list
//
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
    if (t.token_type != EXECUTE)
        syntax_error();
}

// poly_decl → poly_header EQUAL poly_body SEMICOLON
void Parser::poly_decl() {
    poly_header();
    expect(EQUAL);

    // If we want to check param-names inside the body, store them in currentPolyParams
    currentPolyParams = polyHeaders.back().paramNames;

    poly_body();
    currentPolyParams.clear();

    expect(SEMICOLON);
}

// poly_name → ID
Token Parser::poly_name() {
    return expect(ID);
}

// id_list → ID | ID COMMA id_list
vector<string> Parser::id_list() {
    vector<string> result;
    Token firstID = expect(ID);
    result.push_back(firstID.lexeme);
    while (lexer.peek(1).token_type == COMMA) {
        expect(COMMA);
        Token nextID = expect(ID);
        result.push_back(nextID.lexeme);
    }
    return result;
}

// poly_header
void Parser::poly_header() {
    Token nameToken = poly_name();
    PolyHeaderInfo current;
    current.name = nameToken.lexeme;
    current.line_no = nameToken.line_no;

    // Check for duplicates:
    for (size_t i = 0; i < polyHeaders.size(); i++) {
        if (polyHeaders[i].name == current.name) {
            // Another polynomial with the same name
            duplicateLines.push_back(current.line_no);
            break;
        }
    }
    declaredPolynomials.insert(current.name);

    if (lexer.peek(1).token_type == LPAREN) {
        expect(LPAREN);
        current.paramNames = id_list();
        expect(RPAREN);
    } else {
        // default param is "x"
        current.paramNames.push_back("x");
    }
    polyHeaders.push_back(current);
}

// poly_body → term_list
void Parser::poly_body() {
    term_list();
}

// term_list → term | term add_operator term_list
void Parser::term_list() {
    term();
    while (lexer.peek(1).token_type == PLUS || lexer.peek(1).token_type == MINUS) {
        add_operator();
        term();
    }
}

void Parser::add_operator() {
    Token t = lexer.peek(1);
    if (t.token_type == MINUS)
        expect(MINUS);
    else if (t.token_type == PLUS)
        expect(PLUS);
    else
        syntax_error();
}

// term → coefficient | coefficient monomial_list | monomial_list
void Parser::term() {
    Token t = lexer.peek(1);
    if (t.token_type == NUM) {
        coefficient();
        Token t1 = lexer.peek(1);
        if (t1.token_type == ID || t1.token_type == LPAREN) {
            monomial_list();
        }
    }
    else if (t.token_type == ID || t.token_type == LPAREN) {
        monomial_list();
    }
    else {
        syntax_error();
    }
}

void Parser::coefficient() {
    expect(NUM);
}

// monomial_list → monomial | monomial monomial_list
void Parser::monomial_list() {
    monomial();
    while (lexer.peek(1).token_type == ID || lexer.peek(1).token_type == LPAREN) {
        monomial();
    }
}

// monomial → primary | primary exponent
void Parser::monomial() {
    primary();
    if (lexer.peek(1).token_type == POWER) {
        exponent();
    }
}

// exponent → POWER NUM
void Parser::exponent() {
    expect(POWER);
    expect(NUM);
}

// primary → ID | LPAREN term_list RPAREN
void Parser::primary() {
    if (lexer.peek(1).token_type == ID) {
        Token varTok = expect(ID);

        // Check if varTok is valid for this polynomial
        // (i.e. does it appear in currentPolyParams?)
        if (!currentPolyParams.empty()) {
            bool valid = false;
            for (auto &p : currentPolyParams) {
                if (p == varTok.lexeme) {
                    valid = true;
                    break;
                }
            }
            if (!valid) {
                invalidMonomialLines.push_back(varTok.line_no);
            }
        }
    }
    else if (lexer.peek(1).token_type == LPAREN) {
        expect(LPAREN);
        term_list();
        expect(RPAREN);
    }
    else {
        syntax_error();
    }
}

// ------------------ execute_section -------------------
void Parser::execute_section() {
    expect(EXECUTE);
    statement_list();
}

// statement_list → statement | statement statement_list
void Parser::statement_list() {
    statement();
    while (lexer.peek(1).token_type == INPUT ||
           lexer.peek(1).token_type == OUTPUT ||
           lexer.peek(1).token_type == ID) {
        statement();
    }
}

// statement → input_statement | output_statement | assign_statement
void Parser::statement() {
    Token nextToken = lexer.peek(1);
    if (nextToken.token_type == INPUT) {
        input_statement();
    }
    else if (nextToken.token_type == OUTPUT) {
        output_statement();
    }
    else if (nextToken.token_type == ID) {
        assign_statement();
    }
    else {
        syntax_error();
    }
}

void Parser::input_statement() {
    expect(INPUT);
    Token varTok = expect(ID);
    Token semi = expect(SEMICOLON);

    // Allocate variable if not already allocated
    allocateVariable(varTok.lexeme);

    // Build the statement
    Statement* stmt = new Statement;
    stmt->type = STMT_INPUT;
    stmt->variable = varTok.lexeme; // store the variable name
    stmt->line_no = varTok.line_no;
    stmt->polyEval = nullptr;

    executeStatements.push_back(stmt);
}

void Parser::output_statement() {
    expect(OUTPUT);
    Token varTok = expect(ID);
    expect(SEMICOLON);

    // Allocate variable if not already allocated
    allocateVariable(varTok.lexeme);

    Statement* stmt = new Statement;
    stmt->type = STMT_OUTPUT;
    stmt->variable = varTok.lexeme;
    stmt->line_no = varTok.line_no;
    stmt->polyEval = nullptr;

    executeStatements.push_back(stmt);
}

void Parser::assign_statement() {
    Token lhsTok = expect(ID);
    allocateVariable(lhsTok.lexeme); // ensure LHS has memory
    expect(EQUAL);

    // Create a new PolyEval
    PolyEval* pe = new PolyEval;
    pe->polyName = "";
    pe->args.clear();

    poly_evaluation(pe);

    expect(SEMICOLON);

    Statement* stmt = new Statement;
    stmt->type = STMT_ASSIGN;
    stmt->variable = lhsTok.lexeme;
    stmt->line_no = lhsTok.line_no;
    stmt->polyEval = pe;

    executeStatements.push_back(stmt);
}

// poly_evaluation → poly_name LPAREN argument_list RPAREN
void Parser::poly_evaluation(PolyEval* pe) {
    Token polyTok = poly_name();
    pe->polyName = polyTok.lexeme;
    if (declaredPolynomials.find(polyTok.lexeme) == declaredPolynomials.end()) {
        undefinedPolyUseLines.push_back(polyTok.line_no);
    }
    expect(LPAREN);
    int argCount = argument_list(pe);
    expect(RPAREN);

    int declaredCount = -1;
    for (size_t i = 0; i < polyHeaders.size(); i++) {
         if (polyHeaders[i].name == polyTok.lexeme) {
             declaredCount = polyHeaders[i].paramNames.size();
             break;
         }
    }
    if (declaredCount != -1 && argCount != declaredCount) {
         wrongArgCountLines.push_back(polyTok.line_no);
    }
}

// argument_list → argument | argument COMMA argument_list
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

// argument → ID | NUM | poly_evaluation
void Parser::argument(PolyEval* pe) {
    Token nextToken = lexer.peek(1);
    if (nextToken.token_type == ID) {
         Token t1 = lexer.peek(2);
         if (t1.token_type == LPAREN) {
              // Instead of pushing "NESTED", create and store the nested PolyEval
              PolyEval* nested = new PolyEval;
              poly_evaluation(nested);
              pe->nestedArgs.push_back(nested);
         } else {
              Token t = expect(ID);
              pe->args.push_back(t.lexeme);
         }
    }
    else if (nextToken.token_type == NUM) {
         Token t = expect(NUM);
         pe->args.push_back(t.lexeme);
    }
    else {
         syntax_error();
    }
}


// ------------------- inputs_section -------------------
//
// inputs_section → INPUTS num_list
//
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

// Allocate memory for variable if not already allocated
int Parser::allocateVariable(const std::string &varName) {
    if (symbolTable.find(varName) == symbolTable.end()) {
        symbolTable[varName] = nextAvailable++;
    }
    return symbolTable[varName];
}

// Dummy stubs for execution (not used unless Task2 is active)
void Parser::execute_program() {
    // Example only; not relevant unless you do Task 2
    int next_input = 0;
    vector<int> inputs;
    for (Statement* stmt : executeStatements) {
        switch (stmt->type) {
            case STMT_INPUT: {
                int loc = symbolTable[stmt->variable];
                // Suppose we read inputs[next_input++], etc.
                break;
            }
            case STMT_ASSIGN: {
                int value = evaluate_polynomial(stmt->polyEval);
                int loc = symbolTable[stmt->variable];
                // mem[loc] = value;
                break;
            }
            case STMT_OUTPUT: {
                int loc = symbolTable[stmt->variable];
                // cout << mem[loc] << endl;
                break;
            }
            default: break;
        }
    }
}

int Parser::evaluate_polynomial(PolyEval* pe) {
    // Dummy “constant” evaluation
    return 42;
}


//   USELESS ASSIGNMENTS (TASK 4)

void Parser::DetectUselessAssignments()
{

    std::function<void(PolyEval*, vector<int>&)> collectUsedVars;
    collectUsedVars = [&](PolyEval* pe, vector<int>& usedVars) {
        if (!pe) return;
        for (auto &arg : pe->args) {
            bool isNumber = true;
            for (char c : arg) {
                if (c < '0' || c > '9') {
                    isNumber = false;
                    break;
                }
            }
            if (!isNumber && arg != "NESTED") {
                if (symbolTable.find(arg) != symbolTable.end()) {
                    usedVars.push_back(symbolTable[arg]);
                }
            }
        }
        for (auto nested : pe->nestedArgs) {
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
            for (int j = i + 1; j < n; j++) {
                if ((executeStatements[j]->type == STMT_ASSIGN ||
                     executeStatements[j]->type == STMT_INPUT) &&
                    executeStatements[j]->variable == var) {
                    if (executeStatements[j]->type == STMT_ASSIGN) {
                        vector<int> usedVars;
                        collectUsedVars(executeStatements[j]->polyEval, usedVars);
                        bool useFound = false;
                        for (int u : usedVars) {
                            if (u == vloc) {
                                useFound = true;
                                break;
                            }
                        }
                        if (useFound) { used = true; break; }
                        else { break; }
                    } else {
                        break;
                    }
                }
                if (executeStatements[j]->type == STMT_OUTPUT &&
                    executeStatements[j]->variable == var) {
                    used = true;
                    break;
                }
                if (executeStatements[j]->type == STMT_ASSIGN) {
                    vector<int> usedVars;
                    collectUsedVars(executeStatements[j]->polyEval, usedVars);
                    for (int u : usedVars) {
                        if (u == vloc) { used = true; break; }
                    }
                    if (used) break;
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
