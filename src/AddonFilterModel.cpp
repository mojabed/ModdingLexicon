#include "AddonFilterModel.h"
#include "AddonModel.h"
#include <spdlog/spdlog.h>

AddonFilterModel::AddonFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent) {

    //setDynamicSortFilter(false);

    m_invalidationTimer.setSingleShot(true);
    m_invalidationTimer.setInterval(100);
    connect(&m_invalidationTimer, &QTimer::timeout, this, &AddonFilterModel::flushPendingFilterInvalidation);

    /*connect(this, &QSortFilterProxyModel::sourceModelChanged, this, [this]() {
        if (sourceModel()) {
            connect(sourceModel(), &QAbstractItemModel::modelReset,
                this, [this]() {
                    invalidateCache();
                    scheduleFilterInvalidation();
                }, Qt::UniqueConnection);
            connect(sourceModel(), &QAbstractItemModel::dataChanged,
                this, &AddonFilterModel::onSourceModelDataChanged, Qt::UniqueConnection);
        }
        });*/
}

void AddonFilterModel::setShowInstalledOnly(bool installed) {
    m_showInstalledOnly = installed;
    invalidateFilter();
}

QString AddonFilterModel::categoryFilter() const {
    return m_categoryFilter;
}

void AddonFilterModel::setCategoryFilter(const QString& categoryId) {
    if (m_categoryFilter != categoryId) {
        m_categoryFilter = categoryId;
        invalidateFilter();
        emit categoryFilterChanged();
    }
}

void AddonFilterModel::invalidateCache() {
    m_cacheValid = false;
}

void AddonFilterModel::scheduleFilterInvalidation() {
    if (!m_filterInvalidationPending) {
        m_filterInvalidationPending = true;
        m_invalidationTimer.start();
    }
}

void AddonFilterModel::flushPendingFilterInvalidation() {
    m_filterInvalidationPending = false;
    invalidateCache();
    invalidateFilter();
}

void AddonFilterModel::onSourceModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
    scheduleFilterInvalidation();
}

bool AddonFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    if (!sourceModel())
        return false;

    const QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);

    if (m_showInstalledOnly) {
        const bool isInstalled = sourceModel()->data(sourceIndex, AddonModel::IsInstalledRole).toBool();
        if (!isInstalled)
            return false;
    }

    if (!m_categoryFilter.isEmpty()) {
        const QString categoryId = sourceModel()->data(sourceIndex, AddonModel::CategoryIdRole).toString();
        if (categoryId != m_categoryFilter)
            return false;
    }

    return true;
}