#include <iostream>
#include <cstdio>

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>
#include <QFile>
#include <QDataStream>
#include <QDir>

#include "plasmium.h"
#include "common.h"

Plasmium::Plasmium()
{
    m_logfilepath =  QDir::homePath().append("/.plasmium.log");
    m_in = new QFile();
    m_in->open(stdin, QIODevice::ReadOnly);
    m_dataStreamIn = new QDataStream(m_in);
    m_textStreamIn = new QTextStream(m_in);

    m_out = new QFile();
    m_out->open(stdout, QIODevice::ReadWrite);
    m_dataStreamOut = new QDataStream(m_out);
    m_textStreamOut = new QTextStream(m_out);
}//Plasmium::Plasmium()

Plasmium::~Plasmium()
{
    delete m_dataStreamIn;
    delete m_textStreamIn;
    m_in->close();
    delete m_in;

    delete m_dataStreamOut;
    delete m_textStreamOut;
    m_out->close();
    delete m_out;

    if (m_notify) {
        delete m_notify;
    }
}//Plasmium::~Plasmium()

void Plasmium::init()
{
    QFile debugFile(m_logfilepath);
    debugFile.open(QIODevice::ReadWrite|QIODevice::Text);
    debugFile.seek(debugFile.size());
    QTextStream debugStream(&debugFile);
    debugStream << endl;
    debugStream << "Starting Plasmium!" << endl << endl;
    debugFile.flush();
    debugFile.close();

    m_notify = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read, this);
    connect(m_notify, SIGNAL(activated(int)), this, SLOT(readNativeMessage()));

}//Plasmium::init()

void Plasmium::refreshTabs()
{
    QJsonObject message
    {
        {"command", "list all tabs"}
    };
    this->sendNativeMessage(QJsonDocument(message));
}//Plasmium::refreshTabs()

void Plasmium::listTopSites()
{
    QJsonObject message
    {
        {"command", "list top sites"}
    };
    this->sendNativeMessage(QJsonDocument(message));
}//Plasmium::listTopSites()

QStringMap Plasmium::getTopSites()
{
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect(this, SIGNAL(listOfTopSites(QStringMap)), &loop, SLOT(quit()));
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    timer.start(1000);
    this->listTopSites();
    loop.exec();
    return m_topsites;
}//Plasmium::getTopSites();

QStringMapMap Plasmium::getTabs()
{
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect(this, SIGNAL(listOfTabs(QStringMapMap)), &loop, SLOT(quit()));
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    timer.start(1000);
    this->refreshTabs();
    loop.exec();
    return m_tabs;
}//Plasmium::getTabs();

void Plasmium::listAllAudibleWindows()
{
    QJsonObject message
    {
        {"command", "list all audible windows"}
    };
    this->sendNativeMessage(QJsonDocument(message));
}//Plasmium::listAllAudibleWindows()

void Plasmium::muteAllTabs()
{
    QJsonObject message
    {
        {"command", "mute all tabs"}
    };
    this->sendNativeMessage(QJsonDocument(message));
}//Plasmium::muteAllTabs()

void Plasmium::muteAllBackgroundTabs()
{
    QJsonObject message
    {
        {"command", "mute all background tabs"}
    };
    this->sendNativeMessage(QJsonDocument(message));
}//Plasmium::muteAllBackgroundTabs()

void Plasmium::unmuteActiveTab()
{
    QJsonObject message
    {
        {"command", "unmute active tab"}
    };
    this->sendNativeMessage(QJsonDocument(message));
}//Plasmium::unmuteActiveTab()

void Plasmium::unmuteAllTabs()
{
    QJsonObject message
    {
        {"command", "unmute all tabs"}
    };
    this->sendNativeMessage(QJsonDocument(message));
}//Plasmium::unmuteAllTabs()


void Plasmium::highlightTab(int windowId, int tabIndex)
{
    QJsonObject message
    {
        {"command", "highlight tab"},
        {"windowId", windowId},
        {"tabIndex", tabIndex}
    };
    this->sendNativeMessage(QJsonDocument(message));
}//Plasmium::highlightTab()

void Plasmium::highlightTab(const PlasmiumChromeTab &tab)
{
    this->highlightTab(tab.getWindowId(), tab.getTabIndex());
}//Plasmium::highlightTab()

void Plasmium::newTab(const QString &uri)
{
    QJsonObject message
    {
        {"command", "new active tab"},
        {"uri", uri}
    };
    this->sendNativeMessage(QJsonDocument(message));
}//Plasmium::newTab()

