#include "janela.h"
#include "ui_janela.h"
#include <QMessageBox>
#include <QDebug>
#include <QTableWidgetItem>
#include "Semantico.h"
#include "Lexico.h"
#include "Sintatico.h"
#include <QHeaderView>

Janela::Janela(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Janela)
{
    ui->setupUi(this);
    connect(ui->botaoCompilar, &QPushButton::clicked, this, &Janela::tratarCliqueBotao);
    ui->tabelaSimbolos->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->resize(1200, 800);
}
Janela::~Janela()
{
    delete ui;
}

void Janela::tratarCliqueBotao() {
    Lexico lex;
    Sintatico sint;
    Semantico sem;

    lex.setInput(ui->entrada->toPlainText().toStdString().c_str());

    // Limpa a tabela e a saida antes de comecar
    ui->tabelaSimbolos->setRowCount(0);
    ui->saida->clear();

    try {
        sint.parse(&lex, &sem);

        // Monta as mensagens (erros + avisos) na area de saida
        QString resultado;
        if (sem.temErros()) {
            resultado = "Compilacao com erros semanticos:\n\n";
        } else {
            resultado = "Compilado com sucesso!\n\n";
        }

        const auto &msgs = sem.getMensagens();
        for (size_t i = 0; i < msgs.size(); i++) {
            resultado += QString::fromStdString(msgs[i]) + "\n";
        }
        ui->saida->setText(resultado);

        // Preenche a tabela de simbolos
        const auto &tab = sem.getTabela();
        ui->tabelaSimbolos->setRowCount((int)tab.size());

        for (size_t i = 0; i < tab.size(); i++) {
            const Simbolo &s = tab[i];

            // Converte o codigo do tipo em string
            QString tipoStr;
            switch (s.tipo) {
            case 0: tipoStr = "int"; break;
            case 1: tipoStr = "char"; break;
            case 2: tipoStr = "float"; break;
            case 3: tipoStr = "string"; break;
            case 4: tipoStr = "bool"; break;
            case 5: tipoStr = "void"; break;
            default: tipoStr = "?"; break;
            }

            // Converte a modalidade
            QString modStr;
            switch (s.modalidade) {
            case VARIAVEL:        modStr = "variavel"; break;
            case VETOR:           modStr = "vetor"; break;
            case PARAMETRO:       modStr = "parametro"; break;
            case PARAMETRO_VETOR: modStr = "param. vetor"; break;
            case FUNCAO:          modStr = "funcao"; break;
            }

            ui->tabelaSimbolos->setItem((int)i, 0, new QTableWidgetItem(QString::fromStdString(s.nome)));
            ui->tabelaSimbolos->setItem((int)i, 1, new QTableWidgetItem(tipoStr));
            ui->tabelaSimbolos->setItem((int)i, 2, new QTableWidgetItem(modStr));
            ui->tabelaSimbolos->setItem((int)i, 3, new QTableWidgetItem(QString::fromStdString(s.escopo)));
            ui->tabelaSimbolos->setItem((int)i, 4, new QTableWidgetItem(s.inicializada ? "sim" : "nao"));
            ui->tabelaSimbolos->setItem((int)i, 5, new QTableWidgetItem(s.usada ? "sim" : "nao"));
        }

        // Ajusta largura das colunas automaticamente
        ui->tabelaSimbolos->resizeColumnsToContents();

        // Mostra o codigo assembly gerado
        ui->saidaAsm->setPlainText(QString::fromStdString(sem.getCodigoAsm()));

    } catch (LexicalError &err) {
        ui->saida->setText(QString("Erro lexico: ") + err.getMessage());
    } catch (SyntacticError &err) {
        ui->saida->setText(QString("Erro sintatico: ") + err.getMessage());
    } catch (SemanticError &err) {
        ui->saida->setText(QString("Erro semantico: ") + err.getMessage());
    }
}
