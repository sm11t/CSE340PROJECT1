/*
 * Copyright (C) Rida Bazzi, 2019
 *
 * Do not share this file with anyone
 */
#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include "lexer.h"

class Parser {
  public:
    Parser();
    void ConsumeAllInput();
    void parse_input();
    void parse_program();
    void parse_tasks_section();

    bool isTaskSelected(int taskNumber);
    bool tasks[7];

  private:
    LexicalAnalyzer lexer;
    void syntax_error();
    Token expect(TokenType expected_type);
};

#endif

