// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Fabian Köhler <me@fkoehler.org>

#include "status.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <algorithm>

void Status::refresh(const QString &executable) {
  QProcess process;
  process.start(executable, {"status", "--json"});

  if (!process.waitForFinished(1000)) {
    qWarning() << "Failed to get tailscale status";
    return;
  }

  read(QJsonDocument::fromJson(process.readAllStandardOutput()).object());

  emit peersChanged(m_peers);
}

void Status::read(const QJsonObject &json) {
  if (json.contains("Version") && json["Version"].isString()) {
    m_version = json["Version"].toString();
  } else {
    qWarning() << "Cannot find string \"Version\"";
  }

  if (json.contains("TUN") && json["TUN"].isBool()) {
    m_is_tun = json["TUN"].toBool();
  } else {
    qWarning() << "Cannot find bool \"TUN\"";
  }

  if (json.contains("BackendState") && json["BackendState"].isString()) {
    m_backend_state = json["BackendState"].toString();
  } else {
    qWarning() << "Cannot find string \"BackendState\"";
  }

  if (json.contains("Self") && json["Self"].isObject()) {
    if (m_self == nullptr) {
      m_self = new Peer();
    }
    m_self->read(json["Self"].toObject());
  } else {
    qWarning() << "Cannot find object \"Self\"";
  }

  m_peers.clear();
  if (json.contains("Peer") && json["Peer"].isObject()) {
    const auto peers_object = json["Peer"].toObject();
    for (const auto &key : peers_object.keys()) {
      Peer *peer = new Peer();
      peer->read(peers_object[key].toObject());
      m_peers.append(peer);
    }
  } else {
    qWarning() << "Cannot find object \"Peer\"";
  }

  std::sort(m_peers.begin(), m_peers.end(),
            [](const Peer *a, const Peer *b) { return a->id() < b->id(); });
}

const QString &Status::version() const { return m_version; }

bool Status::isTUN() const { return m_is_tun; }

const QString &Status::backendState() const { return m_backend_state; }

const Peer *Status::self() const { return m_self; }

const QList<Peer *> &Status::peers() const { return m_peers; }