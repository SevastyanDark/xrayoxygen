#ifndef ABOUTLAUNCHER_H
#define ABOUTLAUNCHER_H

#include <QDialog>

namespace Ui {
class AboutLauncher;
}

class AboutLauncher : public QDialog
{
    Q_OBJECT

public:
    explicit AboutLauncher(QWidget *parent = 0);
    ~AboutLauncher();

private:
    Ui::AboutLauncher *ui;
};

#endif // ABOUTLAUNCHER_H
