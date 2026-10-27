// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
// The QOR core, built from demiurge-chain at the pinned commit (QOR-0003), answers QOR OS through its C bridge: a
// request crosses as JSON and its answer comes back, an amount crosses as a decimal string of embers and is never
// rounded, and a refusal comes back as the error an interface reads. Nothing the bridge offers answers with a key.

#include "qor_core.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QTest>

using namespace Qt::StringLiterals;

namespace {

// One request through the bridge. Every answer qor_core_call returns is given back to qor_core_free, once.
QJsonObject call(const QString &command, const QJsonObject &args = {})
{
    const QByteArray request =
        QJsonDocument(QJsonObject { { u"command"_s, command }, { u"args"_s, args } }).toJson(QJsonDocument::Compact);
    char *answer = qor_core_call(request.constData());
    const QJsonObject result = QJsonDocument::fromJson(QByteArray(answer)).object();
    qor_core_free(answer);
    return result;
}

QJsonObject ok(const QJsonObject &answer) { return answer.value(u"ok"_s).toObject(); }

QString errorKind(const QJsonObject &answer) { return answer.value(u"error"_s).toObject().value(u"kind"_s).toString(); }

} // namespace

class TestQorCore : public QObject
{
    Q_OBJECT

private slots:
    void the_core_says_its_version()
    {
        const QString version = QString::fromUtf8(qor_core_version());
        QVERIFY(!version.isEmpty());
        QCOMPARE(ok(call(u"core_version"_s)).value(u"version"_s).toString(), version);
    }

    void the_units_are_cgt_sparks_and_embers()
    {
        const QJsonObject units = ok(call(u"token_info"_s));
        QCOMPARE(units.value(u"symbol"_s).toString(), u"CGT"_s);
        QCOMPARE(units.value(u"decimals"_s).toInt(), 18);
        QCOMPARE(units.value(u"sub_units_per_cgt"_s).toInt(), 1000);
        QCOMPARE(units.value(u"atomic_unit"_s).toString(), u"ember"_s);
    }

    void an_amount_typed_in_cgt_or_sparks_is_read_exactly()
    {
        const QJsonObject read = ok(call(u"parse_amount"_s, { { u"text"_s, u"1.25 CGT"_s } }));
        QCOMPARE(read.value(u"embers"_s).toString(), u"1250000000000000000"_s);
        QCOMPARE(read.value(u"sparks"_s).toString(), u"1,250 Sparks"_s);
        QCOMPARE(ok(call(u"parse_amount"_s, { { u"text"_s, u"250 Sparks"_s } })).value(u"embers"_s).toString(),
                 u"250000000000000000"_s);
    }

    void the_largest_amount_crosses_as_a_string_and_is_never_rounded()
    {
        // u128's largest value: far beyond what a double holds exactly, so it must stay a string end to end.
        const QJsonObject shown =
            ok(call(u"format_sparks"_s, { { u"embers"_s, u"340282366920938463463374607431768211455"_s } }));
        QVERIFY2(shown.value(u"cgt"_s).toString().startsWith(u"340,282,366,920,938,463,463."_s),
                 qPrintable(shown.value(u"cgt"_s).toString()));
    }

    void excess_precision_is_refused_never_truncated()
    {
        QCOMPARE(errorKind(call(u"parse_amount"_s, { { u"text"_s, u"0.0000000000000000001"_s } })), u"bad_amount"_s);
        for (const QString &bad : { u"-1"_s, u"+1"_s, u""_s, u"1e3"_s, u"1.0"_s })
            QCOMPARE(errorKind(call(u"format_sparks"_s, { { u"embers"_s, bad } })), u"bad_amount"_s);
    }

    void an_unknown_command_is_refused_and_no_command_reads_a_key()
    {
        QCOMPARE(errorKind(call(u"vault_secret"_s)), u"internal"_s);
        QCOMPARE(errorKind(call(u"parse_amount"_s, { { u"amount"_s, u"1"_s } })), u"internal"_s);
    }

    void a_malformed_request_still_gets_an_answer()
    {
        char *answer = qor_core_call("not json");
        QVERIFY(answer);
        const QJsonObject result = QJsonDocument::fromJson(QByteArray(answer)).object();
        qor_core_free(answer);
        QVERIFY(result.contains(u"error"_s));
        qor_core_free(nullptr); // null is ignored
    }
};

QTEST_GUILESS_MAIN(TestQorCore)
#include "tst_qorcore.moc"
