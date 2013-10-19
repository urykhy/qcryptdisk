#ifndef DIALOG_H
#define DIALOG_H

#include <QtGui/QDialog>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QtCore/qnamespace.h>

namespace Ui
{
    class DialogClass;
}

#include <util.h>

class Dialog : public QDialog
{
    Q_OBJECT
public:
    Dialog(QWidget *parent = 0);
    ~Dialog();


private:
    Ui::DialogClass *ui;
    Mount mount;

    QString get_current_name();
    QFutureWatcher<void> watcher;
    void run_work(std::function<void(void)> f);
    void refresh();

private slots:
    void operationDone(void);
    void mount_cb(std::function<void(void)>);

    void on_tableWidget_doubleClicked(QModelIndex index);
    void on_button_mount_clicked();
    void on_button_umount_clicked();
    void on_button_refresh_clicked();
    void on_button_disconnect_clicked();
};

#endif // DIALOG_H
