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


static CpuStorage *store = new CpuStorage();

void CpuProf::parseFrames(QMap<int, QVariant> frames) {
    if(frames.value(RBKit::methodName).toString() == "") {
        return;
    }

    if(!store->exists(frames.value(RBKit::methodName).toString()) ) {
        //method not yet added to the datastructure, add it
        store->addNewNode(frames);
    } else {
        //method already added, update the values
        store->updateExistingMethod(frames);
    }
}

void CpuProf::decodeMap(QList<QMap<int, QVariant> > data) {
    for(int i = 0; i < data.size() ; i++) {
        //qDebug() << data[i];
        //detect starting of new frame
        parseFrames(data[i]);
        store->incrementSampleCount();
    }
    store->clearFrameStack();
}

void CpuProf::startTraversals() {
    qDebug() << "STARTING TRAVERSALS\n\n";
    //flatprofile traversal
    store->traverseFlatProfile();

    qDebug() << "\n\n Call Graph";
    //callgraph traversal
    store->handleCallGraph();
}
