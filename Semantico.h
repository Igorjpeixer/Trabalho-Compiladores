#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "Token.h"
#include "SemanticError.h"
#include "SemanticTable.h"

#include <string>
#include <vector>

// Modalidades de identificador
enum Modalidade {
    VARIAVEL,
    VETOR,
    PARAMETRO,
    PARAMETRO_VETOR,
    FUNCAO
};

// Tipo de um operando na pilha de geracao de codigo
enum KindOperando {
    K_IMM,   // literal inteiro (immediate)
    K_MEM,   // endereco (variavel ou temporario)
    K_ACUM,  // valor que esta atualmente no ACC
    K_REL    // resultado de comparacao (ACC = esq - dir), guarda o operador
};

// Codigos dos operadores relacionais (para escolher o desvio)
enum RelOp {
    R_GT,   // >
    R_LT,   // <
    R_GE,   // >=
    R_LE,   // <=
    R_EQ,   // ==
    R_NE    // !=
};

// Operadores aritmeticos/bit-a-bit com mnemonico no BIP
enum AritOp {
    A_ADD,  // +  -> ADD / ADDI
    A_SUB,  // -  -> SUB / SUBI
    A_AND,  // &  -> AND / ANDI
    A_OR,   // |  -> OR  / ORI
    A_XOR   // ^  -> XOR / XORI
};

// Simbolo na tabela
struct Simbolo {
    std::string nome;
    std::string nomeAsm;   // nome no assembly (com prefixo da funcao, se aplicavel)
    int tipo;
    Modalidade modalidade;
    std::string escopo;
    bool inicializada;
    bool usada;
    int linha;
    int tamanhoVetor;
};

// Operando da pilha de avaliacao de expressao
struct Operando {
    int kind;             // K_IMM, K_MEM, K_ACUM, K_REL
    std::string valor;    // IMM: literal decimal | MEM: nome asm do endereco
    int relOp;            // valido se kind == K_REL
    int tipo;             // tipo do valor (para checagem de parametros)
};

// Contexto de uma chamada de funcao em andamento (suporta aninhamento)
struct CallCtx {
    std::string nomeFunc;                // nome do usuario
    std::string rotulo;                  // _PRINCIPAL ou _NOME
    std::vector<std::string> paramAsm;   // nomes asm dos parametros, em ordem
    std::vector<int> paramTipo;          // tipos dos parametros
    int numArgs;                         // quantos argumentos ja passaram
    int tipoRetorno;
    int pos;
    bool existe;
};

// Contexto de uma estrutura de controle aberta (suporta aninhamento)
struct CtxControle {
    std::string lFalso;   // se: alvo quando condicao falsa
    std::string lFim;     // rotulo de fim
    std::string lIni;     // inicio do laco (while/do/for)
    bool teveElse;        // se_senao
    // for: incremento e' bufferizado e reemitido depois do corpo
    int marcaIncr;                       // posicao no codigoAsm onde o incremento comeca
    std::vector<std::string> codIncr;    // linhas do incremento guardadas
};

class Semantico
{
public:
    Semantico();

    void executeAction(int action, const Token *token);
    void reset();

    const std::vector<Simbolo>& getTabela() const { return tabela; }
    const std::vector<std::string>& getMensagens() const { return mensagens; }
    bool temErros() const { return houveErro; }

    std::string getCodigoAsm() const;

private:
    // Tabela de simbolos e escopos
    std::vector<Simbolo> tabela;
    std::vector<std::string> pilhaEscopos;
    int contadorBloco;

    // Estado atual
    int tipoAtual;
    std::string nomeFuncaoAtual;
    std::string ultimoIdAtribuicao;
    std::string ultimoLiteralInt;   // so para deduzir tamanho de vetor

    // ===== GERACAO DE CODIGO (T5) =====
    std::vector<std::string> codigoAsm;
    std::vector<Operando> pilhaOperandos;
    std::vector<CtxControle> pilhaCtrl;
    int contadorLabel;              // gera R1, R2, ...
    int maxTempNum;                 // maior temp usado (temp1..tempN) para a .data
    int proxSpill;                  // proximo temp de "derramamento" (>=3), reinicia por comando
    bool emEscreva;
    bool emLeia;

    // ===== FUNCOES (T6) =====
    std::vector<CallCtx> pilhaChamadas;   // chamadas em andamento (aninhamento)
    bool funcAtualEhMain;                 // a funcao sendo definida e main/principal?

    // Mensagens
    std::vector<std::string> mensagens;
    bool houveErro;

    // Escopo
    void abreEscopo(const std::string &nome);
    void fechaEscopo();
    std::string escopoAtual() const;

    // Tabela
    bool jaDeclaradoNoEscopo(const std::string &nome);
    Simbolo* buscaSimbolo(const std::string &nome);
    void inserirSimbolo(const std::string &nome, int tipo, Modalidade mod, int linha);
    bool ehSimboloInt(const std::string &nome);

    // Geracao de codigo
    void emite(const std::string &instrucao);
    void gerarDataSection();
    void limpaOperandos();

    void pushImm(const std::string &valor);
    void pushMem(const std::string &nome);
    void carregaNoAcc(const Operando &op);      // LD / LDI (derrama ACUM vivo antes)
    void aplica(int aritOp, const Operando &op);// ADD/ADDI/SUB/...
    void derramaAcum();                         // salva um ACUM vivo num temp
    std::string mnemMem(int aritOp);
    std::string mnemImed(int aritOp);
    std::string novoTempSpill();
    void marcaTemp(int n);

    void gerarBinaria(int aritOp);
    void gerarRelacional(int relOp);
    void gerarUnarioMenos();
    void emitirDesvioCondicao(const std::string &label, bool desviarSeFalso);
    std::string novoLabel();

    // Funcoes (T6)
    bool ehEntrada(const std::string &nome) const;   // main ou principal
    std::string calcNomeAsm(const std::string &nome);// nome asm no contexto atual
    std::string nomeAsmDe(const std::string &userName);
    std::string rotuloDe(const std::string &funcNome);// _PRINCIPAL / _NOME

    // Mensagens
    void avisoNaoUsadas();
    void erro(const std::string &msg, int pos);
    void aviso(const std::string &msg, int pos);
};

#endif
