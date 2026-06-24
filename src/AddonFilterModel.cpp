#include "AddonFilterModel.h"
#include "AddonModel.h"
#include <spdlog/spdlog.h>

AddonFilterModel::AddonFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(false);

    connect(this, &QSortFilterProxyModel::sourceModelChanged, this, [this]() {
        if (sourceModel()) {
            connect(sourceModel(), &QAbstractItemModel::modelReset,
                this, &AddonFilterModel::invalidateFilter, Qt::UniqueConnection);
            connect(sourceModel(), &QAbstractItemModel::dataChanged,
                this, &AddonFilterModel::onSourceModelDataChanged, Qt::UniqueConnection);
        }
    });
}

void AddonFilterModel::setShowInstalledOnly(bool installed) {
    if (m_showInstalledOnly != installed) {
        m_showInstalledOnly = installed;
        invalidateCache();
        invalidateFilter();
    }
}

void AddonFilterModel::invalidateCache() {
    m_cacheValid = false;
    m_installedRowsCache.clear();
}

void AddonFilterModel::onSourceModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
    if (roles.contains(AddonModel::IsInstalledRole)) {
        invalidateCache();
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

    return model->data(index, AddonModel::IsInstalledRole).toBool();
}
