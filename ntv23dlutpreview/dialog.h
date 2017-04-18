#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QThread>
#include "workerthreads.h"
#include "ui_dialog.h"

const uint32_t kMaxNumInputs	(8);

class CWorker;

namespace Ui {
class Dialog;
}

class Dialog : public QDialog , public Ui::Dialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();



public slots:
    void pixelFormatChanged(int newIndex);

    void cardIndexChanged(int newIndex);
    void updateFramesDropped(int frameDropped);
    void updateFramesCaptured(int frameCaptured);
    void updateInputs();
    void readDroppedFile(QString fileName);
    void grabButtonPressed();

private:
//    Ui::Dialog *ui;

    void createWorker();
    void killWorker();
    CWorker* mWorker;
    QThread* mWorkerThread;

    void setupBoardCombo();
    void setupPixelFormatCombo();


};

#endif // DIALOG_H
