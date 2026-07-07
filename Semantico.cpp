#include "Semantico.h"
#include "Constants.h"

#include <iostream>
#include <sstream>

// =============================================================
// CONSTRUTOR e RESET
// =============================================================
Semantico::Semantico() {
    reset();
}

void Semantico::reset() {
    tabela.clear();
    pilhaEscopos.clear();
    mensagens.clear();
    codigoAsm.clear();
    pilhaOperandos.clear();
    pilhaCtrl.clear();
    pilhaEscopos.push_back("global");
    contadorBloco = 0;
    tipoAtual = ERR;
    nomeFuncaoAtual = "";
    ultimoIdAtribuicao = "";
    ultimoLiteralInt = "";
    contadorLabel = 0;
    maxTempNum = 0;
    proxSpill = 3;
    emEscreva = false;
    emLeia = false;
    houveErro = false;
    pilhaChamadas.clear();
    funcAtualEhMain = false;
}

// FUNCOES (T6)
static std::string paraMaiusc(const std::string &s) {
    std::string r = s;
    for (size_t i = 0; i < r.size(); i++)
        if (r[i] >= 'a' && r[i] <= 'z') r[i] = char(r[i] - 'a' + 'A');
    return r;
}

bool Semantico::ehEntrada(const std::string &nome) const {
    return nome == "main" || nome == "principal";
}

std::string Semantico::calcNomeAsm(const std::string &nome) {
    if (!nomeFuncaoAtual.empty() && !ehEntrada(nomeFuncaoAtual))
        return nomeFuncaoAtual + "_" + nome;
    return nome;
}

std::string Semantico::nomeAsmDe(const std::string &userName) {
    Simbolo *s = buscaSimbolo(userName);
    if (s != nullptr && !s->nomeAsm.empty()) return s->nomeAsm;
    return userName;
}

std::string Semantico::rotuloDe(const std::string &funcNome) {
    if (ehEntrada(funcNome)) return "_PRINCIPAL";
    return "_" + paraMaiusc(funcNome);
}

// ESCOPOS
void Semantico::abreEscopo(const std::string &nome) { pilhaEscopos.push_back(nome); }
void Semantico::fechaEscopo() { if (pilhaEscopos.size() > 1) pilhaEscopos.pop_back(); }
std::string Semantico::escopoAtual() const { return pilhaEscopos.back(); }

// TABELA DE SIMBOLOS
bool Semantico::jaDeclaradoNoEscopo(const std::string &nome) {
    std::string esc = escopoAtual();
    for (size_t i = 0; i < tabela.size(); i++)
        if (tabela[i].nome == nome && tabela[i].escopo == esc) return true;
    return false;
}

Simbolo* Semantico::buscaSimbolo(const std::string &nome) {
    for (int i = (int)pilhaEscopos.size() - 1; i >= 0; i--) {
        const std::string &esc = pilhaEscopos[i];
        for (size_t j = 0; j < tabela.size(); j++)
            if (tabela[j].nome == nome && tabela[j].escopo == esc) return &tabela[j];
    }
    for (size_t j = 0; j < tabela.size(); j++)
        if (tabela[j].nome == nome && tabela[j].escopo == "global"
            && tabela[j].modalidade == FUNCAO) return &tabela[j];
    return nullptr;
}

void Semantico::inserirSimbolo(const std::string &nome, int tipo, Modalidade mod, int linha) {
    Simbolo s;
    s.nome = nome; s.tipo = tipo; s.modalidade = mod;
    s.escopo = escopoAtual();
    s.inicializada = (mod == PARAMETRO || mod == PARAMETRO_VETOR);
    s.usada = false; s.linha = linha; s.tamanhoVetor = 0;
    s.nomeAsm = (mod == FUNCAO) ? nome : calcNomeAsm(nome);
    tabela.push_back(s);
}

bool Semantico::ehSimboloInt(const std::string &nome) {
    Simbolo *s = buscaSimbolo(nome);
    return s != nullptr && s->tipo == INT;
}

// GERACAO DE CODIGO
void Semantico::emite(const std::string &instrucao) { codigoAsm.push_back(instrucao); }

