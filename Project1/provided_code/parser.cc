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
 
 Parser::Parser() {
     for (int i = 0; i < 7; i++) {
         tasks[i] = false;
     }
 }
 
 bool Parser::isTaskSelected(int taskNumber) {
     if (taskNumber >= 1 && taskNumber <= 6)
         return tasks[taskNumber];
     return false;
 }
 
 void Parser::parse_input() {
     parse_program();
     expect(END_OF_FILE);
 }
 
 void Parser::parse_program() {
     parse_tasks_section();
     parse_poly_section();
     // (Additional sections such as EXECUTE and INPUTS would be parsed here.)
     printPolyDeclarations();
 }
 
 void Parser::parse_tasks_section() {
     expect(TASKS);
     while (lexer.peek(1).token_type == NUM) {
         Token t = lexer.GetToken();
         int task_num = std::stoi(t.lexeme);
         if (task_num >= 1 && task_num <= 6)
             tasks[task_num] = true;
         else
             syntax_error();
     }
 }
 
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
     decl.body   = body;
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
         header.params.push_back("x");
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
     // Parse the first term.
     Term t = parse_term();
     body.terms.push_back(t);
     // Parse additional terms with PLUS or MINUS.
     while (lexer.peek(1).token_type == PLUS || lexer.peek(1).token_type == MINUS) {
         Token op = lexer.GetToken();
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
     mono.exponent = 1; // Default exponent.
     mono.primary = parse_primary();
     if (lexer.peek(1).token_type == POWER) {
         expect(POWER);
         Token numToken = expect(NUM);
         mono.exponent = std::stoi(numToken.lexeme);
     }
     return mono;
 }
 
 Primary Parser::parse_primary() {
     Primary prim;
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
 
 void Parser::ConsumeAllInput() {
     parse_input();
 }
 
 // ---------- Helper Functions for String Conversion ----------
 
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
 
 // ---------- Main Function ----------
 int main() {
     Parser parser;
     parser.parse_program();
     return 0;
 }
 