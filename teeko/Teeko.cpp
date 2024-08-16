#include "Teeko.h"
#include "ui_Teeko.h"

#include <QDebug>
#include <QMessageBox>
#include <QActionGroup>
#include <QSignalMapper>

Teeko::Teeko(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::Teeko),
      m_player(Player::player(Player::Red)),
      m_phase(Teeko::DropPhase) {

    ui->setupUi(this);

    QObject::connect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(reset()));
    QObject::connect(ui->actionQuit, SIGNAL(triggered(bool)), qApp, SLOT(quit()));
    QObject::connect(ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(showAbout()));

    QSignalMapper* map = new QSignalMapper(this);
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            QString holeName = QString("hole%1%2").arg(row).arg(col);
            Hole* hole = this->findChild<Hole*>(holeName);
            Q_ASSERT(hole != nullptr);

            m_board[row][col] = hole;

            int id = row * 5 + col;
            map->setMapping(hole, id);
            QObject::connect(hole, SIGNAL(clicked(bool)), map, SLOT(map()));
        }
    }
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QObject::connect(map, SIGNAL(mapped(int)), this, SLOT(play(int)));
#else
    QObject::connect(map, SIGNAL(mappedInt(int)), this, SLOT(play(int)));
#endif

    // When the turn ends, switch the player.
    QObject::connect(this, SIGNAL(turnEnded()), this, SLOT(switchPlayer()));

    // Quando alguém ganhar, popup de vencedor, reseta o game e come um clique fantasma.
    QObject::connect(this, SIGNAL(gameOver()), this, SLOT(showGameOver()));
    QObject::connect(this, SIGNAL(newGame()), this, SLOT(eat()));//flag para ignorar o primeiro clique fantasma quando um jogo termina e reseta automaticamente.
    QObject::connect(this, SIGNAL(newGame()), this, SLOT(reset()));


    this->reset();
    this->adjustSize();
    this->setFixedSize(this->size());

}

Teeko::~Teeko() {
    delete ui;
}

void Teeko::setPhase(Teeko::Phase phase) {
    if (m_phase != phase) {
        m_phase = phase;
    }
}

void Teeko::play(int id) {

    int row = id/5;
    int col = id%5;

    Hole* hole = m_board[row][col];

    if(m_phase == DropPhase){
        if (hole->isEmpty()){

            hole->setPlayer(Teeko::m_player);

            if(checkPosition() == true){
                emit gameOver();
                emit newGame();
            }
            ++m_dropCount;
            if (m_dropCount == 8)setPhase(Teeko::MovePhase);

            emit turnEnded();

        }

    }else if(m_phase == MovePhase){

        if (hole->player() == m_player){

            for (int row = 0; row < 5; row++) {
                for (int col = 0; col < 5; col++) {
                    if(m_board[row][col]->isSelected()) m_board[row][col]->setState(Hole::Used);
                    if(m_board[row][col]->isPlayable()) m_board[row][col]->setState(Hole::Empty);
                }
            }//se você selecionar outro used hole quando ja estiver um com selected, esse loop varre e limpa a animação da seleção anterior.

            hole->setState(Hole::Selected);
            //deixa o hole do player como selected

            if(col+1 < 5) if(m_board[row][col+1]->isEmpty())
                m_board[row][col+1]->setState(Hole::Playable);
            if(col+1 < 5 && row+1 < 5) if(m_board[row+1][col+1]->isEmpty())
                m_board[row+1][col+1]->setState(Hole::Playable);
            if(row+1 < 5) if(m_board[row+1][col]->isEmpty())
                m_board[row+1][col]->setState(Hole::Playable);
            if(row+1 < 5 && col-1 >= 0) if(m_board[row+1][col-1]->isEmpty())
                m_board[row+1][col-1]->setState(Hole::Playable);
            if(col-1 >= 0) if(m_board[row][col-1]->isEmpty())
                m_board[row][col-1]->setState(Hole::Playable);
            if(row-1 >= 0 && col-1 >= 0) if(m_board[row-1][col-1]->isEmpty())
                m_board[row-1][col-1]->setState(Hole::Playable);
            if(row-1 >= 0) if(m_board[row-1][col]->isEmpty())
                m_board[row-1][col]->setState(Hole::Playable);
            if(row-1 >= 0 && col+1 < 5) if(m_board[row-1][col+1]->isEmpty())
                m_board[row-1][col+1]->setState(Hole::Playable);
            //deixa todos em volta do selected que estão livres como playable

        }

        if(hole->isPlayable()){

            for (int row = 0; row < 5; row++) {
                for (int col = 0; col < 5; col++) {
                    if(m_board[row][col]->isSelected()){
                        m_board[row][col]->setPlayer(nullptr);
                        m_board[row][col]->setState(Hole::Empty);
                    }
                    if(m_board[row][col]->isPlayable()) m_board[row][col]->setState(Hole::Empty);
                }
            }//acha os holes animados que estavam select ou playable e seta eles para empty

            hole->setPlayer(Teeko::m_player);

            if(checkPosition() == true){
                emit gameOver();
                emit newGame();
            }
            else emit turnEnded();
        }
    }

    if(!m_eatDone){
        m_eatDone = true;
        reset();//primeiro clique de um novo jogo consecutivo cai aqui, pois está dando clique fantasma (tire esse item para ver o newgame automático voltando o jogo na vez do azul).
    }
}

