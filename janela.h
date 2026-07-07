#ifndef JANELA_H
#define JANELA_H

#include <QMainWindow>

namespace Ui {
class Janela;
}

class Janela : public QMainWindow
{
    Q_OBJECT

public:
    explicit Janela(QWidget *parent = 0);
    ~Janela();

private slots:
    void tratarCliqueBotao();

private:
    Ui::Janela *ui;
};

#endif // JANELA_H
