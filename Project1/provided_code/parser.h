/*
 * Copyright (C) Rida Bazzi, 2019
 *
 * Do not share this file with anyone
 */
#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include <vector>
#include "lexer.h"

// ---------- Data Structures for POLY Section ----------

// Represents a polynomial header (name and parameters)
struct PolyHeader {
    std::string name;                  // e.g. "F"
    std::vector<std::string> params;   // e.g. {"x", "y"}
};

// Forward declaration for PolyBody (used in Primary)
struct PolyBody;

// Represents a primary expression
struct Primary {
    enum Type { VAR, EXPR } type;      // either a variable or a parenthesized expression
    std::string var;                   // if type == VAR
    PolyBody* group;                   // if type == EXPR (points to the inner expression)
};

// Represents a monomial: a primary possibly raised to an exponent
struct Monomial {
    Primary primary;
    int exponent;                      // default is 1 if omitted
};

// Represents a term: an optional coefficient followed by a monomial list
struct Term {
    int coefficient;                   // default is 1 if not specified
    std::vector<Monomial> monomials;
};

// Represents a polynomial body (i.e. a term list)
struct PolyBody {
    std::vector<Term> terms;
};

// Represents a complete polynomial declaration.
struct PolyDecl {
    PolyHeader header;
    PolyBody body;
};

// ---------- Data Structures for EXECUTE Section ----------

enum StmtType { INPUT_STMT, OUTPUT_STMT, ASSIGN_STMT };

struct Statement {
    StmtType stmt_type;
    std::string var;       // For INPUT, OUTPUT, or the LHS of an assignment.
    // For assignments, we store the parsed poly_evaluation as a PolyBody.
    PolyBody poly_eval;
};

typedef std::vector<Statement> StmtList;

// ----------------- Parser Class ---------------------
class Parser {
  public:
    Parser();
    void ConsumeAllInput();
    void parse_input();
    void parse_program();
    void parse_tasks_section();
    void parse_poly_section();
    void parse_execute_section();
    void parse_inputs_section();

    // POLY Section functions
    void parse_poly_decl_list();
    void parse_poly_decl();
    PolyHeader parse_poly_header();
    std::vector<std::string> parse_id_list();
    PolyBody parse_poly_body();

    // Expression parsing functions (for polynomial bodies)
    Term parse_term();
    std::vector<Monomial> parse_monomial_list();
    Monomial parse_monomial();
    Primary parse_primary();

    // EXECUTE Section functions
    void parse_statement_list();
    void parse_input_statement();
    void parse_output_statement();
    void parse_assign_statement();
    PolyBody parse_poly_evaluation(); 
    void parse_argument_list();
    void parse_argument();

    // INPUTS Section function
    void parse_num_list();

    // Debug printing functions
    void printPolyDeclarations();
    void printFullInput();

    bool isTaskSelected(int taskNumber);
    bool tasks[7];

    // Storage for EXECUTE and INPUTS sections:
    StmtList statements;
    std::vector<int> inputs;

  private:
    LexicalAnalyzer lexer;
    void syntax_error();
    Token expect(TokenType expected_type);
    std::vector<PolyDecl> polyDeclarations;
};

// ----------Helper functions----------
std::string primaryToString(const Primary& prim);
std::string monomialToString(const Monomial& mono);
std::string termToString(const Term& term);
std::string polyBodyToString(const PolyBody& body);

#endif
