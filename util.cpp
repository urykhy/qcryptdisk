
#include <util.h>

namespace aux {
    QString fstab("/etc/fstab");
    QString mounts("/proc/mounts");
    QString crypttab("/etc/crypttab");
    QString prefix("/dev/mapper/");
}


int Mount::do_umount(const QString& name)
{
    QString program="/bin/umount";
    QStringList arguments;
    QString mount_point( fstab.mountpoint(name) );
    arguments << mount_point;

    return just_run(program, arguments)==0;
}


int Mount::do_mount(const QString& name)
{
    QString program="/bin/mount";
    QStringList arguments;
    QString mount_point( fstab.mountpoint(name) );
    arguments << mount_point;

    return just_run(program, arguments)==0;
    // 0 - mount success
}

int Mount::do_cryptdisk_start(const QString& qname, const QString& pass)
{

    // sudo cryptsetup -T 1 luksOpen <location> <name>
    QString program="/usr/bin/sudo";
    QStringList arguments;
    QString qlocation = ctab.location(qname);

    arguments << "/sbin/cryptsetup" << "-T" << "1" << "luksOpen" << qlocation << qname;

    QProcess proc(NULL);
    proc.start(program, arguments);

    if (!proc.waitForStarted()) {
        std::cerr << "fail to sudo cryptsetup" << std::endl;
    }

    std::string pass_ = pass.toStdString();
    proc.write(pass_.c_str(), pass_.length());
    proc.closeWriteChannel();

    while(!proc.waitForFinished()) {
        std::cerr << "still not finished!" << std::endl;
    }

    int code = proc.exitCode();
    std::cerr << "exit code: " << code << std::endl;
    return code==0;
    // 0 - success
    // 255 - wrong password
}

int Mount::do_cryptdisk_stop(const QString& name)
{
    QString program="/usr/bin/sudo";
    QStringList arguments;

    arguments << "/sbin/cryptsetup" << "luksClose" << name;

    return just_run(program, arguments)==0;
}

void Mount::run_unmount(const QString& name)
{
    QString location = ctab.location(name);
    if (location.length()) {
        State state = mounts.state(location, name);
        switch(state) {
            case mounted:
                if (do_umount(name)) {
                    do_cryptdisk_stop(name);
                }
                break;
            case dm_started:
                do_cryptdisk_stop(name);
                break;
        }
    }
}


void Mount::run_mount(QWidget* parent, const QString& name)
{
    QString location = ctab.location(name);
    if (location.length()) {
        State state = mounts.state(location, name);
        switch(state) {
        case disconnected: {
                QMessageBox msgBox;
                msgBox.setWindowTitle("Fail to mount");
                msgBox.setText("You can't mount a disconnected drive. Please, connect it first");
                msgBox.exec();
            }
            break;
        case connected: {
                bool ok;
                QString cap("Unlocking");
                cap.append(' ');
                cap.append(name);
                QString msg ("Please, enter passphrase");
                QString text = QInputDialog::getText(parent,
                                                     cap,
                                                     msg,
                                                     QLineEdit::Password,
                                                     "",
                                                     &ok);
                if (ok && !text.isEmpty()) {
                    if(do_cryptdisk_start(name, text)) {
                        do_mount(name);
                    } else {
                        QMessageBox msgBox;
                        msgBox.setWindowTitle("Fail to mount");
                        msgBox.setText("Wrong password");
                        msgBox.exec();
                    }
                }
                break;
            }
        case dm_started:
            do_mount(name);
        default: /* nothing to do here */
            ;;
        }
    }
}

int Mount::just_run(QString& program, QStringList& arguments)
{
    QProcess proc(NULL);
    std::cerr << "starting " << program.toStdString() << std::endl;
    proc.start(program, arguments);

    if (!proc.waitForStarted()) {
        std::cerr << "fail to start" << program.toStdString() << std::endl;
    }

    while(!proc.waitForFinished()) {
        std::cerr << "still not finished!" << std::endl;
    }

    int code = proc.exitCode();
    std::cerr << "exit code: " << code << std::endl;
    return code;
}


