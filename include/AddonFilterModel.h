#pragma once

#include <QSortFilterProxyModel>
#include <QtQml>
#include <QSet>
#include <QTimer>

class AddonFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(AddonFilterModel)
    Q_PROPERTY(QString categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY categoryFilterChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)

public:
    explicit AddonFilterModel(QObject* parent = nullptr);

    Q_INVOKABLE void setShowInstalledOnly(bool installed);
    Q_INVOKABLE void setCategoryFilter(const QString& categoryId);
    Q_INVOKABLE void setSearchText(const QString& text);
    Q_INVOKABLE void setExcludeBelowApiVersion(int minApi);
    Q_INVOKABLE void refreshFilter();

    QString categoryFilter() const;
    QString searchText() const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

signals:
    void categoryFilterChanged();
    void searchTextChanged();

private slots:
    void onSourceModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void flushPendingFilterInvalidation();

private:
    bool m_showInstalledOnly = false;
    QString m_categoryFilter;
    QString m_searchText;
    int m_excludeBelowApiVersion = 0;
    mutable QSet<int> m_installedRowsCache;
    mutable bool m_cacheValid = false;
    QTimer m_invalidationTimer;
    bool m_filterInvalidationPending = false;

    void invalidateCache();
    void scheduleFilterInvalidation();
};