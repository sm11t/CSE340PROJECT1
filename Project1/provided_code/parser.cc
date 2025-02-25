#include "parser.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <string>
using namespace std;

std::vector<Statement*> executeStatements;
std::unordered_map<std::string,int> symbolTable;
int mem[1000] = {0};
int nextAvailable = 0;

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
    inputIndex = 0; 
}

void Parser::input() {
    program();
    expect(END_OF_FILE);

    // ---------- Check Task 1 semantic errors ----------
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

    // ---------- Task 2: Execute statements ----------
    if (tasks[2]) {
        for (Statement* st : executeStatements) {
            if (st->type == STMT_INPUT) {
                if (inputIndex < (int)inputValues.size()) {
                    mem[symbolTable[st->variable]] = inputValues[inputIndex++];
                } else {
                    mem[symbolTable[st->variable]] = 0;
                }
            }
            else if (st->type == STMT_OUTPUT) {
                int loc = symbolTable[st->variable];
                cout << mem[loc] << endl;
            }
            else if (st->type == STMT_ASSIGN) {
                int value = evaluate_polynomial(st->polyEval);
                mem[symbolTable[st->variable]] = value;
            }
        }
    }

    // ---------- Task 3 ----------
    if (tasks[3]) {
        DetectUninitializedVars();
    }

    // ---------- Task 4 ----------
    if (tasks[4]) {
        DetectUselessAssignments();
    }

    // ---------- Task 5 ----------
    if (tasks[5]) {
        for (auto &ph : polyHeaders) {
            cout << ph.name << ": " << ph.degree << endl;
        }
    }
}

/** program -> tasks_section poly_section execute_section inputs_section */
void Parser::program() {
    tasks_section();
    poly_section();
    execute_section();
    inputs_section();
}

/** TASKS num_list */
void Parser::tasks_section() {
    expect(TASKS);
    tasknum_list();
}

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

/** poly_section -> POLY poly_decl_list */
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

/** poly_decl -> poly_header EQUAL poly_body SEMICOLON */
void Parser::poly_decl() {
    poly_header();
    expect(EQUAL);

    int d = parsePolyBody();
    polyHeaders.back().degree = d;
    currentPolyParams.clear();

    expect(SEMICOLON);
}

/** poly_header -> poly_name [ LPAREN id_list RPAREN ]? */
void Parser::poly_header() {
    Token nameToken = poly_name();
    PolyHeaderInfo info;
    info.name = nameToken.lexeme;
    info.line_no = nameToken.line_no;
    info.degree = 0;
    info.astRoot = nullptr;

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
    currentPolyParams = polyHeaders.back().paramNames;
}

/** poly_name -> ID */
Token Parser::poly_name() {
    return expect(ID);
}

