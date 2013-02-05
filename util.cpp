
#include <util.h>

namespace aux {
    const QString fstab("/etc/fstab");
    const QString mounts("/proc/mounts");
    const QString crypttab("/etc/crypttab");
    const QString prefix("/dev/mapper/");
    const QString mount="/bin/mount";
    const QString umount="/bin/umount";
    const QString sudo="/usr/bin/sudo";
    const QString cryptsetup="/sbin/cryptsetup";
}


int Mount::do_umount(const QString& name)
{
    QStringList arguments;
    QString mount_point( fstab.mountpoint(name) );
    arguments << mount_point;
    return just_run(aux::umount, arguments)==0;
}


int Mount::do_mount(const QString& name)
{
    QStringList arguments;
    QString mount_point( fstab.mountpoint(name) );
    arguments << mount_point;
    return just_run(aux::mount, arguments)==0;
    // 0 - mount success
}

int Mount::do_cryptdisk_start(const QString& qname, const QString& pass)
{
    LongOperation cursor();

    // sudo cryptsetup -T 1 luksOpen <location> <name>

    QStringList arguments;
    QString qlocation = ctab.location(qname);

    arguments << aux::cryptsetup << "-T" << "1" << "luksOpen" << qlocation << qname;

    QProcess proc(NULL);
    proc.start(aux::sudo, arguments);

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
    return code == 0;
    // 0 - success
    // 255 - wrong password
}

int Mount::do_cryptdisk_stop(const QString& name)
{
    QStringList arguments;
    arguments << aux::cryptsetup << "luksClose" << name;
    return just_run(aux::sudo, arguments)==0;
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

void Mount::run_mount(const QString& name)
{
    QString location = ctab.location(name);
    if (location.length()) {
        State state = mounts.state(location, name);
        switch(state) {
        case disconnected: {
            emit signal([&](){
                 QMessageBox msgBox;
                 msgBox.setWindowTitle("Fail to mount");
                 msgBox.setText("You can't mount a disconnected drive. Please, connect it first");
                 msgBox.exec();});
        }
        break;
        case connected: {
            bool ok;
            QString cap("Unlocking");
            cap.append(' ');
            cap.append(name);
            QString msg ("Please, enter passphrase");
            QString text;
            emit (signal([&](){
                text = QInputDialog::getText(parent,
                                             cap,
                                             msg,
                                             QLineEdit::Password,
                                             "",
                                             &ok);
            }));
            if (ok && !text.isEmpty()) {
                if(do_cryptdisk_start(name, text))
                {
                    do_mount(name);
                } else {
                    emit (signal([&](){
                        QMessageBox msgBox;
                        msgBox.setWindowTitle("Fail to mount");
                        msgBox.setText("Wrong password");
                        msgBox.exec();
                    }));
                }
            }
        }
        break;
        case dm_started:
            do_mount(name);
        default: /* nothing to do here */
            ;;
        }
    }
}


void Mount::refresh() {
    ctab.refresh();
    fstab.refresh();
    mounts.refresh();
}


int Mount::just_run(const QString& program, QStringList& arguments)
{
    LongOperation cursor();
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


