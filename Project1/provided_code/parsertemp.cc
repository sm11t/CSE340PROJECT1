
#include "parser.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <cstring> // for stoi if needed
using namespace std;

// ==================== ERROR HANDLING ====================
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

// ==================== CONSTRUCTOR ====================
Parser::Parser() {
    for (int i = 0; i < 7; i++) {
        tasks[i] = false;
    }
    nextAvailable = 0;
    nextInputIndex = 0;
    // Initialize mem to 0
    for (int i=0; i<MEM_SIZE; i++) {
        mem[i] = 0;
    }
}

// ==================== CONSUME ALL INPUT (DEBUG) ====================
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

// ==================== MAIN INPUT FUNCTION ====================
void Parser::input() {
    program();
    expect(END_OF_FILE);

    if (!duplicateLines.empty()) {
        sort(duplicateLines.begin(), duplicateLines.end());
        cout << "Semantic Error Code 1:";
        for (auto &x : duplicateLines) cout << " " << x;
        cout << endl;
        exit(1);
    }
    if (!invalidMonomialLines.empty()) {
        sort(invalidMonomialLines.begin(), invalidMonomialLines.end());
        cout << "Semantic Error Code 2:";
        for (auto &x : invalidMonomialLines) cout << " " << x;
        cout << endl;
        exit(1);
    }
    if (!undefinedPolyUseLines.empty()) {
        sort(undefinedPolyUseLines.begin(), undefinedPolyUseLines.end());
        cout << "Semantic Error Code 3:";
        for (auto &x : undefinedPolyUseLines) cout << " " << x;
        cout << endl;
        exit(1);
    }
    if (!wrongArgCountLines.empty()) {
        sort(wrongArgCountLines.begin(), wrongArgCountLines.end());
        cout << "Semantic Error Code 4:";
        for (auto &x : wrongArgCountLines) cout << " " << x;
        cout << endl;
        exit(1);
    }
    if (tasks[2]) {
        execute_program();
    }
}

// ==================== GRAMMAR: program -> tasks_section poly_section execute_section inputs_section ====================
void Parser::program() {
    tasks_section();
    poly_section();
    execute_section();
    inputs_section();
}

// ==================== tasks_section -> TASKS num_list ====================
void Parser::tasks_section() {
    expect(TASKS);
    tasknum_list();
}

// Parse the numbers after TASKS
void Parser::tasknum_list() {
    Token t = expect(NUM);
    {
        int task_num = stoi(t.lexeme);
        if (task_num < 1 || task_num > 6) syntax_error();
        tasks[task_num] = true;
    }
    while (lexer.peek(1).token_type == NUM) {
        t = expect(NUM);
        int task_num = stoi(t.lexeme);
        if (task_num < 1 || task_num > 6) syntax_error();
        tasks[task_num] = true;
    }
}

// ==================== poly_section -> POLY poly_decl_list ====================
void Parser::poly_section() {
    expect(POLY);
    poly_decl_list();
}

// poly_decl_list -> poly_decl | poly_decl poly_decl_list
void Parser::poly_decl_list() {
    poly_decl();
    while (lexer.peek(1).token_type == ID) {
        poly_decl();
    }
    // Force next token to be EXECUTE:
    if (lexer.peek(1).token_type != EXECUTE) {
        syntax_error();
    }
}


// ==================== poly_decl -> poly_header EQUAL poly_body SEMICOLON ====================
void Parser::poly_decl() {
    // Capture the old size
    size_t oldSize = polyHeaders.size();

    poly_header(); // fills polyHeaders.back() with name, paramNames

    // Check for duplicates
    PolyHeaderInfo &hdr = polyHeaders.back();
    // If name was already declared, mark duplicates
    for (size_t i = 0; i < oldSize; i++) {
        if (polyHeaders[i].name == hdr.name) {
            duplicateLines.push_back(hdr.line_no);
            break;
        }
    }
    declaredPolynomials.insert(hdr.name);

    expect(EQUAL);

    //parse the polynomial body into IR
    TermList* bodyIR = parse_poly_body_IR();

    expect(SEMICOLON);

    // Build a new PolynomialDeclaration
    PolynomialDeclaration* pd = new PolynomialDeclaration;
    pd->name = hdr.name;
    pd->paramNames = hdr.paramNames;
    pd->line_no = hdr.line_no;
    pd->body = bodyIR;

    //Create paramIndex map
    for (int i = 0; i < (int)pd->paramNames.size(); i++) {
        pd->paramIndex[pd->paramNames[i]] = i;
    }

    // Save in polyDecls
    polyDecls.push_back(pd);
}

