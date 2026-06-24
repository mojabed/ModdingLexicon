#include "AddonModel.h"

static QString iconForCategoryId(const QString& categoryId) {
    if (categoryId.isEmpty()) {
        return "qrc:/icons/vial.png";
    }

    const auto hash = qHash(categoryId);

    switch (hash % 6u) {
    case 0:
        return "qrc:/icons/vial.png";
    case 1:
        return "qrc:/icons/book.png";
    case 2:
        return "qrc:/icons/shield.png";
    case 3:
        return "qrc:/icons/star.png";
    case 4:
        return "qrc:/icons/gear.png";
    default:
        return "qrc:/icons/puzzle.png";
    }
}

AddonModel::AddonModel(QObject* parent)
    : QAbstractListModel(parent) {}

AddonModel::~AddonModel() {}

int AddonModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return m_mods.count();
}

QVariant AddonModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_mods.count())
        return QVariant();

    const ModInfo& mod = m_mods.at(index.row());

    switch (role) {
    case IdRole:
        return mod.id;
    case CategoryIdRole:
        return mod.categoryId;
    case TitleRole:
        return mod.title;
    case AuthorRole:
        return mod.author;
    case VersionRole:
        return mod.version;
    case LastUpdateRole:
        return mod.lastUpdate;
    case LastUpdatedRole:
        return mod.lastUpdated;
    case FileInfoUriRole:
        return mod.fileInfoUri;
    case GameVersionsRole:
        return mod.gameVersions;
    case ChecksumRole:
        return mod.checksum;
    case LibraryRole:
        return mod.library;
    case DonationUrlRole:
        return mod.donationUrl;
    case IsInstalledRole:
        return mod.isInstalled;
    case InstallPathRole:
        return mod.installPath;
    case SizeInBytesRole:
        return mod.sizeInBytes;
    case DownloadUrlRole:
        return mod.downloadUrl;
    case DownloadsRole:
        return mod.downloads;
    case DownloadsMonthlyRole:
        return mod.downloadsMonthly;
    case FavoritesRole:
        return mod.favorites;
    case HasUpdateRole:
        return mod.hasUpdate;
    case FormattedDateRole:
        return mod.getFormattedDate();
    case IconSourceRole:
        return iconForCategoryId(mod.categoryId);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> AddonModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "modId";
    roles[CategoryIdRole] = "categoryId";
    roles[TitleRole] = "title";
    roles[AuthorRole] = "author";
    roles[VersionRole] = "version";
    roles[LastUpdateRole] = "lastUpdate";
    roles[LastUpdatedRole] = "lastUpdated";
    roles[FileInfoUriRole] = "fileInfoUri";
    roles[GameVersionsRole] = "gameVersions";
    roles[ChecksumRole] = "checksum";
    roles[LibraryRole] = "library";
    roles[DonationUrlRole] = "donationUrl";
    roles[IsInstalledRole] = "isInstalled";
    roles[InstallPathRole] = "installPath";
    roles[SizeInBytesRole] = "sizeInBytes";
    roles[DownloadUrlRole] = "downloadUrl";
    roles[DownloadsRole] = "downloads";
    roles[DownloadsMonthlyRole] = "downloadsMonthly";
    roles[FavoritesRole] = "favorites";
    roles[HasUpdateRole] = "hasUpdate";
    roles[FormattedDateRole] = "formattedDate";
    roles[IconSourceRole] = "iconSource";
    return roles;
}

int AddonModel::count() const {
    return m_mods.count();
}

void AddonModel::setMods(const QList<ModInfo>& mods) {
    beginResetModel();
    m_mods = mods;
    endResetModel();
    emit countChanged();
}

void AddonModel::clear() {
    beginResetModel();
    m_mods.clear();
    endResetModel();
    emit countChanged();
}

ModInfo AddonModel::getModAt(int index) const {
    if (index >= 0 && index < m_mods.count()) {
        return m_mods.at(index);
    }
    return ModInfo();
}