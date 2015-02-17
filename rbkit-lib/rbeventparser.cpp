#include "rbeventparser.h"
#include "mpparser.h"
#include <QDebug>


RBKit::EventDataBase*
RBKit::EventParser::eventFromMsgpackObject(const msgpack::object& object) const
{
    auto map = object.as< QMap<unsigned int, msgpack::object> >();

    auto eventType = guessEvent(object);
    auto ts = map[RBKit::EfTimestamp].as<double>();
    auto timestamp = QDateTime::fromMSecsSinceEpoch(ts);
    auto payload = map[RBKit::EfPayload];

    RBKit::EventDataBase* event(nullptr);
    switch (eventType) {
    case RBKit::EtObjCreated:
        event = new RBKit::EvtNewObject(timestamp, eventType,
                                        payload.as< RBKit::ObjectDetailPtr >());
        break;

    case RBKit::EtObjDestroyed:
    {
        auto map = payload.via.map.ptr;
        auto id = (map->val).as< unsigned long long >();
        event = new RBKit::EvtDelObject(timestamp, eventType, id);
    }
    break;

    case RBKit::EtGcStats:
        event = new RBKit::EvtGcStats(timestamp, eventType,
                                      payload.as<QVariantMap>());
        break;

    case RBKit::EtGcStart:
        event = new RBKit::EvtGcStart(timestamp, eventType);
        break;

    case RBKit::EtGcStartM:
        event = new RBKit::EvtGcStartM(timestamp, eventType);
        break;

    case RBKit::EtGcEndS:
        event = new RBKit::EvtGcStop(timestamp, eventType);
        break;

    case RBKit::EtObjectSpaceDump:
        event = new RBKit::EvtObjectDump(timestamp, eventType,
                                         parseObjects(payload),
                                         rawMessage);
        break;

    case RBKit::EtHandshake:
        event = new RBKit::EvtHandshake(timestamp, eventType, payload.as<QVariantMap>());
        break;

    default:
        qDebug() << "Unable to parse event of type: " << eventType;
    }

    return event;
}


QList<RBKit::EventPtr>
RBKit::EventParser::parseEvtArray(const msgpack::object& objarray) const
{
    QList<RBKit::EventPtr> events;

    auto array = objarray.as< QList<msgpack::object> >();
    for (auto& evtObj : array) {
        auto event = eventFromMsgpackObject(evtObj);
        events.append(RBKit::EventPtr(event));
    }

    return events;
}


RBKit::EventParser::EventParser(const QByteArray& message)
    : rawMessage(message)
{
    if (5*1024*1024 < message.size()) {
        qDebug() << "probably got objdump" << QTime::currentTime();
    }

    msgpack::unpack(&unpacked, message.data(), message.size());
}


RBKit::EventDataBase* RBKit::EventParser::parseEvent() const
{
    // by default event will be of type collection
    auto eventType = guessEvent(unpacked.get());
    Q_ASSERT(RBKit::EtEventCollection == eventType);

    auto map = unpacked.get().as< QMap<unsigned int, msgpack::object> >();
    auto timestamp = map[RBKit::EfTimestamp].as<double>();
    auto ts = QDateTime::fromMSecsSinceEpoch(timestamp);

    auto counter = map[RBKit::EfMessageCounter].as<unsigned long long>();

    auto events = parseEvtArray(map[RBKit::EfPayload]);
    return new RBKit::EvtCollection(ts, eventType, events, counter);
}

RBKit::EvtHandshake *RBKit::EventParser::parseHandShake() const
{
    EvtHandshake *handShakeEvent = dynamic_cast<EvtHandshake *>(eventFromMsgpackObject(unpacked.get()));
    return handShakeEvent;
}


RBKit::EventType
RBKit::EventParser::guessEvent(const msgpack::object& object) const
{
    auto map = object.as< QMap<unsigned int, msgpack::object> >();
    auto eventType = map[RBKit::EfEventType].as<unsigned int>();
    return static_cast<RBKit::EventType>(eventType);
}


QList<RBKit::ObjectDetailPtr>
RBKit::EventParser::parseObjects(const msgpack::object& object) const
{
    qDebug() << "parsing objects begin:" << QTime::currentTime();

    QList<RBKit::ObjectDetailPtr> objects;

    msgpack::object_array list = object.via.array;
    for (uint32_t iter = 0; iter != list.size; ++iter) {
        objects << list.ptr[iter].as< RBKit::ObjectDetailPtr >();
    }

    qDebug() << "parsing objects end:" << QTime::currentTime();
    qDebug() << objects.size();
    return objects;
}

const msgpack::object& RBKit::EventParser::rawObjectDump() const
{
    auto map = unpacked.get().as< QMap<unsigned int, msgpack::object> >();
    auto objarray = map[RBKit::EfPayload];

    auto array = objarray.as< QList<msgpack::object> >();

    for (auto& evtObj : array) {
        auto eventType = guessEvent(evtObj);
        if (RBKit::EtObjectSpaceDump == eventType) {
            auto map = object.as< QMap<unsigned int, msgpack::object> >();
            return map[RBKit::EfPayload];
        }
    }

    Q_ASSERT(0);
}