// ==================== poly_header -> poly_name [LPAREN id_list RPAREN] ====================
void Parser::poly_header() {
    Token nameToken = poly_name();
    PolyHeaderInfo current;
    current.name = nameToken.lexeme;
    current.line_no = nameToken.line_no;
    current.paramNames.clear();

    Token t = lexer.peek(1);
    if (t.token_type == LPAREN) {
        expect(LPAREN);
        current.paramNames = id_list();
        expect(RPAREN);
    } else {
        // default param "x"
        current.paramNames.push_back("x");
    }
    polyHeaders.push_back(current);
    currentPolyParams = current.paramNames; 
}

// ==================== poly_name -> ID ====================
Token Parser::poly_name() {
    return expect(ID);
}

// ==================== id_list -> ID | ID COMMA id_list ====================
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

// --------------------------------------------------------------------
// ==================== parse_poly_body_IR() => TermList* ====================
// Instead of void, now we actually build the IR.
TermList* Parser::parse_poly_body_IR() {
    // This is basically 'term_list' in your grammar.
    return parse_term_list_IR();
}

// term_list -> term | term add_operator term_list
TermList* Parser::parse_term_list_IR() {
    TermList* tlist = new TermList();

    // Parse the first term
    Term firstT = parse_term_IR();
    tlist->terms.push_back(firstT);

    // Check for plus or minus repeatedly
    Token tk = lexer.peek(1);
    while (tk.token_type == PLUS || tk.token_type == MINUS) {
        // read the operator
        if (tk.token_type == PLUS) {
            expect(PLUS);
            tlist->addOps.push_back('+');
        }
        else {
            expect(MINUS);
            tlist->addOps.push_back('-');
        }
        // parse the next term
        Term nxt = parse_term_IR();
        tlist->terms.push_back(nxt);

        tk = lexer.peek(1);
    }
    return tlist;
}

// We add a helper to parse add_operators into a vector if needed
void Parser::add_operator(std::vector<char> &ops) {
    Token t = lexer.peek(1);
    if (t.token_type == MINUS) {
        expect(MINUS);
        ops.push_back('-');
    } else if (t.token_type == PLUS) {
        expect(PLUS);
        ops.push_back('+');
    } else {
        syntax_error();
    }
}

// term -> coefficient | coefficient monomial_list | monomial_list
Term Parser::parse_term_IR() {
    Term term;
    term.coefficient = 1; // default if no explicit coefficient

    Token t = lexer.peek(1);

    // Check if next token is a NUM => parse as coefficient
    if (t.token_type == NUM) {
        // parse coefficient
        Token numTok = expect(NUM);
        term.coefficient = stoi(numTok.lexeme);

        // see if next is ID or LPAREN => parse monomial_list
        Token t1 = lexer.peek(1);
        if (t1.token_type == ID || t1.token_type == LPAREN) {
            // parse monomial_list
            // monomial_list -> one or more monomial
            while (true) {
                Monomial m = parse_monomial_IR();
                term.monomials.push_back(m);
                // if next is ID or LPAREN, keep going
                Token t2 = lexer.peek(1);
                if (t2.token_type == ID || t2.token_type == LPAREN) {
                    // continue
                } else {
                    break;
                }
            }
        }
        else {
            // just coefficient
        }
    }
    else if (t.token_type == ID || t.token_type == LPAREN) {
        // parse monomial_list
        term.coefficient = 1;
        while (true) {
            Monomial m = parse_monomial_IR();
            term.monomials.push_back(m);
            Token t2 = lexer.peek(1);
            if (t2.token_type == ID || t2.token_type == LPAREN) {
                // keep reading monomials
            } else {
                break;
            }
        }
    }
    else {
        syntax_error();
    }
    return term;
}

