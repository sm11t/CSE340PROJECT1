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

// Represents a term: an optional coefficient followed by one or more monomials
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

// ----------------- Parser Class ---------------------
class Parser {
  public:
    Parser();
    void ConsumeAllInput();
    void parse_input();
    void parse_program();
    void parse_tasks_section();

    void parse_poly_section();
    void parse_poly_decl_list();
    void parse_poly_decl();
    PolyHeader parse_poly_header();
    std::vector<std::string> parse_id_list();
    PolyBody parse_poly_body();

    // Expression parsing functions
    Term parse_term();
    std::vector<Monomial> parse_monomial_list();
    Monomial parse_monomial();
    Primary parse_primary();

    void printPolyDeclarations();

    bool isTaskSelected(int taskNumber);
    bool tasks[7];

  private:
    LexicalAnalyzer lexer;
    void syntax_error();
    Token expect(TokenType expected_type);
    std::vector<PolyDecl> polyDeclarations;
};

// ---------- Helper functions for converting the expression to string ----------
std::string primaryToString(const Primary& prim);
std::string monomialToString(const Monomial& mono);
std::string termToString(const Term& term);
std::string polyBodyToString(const PolyBody& body);

#endif
