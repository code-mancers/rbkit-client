#include <QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <typeinfo>
#include "cpustorage.h"
#include "cpumapping.h"
#include <iostream>
#include <string.h>
#include <QDebug>
#include <QList>

#include "cpuprof.h"


static storage *store = new storage();

void CpuProf::parseFrames(QMap<int, QVariant> *frames) {
    if(!store->exists(frames->value(MAPS::method_name).toString()) ) {
        if(frames->value(MAPS::method_name).toString() != "") {
            //method not yet added to the datastructure, add it
            store->addNewNode(*frames);
        }
    } else {
        //method already added, update the values
        //store->updateExistingMethod(*frames);
    }
}

void CpuProf::decodeMap(QList<QMap<int, QVariant> > data) {
    for(int i = 0; i < data.size() ; i++) {
        //qDebug() << data[i];
        //detect starting of new frame
        parseFrames(&data[i]);
        store->incrementSampleCount();
    }
    store->clearFrameStack();
}

void CpuProf::startTraversals() {
    qDebug() << "STARTING TRAVERSALS\n\n";
    //flatprofile traversal
    store->traverseFlatProfile();
    //callgraph traversal
    //store->handleCallGraph();
}

CpuProf::~CpuProf() {
    startTraversals();
}