/** id_list -> ID | ID COMMA id_list */
std::vector<std::string> Parser::id_list() {
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

/** parsePolyBody -> parseTermListAST */
int Parser::parsePolyBody() {
    int overallDeg = 0;
    TermNode* root = parseTermListAST(overallDeg);
    polyHeaders.back().astRoot = root;
    return overallDeg;
}

/** term_list -> term [ plus_or_minus term_list ]* */
TermNode* Parser::parseTermListAST(int &degreeOut) {
    int firstTermDeg = 0;
    TermNode* firstTerm = parseTermAST(firstTermDeg);
    TermNode* current = firstTerm;
    degreeOut = firstTermDeg;

    while (true) {
        Token t = lexer.peek(1);
        if (t.token_type == PLUS) {
            lexer.GetToken();
            current->addop = ADDOP_PLUS;

            int nextTermDeg = 0;
            TermNode* nextTerm = parseTermAST(nextTermDeg);
            current->next = nextTerm;
            current = nextTerm;

            if (nextTermDeg > degreeOut) degreeOut = nextTermDeg;
        }
        else if (t.token_type == MINUS) {
            lexer.GetToken();
            current->addop = ADDOP_MINUS;

            int nextTermDeg = 0;
            TermNode* nextTerm = parseTermAST(nextTermDeg);
            current->next = nextTerm;
            current = nextTerm;

            if (nextTermDeg > degreeOut) degreeOut = nextTermDeg;
        }
        else {
            current->addop = ADDOP_NONE;
            break;
        }
    }
    return firstTerm;
}

/** term -> [coefficient] [monomial_list] */
TermNode* Parser::parseTermAST(int &maxDeg) {
    TermNode* term = new TermNode;
    term->coefficient = 1;
    term->monomials = nullptr;
    term->next = nullptr;
    term->addop = ADDOP_NONE;

    maxDeg = 0;

    Token t = lexer.peek(1);
    if (t.token_type == NUM) {
        Token coeffTok = lexer.GetToken();
        term->coefficient = stoi(coeffTok.lexeme);

        TokenType nxt = lexer.peek(1).token_type;
        if (nxt == ID || nxt == LPAREN) {
            int sumDeg = 0;
            MonomialNode* mlist = parseMonomialListAST(sumDeg);
            term->monomials = mlist;
            maxDeg = sumDeg;
        } else {
            maxDeg = 0;
        }
    }
    else if (t.token_type == ID || t.token_type == LPAREN) {
        int sumDeg = 0;
        MonomialNode* mlist = parseMonomialListAST(sumDeg);
        term->monomials = mlist;
        maxDeg = sumDeg;
    }
    else {
        syntax_error();
    }
    return term;
}

/** monomial_list -> monomial [ monomial_list ] */
MonomialNode* Parser::parseMonomialListAST(int& totalDeg) {
    totalDeg = 0;
    MonomialNode* head = nullptr;
    MonomialNode* curr = nullptr;

    while (true) {
        TokenType tt = lexer.peek(1).token_type;
        if (tt != ID && tt != LPAREN) {
            break;
        }
        int thisMonomialDeg = 0;
        MonomialNode* m = parseMonomialAST(thisMonomialDeg);
        if (!head) {
            head = m;
            curr = m;
        } else {
            curr->next = m;
            curr = m;
        }
        totalDeg += thisMonomialDeg;
    }
    return head;
}

/** monomial -> primary [ POWER NUM ] */
MonomialNode* Parser::parseMonomialAST(int& degOut) {
    MonomialNode* mn = new MonomialNode;
    mn->next = nullptr;

    int pdeg = 0;
    mn->primary = parsePrimaryAST(pdeg);

    if (lexer.peek(1).token_type == POWER) {
        lexer.GetToken();
        Token numTok = expect(NUM);
        int expVal = stoi(numTok.lexeme);
        mn->exponent = expVal;
        degOut = pdeg * expVal;
    } else {
        mn->exponent = 1;
        degOut = pdeg;
    }
    return mn;
}

/** primary -> ID | LPAREN term_list RPAREN */
PrimaryNode* Parser::parsePrimaryAST(int& degOut) {
    PrimaryNode* prim = new PrimaryNode;
    prim->isParen = false;
    prim->varName = "";
    prim->subTermList = nullptr;
    degOut = 0;

    Token t = lexer.peek(1);
    if (t.token_type == ID) {
        Token varTok = lexer.GetToken();
        bool found = false;
        for (auto &p : currentPolyParams) {
            if (p == varTok.lexeme) {
                found = true;
                break;
            }
        }
        if (!found && !currentPolyParams.empty()) {
            invalidMonomialLines.push_back(varTok.line_no);
        }
        prim->varName = varTok.lexeme;
        degOut = 1;
    }
    else if (t.token_type == LPAREN) {
        lexer.GetToken();
        int subDeg = 0;
        TermNode* subRoot = parseTermListAST(subDeg);
        prim->isParen = true;
        prim->subTermList = subRoot;
        degOut = subDeg;
        expect(RPAREN);
    }
    else {
        syntax_error();
    }
    return prim;
}

/** execute_section -> EXECUTE statement_list */
void Parser::execute_section() {
    expect(EXECUTE);
    statement_list();
}

/** statement_list -> statement { statement } */
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

/** statement -> input_statement | output_statement | assign_statement */
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

/** input_statement -> INPUT ID SEMICOLON */
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

/** output_statement -> OUTPUT ID SEMICOLON */
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

/** assign_statement -> ID EQUAL poly_evaluation SEMICOLON */
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

/** poly_evaluation -> poly_name LPAREN argument_list RPAREN */
void Parser::poly_evaluation(PolyEval* pe) {
    Token polyTok = poly_name();
    pe->polyName = polyTok.lexeme;
    if (declaredPolynomials.find(pe->polyName) == declaredPolynomials.end()) {
        undefinedPolyUseLines.push_back(polyTok.line_no);
    }
    expect(LPAREN);
    int c = argument_list(pe);
    expect(RPAREN);

    int declaredCount = -1;
    for (auto &ph : polyHeaders) {
        if (ph.name == pe->polyName) {
            declaredCount = (int)ph.paramNames.size();
            break;
        }
    }
    if (declaredCount != -1 && c != declaredCount) {
        wrongArgCountLines.push_back(polyTok.line_no);
    }
}

/** argument_list -> argument { COMMA argument } */
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

/** argument -> ID | NUM | poly_evaluation */
void Parser::argument(PolyEval* pe) {
    Token t = lexer.peek(1);
    if (t.token_type == ID) {
        Token t2 = lexer.peek(2);
        if (t2.token_type == LPAREN) {
            PolyEval* nested = new PolyEval;
            poly_evaluation(nested);
            pe->nestedArgs.push_back(nested);
        } else {
            Token idTok = expect(ID);
            allocateVariable(idTok.lexeme);
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

/** inputs_section -> INPUTS num_list */
void Parser::inputs_section() {
    expect(INPUTS);
    inputnum_list();
}

/** inputnum_list -> NUM { NUM } */
void Parser::inputnum_list() {
    Token first = expect(NUM);
    inputValues.push_back(stoi(first.lexeme));
    while (lexer.peek(1).token_type == NUM) {
        Token more = expect(NUM);
        inputValues.push_back(stoi(more.lexeme));
    }
}

int Parser::allocateVariable(const string &varName) {
    if (symbolTable.find(varName) == symbolTable.end()) {
        symbolTable[varName] = nextAvailable++;
    }
    return symbolTable[varName];
}

void Parser::execute_program() {
    // not automatically called
}

int Parser::evaluate_polynomial(PolyEval* pe) {
    PolyHeaderInfo* ph = nullptr;
    for (auto &hdr : polyHeaders) {
        if (hdr.name == pe->polyName) {
            ph = &hdr;
            break;
        }
    }
    if (!ph) {
        return 0; // not declared
    }

    // gather arg values
    vector<int> argValues;
    function<int(const string&)> getValueOfArg = [&](const string &arg)->int {
        bool allDigits = true;
        for (char c : arg) {
            if (c < '0' || c > '9') {
                allDigits=false; 
                break;
            }
        }
        if (allDigits) {
            return stoi(arg);
        } else {
            auto it = symbolTable.find(arg);
            if (it == symbolTable.end()) return 0;
            return mem[it->second];
        }
    };

    int needed = (int)ph->paramNames.size();
    int iArgs = 0;
    int iNested = 0;

    for (int i = 0; i < needed; i++) {
        if (iArgs < (int)pe->args.size()) {
            int val = getValueOfArg(pe->args[iArgs]);
            argValues.push_back(val);
            iArgs++;
        }
        else {
            if (iNested < (int)pe->nestedArgs.size()) {
                int val = evaluate_polynomial(pe->nestedArgs[iNested]);
                argValues.push_back(val);
                iNested++;
            }
            else {
                argValues.push_back(0);
            }
        }
    }

    unordered_map<string,int> paramMap;
    for (int i = 0; i < (int)ph->paramNames.size(); i++) {
        paramMap[ ph->paramNames[i] ] = i;
    }
    return evalPolyAST(ph->astRoot, argValues, paramMap);
}

int Parser::evalPolyAST(TermNode* termList,
                        const vector<int>& argValues,
                        const unordered_map<string,int>& paramMap) {
    if (!termList) return 0;
    int val = evalTermNode(termList, argValues, paramMap);

    if (termList->addop == ADDOP_NONE) {
        return val;
    }
    else if (termList->addop == ADDOP_PLUS) {
        return val + evalPolyAST(termList->next, argValues, paramMap);
    }
    else {
        return val - evalPolyAST(termList->next, argValues, paramMap);
    }
}

int Parser::evalTermNode(TermNode* t,
                         const vector<int>& argValues,
                         const unordered_map<string,int>& paramMap) {
    if (!t) return 0;
    int product = 1;
    if (t->monomials) {
        product = evalMonomialList(t->monomials, argValues, paramMap);
    }
    return t->coefficient * product;
}

int Parser::evalMonomialList(MonomialNode* mono,
                             const vector<int>& argValues,
                             const unordered_map<string,int>& paramMap) {
    if (!mono) return 1;
    int baseVal = evalPrimary(mono->primary, argValues, paramMap);

    int result = 1;
    for (int i = 0; i < mono->exponent; i++) {
        result *= baseVal;
    }
    if (mono->next) {
        return result * evalMonomialList(mono->next, argValues, paramMap);
    }
    return result;
}

int Parser::evalPrimary(PrimaryNode* prim,
                        const vector<int>& argValues,
                        const unordered_map<string,int>& paramMap) {
    if (!prim) return 0;
    if (!prim->isParen) {
        auto it = paramMap.find(prim->varName);
        if (it == paramMap.end()) {
            return 0;
        }
        int idx = it->second;
        if (idx < 0 || idx >= (int)argValues.size()) return 0;
        return argValues[idx];
    } else {
        return evalPolyAST(prim->subTermList, argValues, paramMap);
    }
}

/** Task 3: Uninitialized variables */
void Parser::DetectUninitializedVars() {
    vector<bool> initialized(nextAvailable, false);
    vector<int> uninitLines;

    function<void(PolyEval*, vector<int>&)> collectUsedVars;
    collectUsedVars = [&](PolyEval* pe, vector<int>& used) {
        if (!pe) return;
        for (auto &arg : pe->args) {
            bool isNum = true;
            for (char c : arg) {
                if (c < '0' || c > '9') {
                    isNum = false; 
                    break;
                }
            }
            if (!isNum) {
                if (symbolTable.find(arg) != symbolTable.end()) {
                    used.push_back(symbolTable[arg]);
                }
            }
        }
        for (auto* nested : pe->nestedArgs) {
            collectUsedVars(nested, used);
        }
    };

    for (Statement* st : executeStatements) {
        switch (st->type) {
            case STMT_INPUT: {
                int loc = symbolTable[st->variable];
                initialized[loc] = true;
                break;
            }
            case STMT_OUTPUT: {
                break;
            }
            case STMT_ASSIGN: {
                vector<int> usedVars;
                collectUsedVars(st->polyEval, usedVars);
                for (int vloc : usedVars) {
                    if (!initialized[vloc]) {
                        uninitLines.push_back(st->line_no);
                    }
                }
                int lhsLoc = symbolTable[st->variable];
                initialized[lhsLoc] = true;
                break;
            }
        }
    }

    if (!uninitLines.empty()) {
        sort(uninitLines.begin(), uninitLines.end());
        cout << "Warning Code 1:";
        for (int ln : uninitLines) {
            cout << " " << ln;
        }
        cout << endl;
    }
}

/** Task 4: Useless assignments */
void Parser::DetectUselessAssignments() {
    function<void(PolyEval*, vector<int>&)> collectUsedVars;
    collectUsedVars = [&](PolyEval* pe, vector<int>& usedVars) {
        if (!pe) return;
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
        for (auto* nested : pe->nestedArgs) {
            collectUsedVars(nested, usedVars);
        }
    };

    vector<int> uselessLines;
    int n = (int)executeStatements.size();
    for (int i = 0; i < n; i++) {
        if (executeStatements[i]->type == STMT_ASSIGN) {
            string var = executeStatements[i]->variable;
            int vloc = symbolTable[var];
            bool used = false;

            for (int j = i+1; j < n; j++) {
                if ((executeStatements[j]->type == STMT_ASSIGN ||
                     executeStatements[j]->type == STMT_INPUT)
                    && executeStatements[j]->variable == var)
                {
                    if (executeStatements[j]->type == STMT_ASSIGN) {
                        vector<int> usedVars;
                        collectUsedVars(executeStatements[j]->polyEval, usedVars);
                        bool found = false;
                        for (int loc : usedVars) {
                            if (loc == vloc) {
                                found = true;
                                break;
                            }
                        }
                        if (found) { used = true; }
                    }
                    break;
                }
                if (executeStatements[j]->type == STMT_OUTPUT &&
                    executeStatements[j]->variable == var)
                {
                    used = true;
                    break;
                }
                if (executeStatements[j]->type == STMT_ASSIGN) {
                    vector<int> usedVars;
                    collectUsedVars(executeStatements[j]->polyEval, usedVars);
                    bool found = false;
                    for (int loc : usedVars) {
                        if (loc == vloc) {
                            found = true;
                            break;
                        }
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

int main() {
    Parser parser;
    parser.input();
    return 0;
}
