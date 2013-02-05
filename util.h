#ifndef UTIL_H
#define UTIL_H

#include <functional>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QMessageBox>
#include <QInputDialog>
#include <QProcess>
#include <QModelIndex>
#include <qapplication.h>

enum State {
    disconnected,
    connected,
    dm_started,
    mounted
};

inline
QString State2String(State s) {
    switch(s) {
        case disconnected: return QString("disconnected");
        case connected: return QString("connected");
        case dm_started: return QString("dm-started");
        case mounted: return QString("mounted");
    }
    return QString();
}

namespace aux {
    extern QString fstab;
    extern QString mounts;
    extern QString crypttab;
    extern QString prefix;
}


struct CryptTab {
    // we want to parse a crypttab file

    // name -> location
    typedef std::map<QString, QString> ContainerT;
    ContainerT cnt;

    CryptTab() { }

    void refresh() {
        cnt.clear();
        QFile file(aux::crypttab);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        QTextStream in(&file);
        
        while(!in.atEnd()) {
            QString line = in.readLine();
            if (line.length()) {
                handle_line(line);
            }
        }
    }

    void handle_line(QString& s) {
        // parse a line here:
        if (s[0]=='#') {
            return;
        }

        QStringList output = s.split(QRegExp("\\s+"), QString::SkipEmptyParts);

        const QString name = output[0];
        const QString location = output[1];
        const QString ciper = output[2];

        if (ciper != "none") {
            std::cerr << "currently only LUKS volumes supported (" << name.toStdString() << " skipped)" << std::endl;
            return;
        }

        cnt.insert(std::make_pair(name, location));
    }

    QString location(const QString& name) {
        ContainerT::iterator i = cnt.find(name);
        if ( i == cnt.end()) {
            return QString();
        }
        return i->second;
    }

};

struct Mounts {
    QStringList mounts;

    void refresh()
    {
        mounts.clear();

        QFile file(aux::mounts);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }
        QTextStream in(&file);

        QString line = in.readLine();
        while( line.length() ) {
            int pos = line.indexOf(' ');
            if (pos != -1) {
                line.resize(pos);
                mounts.push_back(line);
            }

            line = in.readLine();
        }
    }

    // check if mounted
    // check if disk connected
    State
    state(const QString& device,
          const QString& name)
    {
        State state = disconnected;
        if (QFile::exists(device) ) {
            state=connected;
        }

        // check if crypto device started
        QString dev_name=aux::prefix+name;
        if(QFile::exists(dev_name)) {
            state=dm_started;
        }

        // check if disk mounted
        if (mounts.contains(dev_name)) {
            state=mounted;
        }

        return state;
    }

};

struct FStab {
    // now we want to parse a /etc/fstab
    // and find mount locations

    // name, mountpoint
    typedef std::map<QString, QString> ContainerT;
    ContainerT cnt;

    FStab()  { }

    void refresh() {
        cnt.clear();

        QFile file(aux::fstab);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }
        QTextStream in(&file);

        while(!in.atEnd()) {
            QString line = in.readLine();
            if (line.length()) {
                handle_line(line);
            }
        }
    }

    void handle_line(QString& s) {
        // parse a line here:
        if (s[0]=='#') {
            return;
        }

        QStringList output = s.split(QRegExp("\\s+"), QString::SkipEmptyParts);

        const QString name = output[0];
        const QString point = output[1];

        cnt.insert(std::make_pair(name, point));

    }

    // get mount point by name
    QString mountpoint(const QString& s) {
        ContainerT::iterator i = cnt.find(aux::prefix+s);
        if ( i == cnt.end()) {
            return QString();
        }
        return i->second;
    }

};

class LongOperation
{
    LongOperation()
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
    }
    ~LongOperation() throw () {
        QApplication::restoreOverrideCursor();
    }
};


// thread - must be implemented this way
// inherit from QObject
// have a signal connected as BlockingQueuedConnection in user
// to a slot
// then thread can emit some code to main thread
// and run some gui stuff (show a MessageBox or ask a password)
class Mount : public QObject
{
    Q_OBJECT
public:
    Mount(QWidget* p) : parent(p) {}
    ~Mount() throw() {}

    // top level functions
    void run_unmount(const QString& name);
    void run_mount(const QString& name);
    void refresh();

signals:
    void signal(std::function<void(void)>);

protected:
    QWidget* parent;
    int do_cryptdisk_start(const QString& name, const QString& pass);
    int do_cryptdisk_stop(const QString& name);
    int do_mount(const QString& name);
    int do_umount(const QString& name);

    // util
    int just_run(QString& program, QStringList& arguments);

public:
    CryptTab ctab;
    FStab fstab;
    Mounts mounts;
};



#endif // UTIL_H
