
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
    arguments << aux::umount << mount_point;
    return just_run(aux::sudo, arguments)==0;
}


int Mount::do_mount(const QString& name)
{
    QStringList arguments;
    QString mount_point( fstab.mountpoint(name) );
    arguments << aux::mount << mount_point;
    return just_run(aux::sudo, arguments)==0;
    // 0 - mount success
}

int Mount::do_cryptdisk_start(const QString& qname, const CryptTab::VolumeInfo& vi, const QString& pass)
{
    LongOperation cursor();

    QStringList arguments;
    QString qlocation = vi.location;

    arguments << aux::cryptsetup << "-T" << "1" << "luksOpen" << qlocation << qname;
    if (!vi.ask_pass) {
        arguments << "--key-file" << vi.keyfile;
    }

    QProcess proc(NULL);
    proc.start(aux::sudo, arguments);

    if (!proc.waitForStarted()) {
        std::cerr << "fail to sudo cryptsetup" << std::endl;
		return 0;
    }

    if (vi.ask_pass) {
        std::string pass_ = pass.toStdString();
        proc.write(pass_.c_str(), pass_.length());
    }
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
    QString location = ctab.location(name).location;
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
    const CryptTab::VolumeInfo vi = ctab.location(name);
    const QString location = vi.location;
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
            QString text;

            if (vi.ask_pass) {    // ask password
                bool ok;
                QString cap("Unlocking");
                cap.append(' ');
                cap.append(name);
                QString msg ("Please, enter passphrase");
                emit (signal([&](){
                    text = QInputDialog::getText(parent,
                                                 cap,
                                                 msg,
                                                 QLineEdit::Password,
                                                 "",
                                                 &ok);
                }));
                if (!ok || text.isEmpty()) {
                    break;
                }
            }

            if(do_cryptdisk_start(name, vi, text))
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
        break;
        case dm_started:
            do_mount(name);
        default: /* nothing to do here */
            ;;
        }
    }
}

static int
get_canonical_name(const std::string& fname, QString& result)
{
	char resolved_path[PATH_MAX];
	char* nname = realpath(fname.c_str(), resolved_path);
	if (nname) {
		result.append(nname);
		return 0;
	}
	std::cerr << "fail to get canonical name for " << fname << ": " << strerror(errno) << std::endl;
	return -1;
}


void Mount::run_disconnect(const QString &name)
{
    QString location = ctab.location(name).location;
    if (!location.length()) {
        return;
    }

    State state = mounts.state(location, name);
    if (state != connected) {
        emit (signal([&](){
              QMessageBox msgBox;
              msgBox.setWindowTitle("Fail to disconnect");
              msgBox.setText("You can disonnect not used drive only");
              msgBox.exec();
        }));
        return;
    }

	QString n;
	if (!get_canonical_name(location.toStdString(), n)) {
		if (n.startsWith("/dev/")) {
			n.remove(0, 5);
            while (n[n.size() - 1].category() != QChar::Letter_Lowercase) {
                n.resize(n.size() - 1);
            }
    		do_disconnect(n);
		}
	}
}

int Mount::do_disconnect(const QString& name)
{
    LongOperation cursor();

	// sudo bash -c "echo 1 > /sys/block/sdb/device/delete"
    QStringList arguments;
    QString cmdline("echo 1 > /sys/block/" + name + "/device/delete");
	arguments << "/bin/bash" << "-c" << cmdline;

    QProcess proc(NULL);
    proc.start(aux::sudo, arguments);

    if (!proc.waitForStarted()) {
        std::cerr << "fail to sudo bash" << std::endl;
		return 0;
    }

    while(!proc.waitForFinished()) {
        std::cerr << "still not finished!" << std::endl;
    }

    int code = proc.exitCode();
    std::cerr << "exit code: " << code << std::endl;
    return code == 0;
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


