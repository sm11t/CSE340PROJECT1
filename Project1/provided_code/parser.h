#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "lexer.h"

enum StatementType {
    STMT_INPUT,
    STMT_OUTPUT,
    STMT_ASSIGN
};

enum ADDOP {
    ADDOP_NONE,
    ADDOP_PLUS,
    ADDOP_MINUS
};

struct PrimaryNode;
struct MonomialNode;
struct TermNode;

/**
 * For polynomial representation:
 * A polynomial is a linked list of TermNode.
 */
struct TermNode {
    int coefficient;
    MonomialNode* monomials; // chain of factors in this term
    ADDOP addop;             // how this term combines with the next
    TermNode* next;          // pointer to the next term
};

struct MonomialNode {
    PrimaryNode* primary;
    int exponent;
    MonomialNode* next;
};

struct PrimaryNode {
    bool isParen;          // false => varName used, true => subTermList used
    std::string varName;
    TermNode* subTermList; // used if isParen == true
};

/**
 * For the EXECUTE section statements:
 */
struct PolyEval {
    std::string polyName;
    std::vector<std::string> args;        // arguments that are either ID or NUM
    std::vector<PolyEval*> nestedArgs;    // nested polynomial calls
};

struct Statement {
    StatementType type;
    std::string variable;  // e.g. "X" for INPUT X; or LHS for assignments; or var in OUTPUT
    PolyEval* polyEval;    // used only if type == STMT_ASSIGN
    int line_no;
};

// Global data structures for the entire program:
extern std::vector<Statement*> executeStatements;
extern std::unordered_map<std::string,int> symbolTable;
extern int mem[1000];
extern int nextAvailable;

/**
 * We store each declared polynomial's header and AST in PolyHeaderInfo.
 */
struct PolyHeaderInfo {
    std::string name;
    int line_no;
    std::vector<std::string> paramNames;  // e.g. (x, y, z)
    int degree;
    TermNode* astRoot;                    // top-level AST pointer
};

class Parser {
public:
    Parser();
    void input();

    // Grammar rules
    void program();
    void tasks_section();
    void tasknum_list();
    void poly_section();
    void poly_decl_list();
    void poly_decl();
    void poly_header();
    Token poly_name();
    std::vector<std::string> id_list();

    int parsePolyBody();
    TermNode* parseTermListAST(int &degreeOut);
    TermNode* parseTermAST(int &maxDeg);
    MonomialNode* parseMonomialListAST(int &totalDeg);
    MonomialNode* parseMonomialAST(int &degOut);
    PrimaryNode* parsePrimaryAST(int &degOut);

    void execute_section();
    void statement_list();
    void statement();
    void input_statement();
    void output_statement();
    void assign_statement();

    void poly_evaluation(PolyEval* pe);
    int argument_list(PolyEval* pe);
    void argument(PolyEval* pe);

    void inputs_section();
    void inputnum_list();

    int allocateVariable(const std::string &varName);

    void execute_program();
    int evaluate_polynomial(PolyEval* pe);

    int evalPolyAST(TermNode* termList,
                    const std::vector<int> &argValues,
                    const std::unordered_map<std::string,int> &paramMap);

    int evalTermNode(TermNode* t,
                     const std::vector<int> &argValues,
                     const std::unordered_map<std::string,int> &paramMap);

    int evalMonomialList(MonomialNode* mono,
                         const std::vector<int> &argValues,
                         const std::unordered_map<std::string,int> &paramMap);

    int evalPrimary(PrimaryNode* prim,
                    const std::vector<int> &argValues,
                    const std::unordered_map<std::string,int> &paramMap);

    // Task 3 and Task 4
    void DetectUninitializedVars();
    void DetectUselessAssignments();

    bool tasks[7]; 

    // For semantic checks (Task 1)
    std::unordered_set<std::string> declaredPolynomials;
    std::vector<int> undefinedPolyUseLines;
    std::vector<int> wrongArgCountLines;
    std::vector<int> duplicateLines;
    std::vector<int> invalidMonomialLines;

private:
    LexicalAnalyzer lexer;
    Token expect(TokenType expected_type);
    void syntax_error();

    // Polynomials
    std::vector<PolyHeaderInfo> polyHeaders;
    std::vector<std::string> currentPolyParams;

    // We store input numbers from INPUTS here
    std::vector<int> inputValues;
    int inputIndex;
};

#endif
