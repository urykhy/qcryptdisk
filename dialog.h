#ifndef DIALOG_H
#define DIALOG_H

#include <QtGui/QDialog>

namespace Ui
{
    class DialogClass;
}

#include <util.h>

class Dialog : public QDialog,
               public Mount
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = 0);
    ~Dialog();

    void refresh();

private:
    Ui::DialogClass *ui;

    QString get_current_name();

private slots:
    void on_tableWidget_doubleClicked(QModelIndex index);
    void on_button_mount_clicked();
    void on_button_umount_clicked();
    void on_button_refresh_clicked();
};

#endif // DIALOG_H
