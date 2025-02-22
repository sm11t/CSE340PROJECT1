#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include "lexer.h"
#include <unordered_set>
#include <vector>
#include <unordered_map>

// Structure for polynomial header info.
struct PolyHeaderInfo {
    std::string name;               
    int line_no;                    
    std::vector<std::string> paramNames;  
    std::vector<Term> body;
};

// Enumeration for execution statement types.
enum StatementType {
    STMT_INPUT,
    STMT_OUTPUT,
    STMT_ASSIGN
};

// Structure representing an execution statement.
struct Statement {
    StatementType type;       // INPUT, OUTPUT, or ASSIGNMENT.
    std::string var;          // Variable name (for INPUT/OUTPUT or left-hand side of assignment).
    // For assignment statements, this will eventually point to a structure representing the polynomial evaluation.
    // For now, we use a void pointer as a placeholder.
    void* polyEval;           
    int line_no;              // Line number (for error reporting if needed).
};

struct Term {
    int coefficient;
    bool isVariable;  // True if this term represents a variable (e.g., "x")
    std::string var;  // The variable name (if applicable)
    int exponent;     // Exponent for the variable (default 1)
};

struct PolyEvalData {
    std::string polyName; // The name of the polynomial being evaluated
    int argValue;         // The value of the single argument for univariate polynomials
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
    int argument_list();
    void argument();
    void inputs_section();

    bool tasks[7];
    std::unordered_set<std::string> declaredPolynomials;
    std::vector<int> undefinedPolyUseLines;
    std::vector<int> wrongArgCountLines;

    // ***** New Execution Data Structures for Task 2 *****
    std::vector<Statement*> stmtList;                    // List of execution statements.
    std::unordered_map<std::string, int> symbolTable;    // Maps variable names to memory locations.
    int mem[1000];                                       // Memory array to hold variable values.
    int nextMemLocation;                                 // Next available memory slot.
    std::vector<int> inputs;                             // List of input numbers from the INPUTS section.
    int nextInput;                                       // Index for the next input value.

    Term parse_term();
    std::vector<Term> parse_term_list();
    PolyEvalData parse_poly_evaluation();
    int evaluate_polynomial(const PolyHeaderInfo &poly, int arg);

    // Function to execute the program (Task 2).
    void execute_program();

  private:
    LexicalAnalyzer lexer;
    void syntax_error();
    Token expect(TokenType expected_type);
    std::vector<PolyHeaderInfo> polyHeaders;
    std::vector<int> duplicateLines;
    std::vector<std::string> currentPolyParams;
    std::vector<int> invalidMonomialLines; // For invalid monomial names.
};

#endif
