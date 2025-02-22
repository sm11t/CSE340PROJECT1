#include "parser.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>

using namespace std;

// ####################### Error Handling #######################
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

// ####################### Constructor #######################
Parser::Parser() {
    // Initialize tasks array
    for (int i = 0; i < 7; i++) {
        tasks[i] = false;
    }
}

// ####################### ConsumeAllInput() #######################
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

// ####################### input() #######################
void Parser::input()
{
    program();
    expect(END_OF_FILE);

    // ###CHECKING FOR SEMANTIC ERROR CODE 1###
    if (!duplicateLines.empty()) {
        sort(duplicateLines.begin(), duplicateLines.end());
        cout << "Semantic Error Code 1:";
        for (size_t i = 0; i < duplicateLines.size(); i++) {
            cout << " " << duplicateLines[i];
        }
        cout << endl;
        exit(1);
    }
    // ###CHECKING FOR SEMANTIC ERROR CODE 2###
    if (!invalidMonomialLines.empty()) {
        sort(invalidMonomialLines.begin(), invalidMonomialLines.end());
        cout << "Semantic Error Code 2:";
        for (size_t i = 0; i < invalidMonomialLines.size(); i++) {
            cout << " " << invalidMonomialLines[i];
        }
        cout << endl;
        exit(1);
    }
    // ###CHECKING FOR SEMANTIC ERROR CODE 3###
    if (!undefinedPolyUseLines.empty()) {
        sort(undefinedPolyUseLines.begin(), undefinedPolyUseLines.end());
        cout << "Semantic Error Code 3:";
        for (size_t i = 0; i < undefinedPolyUseLines.size(); i++) {
            cout << " " << undefinedPolyUseLines[i];
        }
        cout << endl;
        exit(1);
    }
}

// ####################### program #######################
// program → tasks_section poly_section execute_section inputs_section
void Parser::program()
{
    tasks_section();
    poly_section();
    execute_section();
    inputs_section();
}
 
// ####################### tasks_section #######################
// tasks_section → TASKS num_list
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
 
// ####################### poly_section #######################
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
    if (t.token_type != EXECUTE)
        syntax_error();
}
 
// poly_decl → poly_header EQUAL poly_body SEMICOLON
void Parser::poly_decl() {
    poly_header();
    expect(EQUAL);
    
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
std::vector<std::string> Parser::id_list() {
    std::vector<std::string> result;
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

    // Duplicate checking 
    for (size_t i = 0; i < polyHeaders.size(); i++) {
        if (polyHeaders[i].name == current.name) {
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
 
// ##################### execute_section #####################
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
    expect(ID);
    expect(SEMICOLON);
}
 
void Parser::output_statement() {
    expect(OUTPUT);
    expect(ID);
    expect(SEMICOLON);
}
 
void Parser::assign_statement() {
    expect(ID);
    expect(EQUAL);
    poly_evaluation();
    expect(SEMICOLON);
}
 
// poly_evaluation → poly_name LPAREN argument_list RPAREN
void Parser::poly_evaluation() {
    Token polyTok = poly_name();
    if (declaredPolynomials.find(polyTok.lexeme) == declaredPolynomials.end()) {
         undefinedPolyUseLines.push_back(polyTok.line_no);
    }
    expect(LPAREN);
    argument_list();
    expect(RPAREN);
}
 
// argument_list → argument | argument COMMA argument_list
void Parser::argument_list() {
    argument();
    while (lexer.peek(1).token_type == COMMA) {
         expect(COMMA);
         argument();
    }
}
 
// argument → ID | NUM | poly_evaluation
void Parser::argument() {
    Token nextToken = lexer.peek(1);
    if (nextToken.token_type == ID) {
         Token t1 = lexer.peek(2);
         if (t1.token_type == LPAREN)
              poly_evaluation();
         else
              expect(ID);
    }
    else if (nextToken.token_type == NUM)
         expect(NUM);
    else
         syntax_error();
}
 
// ####################### inputs_section #######################
// inputs_section → INPUTS num_list
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
 
 int main() {
     Parser parser;
     parser.input();
     return 0;
 }