// monomial -> primary | primary exponent
Monomial Parser::parse_monomial_IR() {
    Monomial m;
    m.exponent = 1;

    Primary p = parse_primary_IR();
    m.primary = p;

    Token tk = lexer.peek(1);
    if (tk.token_type == POWER) {
        expect(POWER);
        Token numTok = expect(NUM);
        m.exponent = stoi(numTok.lexeme);
    }
    return m;
}

// primary -> ID | LPAREN term_list RPAREN
Primary Parser::parse_primary_IR() {
    Primary pr;
    Token t = lexer.peek(1);
    if (t.token_type == ID) {
        Token varTok = expect(ID);
        pr.ptype = Primary::VAR;
        pr.id = varTok.lexeme;
        pr.subTermList = nullptr;


        bool valid = false;
        for (auto &p : currentPolyParams) {
            if (p == varTok.lexeme) {
                valid = true;
                break;
            }
        }
        if (!valid) {
            // Mark line as invalid monomial name
            invalidMonomialLines.push_back(varTok.line_no);
        }
    }
    else if (t.token_type == LPAREN) {
        expect(LPAREN);
        pr.ptype = Primary::EXPR;
        pr.subTermList = parse_term_list_IR(); 
        expect(RPAREN);
    }
    else {
        syntax_error();
    }
    return pr;
}

// ==================== EXECUTE SECTION ====================
void Parser::execute_section() {
    expect(EXECUTE);
    statement_list();
}

void Parser::statement_list() {
    while (true) {
        Token nxt = lexer.peek(1);
        if (nxt.token_type == INPUT ||
            nxt.token_type == OUTPUT ||
            nxt.token_type == ID)
        {
            statement();
        } else {
            break;
        }
    }
}

// statement -> input_statement | output_statement | assign_statement
void Parser::statement() {
    Token nxt = lexer.peek(1);
    if (nxt.token_type == INPUT) {
        input_statement();
    }
    else if (nxt.token_type == OUTPUT) {
        output_statement();
    }
    else if (nxt.token_type == ID) {
        assign_statement();
    }
    else {
        syntax_error();
    }
}

void Parser::input_statement() {
    Token kw = expect(INPUT);
    Token varTok = expect(ID);
    expect(SEMICOLON);

    allocateVariable(varTok.lexeme);

    Statement* stmt = new Statement;
    stmt->type = STMT_INPUT;
    stmt->variable = varTok.lexeme;
    stmt->polyEval = nullptr;
    stmt->line_no = varTok.line_no;

    executeStatements.push_back(stmt);
}

void Parser::output_statement() {
    Token kw = expect(OUTPUT);
    Token varTok = expect(ID);
    expect(SEMICOLON);

    allocateVariable(varTok.lexeme);

    Statement* stmt = new Statement;
    stmt->type = STMT_OUTPUT;
    stmt->variable = varTok.lexeme;
    stmt->polyEval = nullptr;
    stmt->line_no = varTok.line_no;

    executeStatements.push_back(stmt);
}

// assign_statement -> ID EQUAL poly_evaluation SEMICOLON
void Parser::assign_statement() {
    Token lhsTok = expect(ID);
    allocateVariable(lhsTok.lexeme);

    expect(EQUAL);

    // Create a new PolyEval
    PolyEval* pe = new PolyEval;
    pe->polyName = "";
    pe->line_no = lhsTok.line_no;

    poly_evaluation(pe); // fills pe->polyName + pe->args

    expect(SEMICOLON);

    Statement* stmt = new Statement;
    stmt->type = STMT_ASSIGN;
    stmt->variable = lhsTok.lexeme;
    stmt->polyEval = pe;
    stmt->line_no = lhsTok.line_no;

    executeStatements.push_back(stmt);
}

