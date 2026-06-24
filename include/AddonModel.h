#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QtQml>

#include "ModType.h"

class AddonModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(AddonModel)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum AddonRoles {
        IdRole = Qt::UserRole + 1,
        CategoryIdRole,
        TitleRole,
        AuthorRole,
        VersionRole,
        LastUpdateRole,
        LastUpdatedRole,
        FileInfoUriRole,
        GameVersionsRole,
        ChecksumRole,
        LibraryRole,
        DonationUrlRole,
        IsInstalledRole,
        InstallPathRole,
        SizeInBytesRole,
        DownloadUrlRole,
        DownloadsRole,
        DownloadsMonthlyRole,
        FavoritesRole,
        HasUpdateRole,
        FormattedDateRole,
        IconSourceRole
    };
    Q_ENUM(AddonRoles)

    explicit AddonModel(QObject* parent = nullptr);
    ~AddonModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;

    Q_INVOKABLE void setMods(const QList<ModInfo>& mods);
    Q_INVOKABLE void clear();
    Q_INVOKABLE ModInfo getModAt(int index) const;

signals:
    void countChanged();

private:
    QList<ModInfo> m_mods;
};