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

public:
    explicit AddonFilterModel(QObject* parent = nullptr);

    Q_INVOKABLE void setShowInstalledOnly(bool installed);
    Q_INVOKABLE void setCategoryFilter(const QString& categoryId);

    QString categoryFilter() const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

signals:
    void categoryFilterChanged();

private slots:
    void onSourceModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void flushPendingFilterInvalidation();

private:
    bool m_showInstalledOnly = false;
    QString m_categoryFilter;
    mutable QSet<int> m_installedRowsCache;
    mutable bool m_cacheValid = false;
    QTimer m_invalidationTimer;
    bool m_filterInvalidationPending = false;

    void invalidateCache();
    void scheduleFilterInvalidation();
};