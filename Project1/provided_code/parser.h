#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "lexer.h"

// The struct that stores info about each declared polynomial.
// We store a 'degree' field for Task 5.
struct PolyHeaderInfo {
    std::string name;               
    int line_no;                    
    std::vector<std::string> paramNames;  
    int degree;  // Computed degree for Task 5
};

// For the EXECUTE section:
enum StatementType {
    STMT_INPUT,
    STMT_OUTPUT,
    STMT_ASSIGN
};

struct PolyEval {
    std::string polyName;
    std::vector<std::string> args;        // simple arguments (IDs or NUMs)
    std::vector<PolyEval*> nestedArgs;    // nested polynomial evaluations
};

struct Statement {
    StatementType type;
    std::string variable;  // LHS for assignment, or var used by INPUT/OUTPUT
    PolyEval* polyEval;    // only for STMT_ASSIGN
    int line_no;           // line number for error/warning reporting
};

// We'll store all statements from the EXECUTE section here.
extern std::vector<Statement*> executeStatements;

// Symbol table mapping variable name -> location index
extern std::unordered_map<std::string,int> symbolTable;
extern int mem[1000];
extern int nextAvailable;

class Parser {
public:
    Parser();
    void input();            // parse the entire input program (program + EOF)
    void ConsumeAllInput();  // prints out the remaining tokens (for debugging)

    // Grammar
    void program();
    void tasks_section();
    void tasknum_list();
    void poly_section();
    void poly_decl_list();
    void poly_decl();
    void poly_header();
    Token poly_name();
    std::vector<std::string> id_list();

    // Single-pass parse of polynomial bodies (also computing degree)
    int parsePolyBody();     
    int parseTermList();     
    int parseTerm();         
    int parseMonomialList(); 
    int parseMonomial();     
    int parsePrimary();      

    // EXECUTE section
    void execute_section();
    void statement_list();
    void statement();
    void input_statement();
    void output_statement();
    void assign_statement();
    void inputs_section();
    void inputnum_list();
    
    // For a polynomial evaluation in assignment's RHS
    void poly_evaluation(PolyEval*);
    int argument_list(PolyEval*);
    void argument(PolyEval*);

    // Memory, execution stubs
    int allocateVariable(const std::string &varName);
    void execute_program();
    int evaluate_polynomial(PolyEval*);

    // Task 4
    void DetectUselessAssignments();
    // Task 3
    void DetectUninitializedVars();

    // We also store tasks
    bool tasks[7]; // tasks[i] is true if Task i is requested

    // For semantic checks (Task 1)
    std::unordered_set<std::string> declaredPolynomials;
    std::vector<int> undefinedPolyUseLines; 
    std::vector<int> wrongArgCountLines;
    std::vector<int> duplicateLines;
    std::vector<int> invalidMonomialLines;

private:
    LexicalAnalyzer lexer;
    void syntax_error();
    Token expect(TokenType expected_type);

    // We'll store polynomial headers (name, param list, degree, line no, etc.)
    std::vector<PolyHeaderInfo> polyHeaders;

    // For checking invalid monomial names inside the current polynomial:
    std::vector<std::string> currentPolyParams;
};

#endif