// poly_evaluation -> poly_name LPAREN argument_list RPAREN
void Parser::poly_evaluation(PolyEval* pe) {
    Token polyTok = poly_name();
    pe->polyName = polyTok.lexeme;
    pe->args.clear();
    pe->line_no = polyTok.line_no;

    // check if declared
    if (declaredPolynomials.find(pe->polyName) == declaredPolynomials.end()) {
        undefinedPolyUseLines.push_back(polyTok.line_no);
    }

    expect(LPAREN);
    int argCount = argument_list(pe);
    expect(RPAREN);

    // check # of args vs declared # of params
    int declaredCount = -1;
    // find in polyDecls
    for (auto *pd : polyDecls) {
        if (pd->name == pe->polyName) {
            declaredCount = pd->paramNames.size();
            break;
        }
    }
    if (declaredCount != -1 && declaredCount != argCount) {
        wrongArgCountLines.push_back(polyTok.line_no);
    }
}

// argument_list -> argument | argument COMMA argument_list
int Parser::argument_list(PolyEval* pe) {
    int count = 0;
    // parse at least one argument
    argument(pe);
    count++;
    while (lexer.peek(1).token_type == COMMA) {
        expect(COMMA);
        argument(pe);
        count++;
    }
    return count;
}

// argument -> ID | NUM | poly_evaluation
void Parser::argument(PolyEval* pe) {
    Token nextToken = lexer.peek(1);

    EvalArg arg;
    if (nextToken.token_type == ID) {
        // check if next is ID LPAREN => nested poly
        Token t1 = lexer.peek(2);
        if (t1.token_type == LPAREN) {
            // nested call
            PolyEval* nested = new PolyEval;
            nested->args.clear();
            nested->line_no = nextToken.line_no;

            poly_evaluation(nested);

            arg.type = ARG_POLYEVAL;
            arg.nestedCall = nested;
            pe->args.push_back(arg);
        } else {
            // just a variable
            Token t = expect(ID);
            allocateVariable(t.lexeme);
            arg.type = ARG_VARIABLE;
            arg.varName = t.lexeme;
            arg.numValue = 0;
            arg.nestedCall = nullptr;
            pe->args.push_back(arg);
        }
    }
    else if (nextToken.token_type == NUM) {
        Token t = expect(NUM);
        arg.type = ARG_NUMBER;
        arg.numValue = stoi(t.lexeme);
        arg.nestedCall = nullptr;
        pe->args.push_back(arg);
    }
    else {
        // maybe nested poly or syntax error
        syntax_error();
    }
}

// ==================== inputs_section -> INPUTS num_list ====================
void Parser::inputs_section() {
    expect(INPUTS);
    inputnum_list();
}

void Parser::inputnum_list() {
    Token first = expect(NUM);
    inputValues.push_back(stoi(first.lexeme));
    while (lexer.peek(1).token_type == NUM) {
        Token t = expect(NUM);
        inputValues.push_back(stoi(t.lexeme));
    }
}

// ==================== HELPER: allocateVariable() ====================
int Parser::allocateVariable(const std::string &varName) {
    auto it = symbolTable.find(varName);
    if (it == symbolTable.end()) {
        symbolTable[varName] = nextAvailable;
        nextAvailable++;
        return nextAvailable-1;
    }
    return it->second;
}

// ==================== EXECUTE PROGRAM (Task 2) ====================
void Parser::execute_program() {
    nextInputIndex = 0;
    for (Statement* stmt : executeStatements) {
        switch (stmt->type) {
        case STMT_INPUT: {
            int loc = symbolTable[stmt->variable];
            if (nextInputIndex < (int)inputValues.size()) {
                mem[loc] = inputValues[nextInputIndex++];
            } else {
                // If not enough inputs, you might do mem[loc] = 0 or error
                mem[loc] = 0;
            }
            break;
        }
        case STMT_OUTPUT: {
            int loc = symbolTable[stmt->variable];
            cout << mem[loc] << endl;
            break;
        }
        case STMT_ASSIGN: {
            int value = evaluate_polynomial(stmt->polyEval);
            int loc = symbolTable[stmt->variable];
            mem[loc] = value;
            break;
        }
        }
    }
}

