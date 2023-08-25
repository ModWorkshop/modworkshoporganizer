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

#ifndef MWSACCESSMANAGER_H
#define MWSACCESSMANAGER_H

#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressDialog>
#include <QTimer>
#include <QWebSocket>
#include <set>

namespace MOBase
{
class IPluginGame;
}
class MWSAccessManager;
class Settings;

/**
 * @brief access manager extended to handle mws links
 **/
class MWSAccessManager : public QNetworkAccessManager
{
  Q_OBJECT
public:
  MWSAccessManager(QObject* parent, Settings* s, const QString& moVersion);

  void setTopLevelWidget(QWidget* w);

  void apiCheck(const QString& apiKey, bool force = false);

  void showCookies() const;

  void clearCookies();

  QString userAgent(const QString& subModule = QString()) const;
  const QString& MOVersion() const;


signals:

  /**
   * @brief emitted when a mws:// link is opened
   *
   * @param url the mws-link
   **/
  void requestMWSDownload(const QString& url);

protected:
  virtual QNetworkReply* createRequest(QNetworkAccessManager::Operation operation,
                                       const QNetworkRequest& request,
                                       QIODevice* device);

  QWidget* m_TopLevel;
  Settings* m_Settings;
  QString m_MOVersion;
};

  void startProgress();
  void stopProgress();

#endif  // MWSACCESSMANAGER_H