void Plasmium::sendNativeMessage(const QJsonDocument &message)
{
    // Calculate 32bit "chrome native byte order"
    // (which seems to imply LittleEndian) integer 
    // identifying the byte length of a message to send.
    quint32 length = message.toJson(QJsonDocument::Compact).length();
    QByteArray size;
    QDataStream sizestream(&size, QIODevice::WriteOnly);
    sizestream.setByteOrder(QDataStream::LittleEndian);
    sizestream << length;

    // Convert message to QByteArray
    QByteArray rawMessage = message.toJson(QJsonDocument::Compact);
    QByteArray rawIndentedMessage = message.toJson(QJsonDocument::Indented);

    // Log to debug file
    QFile debugFile(m_logfilepath);
    debugFile.open(QIODevice::ReadWrite|QIODevice::Text);
    debugFile.seek(debugFile.size());
    QTextStream debugStream(&debugFile);
    debugStream << "Sending:" << endl;
    debugStream << size.toHex() << "    " << length << endl;
    debugStream << rawMessage.toHex() << endl << rawIndentedMessage << endl;
    debugFile.flush();
    debugFile.close();

    // Write actual bytes to stdout
    QFile o;
    o.open(stdout, QIODevice::WriteOnly);
    o.write(size.append(rawMessage));
    o.flush();
    o.close();
    
}//Plasmium::sendNativeMessage()

void Plasmium::readNativeMessage()
{
    // Open stdin as a QFile and create a LittleEndian DataStream reader on it
    QFile qtin;
    qtin.open(stdin, QIODevice::ReadOnly);
    QDataStream in(&qtin);
    in.setByteOrder(QDataStream::LittleEndian);

    // Read 4 bytes of data (in LittleEndian format) from DataStream on stdin into a unsigned 32-bit integer
    // representing the length of data to be read
    quint32 length;
    QByteArray array;
    in >> length;

    // Get ASCII HEX byte representation of data length variable
    QByteArray sizearray;
    QDataStream sizestream(&sizearray, QIODevice::WriteOnly);
    sizestream << length;
    
    // If there is data specified to be read, read it
    if (length > 0) {
        
        // Some debug logging
        QFile debugFile(m_logfilepath);
        debugFile.open(QIODevice::ReadWrite|QIODevice::Text);
        debugFile.seek(debugFile.size());
        QTextStream debugStream(&debugFile);
        debugStream << "Reading:" << endl;
        debugStream << sizearray.toHex() << "    " << length << endl;

        // Actually read data from stdin into a QJsonDocument, via a QByteArray
        char* s = new char[length];
        qtin.read(s, length);//>> input;//char* s;
        QByteArray b(s);
        if (static_cast<quint32>(b.length()) > length) {
            b.chop(b.length() - length);
        }
        delete s;
        QJsonParseError p;
        QJsonDocument message = QJsonDocument::fromJson(b, &p);

        // Some more debug logging
        debugStream << QString(b).toUtf8().toHex() << endl;
        debugStream << message.toJson(QJsonDocument::Indented) << endl;

        // Do some actual parsing of message
        if (message.object().value("command") == "list of top sites") {
            QJsonArray topsites = message.object().value("sites").toArray();
            m_topsites = QStringMap();
            debugStream << array.length() << endl;
            for (QJsonArray::const_iterator iter = topsites .constBegin(), end = topsites.constEnd(); iter < end; ++iter) {
                m_topsites.insert((*iter).toObject().value("uri").toString(), (*iter).toObject().value("title").toString());
                debugStream << (*iter).toObject().value("uri").toString() << endl;
            }
            emit listOfTopSites(m_topsites);
        }
        // Do some actual parsing of message
        if (message.object().value("command") == "list of tabs") {
            QJsonObject windows = message.object().value("list").toObject();
            m_tabs= QStringMapMap();
            debugStream << windows.length() << endl;
            for (QJsonObject::const_iterator iter = windows.constBegin(), end = windows.constEnd(); iter != end; ++iter) {
                QJsonArray tabs = (*iter).toObject().value("windowInfo").toObject().value("tabs").toArray();
                for (QJsonArray::const_iterator tabIter = tabs.constBegin(), tabEnd = tabs.constEnd(); tabIter < tabEnd; ++tabIter) {
                    QStringMap values;
                    values.insert("title", (*tabIter).toObject().value("title").toString());
                    values.insert("uri", (*tabIter).toObject().value("url").toString());
                    values.insert("favicon", (*tabIter).toObject().value("favIconUrl").toString());
                    values.insert("windowId", QString("%1").arg((*tabIter).toObject().value("windowId").toInt()));
                    values.insert("tabIndex", QString("%1").arg((*tabIter).toObject().value("index").toInt()));
                    m_tabs.insert((*tabIter).toObject().value("url").toString(), values);
                    debugStream << (*tabIter).toObject().value("url").toString() << endl;
                }
            }
            emit listOfTabs(m_tabs);
        }

        // Close debugging for now
        debugFile.flush();
        debugFile.close();

    }
}//Plasmium::readNativeMessage()

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    qDBusRegisterMetaType<QStringMap>();
    qDBusRegisterMetaType<QStringMapMap>();

    if (!QDBusConnection::sessionBus().isConnected()) {
        // log
        return 1;
    }

    if (!QDBusConnection::sessionBus().registerService(SERVICE_NAME)) {
        //log
        exit(1);
    }

    Plasmium plasmium;
    plasmium.init();
    QDBusConnection::sessionBus().registerObject("/", &plasmium, QDBusConnection::ExportScriptableSlots|QDBusConnection::ExportScriptableSignals);

    app.exec();
    return 0;
}//main
