#ifndef PLASMIUM_H
#define PLASMIUM_H

#include <QtCore/QObject>
#include <QSocketNotifier>
#include <QtCore/QStringList>
#include <QtCore/QString>
#include <QFile>
#include <QTextStream>
#include <QDataStream>

#include "plasmiumchrometab.h"
#include "common.h"

class Plasmium: public QObject
{
    Q_OBJECT
    QString m_logfilepath;
    QStringMap m_topsites;
    QStringMapMap m_tabs;
    QSocketNotifier *m_notify;
    QFile *m_in, *m_out;
    QDataStream *m_dataStreamIn, *m_dataStreamOut;
    QTextStream *m_textStreamIn, *m_textStreamOut;

public:
    Plasmium();
    ~Plasmium();
    void init();

public slots:
    Q_SCRIPTABLE void refreshTabs();
    Q_SCRIPTABLE QStringMapMap getTabs();
    Q_SCRIPTABLE void listTopSites();
    Q_SCRIPTABLE QStringMap getTopSites();
    Q_SCRIPTABLE void muteAllTabs();
    Q_SCRIPTABLE void muteAllBackgroundTabs();
    Q_SCRIPTABLE void listAllAudibleWindows();
    Q_SCRIPTABLE void unmuteActiveTab();
    Q_SCRIPTABLE void unmuteAllTabs();
    Q_SCRIPTABLE void highlightTab(int windowId, int tabIndex);
    Q_SCRIPTABLE void highlightTab(const PlasmiumChromeTab &tab);
    Q_SCRIPTABLE void newTab(const QString &uri);

signals:
    Q_SCRIPTABLE void listOfTopSites(QStringMap topsites);
    Q_SCRIPTABLE void listOfTabs(QStringMapMap tabs);

private:
    void sendNativeMessage(const QJsonDocument &message);

private Q_SLOTS:
    void readNativeMessage();
};


#endif // PLASMIUM_H
