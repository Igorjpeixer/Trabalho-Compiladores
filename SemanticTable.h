#ifndef SEMANTICTABLE_H
#define SEMANTICTABLE_H

#include <string>

// Codigos de tipos
#define INT 0
#define CHA 1
#define FLO 2
#define STR 3
#define BOO 4
#define VOI 5
#define ERR 6
#define IND 7  // tipo Indefinido (resultado de comparacao -> bool)

// Codigos de operacoes
#define SUM 0  // +
#define SUB 1  // -
#define MUL 2  // *
#define DIV 3  // /
#define MOD 4  // %
#define REL 5  // > < >= <= == !=
#define AND 6  // &&
#define OR  7  // ||
#define BIT 8  // & | ^ ~ << >>
#define NEG 9  // ! - ~ unario

// Tabela de Compatibilidade de OPERACOES
// resultType[op][tipo1][tipo2] -> tipo do resultado
//
// Linhas/colunas: INT(0), CHA(1), FLO(2), STR(3), BOO(4), VOI(5), ERR(6), IND(7)

class SemanticTable {
public:
    static int resultType(int op, int t1, int t2) {
        // se algum dos lados ja eh ERR, propaga ERR
        if (t1 == ERR || t2 == ERR) return ERR;

        switch (op) {
            case SUM: case SUB: case MUL: case DIV:
                // INT op INT = INT
                if (t1 == INT && t2 == INT) return INT;
                // FLO op FLO = FLO
                if (t1 == FLO && t2 == FLO) return FLO;
                // INT op FLO = FLO (promocao)
                if ((t1 == INT && t2 == FLO) || (t1 == FLO && t2 == INT)) return FLO;
                // CHAR op CHAR = CHAR (concatenacao simples)
                if (op == SUM && t1 == CHA && t2 == CHA) return STR;
                // STRING + STRING = STRING (concatenacao)
                if (op == SUM && t1 == STR && t2 == STR) return STR;
                // STRING + CHAR = STRING
                if (op == SUM && ((t1 == STR && t2 == CHA) || (t1 == CHA && t2 == STR))) return STR;
                return ERR;

            case MOD:
                // % so funciona com INT
                if (t1 == INT && t2 == INT) return INT;
                return ERR;

            case REL:
                // comparacoes de mesmo tipo numerico ou char retornam BOO
                if ((t1 == INT || t1 == FLO || t1 == CHA) &&
                    (t2 == INT || t2 == FLO || t2 == CHA)) return BOO;
                if (t1 == BOO && t2 == BOO) return BOO;
                if (t1 == STR && t2 == STR) return BOO;
                return ERR;

            case AND: case OR:
                // logicos so com BOO
                if (t1 == BOO && t2 == BOO) return BOO;
                return ERR;

            case BIT:
                // bit a bit so com INT
                if (t1 == INT && t2 == INT) return INT;
                return ERR;

            case NEG:
                // - INT = INT, - FLO = FLO, ! BOO = BOO, ~ INT = INT
                return t1;
        }
        return ERR;
    }

    // Tabela de Compatibilidade de ATRIBUICOES
    // attribType[esquerda][direita] -> "ok", "erro", "aviso"
    static std::string attribType(int dest, int src) {
        if (dest == ERR || src == ERR) return "erro";

        // mesmo tipo sempre ok
        if (dest == src) return "ok";

        // INT recebe FLO -> aviso (truncamento)
        if (dest == INT && src == FLO) return "aviso_trunc";
        // FLO recebe INT -> ok (promocao)
        if (dest == FLO && src == INT) return "ok";
        // INT recebe CHA -> aviso (conversao)
        if (dest == INT && src == CHA) return "aviso_conv";
        // CHA recebe INT -> aviso (conversao)
        if (dest == CHA && src == INT) return "aviso_conv";

        return "erro";
    }

    // Converte codigo de tipo em string
    static std::string tipoNome(int t) {
        switch (t) {
            case INT: return "int";
            case CHA: return "char";
            case FLO: return "float";
            case STR: return "string";
            case BOO: return "bool";
            case VOI: return "void";
            case ERR: return "erro";
            case IND: return "indefinido";
        }
        return "?";
    }

    // Converte nome do tipo (lexema) em codigo
    static int tipoCodigo(const std::string &nome) {
        if (nome == "int") return INT;
        if (nome == "char") return CHA;
        if (nome == "float") return FLO;
        if (nome == "string") return STR;
        if (nome == "bool") return BOO;
        if (nome == "void") return VOI;
        return ERR;
    }
};

#endif
