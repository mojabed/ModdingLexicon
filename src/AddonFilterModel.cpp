#include "AddonFilterModel.h"
#include "AddonModel.h"
#include <spdlog/spdlog.h>

AddonFilterModel::AddonFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(false);

    connect(this, &QSortFilterProxyModel::sourceModelChanged, this, [this]() {
        if (sourceModel()) {
            // Only invalidate filter when source model changes data
            connect(sourceModel(), &QAbstractItemModel::modelReset,
                this, &AddonFilterModel::invalidateFilter, Qt::UniqueConnection);
        }
    });
}

void AddonFilterModel::setShowInstalledOnly(bool installed) {
    if (m_showInstalledOnly != installed) {
        m_showInstalledOnly = installed;
        invalidateFilter();
    }
}

bool AddonFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    if (!m_showInstalledOnly) {
        return true;
    }

    QAbstractItemModel* model = sourceModel();
    if (!model) {
        return false;
    }

    QModelIndex index = model->index(sourceRow, 0, sourceParent);
    if (!index.isValid()) {
        return false;
    }

    //QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    //QVariant isInstalledVariant = sourceModel()->data(index, AddonModel::IsInstalledRole);
    return model->data(index, AddonModel::IsInstalledRole).toBool();
    //return isInstalledVariant.toBool();
}