void Semantico::limpaOperandos() {
    pilhaOperandos.clear();
    proxSpill = 3;
}

void Semantico::pushImm(const std::string &valor) {
    Operando op; op.kind = K_IMM; op.valor = valor; op.relOp = 0; op.tipo = INT;
    pilhaOperandos.push_back(op);
}

void Semantico::pushMem(const std::string &nome) {
    Operando op; op.kind = K_MEM; op.valor = nome; op.relOp = 0; op.tipo = INT;
    pilhaOperandos.push_back(op);
}

void Semantico::marcaTemp(int n) { if (n > maxTempNum) maxTempNum = n; }

std::string Semantico::novoTempSpill() {
    int n = proxSpill++;
    marcaTemp(n);
    return "temp" + std::to_string(n);
}

void Semantico::derramaAcum() {
    for (size_t i = 0; i < pilhaOperandos.size(); i++) {
        if (pilhaOperandos[i].kind == K_ACUM) {
            std::string t = novoTempSpill();
            emite("STO " + t);
            pilhaOperandos[i].kind = K_MEM;
            pilhaOperandos[i].valor = t;
            return;
        }
    }
}

void Semantico::carregaNoAcc(const Operando &op) {
    if (op.kind == K_ACUM) return;
    derramaAcum();
    if (op.kind == K_IMM) emite("LDI " + op.valor);
    else                  emite("LD " + op.valor);
}

std::string Semantico::mnemMem(int aritOp) {
    switch (aritOp) {
        case A_ADD: return "ADD";  case A_SUB: return "SUB";
        case A_AND: return "AND";  case A_OR:  return "OR";
        case A_XOR: return "XOR";
    }
    return "ADD";
}
std::string Semantico::mnemImed(int aritOp) {
    switch (aritOp) {
        case A_ADD: return "ADDI"; case A_SUB: return "SUBI";
        case A_AND: return "ANDI"; case A_OR:  return "ORI";
        case A_XOR: return "XORI";
    }
    return "ADDI";
}

void Semantico::aplica(int aritOp, const Operando &op) {
    if (op.kind == K_IMM) emite(mnemImed(aritOp) + " " + op.valor);
    else                  emite(mnemMem(aritOp)  + " " + op.valor);
}


void Semantico::gerarBinaria(int aritOp) {
    if (pilhaOperandos.size() < 2) return;
    Operando b = pilhaOperandos.back(); pilhaOperandos.pop_back();
    Operando a = pilhaOperandos.back(); pilhaOperandos.pop_back();

    bool comuta = (aritOp == A_ADD || aritOp == A_AND || aritOp == A_OR || aritOp == A_XOR);

    if (a.kind == K_ACUM) {
        if (b.kind == K_ACUM) { derramaAcum(); }
        aplica(aritOp, b);
    } else if (b.kind == K_ACUM) {
        if (comuta) {
            aplica(aritOp, a);
        } else {
            std::string t = novoTempSpill();
            emite("STO " + t);
            carregaNoAcc(a);
            emite(mnemMem(aritOp) + " " + t);
        }
    } else {
        carregaNoAcc(a);
        aplica(aritOp, b);
    }

    Operando r; r.kind = K_ACUM; r.valor = ""; r.relOp = 0; r.tipo = INT;
    pilhaOperandos.push_back(r);
}

void Semantico::gerarRelacional(int relOp) {
    if (pilhaOperandos.size() < 2) return;
    Operando b = pilhaOperandos.back(); pilhaOperandos.pop_back();
    Operando a = pilhaOperandos.back(); pilhaOperandos.pop_back();

    if (a.kind == K_ACUM) {
        emite("STO temp1");
        carregaNoAcc(b);
        emite("STO temp2");
    } else if (b.kind == K_ACUM) {
        emite("STO temp2");
        carregaNoAcc(a);
        emite("STO temp1");
    } else {
        carregaNoAcc(a); emite("STO temp1");
        carregaNoAcc(b); emite("STO temp2");
    }
    emite("LD temp1");
    emite("SUB temp2");
    marcaTemp(2);

    Operando r; r.kind = K_REL; r.relOp = relOp; r.valor = ""; r.tipo = BOO;
    pilhaOperandos.push_back(r);
}

