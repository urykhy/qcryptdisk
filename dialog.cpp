#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::DialogClass), mount(this)
{
    ui->setupUi(this);
    connect(&watcher, SIGNAL(finished()), this, SLOT(operationDone()));
    connect(&mount, SIGNAL(signal(std::function<void(void)>)),
            this,   SLOT(mount_cb(std::function<void(void)>)),
            Qt::BlockingQueuedConnection);
    refresh();
}

Dialog::~Dialog()
{
    delete ui;
}

void
Dialog::refresh()
{
    mount.refresh();

    ui->tableWidget->setRowCount(mount.ctab.cnt.size());
    QTableWidgetItem* item = 0;
    int r = 0;
    for(CryptTab::ContainerT::iterator i = mount.ctab.cnt.begin();
        i != mount.ctab.cnt.end();
        ++i)
    {
        QString mp = mount.fstab.mountpoint(i->first);
        if ( mp.length() ) {
            QString tooltip(i->second);

            std::cerr << "found mountpoint: " << mp.toStdString() << " for " << i->first.toStdString() << std::endl;
            item = new QTableWidgetItem(i->first);
            item->setToolTip(tooltip);
            ui->tableWidget->setItem(r,0,item);

            item = new QTableWidgetItem(mp);
            ui->tableWidget->setItem(r,1,item);

            QString state = State2String(mount.mounts.state(i->second, i->first));
            item = new QTableWidgetItem(state);
            ui->tableWidget->setItem(r,2,item);
            ++r;
        }
    }

    ui->tableWidget->setRowCount(r);
    //std::cerr << "row count now " << ui->tableWidget->rowCount() << std::endl;

}

void Dialog::on_button_refresh_clicked()
{
    refresh();
}

void Dialog::run_work(std::function<void(void)> f)
{
    ui->button_mount->setEnabled(false);
    ui->button_umount->setEnabled(false);

    QFuture<void> future = QtConcurrent::run(f);
    watcher.setFuture(future);
}

void Dialog::on_button_mount_clicked()
{
    QString name = get_current_name();
    std::cerr << "run mount for " << name.toStdString() << std::endl;
    run_work(std::bind(&Mount::run_mount, &mount, name));
}

void Dialog::on_button_umount_clicked()
{
    QString name = get_current_name();
    std::cerr << "run umount for " << name.toStdString() << std::endl;
    run_work(std::bind(&Mount::run_unmount, &mount, name));
}

void Dialog::operationDone(void)
{
    refresh();
    ui->button_mount->setEnabled(true);
    ui->button_umount->setEnabled(true);

}

QString Dialog::get_current_name()
{
    QList<QTableWidgetItem *> sel = ui->tableWidget->selectedItems();

    // row-selection mode
    // we have 3 elements in row
    if (sel.size() != ui->tableWidget->columnCount()) {
        return QString();
    }

    return sel[0]->text();
}

void Dialog::on_tableWidget_doubleClicked(QModelIndex index)
{
    Q_UNUSED(index);
    QString name = get_current_name();
    QString location = mount.ctab.location(name);

    State state = mount.mounts.state(location, name);
    if (state == mounted) {
        on_button_umount_clicked();
    } else if (state == connected) {
        on_button_mount_clicked();
    }
}

void Dialog::mount_cb(std::function<void(void)> f)
{
    f();
}


void Dialog::on_button_disconnect_clicked()
{
// /sys/block/$1/device/delete
    QString name = get_current_name();
    std::cerr << "disconnecting " << name.toStdString() << std::endl;
    run_work(std::bind(&Mount::run_disconnect, &mount, name));
}
