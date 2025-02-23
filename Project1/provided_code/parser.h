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

enum StatementType {
    STMT_INPUT,
    STMT_OUTPUT,
    STMT_ASSIGN
};

// Structure to represent a polynomial evaluation.
struct PolyEval {
    std::string polyName;           // Name of the polynomial to evaluate.
    std::vector<std::string> args;  // For simplicity, store each argument as a string.
    // (Later you can extend this to distinguish numbers, IDs, or nested evaluations.)
};

// Structure to represent an execution statement.
struct Statement {
    StatementType type;   // INPUT, OUTPUT, or ASSIGN.
    std::string variable; // For INPUT/OUTPUT or the left-hand side in ASSIGN.
    PolyEval* polyEval;   // Only used for assignment statements.
    int line_no;          // For error reporting (optional).
};

std::vector<Statement*> executeStatements;

#include <unordered_map>
std::unordered_map<std::string, int> symbolTable;

const int MEM_SIZE = 1000;
int mem[MEM_SIZE] = {0}; 

int nextAvailable = 0;





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
    void poly_evaluation(PolyEval*);
    int argument_list(PolyEval*);
    void argument(PolyEval*);
    void inputs_section();

    int allocateVariable(const std::string &varName);
    void execute_program();
    int evaluate_polynomial(PolyEval* polyEval);

    bool tasks[7];

    std::unordered_set<std::string> declaredPolynomials;    //USING FOR ERROR CODE
    std::vector<int> undefinedPolyUseLines;                 //USING FOR ERROR CODE
    std::vector<int> wrongArgCountLines;                    //USING FOR ERROR CODE

    

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