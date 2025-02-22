#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include "lexer.h"
#include <unordered_set>
#include <vector>

struct PolyHeaderInfo {
    std::string name;               
    int line_no;                    
    std::vector<std::string> paramNames;  
};



class Parser {
  public:
    Parser();                     // Constructor
    void ConsumeAllInput();       // Prints remaining tokens after parsing
    void input();                 // Called from main(); calls program(), then expects EOF

    // Grammar productions
    void program();
    void tasks_section();
    void tasknum_list();
    void inputnum_list();
    void poly_section();
    void poly_decl_list();
    void poly_decl();
    void poly_header();
    std::vector<std::string> id_list();
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
    Token poly_name();
    void poly_evaluation();
    void argument_list();
    void argument();
    void inputs_section();

    bool tasks[7];

    std::unordered_set<std::string> declaredPolynomials;
    std::vector<int> undefinedPolyUseLines;

  private:
    LexicalAnalyzer lexer;
    void syntax_error();
    Token expect(TokenType expected_type);
    std::vector<PolyHeaderInfo> polyHeaders;
    std::vector<int> duplicateLines;
    std::vector<std::string> currentPolyParams;
    std::vector<int> invalidMonomialLines; // To record line numbers for invalid monomial names.
    

};

#endif