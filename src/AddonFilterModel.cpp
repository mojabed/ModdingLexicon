#include "AddonFilterModel.h"
#include "AddonModel.h"
#include <spdlog/spdlog.h>

AddonFilterModel::AddonFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent) {

    //setDynamicSortFilter(false);

    m_invalidationTimer.setSingleShot(true);
    m_invalidationTimer.setInterval(100);
    connect(&m_invalidationTimer, &QTimer::timeout, this, &AddonFilterModel::flushPendingFilterInvalidation);
}

void AddonFilterModel::setShowInstalledOnly(bool installed) {
    m_showInstalledOnly = installed;
    invalidateFilter();
}

QString AddonFilterModel::categoryFilter() const {
    return m_categoryFilter;
}

void AddonFilterModel::setCategoryFilter(const QString& categoryId) {
    spdlog::info("setCategoryFilter called with: '{}'", categoryId.toStdString());
    spdlog::info("Source model has {} rows", sourceModel() ? sourceModel()->rowCount() : 0);

    if (m_categoryFilter != categoryId) {
        m_categoryFilter = categoryId;
        spdlog::info("Filter changed to: '{}', invalidating filter", categoryId.toStdString());
        invalidateFilter();
        emit categoryFilterChanged();

        spdlog::info("After invalidateFilter, filtered row count: {}", rowCount());
    }
}

QString AddonFilterModel::searchText() const {
    return m_searchText;
}

void AddonFilterModel::setSearchText(const QString& text) {
    if (m_searchText != text) {
        m_searchText = text;
        invalidateFilter();
        emit searchTextChanged();
    }
}

void AddonFilterModel::setExcludeBelowApiVersion(int minApi) {
    if (m_excludeBelowApiVersion != minApi) {
        m_excludeBelowApiVersion = minApi;
        invalidateFilter();
    }
}

int AddonFilterModel::excludeBelowApiVersion() const {
    return m_excludeBelowApiVersion;
}

void AddonFilterModel::refreshFilter() {
    invalidateFilter();
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

    if (m_categoryFilter.isEmpty() && m_showInstalledOnly) {
        const bool isInstalled = sourceModel()->data(sourceIndex, AddonModel::IsInstalledRole).toBool();
        if (!isInstalled)
            return false;
    }

    if (!m_categoryFilter.isEmpty()) {
        const QString categoryId = sourceModel()->data(sourceIndex, AddonModel::CategoryIdRole).toString();
        if (categoryId != m_categoryFilter)
            return false;
    }

    if (!m_searchText.isEmpty()) {
        const QString title = sourceModel()->data(sourceIndex, AddonModel::TitleRole).toString();
        const QString author = sourceModel()->data(sourceIndex, AddonModel::AuthorRole).toString();
        if (!title.contains(m_searchText, Qt::CaseInsensitive) &&
            !author.contains(m_searchText, Qt::CaseInsensitive)) {
            return false;
        }
    }

    if (m_excludeBelowApiVersion > 0) {
        const int apiVersion = sourceModel()->data(sourceIndex, AddonModel::ApiVersionRole).toInt();
        if (apiVersion > 0 && apiVersion < m_excludeBelowApiVersion) {
            const bool isInstalled = sourceModel()->data(sourceIndex, AddonModel::IsInstalledRole).toBool();
            if (!isInstalled)
                return false;
        }
    }

    return true;
}