#!/bin/sh
flex -o lexer.c workflow.l
bison -d -v -Werror -Wall --report=all -o parser.c workflow.y
