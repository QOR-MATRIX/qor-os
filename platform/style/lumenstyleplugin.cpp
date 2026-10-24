// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "lumenstyle.h"

#include <QtWidgets/QStylePlugin>

using namespace Qt::StringLiterals;

class LumenStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QStyleFactoryInterface_iid FILE "lumen.json")

public:
    QStyle *create(const QString &key) override
    {
        return key.compare(u"lumen"_s, Qt::CaseInsensitive) == 0 ? new qtos::LumenStyle() : nullptr;
    }
};

#include "lumenstyleplugin.moc"
