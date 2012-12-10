#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::DialogClass)
{
    ui->setupUi(this);    
}

Dialog::~Dialog()
{
    delete ui;
}

void
Dialog::refresh()
{
    ctab.refresh();
    fstab.refresh();
    mounts.refresh();

    ui->tableWidget->setRowCount(ctab.cnt.size());
    QTableWidgetItem* item = 0;
    int r = 0;
    for(CryptTab::ContainerT::iterator i = ctab.cnt.begin();
        i != ctab.cnt.end();
        ++i)
    {
        QString mp = fstab.mountpoint(i->first);
        if ( mp.length() ) {
            QString tooltip(i->second);

            std::cerr << "found mountpoint: " << mp.toStdString() << " for " << i->first.toStdString() << std::endl;
            item = new QTableWidgetItem(i->first);
            item->setToolTip(tooltip);
            ui->tableWidget->setItem(r,0,item);

            item = new QTableWidgetItem(mp);
            ui->tableWidget->setItem(r,1,item);

            QString state = State2String(mounts.state(i->second, i->first));
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


void Dialog::on_button_mount_clicked()
{
    QString name = get_current_name();
    std::cerr << "run mount for " << name.toStdString() << std::endl;
    run_mount(this, name);
    refresh();
}

void Dialog::on_button_umount_clicked()
{
    QString name = get_current_name();
    std::cerr << "run umount for " << name.toStdString() << std::endl;
    run_unmount(name);
    refresh();
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
    QString name = get_current_name();
    QString location = ctab.location(name);

    State state = mounts.state(location, name);
    if (state == mounted) {
        run_unmount(name);
        refresh();
    } else if (state == connected) {
        run_mount(this, name);
        refresh();
    }
}