void Teeko::switchPlayer() {
    // Switch the player.
    m_player = m_player->other();

    // Finally, update the status bar.
    this->updateStatusBar();
}

void Teeko::reset() {
    // Reset board.

    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            Hole* hole = m_board[row][col];
            hole->setPlayer(nullptr);
            hole->setState(Hole::Empty);
        }
    }

    // Reset the player.
    m_player = Player::player(Player::Red);

    // Reset to drop phase.
    setPhase(DropPhase);

    // Finally, update the status bar.
    this->updateStatusBar();

    // Zera o contador de fase
    m_dropCount = 0;
}

bool Teeko:: checkPosition(){

    int combo = 0, row, col, attmpt;


    for(row = 0; row < 5; row++){
        for (attmpt = 0; attmpt < 2; attmpt++) {
            for (col = 0; col < 4; col++) {//check de seq. de linhas ( de 0 a 1 pq começando do segundo hole, o combo ja vai até o final do tabuleiro)
                Hole* hole = m_board[row][col+attmpt];

                if(hole->player() == m_player){

                    combo++;
                    if(combo == 4) return true;

                }
            }

            combo = 0;

        }
    }


    for (col = 0; col < 5; col++) {
        for (attmpt = 0; attmpt < 2; attmpt++) {//check de seq. de colunas ( de 0 a 1 pq começando do segundo hole, o combo ja vai até o final do tabuleiro)
            for(row = 0; row < 4; row++){
                Hole* hole = m_board[row+attmpt][col];

                if(hole->player() == m_player){

                    combo++;
                    if(combo == 4) return true;

                }
            }

            combo = 0;
        }
    }


    for(row = 0; row < 2; row++){
        for(col = 3; col < 5; col++){//check de seq. de diagonais secundárias (pegamos de ref. cada hole do quadrado do canto superior direito e testamos a diagonal secundária de 4 posiçoes dali em diante. essas 4 possibilidades descrevem todas as possiveis para esse caso de diagonal.)
            for(attmpt = 0; attmpt < 4; attmpt++){

                Hole* hole = m_board[row+attmpt][col-attmpt];

                if(hole->player() == m_player){

                    combo++;

                    if(combo == 4) return true;

                }

            }

            combo = 0;

        }
    }




    for(row = 0; row < 2; row++){
        for(col = 0; col < 2; col++){//check de seq. de diagonais principais (pegamos de ref. cada hole do quadrado do canto superior esquerdo e testamos a diagonal primaria de 4 posiçoes dali em diante. essas 4 possibilidades descrevem todas as possiveis para esse caso de diagonal.)
            for(attmpt = 0; attmpt < 4; attmpt++){

                Hole* hole = m_board[row+attmpt][col+attmpt];

                if(hole->player() == m_player){

                    combo++;

                    if(combo == 4) return true;

                }

            }

            combo = 0;

        }
    }



    for(row = 0; row < 4; row++){
        for(col = 0; col < 4; col++){//check de seq. de quadrados (0 a 3 pq estamos pegando o começo da varredura de cima na esquerda para o oposto, logo, o quadrado vai ser escaneado nessa direção e comerá os últimos holes das outras bordas.)

            Hole* hole;

            hole = m_board[row][col];
            if(hole->player() == m_player)combo++;
            hole = m_board[row][col+1];
            if(hole->player() == m_player)combo++;
            hole = m_board[row+1][col+1];
            if(hole->player() == m_player)combo++;
            hole = m_board[row+1][col];
            if(hole->player() == m_player)combo++;

            if(combo == 4) return true;

            combo = 0;


        }
    }

    return false;

}



void Teeko::showAbout() {
    QMessageBox::information(this, tr("Sobre"), tr("Teeko\n\nAugusto de Oliveira Soares augusto.oliveira.soares123@gmail.com\n--------------------------------------------------------Ana Clara Cunha Lopes anaclara28lopes@gmail.com"));
}

void Teeko::showGameOver(){

    QMessageBox::information(this, tr("Vencedor"), tr(" Teeko! Parabéns! O %1 venceu.").arg(m_player->name()));

}

void Teeko::updateStatusBar() {
    QString phase(m_phase == Teeko::DropPhase ? tr("colocar") : tr("mover"));

    ui->statusbar->showMessage(tr("Fase de %1: vez do %2")
                               .arg(phase)
                               .arg(m_player->name()));
}

void Teeko:: eat(){ m_eatDone = false; }
