#include "diffviewform.h"
#include "ui_heapdumpform.h"
#include "centralwidget.h"
#include "parentviewform.h"
#include "model/heap_item_types/baseheapitem.h"
#include <QStatusBar>
#include "model/appstate.h"

DiffViewForm::DiffViewForm(QWidget* parent, int _snapShotVersion)
    : HeapDumpForm(parent, _snapShotVersion), parentViewForm(nullptr), parentLabel(nullptr), diffLabel(nullptr)
{
}

DiffViewForm::~DiffViewForm()
{
    if (parentViewForm)  {
        delete parentViewForm;
    }

    if (parentLabel) {
        delete parentLabel;
    }

    if (diffLabel) {
        delete diffLabel;
    }
}

void DiffViewForm::treeNodeSelected(const QModelIndex &index)
{
    QModelIndex sourceIndex = proxyModel->mapToSource(index);
    RBKit::BaseHeapItem *nodeItem = static_cast<RBKit::BaseHeapItem *>(sourceIndex.internalPointer());
    if (nodeItem != NULL) {
        parentWindow->showStatusMessage(nodeItem->leadingIdentifier());
        if (!parentViewForm) {
            initializeParentView();
        }
        updateParentView(nodeItem);
    }
}

void DiffViewForm::updateParentView(RBKit::BaseHeapItem *heapItem)
{
    parentViewForm->reset();
    RBKit::BaseHeapItem *parentHeapItem = heapItem->getObjectParents(rootItem);
    parentViewForm->loadFromSpecifiedRoot(parentHeapItem);
}

void DiffViewForm::initializeParentView()
{
    parentLabel = new QLabel("<b> Parents of selected object </b>");
    ui->treeVerticalLayout->addWidget(parentLabel);
    parentViewForm = new HeapDumpForm(this, -1);
    parentViewForm->setMaximumHeight(200);
    parentViewForm->setContextDetail(RbkitContextDetail::ONLY_FILE);
    parentViewForm->setParentWindow(getParentWindow());
    ui->treeVerticalLayout->addWidget(parentViewForm);
    parentViewForm->show();
}

void DiffViewForm::setSnapshotDiffNumbers(QList<int> selectedSnapshots)
{
    diffLabel = new QLabel(QString("<b> Showing Comparison of %0-%1</b>")
                           .arg(RBKit::AppState::getInstance()->getSnapshotName(selectedSnapshots.at(1)))
                           .arg(RBKit::AppState::getInstance()->getSnapshotName(selectedSnapshots.at(0))));

    ui->treeVerticalLayout->insertWidget(0, diffLabel);
}
