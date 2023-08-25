/*
Copyright (C) 2012 Sebastian Herbord. All rights reserved.

This file is part of Mod Organizer.

Mod Organizer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Mod Organizer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Mod Organizer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mwsaccessmanager.h"
#include "iplugingame.h"
#include "mwsinterface.h"
#include "mwsurl.h"
#include "persistentcookiejar.h"
#include "report.h"
#include "selfupdater.h"
#include "settings.h"
#include "utility.h"
#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QPushButton>
#include <QThread>
#include <QUrlQuery>

using namespace MOBase;
using namespace std::chrono_literals;

const QString ModworkshopBaseUrl("https://api.modworkshop.net/");


MWSAccessManager::MWSAccessManager(QObject* parent, Settings* s,
                                   const QString& moVersion)
    : QNetworkAccessManager(parent), m_Settings(s), m_MOVersion(moVersion)
{
  if (m_Settings) {
    setCookieJar(new PersistentCookieJar(QDir::fromNativeSeparators(
        m_Settings->paths().cache() + "/modworkshop_cookies.dat")));
  }
}

void MWSAccessManager::setTopLevelWidget(QWidget* w)
{
  if (w) {
    if (m_ProgressDialog) {
      m_ProgressDialog->setParentWidget(w);
    }
  } else {
    m_ProgressDialog.reset();
  }
}

QNetworkReply*
MWSAccessManager::createRequest(QNetworkAccessManager::Operation operation,
                                const QNetworkRequest& request, QIODevice* device)
{
  if (request.url().scheme() != "mws") {
    return QNetworkAccessManager::createRequest(operation, request, device);
  }
  if (operation == GetOperation) {
    emit requestMWSDownload(request.url().toString());

    // eat the request, everything else will be done by the download manager
    return QNetworkAccessManager::createRequest(QNetworkAccessManager::GetOperation,
                                                QNetworkRequest(QUrl()));
  } else if (operation == PostOperation) {
    return QNetworkAccessManager::createRequest(operation, request, device);
    ;
  } else {
    return QNetworkAccessManager::createRequest(operation, request, device);
  }
}

void MWSAccessManager::showCookies() const
{
  QUrl url(ModworkshopBaseUrl + "/");
  for (const QNetworkCookie& cookie : cookieJar()->cookiesForUrl(url)) {
    log::debug("{} - {} (expires: {})", cookie.name().constData(),
               cookie.value().constData(), cookie.expirationDate().toString());
  }
}

void MWSAccessManager::clearCookies()
{
  PersistentCookieJar* jar = qobject_cast<PersistentCookieJar*>(cookieJar());
  if (jar != nullptr) {
    jar->clear();
  } else {
    log::warn("failed to clear cookies, invalid cookie jar");
  }
}


bool MWSAccessManager::validated() const
{
  if (m_validationState == Valid) {
    return true;
  }

  if (m_validator.isActive()) {
    const_cast<MWSAccessManager*>(this)->startProgress();
  }

  return false;
}

void MWSAccessManager::apiCheck(const QString& apiKey, bool force)
{

  if (m_Settings && m_Settings->network().offlineMode()) {
    return;
  }
}

const QString& MWSAccessManager::MOVersion() const
{
  return m_MOVersion;
}

QString MWSAccessManager::userAgent(const QString& subModule) const
{
  QStringList comments;
  QString os;
  if (QSysInfo::productType() == "windows")
    comments << ((QSysInfo::kernelType() == "winnt") ? "Windows_NT " : "Windows ") +
                    QSysInfo::kernelVersion();
  else
    comments << QSysInfo::kernelType().left(1).toUpper() + QSysInfo::kernelType().mid(1)
             << QSysInfo::productType().left(1).toUpper() +
                    QSysInfo::kernelType().mid(1) + " " + QSysInfo::productVersion();
  if (!subModule.isEmpty()) {
    comments << "module: " + subModule;
  }
  comments << ((QSysInfo::buildCpuArchitecture() == "x86_64") ? "x64" : "x86");

  return QString("Mod Organizer/%1 (%2) Qt/%3")
      .arg(m_MOVersion, comments.join("; "), qVersion());
}
