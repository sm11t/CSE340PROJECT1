/*
 * Copyright (C) Rida Bazzi, 2020
 *
 * Do not share this file with anyone
 */

 #include "parser.h"
 #include <iostream>
 #include <cstdlib>
 #include <sstream>
 
 using namespace std;
 
 // ---------------------- Error Handling ----------------------
 void Parser::syntax_error() {
     cout << "SYNTAX ERROR !!!!!&%!!\n";
     exit(1);
 }
 
 Token Parser::expect(TokenType expected_type) {
     Token t = lexer.GetToken();
     if (t.token_type != expected_type)
         syntax_error();
     return t;
 }
 
 // ---------------------- Constructor -------------------------
 Parser::Parser() {
     for (int i = 0; i < 7; i++)
         tasks[i] = false;
 }
 
 bool Parser::isTaskSelected(int taskNumber) {
     if (taskNumber >= 1 && taskNumber <= 6)
         return tasks[taskNumber];
     return false;
 }
 
 // ---------------------- Overall Parsing ---------------------
 void Parser::ConsumeAllInput() {
     parse_input();
 }
 
 void Parser::parse_input() {
     parse_program();
     expect(END_OF_FILE);
 }
 
 void Parser::parse_program() {
     parse_tasks_section();
     parse_poly_section();
     parse_execute_section();
     parse_inputs_section();
     printFullInput();
 }
 
 // ---------------------- TASKS Section -----------------------
 void Parser::parse_tasks_section() {
     expect(TASKS);
     parse_num_list(); // num_list → NUM | NUM num_list
 }
 
 void Parser::parse_num_list() {
     Token t = expect(NUM);
     // For now, we simply consume NUM tokens.
     if (lexer.peek(1).token_type == NUM)
         parse_num_list();
 }
 
 // ---------------------- POLY Section ------------------------
 void Parser::parse_poly_section() {
     expect(POLY);
     parse_poly_decl_list();
 }
 
 void Parser::parse_poly_decl_list() {
     parse_poly_decl();
     Token t = lexer.peek(1);
     if (t.token_type == ID)
         parse_poly_decl_list();
 }
 
 void Parser::parse_poly_decl() {
     PolyHeader header = parse_poly_header();
     expect(EQUAL);
     PolyBody body = parse_poly_body();
     expect(SEMICOLON);
     
     PolyDecl decl;
     decl.header = header;
     decl.body = body;
     polyDeclarations.push_back(decl);
 }
 
 PolyHeader Parser::parse_poly_header() {
     PolyHeader header;
     Token polyName = expect(ID);
     header.name = polyName.lexeme;
     if (lexer.peek(1).token_type == LPAREN) {
         expect(LPAREN);
         header.params = parse_id_list();
         expect(RPAREN);
     } else {
         header.params.push_back("x"); // default parameter
     }
     return header;
 }
 
 std::vector<std::string> Parser::parse_id_list() {
     std::vector<std::string> params;
     Token firstId = expect(ID);
     params.push_back(firstId.lexeme);
     while (lexer.peek(1).token_type == COMMA) {
         expect(COMMA);
         Token nextId = expect(ID);
         params.push_back(nextId.lexeme);
     }
     return params;
 }
 
 PolyBody Parser::parse_poly_body() {
     PolyBody body;
     // term_list → term | term add_operator term_list
     Term t = parse_term();
     body.terms.push_back(t);
     while (lexer.peek(1).token_type == PLUS || lexer.peek(1).token_type == MINUS) {
         Token op = lexer.GetToken(); // consume PLUS or MINUS
         Term nextTerm = parse_term();
         if (op.token_type == MINUS)
             nextTerm.coefficient = -nextTerm.coefficient;
         body.terms.push_back(nextTerm);
     }
     return body;
 }
 
 Term Parser::parse_term() {
     Term term;
     term.coefficient = 1;
     // term → monomial_list | coefficient monomial_list | coefficient
     if (lexer.peek(1).token_type == NUM) {
         Token numToken = lexer.GetToken();
         term.coefficient = std::stoi(numToken.lexeme);
         if (lexer.peek(1).token_type == ID || lexer.peek(1).token_type == LPAREN)
             term.monomials = parse_monomial_list();
     } else {
         term.monomials = parse_monomial_list();
     }
     return term;
 }
 
 std::vector<Monomial> Parser::parse_monomial_list() {
     std::vector<Monomial> list;
     list.push_back(parse_monomial());
     while (lexer.peek(1).token_type == ID || lexer.peek(1).token_type == LPAREN) {
         list.push_back(parse_monomial());
     }
     return list;
 }
 
 Monomial Parser::parse_monomial() {
     Monomial mono;
     mono.exponent = 1;
     mono.primary = parse_primary();
     // monomial → primary | primary exponent
     if (lexer.peek(1).token_type == POWER) {
         expect(POWER);
         Token numToken = expect(NUM);
         mono.exponent = std::stoi(numToken.lexeme);
     }
     return mono;
 }
 
 Primary Parser::parse_primary() {
     Primary prim;
     // primary → ID | LPAREN term_list RPAREN
     if (lexer.peek(1).token_type == ID) {
         Token idToken = expect(ID);
         prim.type = Primary::VAR;
         prim.var = idToken.lexeme;
         prim.group = nullptr;
     } else if (lexer.peek(1).token_type == LPAREN) {
         expect(LPAREN);
         PolyBody innerBody = parse_poly_body();
         expect(RPAREN);
         prim.type = Primary::EXPR;
         prim.group = new PolyBody(innerBody);
     } else {
         syntax_error();
     }
     return prim;
 }
 
 // ---------------------- EXECUTE Section ---------------------
 void Parser::parse_execute_section() {
     expect(EXECUTE);
     parse_statement_list();
 }
 
 void Parser::parse_statement_list() {
     // statement_list → statement | statement statement_list
     while (true) {
         Token t = lexer.peek(1);
         if (t.token_type == INPUT)
             parse_input_statement();
         else if (t.token_type == OUTPUT)
             parse_output_statement();
         else if (t.token_type == ID)
             parse_assign_statement();
         else
             break;
     }
 }
 
 void Parser::parse_input_statement() {
     Statement stmt;
     expect(INPUT);
     Token varToken = expect(ID);
     expect(SEMICOLON);
     stmt.stmt_type = INPUT_STMT;
     stmt.var = varToken.lexeme;
     statements.push_back(stmt);
 }
 
 void Parser::parse_output_statement() {
     Statement stmt;
     expect(OUTPUT);
     Token varToken = expect(ID);
     expect(SEMICOLON);
     stmt.stmt_type = OUTPUT_STMT;
     stmt.var = varToken.lexeme;
     statements.push_back(stmt);
 }
 
 void Parser::parse_assign_statement() {
     Statement stmt;
     Token varToken = expect(ID);
     expect(EQUAL);
     PolyBody evalBody = parse_poly_evaluation();
     expect(SEMICOLON);
     stmt.stmt_type = ASSIGN_STMT;
     stmt.var = varToken.lexeme;
     stmt.poly_eval = evalBody;
     statements.push_back(stmt);
 }
 
 PolyBody Parser::parse_poly_evaluation() {
     // poly_evaluation → poly_name LPAREN argument_list RPAREN
     // For our purposes, we'll parse this similarly to a poly_body expression.
     PolyBody evalBody;
     expect(ID); // poly_name; we don't store it here.
     expect(LPAREN);
     evalBody = parse_poly_body();
     expect(RPAREN);
     return evalBody;
 }
 
 void Parser::parse_argument_list() {
     // Not used separately because we re-use poly evaluation parsing.
     parse_argument();
     while (lexer.peek(1).token_type == COMMA) {
         expect(COMMA);
         parse_argument();
     }
 }
 
 void Parser::parse_argument() {
     // argument → ID | NUM | poly_evaluation
     Token t = lexer.peek(1);
     if (t.token_type == ID) {
         if (lexer.peek(2).token_type == LPAREN)
             parse_poly_evaluation();
         else
             expect(ID);
     } else if (t.token_type == NUM) {
         expect(NUM);
     } else {
         parse_poly_evaluation();
     }
 }
 
 // ---------------------- INPUTS Section ---------------------
 void Parser::parse_inputs_section() {
     expect(INPUTS);
     while (lexer.peek(1).token_type == NUM) {
         Token t = expect(NUM);
         inputs.push_back(std::stoi(t.lexeme));
     }
 }
 
 // ---------------------- Full Print Function ---------------------
 void Parser::printFullInput() {
     cout << "Tasks: ";
     for (int i = 1; i < 7; i++) {
         if (tasks[i])
             cout << i << " ";
     }
     cout << "\n\n";
     
     cout << "Polynomial Declarations:\n";
     for (const auto &decl : polyDeclarations) {
         cout << "Poly Name: " << decl.header.name << "\n";
         cout << "Parameters: ";
         for (const auto &param : decl.header.params)
             cout << param << " ";
         cout << "\nBody: " << polyBodyToString(decl.body) << "\n";
         cout << "--------------------------\n";
     }
     cout << "\n";
     

     cout << "Execute Statements:\n";
     for (const auto &stmt : statements) {
         switch (stmt.stmt_type) {
             case INPUT_STMT:
                 cout << "INPUT " << stmt.var << ";\n";
                 break;
             case OUTPUT_STMT:
                 cout << "OUTPUT " << stmt.var << ";\n";
                 break;
             case ASSIGN_STMT:
                 cout << stmt.var << " = " << polyBodyToString(stmt.poly_eval) << ";\n";
                 break;
         }
     }
     cout << "\n";
     

     cout << "Inputs: ";
     for (size_t i = 0; i < inputs.size(); i++) {
         cout << inputs[i] << " ";
     }
     cout << "\n";
 }
 

 void Parser::printPolyDeclarations() {
     for (const auto &decl : polyDeclarations) {
         cout << "Poly Name: " << decl.header.name << "\n";
         cout << "Parameters: ";
         for (const auto &param : decl.header.params)
             cout << param << " ";
         cout << "\nBody: " << polyBodyToString(decl.body) << "\n";
         cout << "--------------------------\n";
     }
 }
 
 // ---------------------- Helper Functions for Converting Expression to String ----------------------
 std::string primaryToString(const Primary& prim) {
     if (prim.type == Primary::VAR)
         return prim.var;
     else {
         std::string res = "(";
         if (prim.group)
             res += polyBodyToString(*(prim.group));
         res += ")";
         return res;
     }
 }
 
 std::string monomialToString(const Monomial& mono) {
     std::ostringstream oss;
     oss << primaryToString(mono.primary);
     if (mono.exponent != 1)
         oss << "^" << mono.exponent;
     return oss.str();
 }
 
 std::string termToString(const Term& term) {
     std::ostringstream oss;
     oss << term.coefficient;
     for (const auto &mono : term.monomials)
         oss << monomialToString(mono);
     return oss.str();
 }
 
 std::string polyBodyToString(const PolyBody& body) {
     std::ostringstream oss;
     if (!body.terms.empty()) {
         oss << termToString(body.terms[0]);
         for (size_t i = 1; i < body.terms.size(); i++) {
             if (body.terms[i].coefficient >= 0)
                 oss << " + " << termToString(body.terms[i]);
             else {
                 std::string termStr = termToString(body.terms[i]);
                 oss << " - " << termStr.substr(1);
             }
         }
     }
     return oss.str();
 }
 
 
 int main() {
     Parser parser;
     parser.parse_program();
     return 0;
 }
 