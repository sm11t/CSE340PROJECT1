/*
 * Copyright (C) Rida Bazzi, 2020
 *
 * Do not share this file with anyone
 *
 * Do not post this file or derivatives of
 * of this file online
 *
 */
#include <iostream>
#include <cstdlib>
#include "parser.h"

using namespace std;

void Parser::syntax_error()
{
    cout << "SYNTAX ERROR !!!!!&%!!\n";
    exit(1);
}

// this function gets a token and checks if it is
// of the expected type. If it is, the token is
// returned, otherwise, synatx_error() is generated
// this function is particularly useful to match
// terminals in a right hand side of a rule.
// Written by Mohsen Zohrevandi
Token Parser::expect(TokenType expected_type)
{
    Token t = lexer.GetToken();
    if (t.token_type != expected_type)
        syntax_error();
    return t;
}

// Parsing

// This function is simply to illustrate the GetToken() function
// you will not need it for your project and you can delete it
// the function also illustrates the use of peek()
void Parser::ConsumeAllInput()
{
    Token token;
    int i = 1;

    token = lexer.peek(i);
    token.Print();
    while (token.token_type != END_OF_FILE)
    {
        i = i + 1;
        token = lexer.peek(i);
        token.Print();
    }

    token = lexer.GetToken();
    token.Print();
    while (token.token_type != END_OF_FILE)
    {
        token = lexer.GetToken();
        token.Print();
    }
}

// *******************FOR STORING TASKS******************************
Parser::Parser() {      //initialising all tasks to off or false
    for (int i = 0; i < 7; i++) {
        tasks[i] = false;
    }
}

bool Parser::isTaskSelected(int taskNumber) {
    if (taskNumber >= 1 && taskNumber <= 6) {
        return tasks[taskNumber];
    }
    return false;
}
// *******************FOR STORING TASKS END*****************************
void Parser::parse_input()
{
    parse_program();
    expect(END_OF_FILE);
}

void Parser::parse_program()
{
    parse_tasks_section();
}

void Parser::parse_tasks_section()
{
    expect(TASKS);
    while (lexer.peek(1).token_type == NUM){
        Token t = lexer.GetToken();
        int task_num = std::stoi(t.lexeme);

        if(task_num >= 1 && task_num <= 6){
            tasks[task_num] = true;
        }
        else{
            syntax_error();
        }
    }
}



int main()
{
    // note: the parser class has a lexer object instantiated in it. You should not be declaring
    // a separate lexer object. You can access the lexer object in the parser functions as shown in the
    // example method Parser::ConsumeAllInput
    // If you declare another lexer object, lexical analysis will not work correctly
    Parser parser;

    parser.parse_program();
}