void Semantico::gerarUnarioMenos() {
    if (pilhaOperandos.empty()) return;
    Operando a = pilhaOperandos.back(); pilhaOperandos.pop_back();
    if (a.kind == K_IMM) {
        long v = 0; try { v = std::stol(a.valor); } catch (...) { v = 0; }
        pushImm(std::to_string(-v));
    } else {
        // 0 - a
        Operando zero; zero.kind = K_IMM; zero.valor = "0"; zero.relOp = 0;
        if (a.kind == K_ACUM) {
            std::string t = novoTempSpill();
            emite("STO " + t);
            emite("LDI 0");
            emite("SUB " + t);
        } else {
            emite("LDI 0");
            emite("SUB " + a.valor);
        }
        Operando r; r.kind = K_ACUM; r.valor = ""; r.relOp = 0; r.tipo = INT;
        pilhaOperandos.push_back(r);
    }
}

void Semantico::emitirDesvioCondicao(const std::string &label, bool desviarSeFalso) {
    if (pilhaOperandos.empty()) { emite("JMP " + label); return; }
    Operando top = pilhaOperandos.back(); pilhaOperandos.pop_back();

    if (top.kind == K_REL) {
        std::string br = "JMP";
        if (desviarSeFalso) {
            switch (top.relOp) {
                case R_GT: br = "BLE"; break;
                case R_LT: br = "BGE"; break;
                case R_GE: br = "BLT"; break;
                case R_LE: br = "BGT"; break;
                case R_EQ: br = "BNE"; break;
                case R_NE: br = "BEQ"; break;
            }
        } else {
            switch (top.relOp) {
                case R_GT: br = "BGT"; break;
                case R_LT: br = "BLT"; break;
                case R_GE: br = "BGE"; break;
                case R_LE: br = "BLE"; break;
                case R_EQ: br = "BEQ"; break;
                case R_NE: br = "BNE"; break;
            }
        }
        emite(br + " " + label);
    } else {
        carregaNoAcc(top);
        emite((desviarSeFalso ? "BEQ " : "BNE ") + label);
    }
    limpaOperandos();
}

std::string Semantico::novoLabel() {
    return "R" + std::to_string(++contadorLabel);
}

// Monta .data + .text
void Semantico::gerarDataSection() {
    std::vector<std::string> dataLines;
    for (size_t i = 0; i < tabela.size(); i++) {
        const Simbolo &s = tabela[i];
        if (s.modalidade == FUNCAO) continue;
        if (s.tipo != INT) continue;
        std::ostringstream oss;
        if (s.modalidade == VETOR) {
            oss << "  " << s.nomeAsm << ": ";
            int n = s.tamanhoVetor > 0 ? s.tamanhoVetor : 1;
            for (int k = 0; k < n; k++) { if (k > 0) oss << ", "; oss << "0"; }
        } else {
            oss << "  " << s.nomeAsm << ": 0";
        }
        dataLines.push_back(oss.str());
    }
    for (int t = 1; t <= maxTempNum; t++)
        dataLines.push_back("  temp" + std::to_string(t) + ": 0");

    std::vector<std::string> out;
    out.push_back(".data");
    for (size_t i = 0; i < dataLines.size(); i++) out.push_back(dataLines[i]);
    out.push_back("");
    out.push_back(".text");
    out.push_back("  JMP _PRINCIPAL");
    for (size_t i = 0; i < codigoAsm.size(); i++) {
        const std::string &l = codigoAsm[i];
        if (!l.empty() && l[l.size() - 1] == ':') out.push_back(l);
        else out.push_back("  " + l);
    }
    codigoAsm = out;
}

std::string Semantico::getCodigoAsm() const {
    std::ostringstream oss;
    for (size_t i = 0; i < codigoAsm.size(); i++) oss << codigoAsm[i] << "\n";
    return oss.str();
}

