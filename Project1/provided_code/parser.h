#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include "lexer.h"
#include <unordered_set>
#include <vector>
#include <unordered_map>

struct PolyHeaderInfo {
    std::string name;               
    int line_no;                    
    std::vector<std::string> paramNames;  
};

enum StmtType { STMT_INPUT, STMT_OUTPUT, STMT_ASSIGN };

struct Statement {
    StmtType type;          // INPUT, OUTPUT, or ASSIGN.
    std::string var;        // For INPUT/OUTPUT: the variable name; for assignment, the LHS.
    Statement* next;        // Pointer to the next statement in the list.
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
    void executeInputStatements();
    void poly_evaluation();
    int argument_list();
    void argument();
    void inputs_section();

    bool tasks[7];

    std::unordered_set<std::string> declaredPolynomials;    // For semantic error checking.
    std::vector<int> undefinedPolyUseLines;                 
    std::vector<int> wrongArgCountLines;                    

    // TASK 2 – Runtime Data Structures:
    std::unordered_map<std::string, int> symbolTable; // maps variable names to memory locations.
    int nextAvailable;                                // next available memory location.
    Statement* stmtList;                              // linked list of statements.

    // NEW: Memory array for variables and input storage.
    std::vector<int> mem;         // e.g., 1000 slots, all initialized to 0.
    std::vector<int> inputValues; // Stores the numbers from the INPUTS section.

  private:
    LexicalAnalyzer lexer;
    void syntax_error();
    Token expect(TokenType expected_type);
    std::vector<PolyHeaderInfo> polyHeaders;
    std::vector<int> duplicateLines;
    std::vector<std::string> currentPolyParams;
    std::vector<int> invalidMonomialLines; 
};

#endif