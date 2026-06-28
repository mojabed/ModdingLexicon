#include "descriptionparser.h"

static QString extractDivByClass(const QString& html, const QString& className)
{
    QString search = "class=\"" + className + "\"";
    int start = html.indexOf(search, 0, Qt::CaseInsensitive);
    if (start == -1) {
        search = "class='" + className + "'";
        start = html.indexOf(search, 0, Qt::CaseInsensitive);
    }
    if (start == -1)
        return {};

    start = html.indexOf('>', start);
    if (start == -1)
        return {};
    ++start;

    int depth = 1;
    int pos = start;
    while (pos < html.length() && depth > 0) {
        int open = html.indexOf("<div", pos, Qt::CaseInsensitive);
        int close = html.indexOf("</div", pos, Qt::CaseInsensitive);

        if (close == -1)
            break;

        if (open != -1 && open < close) {
            ++depth;
            pos = open + 4;
        } else {
            --depth;
            if (depth == 0)
                return html.mid(start, close - start).trimmed();
            pos = close + 6;
        }
    }
    return {};
}

static QString stripTags(const QString& html)
{
    QString out;
    out.reserve(html.size());
    bool inTag = false;
    for (const QChar& ch : html) {
        if (ch == u'<')
            inTag = true;
        else if (ch == u'>')
            inTag = false;
        else if (!inTag)
            out += ch;
    }
    return out.trimmed();
}

QString extractDescription(const QString& html)
{
    static const char* descMarkers[] = {
        "<h2>Description</h2>", "<h3>Description</h3>",
        "<h2>About</h2>", "<h3>About</h3>",
        ">Description<", ">Description:</"
    };
    for (const char* marker : descMarkers) {
        int idx = html.indexOf(QLatin1StringView(marker), 0, Qt::CaseInsensitive);
        if (idx != -1) {
            int start = html.indexOf(QLatin1Char('>'), idx) + 1;
            int end = html.length();
            static const char* stoppers[] = {
                "<h2", "<h3", "<h4", "<hr", "download",
                "file-info", "fileinfo", "changelog", "comments"
            };
            for (const char* stop : stoppers) {
                int s = html.indexOf(QLatin1StringView(stop), start, Qt::CaseInsensitive);
                if (s != -1 && s < end)
                    end = s;
            }
            if (end - start > 100)
                return html.mid(start, end - start).trimmed();
        }
    }

    static const char* classes[] = {
        "postmessage", "esoui-description", "description",
        "mod-description", "addon-description"
    };
    for (const char* cls : classes) {
        QString desc = extractDivByClass(html, QString::fromLatin1(cls));
        if (!desc.isEmpty())
            return desc;
    }

    QString out;
    out.reserve(html.size());
    int i = 0;
    while (i < html.size()) {
        if (html.at(i) == u'<' && i + 6 < html.size()) {
            QChar c1 = html.at(i + 1);
            QChar c2 = html.at(i + 2);
            if ((c1 == u's' || c1 == u'S') && (c2 == u'c' || c2 == u'C')) {
                int end = html.indexOf(QLatin1StringView("</script>"), i + 7, Qt::CaseInsensitive);
                if (end != -1) {
                    out += u' ';
                    i = end + 9;
                    continue;
                }
            } else if ((c1 == u's' || c1 == u'S') && (c2 == u't' || c2 == u'T')) {
                int end = html.indexOf(QLatin1StringView("</style>"), i + 6, Qt::CaseInsensitive);
                if (end != -1) {
                    out += u' ';
                    i = end + 8;
                    continue;
                }
            }
        }
        out += html.at(i);
        ++i;
    }

    QString trimmed = out.trimmed();
    return trimmed.size() > 100 ? trimmed : QString();
}