// ==================== ACTUAL EVALUATION OF A POLYEVAL ====================
int Parser::evaluate_polynomial(PolyEval* pe) {
    if (!pe) return 0;

    // 1) Find the polynomial’s IR
    PolynomialDeclaration* found = nullptr;
    for (auto *pd : polyDecls) {
        if (pd->name == pe->polyName) {
            found = pd;
            break;
        }
    }
    if (!found) {
        // Not declared or not found
        return 0;
    }

    // 2) Evaluate each argument into an integer
    vector<int> argVals;
    argVals.reserve(pe->args.size());
    for (auto &A : pe->args) {
        if (A.type == ARG_NUMBER) {
            argVals.push_back(A.numValue);
        }
        else if (A.type == ARG_VARIABLE) {
            // read from mem
            int loc = symbolTable[A.varName]; 
            argVals.push_back(mem[loc]);
        }
        else if (A.type == ARG_POLYEVAL) {
            // recursively evaluate
            int val = evaluate_polynomial(A.nestedCall);
            argVals.push_back(val);
        }
    }

    // 3) Evaluate the polynomial body with these arguments
    //    For a polynomial with param list [p1, p2, ...], 
    //    argVals[i] is the value for p_i.
    int result = evaluate_polynomial_body(found->body, argVals, found);
    return result;
}

// evaluate the IR of a polynomial
int Parser::evaluate_polynomial_body(TermList* tlist,
                                     const vector<int> &argVals,
                                     PolynomialDeclaration* pd)
{
    if (!tlist) return 0;
    if (tlist->terms.empty()) return 0;

    int total = 0;
    for (int i = 0; i < (int)tlist->terms.size(); i++) {
        int val = evaluate_term(tlist->terms[i], argVals, pd);
        if (i == 0) {
            total = val;
        } else {
            // see if we add or subtract
            char op = tlist->addOps[i-1]; // e.g. '+' or '-'
            if (op == '+') total += val;
            else total -= val;
        }
    }
    return total;
}

int Parser::evaluate_term(const Term &t,
                          const vector<int> &argVals,
                          PolynomialDeclaration* pd)
{
    // a term is coefficient * product_of_monomials
    int result = t.coefficient;
    for (auto &m : t.monomials) {
        int mval = evaluate_monomial(m, argVals, pd);
        result *= mval;
    }
    return result;
}

int Parser::evaluate_monomial(const Monomial &m,
                              const vector<int> &argVals,
                              PolynomialDeclaration* pd)
{
    // base = evaluate primary
    int base = evaluate_primary(m.primary, argVals, pd);

    // raise to exponent
    int result = 1;
    for (int i = 0; i < m.exponent; i++) {
        result *= base;
    }
    return result;
}

int Parser::evaluate_primary(const Primary &p,
                             const vector<int> &argVals,
                             PolynomialDeclaration* pd)
{
    // If it's a variable, we look up paramIndex, then argVals
    // If it's an expression, we evaluate that subTermList
    if (p.ptype == Primary::VAR) {
        auto it = pd->paramIndex.find(p.id);
        if (it == pd->paramIndex.end()) {
            // invalid, treat as 0 or already error
            return 0;
        }
        int idx = it->second;
        if (idx < 0 || idx >= (int)argVals.size()) {
            return 0; // mismatch
        }
        return argVals[idx];
    }
    else { 
        // sub expression
        return evaluate_polynomial_body(p.subTermList, argVals, pd);
    }
}

int main() {
    Parser parser;
    parser.input();
    return 0;
   }