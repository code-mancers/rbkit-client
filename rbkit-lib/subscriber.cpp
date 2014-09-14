#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QScopedPointer>

#include "nzmqt/nzmqt.hpp"

#include "subscriber.h"
#include "zmqsockets.h"
#include "rbcommands.h"
#include "jsbridge.h"


static const int rbkcZmqTotalIoThreads = 1;
static const int timerIntervalInMs = 1500;

// TODO: Move it to utils, or some common file.
template <class K, class V>
QVariantMap hashToQVarMap(const QHash<K, V>&& hash) {
    QVariantMap map;

    auto iter = hash.constBegin();
    for (; iter != hash.constEnd(); ++iter) {
        map[iter.key()] = iter.value();
    }

    return map;
}


Subscriber::Subscriber(RBKit::JsBridge* bridge)
    :jsBridge(bridge)
{
    commandSocket = new RBKit::ZmqCommandSocket(this);
    eventSocket   = new RBKit::ZmqEventSocket(this);
    objectStore = new RBKit::ObjectStore();
    connect(eventSocket->getSocket(), SIGNAL(messageReceived(const QList<QByteArray>&)),
           this, SLOT(onMessageReceived(const QList<QByteArray>&)));

    // initialize the timer, mark it a periodic one, and connect to timeout.
    m_timer = new QTimer(this);
    m_timer->setInterval(timerIntervalInMs);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(onTimerExpiry()));
}

void Subscriber::triggerGc() {
    RBKit::CmdTriggerGC triggerGC_Command;
    qDebug() << "Triggering GC";
    commandSocket->sendCommand(triggerGC_Command);
}

void Subscriber::takeSnapshot()
{
   RBKit::CmdObjSnapshot triggerSnapshot;
   qDebug() << "Taking snapshot";
   commandSocket->sendCommand(triggerSnapshot);
}

Subscriber::~Subscriber()
{
    stop();

    commandSocket->stop();
    delete commandSocket;

    eventSocket->stop();
    delete eventSocket;

    emit disconnected();
}

void Subscriber::startListening(QString commandsUrl, QString eventsUrl)
{
    qDebug() << "Got " << commandsUrl << eventsUrl;

    try
    {
        commandSocket->start(commandsUrl);
        eventSocket->start(eventsUrl);
    }
    catch(zmq::error_t err)
    {
        QString str = QString::fromUtf8(err.what());
        qDebug() << str ;
        emit errored(str);
        return;
    }

    RBKit::CmdStartProfile startCmd;
    commandSocket->sendCommand(startCmd);

    m_timer->start();

    emit connected();
    qDebug() << "started";
}

void Subscriber::stop()
{
    RBKit::CmdStopProfile stopCmd;
    commandSocket->sendCommand(stopCmd);
    objectStore->reset();
    qDebug() << "stopped";
}


void Subscriber::onMessageReceived(const QList<QByteArray>& rawMessage)
{
    for (QList<QByteArray>::ConstIterator iter = rawMessage.begin();
         rawMessage.end() != iter; ++iter)
    {
        RBKit::EventDataBase* event = RBKit::parseEvent(*iter);

        if (NULL != event) {
            event->process(*this);
        }

        delete event;
    }
}


void Subscriber::processEvent(const RBKit::EvtNewObject& objCreated)
{
    RBKit::ObjectDetail *objectDetail = new RBKit::ObjectDetail(objCreated.className, objCreated.objectId);
    objectStore->addObject(objectDetail);
}

void Subscriber::processEvent(const RBKit::EvtDelObject& objDeleted)
{
    quint64 objectId = objDeleted.objectId;
    objectStore->removeObject(objectId);
}

void Subscriber::processEvent(const RBKit::EvtGcStats& stats)
{
    static const QString eventName("gc_stats");
    jsBridge->sendMapToJs(eventName, stats.timestamp, stats.payload);
}


void Subscriber::processEvent(const RBKit::EvtGcStart &gcEvent) {
    qDebug() << "Received gc start" << gcEvent.timestamp;
    static const QString eventName("gc_start");
    QVariantMap map;
    jsBridge->sendMapToJs(eventName, gcEvent.timestamp, map);
}

void Subscriber::processEvent(const RBKit::EvtGcStop &gcEvent)
{
    qDebug() << "Received gc stop" << gcEvent.timestamp;
    static const QString eventName("gc_stop");
    // update generation of objects that have survived the GC
    objectStore->updateObjectGeneration();
    jsBridge->sendMapToJs(eventName, gcEvent.timestamp, QVariantMap());

    QVariantMap youngGen = hashToQVarMap(objectStore->youngGenStats());
    jsBridge->sendMapToJs("young_gen", gcEvent.timestamp, youngGen);

    QVariantMap secondGen = hashToQVarMap(objectStore->secondGenStats());
    jsBridge->sendMapToJs("second_gen", gcEvent.timestamp, secondGen);

    QVariantMap oldGen = hashToQVarMap(objectStore->oldGenStats());
    jsBridge->sendMapToJs("old_gen", gcEvent.timestamp, oldGen);
}


void Subscriber::processEvent(const RBKit::EvtObjectDump& dump)
{
    auto previousKeys  = objectStore->keys();
    const auto listOfObjects = dump.payload;

    for (auto iter = listOfObjects.begin(); iter != listOfObjects.end(); ++iter) {
        auto details = (*iter).toMap();
        auto objectId = RBKit::StringUtil::hextoInt(details["object_id"].toString());
        auto className = details["class_name"].toString();

        QScopedPointer<RBKit::ObjectDetail> objectDetail(
            new RBKit::ObjectDetail(className, objectId));

        objectDetail->fileName = details["file"].toString();
        objectDetail->lineNumber = details["line"].toInt();
        objectDetail->addReferences(details["references"].toList());
        objectDetail->size = details["size"].toInt();

        if (objectStore->hasKey(objectId)) {
            // remove the object from previousKeys because we still want
            // this object. and update the object store with details
            // that we have got from object dump.
            previousKeys.removeOne(objectDetail->objectId);
            objectStore->updateObject(objectDetail.data());
        } else {
            objectStore->addObject(objectDetail.take());
        }
    }

    quint64 objectId(0);
    foreach (objectId, previousKeys) {
        objectStore->removeObject(objectId);
    }

    RBKit::SqlConnectionPool::getInstance()->loadSnapshot(objectStore);
    emit objectDumpAvailable(RBKit::SqlConnectionPool::getInstance()->getCurrentVersion());
}


void Subscriber::onTimerExpiry()
{
    static const QString eventName("object_stats");

    // qDebug() << m_type2Count;
    QVariantMap data = objectStore->getObjectTypeCountMap();
    if (!data.empty()) {
        jsBridge->sendMapToJs(eventName, QDateTime(), data);
    }
}
