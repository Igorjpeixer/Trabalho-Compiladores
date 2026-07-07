#ifndef CONSTANTS_H
#define CONSTANTS_H

enum TokenId 
{
    EPSILON  = 0,
    DOLLAR   = 1,
    t_LITERAL_INT = 2,
    t_LITERAL_BIN = 3,
    t_LITERAL_HEX = 4,
    t_LITERAL_FLOAT = 5,
    t_LITERAL_CHAR = 6,
    t_LITERAL_STR = 7,
    t_SE = 8,
    t_SENAO = 9,
    t_ENQUANTO = 10,
    t_PARA = 11,
    t_PARA_CADA = 12,
    t_RETORNE = 13,
    t_INTEIRO = 14,
    t_REAL = 15,
    t_BOOLEANO = 16,
    t_CARACTER = 17,
    t_STRING = 18,
    t_VAZIO = 19,
    t_VERDADE = 20,
    t_FALSO = 21,
    t_PRINCIPAL = 22,
    t_ESCREVA = 23,
    t_LEIA = 24,
    t_FIM = 25,
    t_FACA = 26,
    t_ID = 27,
    t_TOKEN_28 = 28, //"="
    t_TOKEN_29 = 29, //"+"
    t_TOKEN_30 = 30, //"-"
    t_TOKEN_31 = 31, //"*"
    t_TOKEN_32 = 32, //"/"
    t_TOKEN_33 = 33, //"%"
    t_TOKEN_34 = 34, //">="
    t_TOKEN_35 = 35, //"<="
    t_TOKEN_36 = 36, //"=="
    t_TOKEN_37 = 37, //"!="
    t_TOKEN_38 = 38, //">"
    t_TOKEN_39 = 39, //"<"
    t_TOKEN_40 = 40, //"&&"
    t_TOKEN_41 = 41, //"||"
    t_TOKEN_42 = 42, //"!"
    t_TOKEN_43 = 43, //">>"
    t_TOKEN_44 = 44, //"<<"
    t_TOKEN_45 = 45, //"&"
    t_TOKEN_46 = 46, //"|"
    t_TOKEN_47 = 47, //"~"
    t_TOKEN_48 = 48, //"^"
    t_TOKEN_49 = 49, //"."
    t_TOKEN_50 = 50, //";"
    t_TOKEN_51 = 51, //":"
    t_TOKEN_52 = 52, //","
    t_TOKEN_53 = 53, //"("
    t_TOKEN_54 = 54, //")"
    t_TOKEN_55 = 55, //"["
    t_TOKEN_56 = 56, //"]"
    t_TOKEN_57 = 57, //"{"
    t_TOKEN_58 = 58, //"}
};

const int STATES_COUNT = 137;

extern int SCANNER_TABLE[STATES_COUNT][256];

extern int TOKEN_STATE[STATES_COUNT];

extern const char *SCANNER_ERROR[STATES_COUNT];

const int FIRST_SEMANTIC_ACTION = 96;

const int SHIFT  = 0;
const int REDUCE = 1;
const int ACTION = 2;
const int ACCEPT = 3;
const int GO_TO  = 4;
const int ERROR  = 5;

extern const int PARSER_TABLE[268][150][2];

extern const int PRODUCTIONS[98][2];

extern const char *PARSER_ERROR[268];

#endif
