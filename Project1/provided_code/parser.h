#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include "lexer.h"

class Parser {
  public:
    Parser();                     // Constructor
    void ConsumeAllInput();       // Prints remaining tokens after parsing
    void input();                 // Called from main(); calls program(), then expects EOF

    // Grammar productions
    void program();
    void tasks_section();
    void num_list();
    void poly_section();
    void poly_decl_list();
    void poly_decl();
    void poly_header();
    void id_list();
    void poly_body();
    void term_list();
    void add_operator();
    void term();
    void coefficient();
    void monomial_list();
    void monomial();
    void exponent();
    void primary();
    void execute_section();
    void statement_list();
    void statement();
    void input_statement();
    void output_statement();
    void assign_statement();
    void poly_name();
    void poly_evaluation();
    void argument_list();
    void argument();
    void inputs_section();

  private:
    LexicalAnalyzer lexer;
    void syntax_error();
    Token expect(TokenType expected_type);
};

#endif
