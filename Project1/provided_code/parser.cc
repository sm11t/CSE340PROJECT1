/*
 * Minimal direct parser that checks the grammar without storing anything.
 * This style is based on your sample code that passes all 90 test cases.
 */

 #include "parser.h"
 #include <iostream>
 #include <cstdlib>
 
 using namespace std;
 
 // ---------------------- Error Handling ----------------------
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
 
 // ---------------------- Constructor -------------------------
 Parser::Parser() {
     // If you want to initialize anything, do it here.
 }
 
 // ---------------------- ConsumeAllInput() -------------------
 void Parser::ConsumeAllInput()
 {
     // Reads and prints any remaining tokens after input() is done.
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
 
 
 void Parser::input()
 {
     program();
     expect(END_OF_FILE);
 }
 
 void Parser::program()
 {
     tasks_section();
     poly_section();
     execute_section();
     inputs_section();
 }
 
 // ---------------------- tasks_section -----------------------

 void Parser::tasks_section()
 {
     expect(TASKS);
     num_list();
 }
 
 void Parser::num_list()
 {
     expect(NUM);
     while (lexer.peek(1).token_type == NUM)
     {
         expect(NUM);
     }
 }
 
 // ---------------------- poly_section ------------------------

 void Parser::poly_section()
 {
     expect(POLY);
     poly_decl_list();
 }
 

 void Parser::poly_decl_list()
 {
     poly_decl();
     while (lexer.peek(1).token_type == ID)
     {
         poly_decl();
     }
 }

 void Parser::poly_decl()
 {
     poly_header();
     expect(EQUAL);
     poly_body();
     expect(SEMICOLON);
 }
 

 void Parser::poly_header()
 {
     poly_name();
     if (lexer.peek(1).token_type == LPAREN)
     {
         expect(LPAREN);
         id_list();
         expect(RPAREN);
     }
 }
 

 void Parser::poly_name()
 {
     expect(ID);
 }
 

 void Parser::id_list()
 {
     expect(ID);
     while (lexer.peek(1).token_type == COMMA)
     {
         expect(COMMA);
         id_list();
     }
 }

 void Parser::poly_body()
 {
     term_list();
 }
 

 void Parser::term_list()
 {
     term();
     while (lexer.peek(1).token_type == PLUS || lexer.peek(1).token_type == MINUS)
     {
         add_operator();
         term();
     }
 }
 

 void Parser::add_operator()
 {
     Token t = lexer.peek(1);
     if (t.token_type == MINUS)
         expect(MINUS);
     else if (t.token_type == PLUS)
         expect(PLUS);
     else
         syntax_error();
 }
 

 void Parser::term()
 {
     Token t = lexer.peek(1);
     if (t.token_type == NUM)
     {
         coefficient();
         Token t1 = lexer.peek(1);
         if (t1.token_type == ID || t1.token_type == LPAREN)
         {
             monomial_list();
         }
     }
     else if (t.token_type == ID || t.token_type == LPAREN)
     {
         monomial_list();
     }
     else
     {
         syntax_error();
     }
 }
 

 void Parser::coefficient()
 {
     expect(NUM);
 }

 void Parser::monomial_list()
 {
     monomial();
     while (lexer.peek(1).token_type == ID || lexer.peek(1).token_type == LPAREN)
     {
         monomial_list();
     }
 }
 

 void Parser::monomial()
 {
     primary();
     if (lexer.peek(1).token_type == POWER)
     {
         exponent();
     }
 }
 

 void Parser::exponent()
 {
     expect(POWER);
     expect(NUM);
 }

 void Parser::primary()
 {
     if (lexer.peek(1).token_type == ID)
     {
         expect(ID);
     }
     else if (lexer.peek(1).token_type == LPAREN)
     {
         expect(LPAREN);
         term_list();
         expect(RPAREN);
     }
     else
     {
         syntax_error();
     }
 }
 
 // ---------------------- execute_section ----------------------

 void Parser::execute_section()
 {
     expect(EXECUTE);
     statement_list();
 }
 

 void Parser::statement_list()
 {
     statement();
     while (lexer.peek(1).token_type == INPUT
         || lexer.peek(1).token_type == OUTPUT
         || lexer.peek(1).token_type == ID)
     {
         statement_list();
     }
 }
 

 void Parser::statement()
 {
     Token nextToken = lexer.peek(1);
     if (nextToken.token_type == INPUT)
     {
         input_statement();
     }
     else if (nextToken.token_type == OUTPUT)
     {
         output_statement();
     }
     else if (nextToken.token_type == ID)
     {
         assign_statement();
     }
     else
     {
         syntax_error();
     }
 }
 

 void Parser::input_statement()
 {
     expect(INPUT);
     expect(ID);
     expect(SEMICOLON);
 }
 

 void Parser::output_statement()
 {
     expect(OUTPUT);
     expect(ID);
     expect(SEMICOLON);
 }
 

 void Parser::assign_statement()
 {
     expect(ID);
     expect(EQUAL);
     poly_evaluation();
     expect(SEMICOLON);
 }
 

 void Parser::poly_evaluation()
 {
     poly_name();
     expect(LPAREN);
     argument_list();
     expect(RPAREN);
 }
 

 void Parser::argument_list()
 {
     argument();
     while (lexer.peek(1).token_type == COMMA)
     {
         expect(COMMA);
         argument_list();
     }
 }
 

 void Parser::argument()
 {
     Token nextToken = lexer.peek(1);
     if (nextToken.token_type == ID)
     {
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
 
 // ---------------------- inputs_section -----------------------
 // inputs_section → INPUTS num_list
 void Parser::inputs_section()
 {
     expect(INPUTS);
     num_list();
 }
 

 // Optional: function to "printFullInput" or do nothing
//  void Parser::printFullInput()
//  {
//      cout << "Parsing finished successfully.\n";
//  }
 
 int main()
 {
     Parser parser;
     parser.input();         
     parser.ConsumeAllInput();  
     return 0;
 }
 