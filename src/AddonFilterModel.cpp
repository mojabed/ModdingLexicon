#include "AddonFilterModel.h"
#include "AddonModel.h"

AddonFilterModel::AddonFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
    , m_sortMode(QStringLiteral("downloads")) {

    setDynamicSortFilter(true);
    sort(0, m_sortOrder);

    connect(this, &QSortFilterProxyModel::sourceModelChanged, this, [this]() {
        sort(0, m_sortOrder);
    });
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

QString AddonFilterModel::sortMode() const {
    return m_sortMode;
}

void AddonFilterModel::setSortMode(const QString& mode) {
    if (m_sortMode != mode) {
        m_sortMode = mode;
        sort(0, m_sortOrder);
        emit sortModeChanged();
    }
}

Qt::SortOrder AddonFilterModel::sortOrder() const {
    return m_sortOrder;
}

void AddonFilterModel::setSortOrder(Qt::SortOrder order) {
    if (m_sortOrder != order) {
        m_sortOrder = order;
        sort(0, m_sortOrder);
        emit sortModeChanged();
    }
}

void AddonFilterModel::refreshFilter() {
    invalidateFilter();
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

bool AddonFilterModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
    if (!sourceModel())
        return false;

    // Prioritize addons with updates — they float to the top
    bool leftUpdate = sourceModel()->data(left, AddonModel::HasUpdateRole).toBool();
    bool rightUpdate = sourceModel()->data(right, AddonModel::HasUpdateRole).toBool();
    if (leftUpdate != rightUpdate)
        return leftUpdate;  // left has update, right doesn't → left comes first

    int leftVal = 0, rightVal = 0;

    if (m_sortMode == QStringLiteral("downloads")) {
        leftVal = sourceModel()->data(left, AddonModel::DownloadsRole).toInt();
        rightVal = sourceModel()->data(right, AddonModel::DownloadsRole).toInt();
    } else if (m_sortMode == QStringLiteral("monthly")) {
        leftVal = sourceModel()->data(left, AddonModel::DownloadsMonthlyRole).toInt();
        rightVal = sourceModel()->data(right, AddonModel::DownloadsMonthlyRole).toInt();
    } else if (m_sortMode == QStringLiteral("favorites")) {
        leftVal = sourceModel()->data(left, AddonModel::FavoritesRole).toInt();
        rightVal = sourceModel()->data(right, AddonModel::FavoritesRole).toInt();
    } else if (m_sortMode == QStringLiteral("api")) {
        leftVal = sourceModel()->data(left, AddonModel::ApiVersionRole).toInt();
        rightVal = sourceModel()->data(right, AddonModel::ApiVersionRole).toInt();
    } else if (m_sortMode == QStringLiteral("title")) {
        int cmp = QString::localeAwareCompare(
            sourceModel()->data(left, AddonModel::TitleRole).toString(),
            sourceModel()->data(right, AddonModel::TitleRole).toString());
        return cmp < 0;
    }

    if (leftVal != rightVal)
        return leftVal < rightVal;

    return QString::localeAwareCompare(
        sourceModel()->data(left, AddonModel::TitleRole).toString(),
        sourceModel()->data(right, AddonModel::TitleRole).toString()) < 0;
}