// MENSAGENS
void Semantico::erro(const std::string &msg, int pos) {
    std::ostringstream oss; oss << "[ERRO] (pos " << pos << ") " << msg;
    mensagens.push_back(oss.str()); houveErro = true;
}
void Semantico::aviso(const std::string &msg, int pos) {
    std::ostringstream oss; oss << "[AVISO] (pos " << pos << ") " << msg;
    mensagens.push_back(oss.str());
}
void Semantico::avisoNaoUsadas() {
    for (size_t i = 0; i < tabela.size(); i++) {
        const Simbolo &s = tabela[i];
        if (s.modalidade == FUNCAO && (s.nome == "principal" || s.nome == "main")) continue;
        if (!s.usada) {
            std::ostringstream oss;
            oss << "[AVISO] '" << s.nome << "' declarado(a) no escopo '"
                << s.escopo << "' e nao foi usado(a)";
            mensagens.push_back(oss.str());
        }
    }
}

// EXECUTE ACTION
void Semantico::executeAction(int action, const Token *token) {
    std::string lexema = token ? token->getLexeme() : "";
    int pos = token ? token->getPosition() : -1;

    switch (action) {

    case 1: { tipoAtual = SemanticTable::tipoCodigo(lexema); break; }

    case 2: {
        limpaOperandos();
        if (jaDeclaradoNoEscopo(lexema))
            erro("identificador '" + lexema + "' ja declarado no escopo '" + escopoAtual() + "'", pos);
        else
            inserirSimbolo(lexema, tipoAtual, VARIAVEL, pos);
        ultimoIdAtribuicao = lexema;
        break;
    }

    case 3: {
        Simbolo *s = buscaSimbolo(lexema);
        if (s == nullptr) { erro("identificador '" + lexema + "' nao declarado", pos); pushImm("0"); }
        else {
            s->usada = true;
            if (!s->inicializada) aviso("'" + lexema + "' usado(a) sem ter sido inicializado(a)", pos);
            pushMem(s->nomeAsm);
            pilhaOperandos.back().tipo = s->tipo;
        }
        break;
    }

    case 4: {
        limpaOperandos();
        Simbolo *s = buscaSimbolo(lexema);
        if (s == nullptr) erro("identificador '" + lexema + "' nao declarado", pos);
        ultimoIdAtribuicao = lexema;
        break;
    }

    case 5: {
        Simbolo *s = buscaSimbolo(ultimoIdAtribuicao);
        if (s != nullptr) {
            s->inicializada = true;
            if (s->tipo == INT) {
                if (emLeia) {
                    emite("LD $in_port");
                    emite("STO " + s->nomeAsm);
                } else if (!pilhaOperandos.empty()) {
                    Operando rhs = pilhaOperandos.back();
                    carregaNoAcc(rhs);
                    emite("STO " + s->nomeAsm);
                }
            }
        }
        emLeia = false;
        limpaOperandos();
        break;
    }

    case 6: { abreEscopo("funcao_" + nomeFuncaoAtual); limpaOperandos(); break; }

    case 7: {
        if (jaDeclaradoNoEscopo(lexema)) erro("funcao '" + lexema + "' ja declarada", pos);
        else { inserirSimbolo(lexema, tipoAtual, FUNCAO, pos); tabela.back().inicializada = true; }
        nomeFuncaoAtual = lexema;
        funcAtualEhMain = ehEntrada(lexema);
        emite(rotuloDe(lexema) + ":");
        break;
    }

    case 8: {
        if (funcAtualEhMain) {
            emite("HLT 0");
        } else if (codigoAsm.empty() || codigoAsm.back().compare(0, 6, "RETURN") != 0) {
            emite("RETURN 0");
        }
        fechaEscopo();
        nomeFuncaoAtual = "";
        funcAtualEhMain = false;
        break;
    }

    case 9: {
        if (jaDeclaradoNoEscopo(lexema)) erro("parametro '" + lexema + "' duplicado", pos);
        else inserirSimbolo(lexema, tipoAtual, PARAMETRO, pos);
        break;
    }

    case 10: {
        if (jaDeclaradoNoEscopo(lexema)) erro("parametro '" + lexema + "' duplicado", pos);
        else inserirSimbolo(lexema, tipoAtual, PARAMETRO_VETOR, pos);
        break;
    }

    case 11: {
        limpaOperandos();
        if (jaDeclaradoNoEscopo(lexema))
            erro("identificador '" + lexema + "' ja declarado no escopo '" + escopoAtual() + "'", pos);
        else { inserirSimbolo(lexema, tipoAtual, VETOR, pos); tabela.back().tamanhoVetor = 1; }
        break;
    }

    case 12: {
        if (!tabela.empty()) {
            Simbolo &s = tabela.back();
            s.inicializada = true;
            if (s.tipo == INT && !pilhaOperandos.empty()) {
                Operando rhs = pilhaOperandos.back();
                carregaNoAcc(rhs);
                emite("STO " + s.nomeAsm);
            }
        }
        limpaOperandos();
        break;
    }

    case 13: {
        Simbolo *s = buscaSimbolo(ultimoIdAtribuicao);
        if (s != nullptr) { s->inicializada = true; s->usada = true; }
        if (pilhaOperandos.size() >= 2) {
            Operando valor  = pilhaOperandos.back(); pilhaOperandos.pop_back();
            Operando indice = pilhaOperandos.back(); pilhaOperandos.pop_back();
            if (indice.kind == K_ACUM) {
                emite("STO temp1");
                carregaNoAcc(valor); emite("STO temp2");
            } else if (valor.kind == K_ACUM) {
                emite("STO temp2");
                carregaNoAcc(indice); emite("STO temp1");
            } else {
                carregaNoAcc(indice); emite("STO temp1");
                carregaNoAcc(valor);  emite("STO temp2");
            }
            emite("LD temp1");
            emite("STO $indr");
            emite("LD temp2");
            emite("STOV " + nomeAsmDe(ultimoIdAtribuicao));
            marcaTemp(2);
        }
        limpaOperandos();
        break;
    }

    case 14: {
        CallCtx ctx;
        ctx.nomeFunc = lexema;
        ctx.numArgs = 0;
        ctx.pos = pos;
        ctx.rotulo = rotuloDe(lexema);
        Simbolo *s = buscaSimbolo(lexema);
        if (s == nullptr) {
            erro("a rotina '" + lexema + "' nao existe", pos);
            ctx.existe = false; ctx.tipoRetorno = ERR;
        } else if (s->modalidade != FUNCAO) {
            erro("'" + lexema + "' nao e uma funcao", pos);
            ctx.existe = false; ctx.tipoRetorno = ERR;
        } else {
            s->usada = true;
            ctx.existe = true;
            ctx.tipoRetorno = s->tipo;
            std::string escFunc = "funcao_" + lexema;
            for (size_t i = 0; i < tabela.size(); i++) {
                if (tabela[i].escopo == escFunc &&
                    (tabela[i].modalidade == PARAMETRO || tabela[i].modalidade == PARAMETRO_VETOR)) {
                    ctx.paramAsm.push_back(tabela[i].nomeAsm);
                    ctx.paramTipo.push_back(tabela[i].tipo);
                }
            }
        }
        pilhaChamadas.push_back(ctx);
        break;
    }

    case 15: {
        std::ostringstream oss; oss << "bloco_" << contadorBloco++;
        abreEscopo(oss.str());
        limpaOperandos();
        break;
    }

    case 16: { fechaEscopo(); break; }

    case 17: { break; }

    case 18: {
        if (emEscreva && !pilhaOperandos.empty()) {
            Operando op = pilhaOperandos.back();
            carregaNoAcc(op);
            emite("STO $out_port");
        }
        limpaOperandos();
        break;
    }

    case 19: { avisoNaoUsadas(); gerarDataSection(); break; }

    case 20: {
        std::string val = lexema;
        try {
            if (val.size() > 2 && val[0] == '0' && (val[1] == 'b' || val[1] == 'B'))
                val = std::to_string(std::stoi(val.substr(2), nullptr, 2));
            else if (val.size() > 2 && val[0] == '0' && (val[1] == 'x' || val[1] == 'X'))
                val = std::to_string(std::stoi(val.substr(2), nullptr, 16));
        } catch (...) {}
        ultimoLiteralInt = val;
        pushImm(val);
        if (!tabela.empty() && tabela.back().modalidade == VETOR && tabela.back().tamanhoVetor == 1) {
            try { int t = std::stoi(val); if (t > 0) tabela.back().tamanhoVetor = t; } catch (...) {}
        }
        break;
    }

    case 21: { emLeia = true; emEscreva = false; limpaOperandos(); break; }
    case 22: { emEscreva = true; emLeia = false; limpaOperandos(); break; }
    case 23: { emEscreva = false; break; }

    case 24: gerarBinaria(A_ADD); break;   // +
    case 25: gerarBinaria(A_SUB); break;   // -
    case 26: gerarBinaria(A_AND); break;   // &
    case 27: gerarBinaria(A_OR);  break;   // |
    case 28: gerarBinaria(A_XOR); break;   // ^

    case 29: gerarRelacional(R_GT); break; // >
    case 30: gerarRelacional(R_LT); break; // <
    case 31: gerarRelacional(R_GE); break; // >=
    case 32: gerarRelacional(R_LE); break; // <=
    case 33: gerarRelacional(R_EQ); break; // ==
    case 34: gerarRelacional(R_NE); break; // !=

    case 35: gerarUnarioMenos(); break;

    //  SE / SE-SENAO
    case 36: {  // apos a condicao
        CtxControle c;
        c.lFalso = novoLabel();
        c.teveElse = false;
        c.marcaIncr = -1;
        pilhaCtrl.push_back(c);
        emitirDesvioCondicao(c.lFalso, true);
        break;
    }
    case 37: {  // inicio do senao
        if (!pilhaCtrl.empty()) {
            CtxControle &c = pilhaCtrl.back();
            c.teveElse = true;
            c.lFim = novoLabel();
            emite("JMP " + c.lFim);
            emite(c.lFalso + ":");
        }
        break;
    }
    case 38: {  // fim do se_senao
        if (!pilhaCtrl.empty()) {
            CtxControle c = pilhaCtrl.back();
            pilhaCtrl.pop_back();
            if (c.teveElse) emite(c.lFim + ":");
            else            emite(c.lFalso + ":");
        }
        break;
    }

    // ENQUANTO
    case 39: {  // apos ENQUANTO (antes da condicao)
        CtxControle c;
        c.lIni = novoLabel();
        c.marcaIncr = -1;
        pilhaCtrl.push_back(c);
        emite(c.lIni + ":");
        break;
    }
    case 40: {  // apos a condicao
        if (!pilhaCtrl.empty()) {
            CtxControle &c = pilhaCtrl.back();
            c.lFim = novoLabel();
            emitirDesvioCondicao(c.lFim, true);
        }
        break;
    }
    case 41: {  // fim do enquanto
        if (!pilhaCtrl.empty()) {
            CtxControle c = pilhaCtrl.back();
            pilhaCtrl.pop_back();
            emite("JMP " + c.lIni);
            emite(c.lFim + ":");
        }
        break;
    }

    // FACA-ENQUANTO
    case 42: {  // apos FACA
        CtxControle c;
        c.lIni = novoLabel();
        c.marcaIncr = -1;
        pilhaCtrl.push_back(c);
        emite(c.lIni + ":");
        break;
    }
    case 43: {  // apos a condicao
        if (!pilhaCtrl.empty()) {
            CtxControle c = pilhaCtrl.back();
            pilhaCtrl.pop_back();
            emitirDesvioCondicao(c.lIni, false);
        }
        break;
    }

    // PARA
    case 44: {  // apos PARA "("
        CtxControle c;
        c.lIni = novoLabel();
        c.lFim = novoLabel();
        c.teveElse = false;
        c.marcaIncr = -1;
        pilhaCtrl.push_back(c);
        break;
    }
    case 45: {  // apos a inicializacao
        if (!pilhaCtrl.empty()) emite(pilhaCtrl.back().lIni + ":");
        break;
    }
    case 46: {  // apos a condicao
        if (!pilhaCtrl.empty()) {
            CtxControle &c = pilhaCtrl.back();
            emitirDesvioCondicao(c.lFim, true);
            c.marcaIncr = (int)codigoAsm.size();
        }
        break;
    }
    case 47: {  // apos o incremento
        if (!pilhaCtrl.empty()) {
            CtxControle &c = pilhaCtrl.back();
            if (c.marcaIncr >= 0 && c.marcaIncr <= (int)codigoAsm.size()) {
                for (int i = c.marcaIncr; i < (int)codigoAsm.size(); i++)
                    c.codIncr.push_back(codigoAsm[i]);
                codigoAsm.erase(codigoAsm.begin() + c.marcaIncr, codigoAsm.end());
            }
        }
        break;
    }
    case 48: {  // fim do para
        if (!pilhaCtrl.empty()) {
            CtxControle c = pilhaCtrl.back();
            pilhaCtrl.pop_back();
            for (size_t i = 0; i < c.codIncr.size(); i++) emite(c.codIncr[i]);
            emite("JMP " + c.lIni);
            emite(c.lFim + ":");
        }
        break;
    }

    case 49: {
        if (pilhaOperandos.size() >= 2) {
            Operando indice  = pilhaOperandos.back(); pilhaOperandos.pop_back();
            Operando nomeVet = pilhaOperandos.back(); pilhaOperandos.pop_back();
            carregaNoAcc(indice);
            emite("STO $indr");
            emite("LDV " + nomeVet.valor);
            Operando r; r.kind = K_ACUM; r.valor = ""; r.relOp = 0; r.tipo = INT;
            pilhaOperandos.push_back(r);
            Simbolo *s = buscaSimbolo(nomeVet.valor);
            if (s != nullptr) s->usada = true;
        }
        break;
    }

    case 50: {
        if (!pilhaOperandos.empty()) {
            Operando indice = pilhaOperandos.back(); pilhaOperandos.pop_back();
            carregaNoAcc(indice);
            emite("STO $indr");
            emite("LD $in_port");
            emite("STOV " + nomeAsmDe(ultimoIdAtribuicao));
            Simbolo *s = buscaSimbolo(ultimoIdAtribuicao);
            if (s != nullptr) { s->inicializada = true; s->usada = true; }
        }
        emLeia = false;
        limpaOperandos();
        break;
    }

    case 51: {
        if (!pilhaOperandos.empty()) {
            Operando v = pilhaOperandos.back();
            carregaNoAcc(v);
        }
        emite("RETURN 0");
        limpaOperandos();
        break;
    }

    case 52: {
        if (pilhaChamadas.empty()) break;
        CallCtx &ctx = pilhaChamadas.back();
        int idx = ctx.numArgs;
        if (!pilhaOperandos.empty()) {
            Operando arg = pilhaOperandos.back(); pilhaOperandos.pop_back();
            if (ctx.existe && idx < (int)ctx.paramAsm.size()) {
                if (arg.tipo != IND && arg.tipo != ERR &&
                    ctx.paramTipo[idx] != IND && ctx.paramTipo[idx] != ERR &&
                    arg.tipo != ctx.paramTipo[idx]) {
                    aviso("tipo incompativel no parametro " + std::to_string(idx + 1) +
                          " da funcao '" + ctx.nomeFunc + "'", ctx.pos);
                }
                carregaNoAcc(arg);
                emite("STO " + ctx.paramAsm[idx]);
            }
        }
        ctx.numArgs++;
        break;
    }

    // FIM DA CHAMADA
    case 53: {
        if (pilhaChamadas.empty()) break;
        CallCtx ctx = pilhaChamadas.back();
        pilhaChamadas.pop_back();
        if (ctx.existe) {
            int esperado = (int)ctx.paramAsm.size();
            if (ctx.numArgs != esperado) {
                erro("a funcao '" + ctx.nomeFunc + "' esperava " + std::to_string(esperado) +
                     " parametro(s) e foi(foram) passado(s) " + std::to_string(ctx.numArgs), ctx.pos);
            }
            emite("CALL " + ctx.rotulo);
            Operando r; r.kind = K_ACUM; r.valor = ""; r.relOp = 0; r.tipo = ctx.tipoRetorno;
            pilhaOperandos.push_back(r);
        }
        break;
    }

    default: break;
    }
}
