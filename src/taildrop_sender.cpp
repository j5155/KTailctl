#include "taildrop_sender.h"
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <libtailctlpp.h>

TaildropSendThread::TaildropSendThread(const QString &target, const QStringList &files, QObject *parent)
    : QThread(parent)
    , mTarget(target)
    , mFiles(files)
    , mBytesSent(0)
    , mBytesTotal(0)
{
    for (const auto &file : files) {
        mBytesTotal += static_cast<quint64>(QFileInfo(file).size());
    }
}

void TaildropSendThread::run()
{
    QByteArray targetBytes = mTarget.toUtf8();
    GoString target{targetBytes.constData(), targetBytes.length()};

    QByteArray fileBytes;
    GoString file;
    for (const auto &f : mFiles) {
        quint64 currentFileBytesSent = 0ul;
        QByteArray fileBytes = f.toUtf8();
        file.p = fileBytes.constData();
        file.n = fileBytes.length();
        tailscale_send_file(target, file, [](unsigned long n) {
            qDebug() << "Bytes sent" << n;
        });
        mBytesSent += static_cast<quint64>(QFileInfo(f).size());
    }
    mBytesSent = mBytesTotal;
}

TaildropSendJob::TaildropSendJob(const QString &target, const QStringList &files, QObject *parent)
    : KJob(parent)
{
    mThread = new TaildropSendThread(target, files, this);
    connect(mThread, &TaildropSendThread::finished, this, &TaildropSendJob::emitResult);
}

TaildropSendJob *TaildropSendJob::selectAndSendFiles(const QString &target)
{
    auto job = new TaildropSendJob(target, QFileDialog::getOpenFileNames(nullptr, "Select files to send", QDir::homePath()));
    job->start();
    return job;
}

void TaildropSendJob::start()
{
    mThread->start();
}

QmlTaildropSender::QmlTaildropSender(QObject *parent)
    : QObject(parent)
{
}

void QmlTaildropSender::selectAndSendFiles(const QString &target)
{
    TaildropSendJob::selectAndSendFiles(target);
}
