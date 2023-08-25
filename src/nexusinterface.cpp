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

#include "modworkshopinterface.h"

#include "bbcode.h"
#include "iplugingame.h"
#include "mwsaccessmanager.h"
#include "selectiondialog.h"
#include "settings.h"
#include "shared/util.h"
#include <log.h>
#include <moassert.h>
#include <utility.h>

#include <QApplication>
#include <QJsonDocument>
#include <QNetworkCookieJar>
#include <QRegularExpression>

#include <regex>

using namespace MOBase;
using namespace MOShared;

//void throttledWarning(const APIUserAccount& user)
//{
//  log::error("You have fewer than {} requests remaining ({}). Only downloads and "
//             "login validation are being allowed.",
//             APIUserAccount::ThrottleThreshold, user.remainingRequests());
//}

ModworkshopBridge::ModworkshopBridge(PluginContainer* pluginContainer, const QString& subModule)
    : m_Interface(&ModworkshopInterface::instance()), m_SubModule(subModule)
{}

void ModworkshopBridge::requestDescription(QString gameName, int modID, QVariant userData)
{
  m_RequestIDs.insert(
      m_Interface->requestDescription(gameName, modID, this, userData, m_SubModule));
}

void ModworkshopBridge::requestFiles(QString gameName, int modID, QVariant userData)
{
  m_RequestIDs.insert(
      m_Interface->requestFiles(gameName, modID, this, userData, m_SubModule));
}

void ModworkshopBridge::requestFileInfo(QString gameName, int modID, int fileID,
                                  QVariant userData)
{
  m_RequestIDs.insert(m_Interface->requestFileInfo(gameName, modID, fileID, this,
                                                   userData, m_SubModule));
}

void ModworkshopBridge::requestDownloadURL(QString gameName, int modID, int fileID,
                                     QVariant userData)
{
  m_RequestIDs.insert(m_Interface->requestDownloadURL(gameName, modID, fileID, this,
                                                      userData, m_SubModule));
}

//void ModworkshopBridge::requestToggleEndorsement(QString gameName, int modID,
//                                           QString modVersion, bool endorse,
//                                           QVariant userData)
//{
//  m_RequestIDs.insert(m_Interface->requestToggleEndorsement(
//      gameName, modID, modVersion, endorse, this, userData, m_SubModule));
//}

//void ModworkshopBridge::requestToggleTracking(QString gameName, int modID, bool track,
//                                        QVariant userData)
//{
//  m_RequestIDs.insert(m_Interface->requestToggleTracking(gameName, modID, track, this,
//                                                         userData, m_SubModule));
//}

void ModworkshopBridge::mwsDescriptionAvailable(QString gameName, int modID,
                                          QVariant userData, QVariant resultData,
                                          int requestID)
{
  std::set<int>::iterator iter = m_RequestIDs.find(requestID);
  if (iter != m_RequestIDs.end()) {
    m_RequestIDs.erase(iter);

    emit descriptionAvailable(gameName, modID, userData, resultData);
  }
}

void ModworkshopBridge::mwsFilesAvailable(QString gameName, int modID, QVariant userData,
                                    QVariant resultData, int requestID)
{
  std::set<int>::iterator iter = m_RequestIDs.find(requestID);
  if (iter != m_RequestIDs.end()) {
    m_RequestIDs.erase(iter);

    QList<ModRepositoryFileInfo*> fileInfoList;

    QVariantMap resultInfo = resultData.toMap();
    QList resultList       = resultInfo["files"].toList();

    for (const QVariant& file : resultList) {
      ModRepositoryFileInfo temp;
      QVariantMap fileInfo = file.toMap();
      temp.uri             = fileInfo["file_name"].toString();
      temp.name            = fileInfo["name"].toString();
      temp.description     = BBCode::convertToHTML(fileInfo["description"].toString());
      temp.license         = BBCode::convertToHTML(fileInfo["license"].toString());
      temp.changelog     = BBCode::convertToHTML(fileInfo["changelog"].toString());
      temp.instructions     = BBCode::convertToHTML(fileInfo["instructions"].toString());
      temp.version         = VersionInfo(fileInfo["version"].toString());
      temp.categoryID      = fileInfo["category_id"].toInt();
      temp.fileID          = fileInfo["file_id"].toInt();
      temp.fileSize        = fileInfo["size"].toInt();
      fileInfoList.append(&temp);
    }

    emit filesAvailable(gameName, modID, userData, fileInfoList);
  }
}

void ModworkshopBridge::mwsFileInfoAvailable(QString gameName, int modID, int fileID,
                                       QVariant userData, QVariant resultData,
                                       int requestID)
{
  std::set<int>::iterator iter = m_RequestIDs.find(requestID);
  if (iter != m_RequestIDs.end()) {
    m_RequestIDs.erase(iter);
    emit fileInfoAvailable(gameName, modID, fileID, userData, resultData);
  }
}

void ModworkshopBridge::mwsDownloadURLsAvailable(QString gameName, int modID, int fileID,
                                           QVariant userData, QVariant resultData,
                                           int requestID)
{
  std::set<int>::iterator iter = m_RequestIDs.find(requestID);
  if (iter != m_RequestIDs.end()) {
    m_RequestIDs.erase(iter);
    emit downloadURLsAvailable(gameName, modID, fileID, userData, resultData);
  }
}

//void ModworkshopBridge::mwsEndorsementsAvailable(QVariant userData, QVariant resultData,
//                                           int requestID)
//{
//  std::set<int>::iterator iter = m_RequestIDs.find(requestID);
//  if (iter != m_RequestIDs.end()) {
//    m_RequestIDs.erase(iter);
//    emit endorsementsAvailable(userData, resultData);
//  }
//}

//void ModworkshopBridge::mwsEndorsementToggled(QString gameName, int modID, QVariant userData,
//                                        QVariant resultData, int requestID)
//{
//  std::set<int>::iterator iter = m_RequestIDs.find(requestID);
//  if (iter != m_RequestIDs.end()) {
//    m_RequestIDs.erase(iter);
//    emit endorsementToggled(gameName, modID, userData, resultData);
//  }
//}

//void ModworkshopBridge::mwsTrackedModsAvailable(QVariant userData, QVariant resultData,
//                                          int requestID)
//{
//  std::set<int>::iterator iter = m_RequestIDs.find(requestID);
//  if (iter != m_RequestIDs.end()) {
//    m_RequestIDs.erase(iter);
//    emit trackedModsAvailable(userData, resultData);
//  }
//}

//void ModworkshopBridge::mwsTrackingToggled(QString gameName, int modID, QVariant userData,
 //                                    bool tracked, int requestID)
//{
//  std::set<int>::iterator iter = m_RequestIDs.find(requestID);
//  if (iter != m_RequestIDs.end()) {
//    m_RequestIDs.erase(iter);
//    emit trackingToggled(gameName, modID, userData, tracked);
//  }
//}

void ModworkshopBridge::mwsRequestFailed(QString gameName, int modID, int fileID,
                                   QVariant userData, int requestID, int errorCode,
                                   const QString& errorMessage)
{
  std::set<int>::iterator iter = m_RequestIDs.find(requestID);
  if (iter != m_RequestIDs.end()) {
    m_RequestIDs.erase(iter);
    emit requestFailed(gameName, modID, fileID, userData, errorCode, errorMessage);
  }
}

QAtomicInt ModworkshopInterface::mwsRequestInfo::s_NextID(0);

static ModworkshopInterface* g_instance = nullptr;

ModworkshopInterface::ModworkshopInterface(Settings* s) : m_PluginContainer(nullptr)
{
  MO_ASSERT(!g_instance);
  g_instance = this;

  m_MOVersion = createVersionInfo();

  m_AccessManager = new mwsAccessManager(this, s, m_MOVersion.displayString(3));

  m_DiskCache = new QNetworkDiskCache(this);

  connect(m_AccessManager, SIGNAL(requestmwsDownload(QString)), this,
          SLOT(downloadRequestedmws(QString)));
}

ModworkshopInterface::~ModworkshopInterface()
{
  cleanup();

  MO_ASSERT(g_instance == this);
  g_instance = nullptr;
}

mwsAccessManager* ModworkshopInterface::getAccessManager()
{
  return m_AccessManager;
}

ModworkshopInterface& ModworkshopInterface::instance()
{
  MO_ASSERT(g_instance);
  return *g_instance;
}

void ModworkshopInterface::setCacheDirectory(const QString& directory)
{
  m_DiskCache->setCacheDirectory(directory);
  m_AccessManager->setCache(m_DiskCache);
}

void ModworkshopInterface::interpretModworkshopFileName(const QString& fileName, QString& modName,
                                            int& modID, bool query)
{
  // guess the mod name from the file name
  static const QRegularExpression complex(
      R"(^([0-9]+)-([a-zA-Z0-9_'"\-.() ]+?)@*.*?\.(zip|rar|7z))");
  // complex regex explanation:
  // group 1: ModworkshopID.
  // group 2: Modname
  // group 3: Version,
  // If no id is present the whole regex does not match.
  static const QRegularExpression simple(R"(^[^a-zA-Z]*([a-zA-Z_ ]+))");
  auto complexMatch = complex.match(fileName);
  auto simpleMatch  = simple.match(fileName);

  // assume not found
  modID = -1;

  if (complexMatch.hasMatch()) {
    modName = complexMatch.captured(2);
    if (!query && !complexMatch.captured(1).isNull()) {
      modID = complexMatch.captured(1).toInt();
    }
  } else if (simpleMatch.hasMatch()) {
    modName = simpleMatch.captured(0);
  } else {
    modName.clear();
  }

  if (query) {
    SelectionDialog selection(tr("Please pick the mod ID for \"%1\"").arg(fileName));
    int index   = 0;
    auto splits = fileName.split(QRegularExpression("[^0-9]"), Qt::KeepEmptyParts);
    for (auto substr : splits) {
      bool ok   = false;
      int value = substr.toInt(&ok);
      if (ok) {
        QString highlight(fileName);
        highlight.insert(index, " *");
        highlight.insert(index + substr.length() + 2, "* ");

        QStringList choice;
        choice << substr;
        choice << (index > 0 ? fileName.left(index - 1) : substr);
        selection.addChoice(substr, highlight, choice);
      }
      index += substr.length() + 1;
    }

    if (selection.numChoices() > 0) {
      if (selection.exec() == QDialog::Accepted) {
        auto choice = selection.getChoiceData().toStringList();
        modID       = choice.at(0).toInt();
        modName     = choice.at(1);
        modName     = modName.replace('_', ' ').trimmed();
        log::debug("user selected mod ID {} and mod name \"{}\"", modID, modName);
      } else {
        log::debug("user canceled mod ID selection");
      }
    } else {
      log::debug("no possible mod IDs found in file name");
    }
  }
}

bool ModworkshopInterface::isURLGameRelated(const QUrl& url) const
{
  QString const name(url.toString());
  return name.startsWith(getGameURL("") + "/");
}

QString ModworkshopInterface::getGameURL(QString gameName) const
{
  IPluginGame* game = getGame(gameName);
  if (game != nullptr) {
    QString gameModworkshopName = game->gameModworkshopName().toLower();
    if (gameModworkshopName.isEmpty()) {
      return "";
    } else {
      return "https://modworkshop.net/g/" + gameModworkshopName;
    }
  } else {
    log::error("getGameURL can't find plugin for {}", gameName);
    return "";
  }
}

QString ModworkshopInterface::getModURL(int modID, QString gameName = "") const
{
  return QString("https://modworkshop.net/mods/%1").arg(modID);
}

std::vector<std::pair<QString, QString>>
ModworkshopInterface::getGameChoices(const MOBase::IPluginGame* game)
{
  std::vector<std::pair<QString, QString>> choices;
  choices.push_back(
      std::pair<QString, QString>(game->gameShortName(), game->gameName()));
  for (QString gameName : game->validShortNames()) {
    for (auto gamePlugin : m_PluginContainer->plugins<IPluginGame>()) {
      if (gamePlugin->gameShortName().compare(gameName, Qt::CaseInsensitive) == 0) {
        choices.push_back(std::pair<QString, QString>(gamePlugin->gameShortName(),
                                                      gamePlugin->gameName()));
        break;
      }
    }
  }
  return choices;
}

bool ModworkshopInterface::isModURL(int modID, const QString& url) const
{
  if (QUrl(url) == QUrl(getModURL(modID))) {
    return true;
  }
}

void ModworkshopInterface::setPluginContainer(PluginContainer* pluginContainer)
{
  m_PluginContainer = pluginContainer;
}

int ModworkshopInterface::requestDescription(QString gameName, int modID, QObject* receiver,
                                       QVariant userData, const QString& subModule,
                                       MOBase::IPluginGame const* game)
{
  mwsRequestInfo requestInfo(modID, mwsRequestInfo::TYPE_DESCRIPTION, userData,
                             subModule, game);
  m_RequestQueue.enqueue(requestInfo);

  connect(this, SIGNAL(mwsDescriptionAvailable(QString, int, QVariant, QVariant, int)),
          receiver,
          SLOT(mwsDescriptionAvailable(QString, int, QVariant, QVariant, int)),
          Qt::UniqueConnection);

  connect(
      this, SIGNAL(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      receiver, SLOT(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      Qt::UniqueConnection);

  nextRequest();
  return requestInfo.m_ID;
}

int ModworkshopInterface::requestModInfo(QString gameName, int modID, QObject* receiver,
                                   QVariant userData, const QString& subModule,
                                   MOBase::IPluginGame const* game)
//{
//  if (m_User.shouldThrottle()) {
//    throttledWarning(m_User);
//    return -1;
//  }

  mwsRequestInfo requestInfo(modID, mwsRequestInfo::TYPE_MODINFO, userData, subModule,
                             game);
  m_RequestQueue.enqueue(requestInfo);

  connect(this, SIGNAL(mwsModInfoAvailable(QString, int, QVariant, QVariant, int)),
          receiver, SLOT(mwsModInfoAvailable(QString, int, QVariant, QVariant, int)),
          Qt::UniqueConnection);

  connect(
      this, SIGNAL(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      receiver, SLOT(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      Qt::UniqueConnection);

  nextRequest();
  return requestInfo.m_ID;
}

int ModworkshopInterface::requestUpdateInfo(QString gameName,
                                      ModworkshopInterface::UpdatePeriod period,
                                      QObject* receiver, QVariant userData,
                                      const QString& subModule,
                                      const MOBase::IPluginGame* game)
{
  //if (m_User.shouldThrottle()) {
  //  throttledWarning(m_User);
  //  return -1;
  //}

  mwsRequestInfo requestInfo(period, mwsRequestInfo::TYPE_CHECKUPDATES, userData,
                             subModule, game);
  m_RequestQueue.enqueue(requestInfo);

  connect(this, SIGNAL(mwsUpdateInfoAvailable(QString, QVariant, QVariant, int)),
          receiver, SLOT(mwsUpdateInfoAvailable(QString, QVariant, QVariant, int)),
          Qt::UniqueConnection);

  connect(
      this, SIGNAL(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      receiver, SLOT(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      Qt::UniqueConnection);

  nextRequest();
  return requestInfo.m_ID;
}

int ModworkshopInterface::requestUpdates(const int& modID, QObject* receiver,
                                   QVariant userData, QString gameName,
                                   const QString& subModule)
{
  //if (m_User.shouldThrottle()) {
  //  throttledWarning(m_User);
  //  return -1;
  //}

  IPluginGame* game = getGame(gameName);
  if (game == nullptr) {
    log::error("requestUpdates can't find plugin for {}", gameName);
    return -1;
  }

  mwsRequestInfo requestInfo(modID, mwsRequestInfo::TYPE_GETUPDATES, userData,
                             subModule, game);
  m_RequestQueue.enqueue(requestInfo);

  connect(this, SIGNAL(mwsUpdatesAvailable(QString, int, QVariant, QVariant, int)),
          receiver, SLOT(mwsUpdatesAvailable(QString, int, QVariant, QVariant, int)),
          Qt::UniqueConnection);

  connect(
      this, SIGNAL(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      receiver, SLOT(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      Qt::UniqueConnection);

  nextRequest();
  return requestInfo.m_ID;
}

void ModworkshopInterface::fakeFiles()
{
  static int id = 42;

  QVariantList result;
  QVariantMap fileMap;
  fileMap["uri"]          = "fakeURI";
  fileMap["name"]         = "fakeName";
  fileMap["description"]  = "fakeDescription";
  fileMap["license"]      = "fakeLicense";
  fileMap["changelog"]    = "fakeChangelog";
  fileMap["instructions"] = "fakeInstructions";
  fileMap["version"]      = "1.0.0";
  fileMap["category_id"]  = "1";
  fileMap["id"]           = "1";
  fileMap["size"]         = "512";
  result.append(fileMap);

  emit mwsFilesAvailable("fakeGame", 1234, "fake", result, id++);
}

int ModworkshopInterface::requestFiles(QString gameName, int modID, QObject* receiver,
                                 QVariant userData, const QString& subModule,
                                 MOBase::IPluginGame const* game)
{
  mwsRequestInfo requestInfo(modID, mwsRequestInfo::TYPE_FILES, userData, subModule,
                             game);
  m_RequestQueue.enqueue(requestInfo);
  connect(this, SIGNAL(mwsFilesAvailable(QString, int, QVariant, QVariant, int)),
          receiver, SLOT(mwsFilesAvailable(QString, int, QVariant, QVariant, int)),
          Qt::UniqueConnection);

  connect(
      this, SIGNAL(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      receiver, SLOT(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      Qt::UniqueConnection);

  nextRequest();
  return requestInfo.m_ID;
}

int ModworkshopInterface::requestFileInfo(QString gameName, int modID, int fileID,
                                    QObject* receiver, QVariant userData,
                                    const QString& subModule)
{
  IPluginGame* gamePlugin = getGame(gameName);
  if (gamePlugin == nullptr) {
    log::error("requestFileInfo can't find plugin for {}", gameName);
    return -1;
  }

  mwsRequestInfo requestInfo(modID, fileID, mwsRequestInfo::TYPE_FILEINFO, userData,
                             subModule, gamePlugin);
  m_RequestQueue.enqueue(requestInfo);

  connect(
      this, SIGNAL(mwsFileInfoAvailable(QString, int, int, QVariant, QVariant, int)),
      receiver, SLOT(mwsFileInfoAvailable(QString, int, int, QVariant, QVariant, int)),
      Qt::UniqueConnection);

  connect(
      this, SIGNAL(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      receiver, SLOT(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      Qt::UniqueConnection);

  nextRequest();
  return requestInfo.m_ID;
}

int ModworkshopInterface::requestDownloadURL(QString gameName, int modID, int fileID,
                                       QObject* receiver, QVariant userData,
                                       const QString& subModule,
                                       MOBase::IPluginGame const* game)
{
  mwsRequestInfo requestInfo(modID, fileID, mwsRequestInfo::TYPE_DOWNLOADURL, userData,
                             subModule, game);
  m_RequestQueue.enqueue(requestInfo);

  connect(this,
          SIGNAL(mwsDownloadURLsAvailable(QString, int, int, QVariant, QVariant, int)),
          receiver,
          SLOT(mwsDownloadURLsAvailable(QString, int, int, QVariant, QVariant, int)),
          Qt::UniqueConnection);

  connect(
      this, SIGNAL(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      receiver, SLOT(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      Qt::UniqueConnection);

  nextRequest();
  return requestInfo.m_ID;
}

//int ModworkshopInterface::requestEndorsementInfo(QObject* receiver, QVariant userData,
//                                           const QString& subModule)
//{
//  mwsRequestInfo requestInfo(mwsRequestInfo::TYPE_ENDORSEMENTS, userData, subModule);
//  m_RequestQueue.enqueue(requestInfo);
//
//  connect(this, SIGNAL(mwsEndorsementsAvailable(QVariant, QVariant, int)), receiver,
//          SLOT(mwsEndorsementsAvailable(QVariant, QVariant, int)),
//          Qt::UniqueConnection);
//
//  connect(
//      this, SIGNAL(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
//      receiver, SLOT(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
//      Qt::UniqueConnection);
//
//  nextRequest();
//  return requestInfo.m_ID;
//}

//int ModworkshopInterface::requestToggleEndorsement(QString gameName, int modID,
//                                             QString modVersion, bool endorse,
//                                             QObject* receiver, QVariant userData,
//                                             const QString& subModule,
//                                             MOBase::IPluginGame const* game)
//{
//  if (m_User.shouldThrottle()) {
//    throttledWarning(m_User);
//    return -1;
//  }

 // mwsRequestInfo requestInfo(modID, modVersion, mwsRequestInfo::TYPE_TOGGLEENDORSEMENT,
 //                            userData, subModule, game);
 // requestInfo.m_Endorse = endorse;
 // m_RequestQueue.enqueue(requestInfo);
 //
 // connect(this, SIGNAL(mwsEndorsementToggled(QString, int, QVariant, QVariant, int)),
 //         receiver, SLOT(mwsEndorsementToggled(QString, int, QVariant, QVariant, int)),
 //         Qt::UniqueConnection);
//
//  connect(
//      this, SIGNAL(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
//      receiver, SLOT(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
//      Qt::UniqueConnection);
//
//  nextRequest();
//  return requestInfo.m_ID;
//}

//int ModworkshopInterface::requestTrackingInfo(QObject* receiver, QVariant userData,
//                                        const QString& subModule)
//{
//  mwsRequestInfo requestInfo(mwsRequestInfo::TYPE_TRACKEDMODS, userData, subModule);
//  m_RequestQueue.enqueue(requestInfo);
//
//  connect(this, SIGNAL(mwsTrackedModsAvailable(QVariant, QVariant, int)), receiver,
//          SLOT(mwsTrackedModsAvailable(QVariant, QVariant, int)), Qt::UniqueConnection);
//
//  connect(
//      this, SIGNAL(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
//      receiver, SLOT(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
//      Qt::UniqueConnection);
//
//  nextRequest();
//  return requestInfo.m_ID;
//}

//int ModworkshopInterface::requestToggleTracking(QString gameName, int modID, bool track,
//                                          QObject* receiver, QVariant userData,
//                                          const QString& subModule,
//                                          MOBase::IPluginGame const* game)
//{
//  if (m_User.shouldThrottle()) {
//    throttledWarning(m_User);
//    return -1;
//  }
//
//  mwsRequestInfo requestInfo(modID, mwsRequestInfo::TYPE_TOGGLETRACKING, userData,
//                             subModule, game);
//  requestInfo.m_Track = track;
//  m_RequestQueue.enqueue(requestInfo);
//
//  connect(this, SIGNAL(mwsTrackingToggled(QString, int, QVariant, bool, int)), receiver,
//          SLOT(mwsTrackingToggled(QString, int, QVariant, bool, int)),
//          Qt::UniqueConnection);
//
//  connect(
//      this, SIGNAL(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
 //     receiver, SLOT(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
 //     Qt::UniqueConnection);
//
//  nextRequest();
//  return requestInfo.m_ID;
//}

int ModworkshopInterface::requestInfoFromMd5(QString gameName, QByteArray& hash,
                                       QObject* receiver, QVariant userData,
                                       const QString& subModule,
                                       MOBase::IPluginGame const* game)
{
  mwsRequestInfo requestInfo(hash, mwsRequestInfo::TYPE_FILEINFO_MD5, userData,
                             subModule, game);
  requestInfo.m_Hash = hash;
  requestInfo.m_AllowedErrors[QNetworkReply::NetworkError::ContentNotFoundError].append(
      404);
  requestInfo.m_IgnoreGenericErrorHandler = true;
  m_RequestQueue.enqueue(requestInfo);

  connect(this, SIGNAL(mwsFileInfoFromMd5Available(QString, QVariant, QVariant, int)),
          receiver, SLOT(mwsFileInfoFromMd5Available(QString, QVariant, QVariant, int)),
          Qt::UniqueConnection);

  connect(
      this, SIGNAL(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      receiver, SLOT(mwsRequestFailed(QString, int, int, QVariant, int, int, QString)),
      Qt::UniqueConnection);

  nextRequest();
  return requestInfo.m_ID;
}

IPluginGame* ModworkshopInterface::getGame(QString gameName) const
{
  auto gamePlugins        = m_PluginContainer->plugins<IPluginGame>();
  IPluginGame* gamePlugin = qApp->property("managed_game").value<IPluginGame*>();
  for (auto plugin : gamePlugins) {
    if (plugin != nullptr &&
        plugin->gameShortName().compare(gameName, Qt::CaseInsensitive) == 0) {
      gamePlugin = plugin;
      break;00
    }
  }
  return gamePlugin;
}

void ModworkshopInterface::cleanup()
{
  m_AccessManager = nullptr;
  m_DiskCache     = nullptr;
}

void ModworkshopInterface::clearCache()
{
  m_DiskCache->clear();
  m_AccessManager->clearCookies();
}

void ModworkshopInterface::nextRequest()
{
  if ((m_ActiveRequest.size() >= MAX_ACTIVE_DOWNLOADS) || m_RequestQueue.isEmpty()) {
    return;
  }

  mwsRequestInfo info = m_RequestQueue.dequeue();
  info.m_Timeout      = new QTimer(this);
  info.m_Timeout->setInterval(60000);

  QJsonObject postObject;
  QJsonDocument postData(postObject);
  bool requestIsDelete = false;

  QString url;
  if (!info.m_Reroute) {
    bool hasParams = false;
    switch (info.m_Type) {
    case mwsRequestInfo::TYPE_DESCRIPTION:
    case mwsRequestInfo::TYPE_MODINFO: {
      url = QString("%1/games/%2/mods/%3")
                .arg(info.m_URL)
                .arg(info.m_GameName)
                .arg(info.m_ModID);
    } break;
    case mwsRequestInfo::TYPE_CHECKUPDATES: {
      QString period;
      switch (info.m_UpdatePeriod) {
      case UpdatePeriod::DAY:
        period = "1d";
        break;
      case UpdatePeriod::WEEK:
        period = "1w";
        break;
      case UpdatePeriod::MONTH:
        period = "1m";
        break;
      }
      url = QString("%1/games/%2/mods/updated?period=%3")
                .arg(info.m_URL)
                .arg(info.m_GameName)
                .arg(period);
    } break;
    case mwsRequestInfo::TYPE_FILES:
    case mwsRequestInfo::TYPE_GETUPDATES: {
      url = QString("%1/games/%2/mods/%3/files")
                .arg(info.m_URL)
                .arg(info.m_GameName)
                .arg(info.m_ModID);
    } break;
    case mwsRequestInfo::TYPE_FILEINFO: {
      url = QString("%1/games/%2/mods/%3/files/%4")
                .arg(info.m_URL)
                .arg(info.m_GameName)
                .arg(info.m_ModID)
                .arg(info.m_FileID);
    } break;
    case mwsRequestInfo::TYPE_DOWNLOADURL: {
      ModRepositoryFileInfo* fileInfo = qobject_cast<ModRepositoryFileInfo*>(
          qvariant_cast<QObject*>(info.m_UserData));
      if (m_User.type() == APIUserAccountTypes::Premium) {
        url = QString("%1/games/%2/mods/%3/files/%4/download_link")
                  .arg(info.m_URL)
                  .arg(info.m_GameName)
                  .arg(info.m_ModID)
                  .arg(info.m_FileID);
      } else if (!fileInfo->nexusKey.isEmpty() && fileInfo->nexusExpires &&
                 fileInfo->nexusDownloadUser == m_User.id().toInt()) {
        url = QString("%1/games/%2/mods/%3/files/%4/download_link?key=%5&expires=%6")
                  .arg(info.m_URL)
                  .arg(info.m_GameName)
                  .arg(info.m_ModID)
                  .arg(info.m_FileID)
                  .arg(fileInfo->nexusKey)
                  .arg(fileInfo->nexusExpires);
      } else {
        log::warn("{}", tr("Aborting download: Either you clicked on a premium-only "
                           "link and your account is not premium, "
                           "or the download link was generated by a different account "
                           "than the one stored in Mod Organizer."));
        return;
      }
    } break;
    case mwsRequestInfo::TYPE_ENDORSEMENTS: {
      url = QString("%1/user/endorsements").arg(info.m_URL);
    } break;
    case mwsRequestInfo::TYPE_TOGGLEENDORSEMENT: {
      QString endorse = info.m_Endorse ? "endorse" : "abstain";
      url             = QString("%1/games/%2/mods/%3/%4")
                .arg(info.m_URL)
                .arg(info.m_GameName)
                .arg(info.m_ModID)
                .arg(endorse);
      postObject.insert("Version", info.m_ModVersion);
      postData.setObject(postObject);
    } break;
    case mwsRequestInfo::TYPE_TOGGLETRACKING: {
      url = QStringLiteral("%1/user/tracked_mods?domain_name=%2")
                .arg(info.m_URL)
                .arg(info.m_GameName);
      postObject.insert("mod_id", info.m_ModID);
      postData.setObject(postObject);
      requestIsDelete = !info.m_Track;
    } break;
    case mwsRequestInfo::TYPE_TRACKEDMODS: {
      url = QStringLiteral("%1/user/tracked_mods").arg(info.m_URL);
    } break;
    case mwsRequestInfo::TYPE_FILEINFO_MD5: {
      url = QStringLiteral("%1/games/%2/mods/md5_search/%3")
                .arg(info.m_URL)
                .arg(info.m_GameName)
                .arg(QString(info.m_Hash.toHex()));
    }
    }
  } else {
    url = info.m_URL;
  }
  QNetworkRequest request(url);
  request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
  request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                       QNetworkRequest::AlwaysNetwork);
  request.setRawHeader("APIKEY", m_User.apiKey().toUtf8());
  request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader,
                    m_AccessManager->userAgent(info.m_SubModule));
  request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader,
                    "application/json");
  request.setRawHeader("Protocol-Version", "1.0.0");
  request.setRawHeader("Application-Name", "MO2");
  request.setRawHeader("Application-Version",
                       QApplication::applicationVersion().toUtf8());

  if (postData.object().isEmpty()) {
    if (!requestIsDelete) {
      info.m_Reply = m_AccessManager->get(request);
    } else {
      info.m_Reply = m_AccessManager->deleteResource(request);
    }
  } else if (!requestIsDelete) {
    info.m_Reply = m_AccessManager->post(request, postData.toJson());
  } else {
    // Qt doesn't support DELETE with a payload as that's technically against the HTTP
    // standard...
    info.m_Reply =
        m_AccessManager->sendCustomRequest(request, "DELETE", postData.toJson());
  }

  connect(info.m_Reply, SIGNAL(finished()), this, SLOT(requestFinished()));
  if (!info.m_IgnoreGenericErrorHandler)
    connect(info.m_Reply, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), this,
            SLOT(requestError(QNetworkReply::NetworkError)));
  connect(info.m_Timeout, SIGNAL(timeout()), this, SLOT(requestTimeout()));
  info.m_Timeout->start();
  m_ActiveRequest.push_back(info);
}

void ModworkshopInterface::downloadRequestedmws(const QString& url)
{
  emit requestmwsDownload(url);
}

void ModworkshopInterface::requestFinished(std::list<mwsRequestInfo>::iterator iter)
{
  QNetworkReply* reply = iter->m_Reply;

  auto error = reply->error();
  if (error != QNetworkReply::NoError) {
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString errorMsg = reply->errorString();

    if (iter->m_AllowedErrors.contains(error) &&
        iter->m_AllowedErrors[error].contains(statusCode)) {
      // These errors are allows to silently happen.  They should be handled in
      // mwsRequestFailed below.
    } else if (statusCode == 429) {
      m_User.limits(parseLimits(reply));

      if (!m_User.exhausted()) {
        log::warn("You appear to be making requests to the Nexus API too quickly and "
                  "are being throttled. Please inform the MO2 team.");
      } else {
        log::warn("All API requests have been consumed and are now being denied.");
      }

      emit requestsChanged(getAPIStats(), m_User);
      log::warn("Error: {}", errorMsg);
    } else {
      QByteArray data = reply->readAll();
      if (!data.isEmpty()) {
        QJsonDocument responseDoc = QJsonDocument::fromJson(data);
        if (!responseDoc.isNull()) {
          auto result = responseDoc.toVariant().toMap();
          auto error  = result.find("error");
          if (error != result.end())
            errorMsg = result.value("error").toString();
        }
      }
    }
    emit mwsRequestFailed(iter->m_GameName, iter->m_ModID, iter->m_FileID,
                          iter->m_UserData, iter->m_ID, statusCode, errorMsg);
  } else {
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode == 301) {
      // redirect request, return request to queue
      iter->m_URL =
          reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
      iter->m_Reroute = true;
      m_RequestQueue.enqueue(*iter);
      // nextRequest();
      return;
    }
    QByteArray data = reply->readAll();
    if (data.isNull() || data.isEmpty() || (strcmp(data.constData(), "null") == 0)) {
      QString nexusError(reply->rawHeader("NexusErrorInfo"));
      if (nexusError.length() == 0) {
        nexusError = tr("empty response");
      }
      log::debug("nexus error: {}", nexusError);
      emit mwsRequestFailed(iter->m_GameName, iter->m_ModID, iter->m_FileID,
                            iter->m_UserData, iter->m_ID, reply->error(), nexusError);
    } else {
      QJsonDocument responseDoc = QJsonDocument::fromJson(data);
      if (!responseDoc.isNull()) {
        QVariant result = responseDoc.toVariant();
        switch (iter->m_Type) {
        case mwsRequestInfo::TYPE_DESCRIPTION: {
          emit mwsDescriptionAvailable(iter->m_GameName, iter->m_ModID,
                                       iter->m_UserData, result, iter->m_ID);
        } break;
        case mwsRequestInfo::TYPE_MODINFO: {
          emit mwsModInfoAvailable(iter->m_GameName, iter->m_ModID, iter->m_UserData,
                                   result, iter->m_ID);
        } break;
        case mwsRequestInfo::TYPE_CHECKUPDATES: {
          emit mwsUpdateInfoAvailable(iter->m_GameName, iter->m_UserData, result,
                                      iter->m_ID);
        } break;
        case mwsRequestInfo::TYPE_FILES: {
          emit mwsFilesAvailable(iter->m_GameName, iter->m_ModID, iter->m_UserData,
                                 result, iter->m_ID);
        } break;
        case mwsRequestInfo::TYPE_GETUPDATES: {
          emit mwsUpdatesAvailable(iter->m_GameName, iter->m_ModID, iter->m_UserData,
                                   result, iter->m_ID);
        } break;
        case mwsRequestInfo::TYPE_FILEINFO: {
          emit mwsFileInfoAvailable(iter->m_GameName, iter->m_ModID, iter->m_FileID,
                                    iter->m_UserData, result, iter->m_ID);
        } break;
        case mwsRequestInfo::TYPE_DOWNLOADURL: {
          emit mwsDownloadURLsAvailable(iter->m_GameName, iter->m_ModID, iter->m_FileID,
                                        iter->m_UserData, result, iter->m_ID);
        } break;
        case mwsRequestInfo::TYPE_ENDORSEMENTS: {
          emit mwsEndorsementsAvailable(iter->m_UserData, result, iter->m_ID);
        } break;
        case mwsRequestInfo::TYPE_TOGGLEENDORSEMENT: {
          emit mwsEndorsementToggled(iter->m_GameName, iter->m_ModID, iter->m_UserData,
                                     result, iter->m_ID);
        } break;
        case mwsRequestInfo::TYPE_TOGGLETRACKING: {
          auto results = result.toMap();
          auto message = results["message"].toString();
          if (message.contains(
                  QRegularExpression("User [0-9]+ is already Tracking Mod: [0-9]+")) ||
              message.contains(
                  QRegularExpression("User [0-9]+ is now Tracking Mod: [0-9]+"))) {
            emit mwsTrackingToggled(iter->m_GameName, iter->m_ModID, iter->m_UserData,
                                    true, iter->m_ID);
          } else if (message.contains(QRegularExpression(
                         "User [0-9]+ is no longer tracking [0-9]+")) ||
                     message.contains(QRegularExpression(
                         "Users is not tracking mod. Unable to untrack."))) {
            emit mwsTrackingToggled(iter->m_GameName, iter->m_ModID, iter->m_UserData,
                                    false, iter->m_ID);
          }
        } break;
        case mwsRequestInfo::TYPE_TRACKEDMODS: {
          emit mwsTrackedModsAvailable(iter->m_UserData, result, iter->m_ID);
        } break;
        case mwsRequestInfo::TYPE_FILEINFO_MD5: {
          emit mwsFileInfoFromMd5Available(iter->m_GameName, iter->m_UserData, result,
                                           iter->m_ID);
        } break;
        }

        m_User.limits(parseLimits(reply));
        emit requestsChanged(getAPIStats(), m_User);
      } else {
        emit mwsRequestFailed(iter->m_GameName, iter->m_ModID, iter->m_FileID,
                              iter->m_UserData, iter->m_ID, reply->error(),
                              tr("invalid response"));
      }
    }
  }
}

void ModworkshopInterface::requestFinished()
{
  QNetworkReply* reply = static_cast<QNetworkReply*>(sender());
  for (std::list<mwsRequestInfo>::iterator iter = m_ActiveRequest.begin();
       iter != m_ActiveRequest.end(); ++iter) {
    if (iter->m_Reply == reply) {
      iter->m_Timeout->stop();
      iter->m_Timeout->deleteLater();
      requestFinished(iter);
      iter->m_Reply->deleteLater();
      m_ActiveRequest.erase(iter);
      nextRequest();
      return;
    }
  }
}

void ModworkshopInterface::requestError(QNetworkReply::NetworkError)
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  if (reply == nullptr) {
    log::warn("invalid sender type");
    return;
  }

  log::error("request ({}) error: {} ({})", reply->url().toString(),
             reply->errorString(), reply->error());
}

void ModworkshopInterface::requestTimeout()
{
  QTimer* timer = qobject_cast<QTimer*>(sender());
  if (timer == nullptr) {
    log::warn("invalid sender type");
    return;
  }
  for (std::list<mwsRequestInfo>::iterator iter = m_ActiveRequest.begin();
       iter != m_ActiveRequest.end(); ++iter) {
    if (iter->m_Timeout == timer) {
      // this abort causes a "request failed" which cleans up the rest
      iter->m_Reply->abort();
      return;
    }
  }
}

APIUserAccount ModworkshopInterface::getAPIUserAccount() const
{
  return m_User;
}

APIStats ModworkshopInterface::getAPIStats() const
{
  APIStats stats;
  stats.requestsQueued = m_RequestQueue.size();

  return stats;
}

namespace
{
QString get_management_url()
{
  return "https://api.nexusmods.com/v1";
}
}  // namespace

ModworkshopInterface::mwsRequestInfo::mwsRequestInfo(
    int modID, ModworkshopInterface::mwsRequestInfo::Type type, QVariant userData,
    const QString& subModule, MOBase::IPluginGame const* game)
    : m_ModID(modID), m_ModVersion("0"), m_FileID(0), m_Reply(nullptr), m_Type(type),
      m_UpdatePeriod(UpdatePeriod::NONE), m_UserData(userData), m_Timeout(nullptr),
      m_Reroute(false), m_ID(s_NextID.fetchAndAddAcquire(1)),
      m_URL(get_management_url()), m_SubModule(subModule),
      m_NexusGameID(game->nexusGameID()), m_GameName(game->gameNexusName()),
      m_Endorse(false), m_Track(false), m_Hash(QByteArray())
{}

ModworkshopInterface::mwsRequestInfo::mwsRequestInfo(
    int modID, QString modVersion, ModworkshopInterface::mwsRequestInfo::Type type,
    QVariant userData, const QString& subModule, MOBase::IPluginGame const* game)
    : m_ModID(modID), m_ModVersion(modVersion), m_FileID(0), m_Reply(nullptr),
      m_Type(type), m_UpdatePeriod(UpdatePeriod::NONE), m_UserData(userData),
      m_Timeout(nullptr), m_Reroute(false), m_ID(s_NextID.fetchAndAddAcquire(1)),
      m_URL(get_management_url()), m_SubModule(subModule),
      m_NexusGameID(game->nexusGameID()), m_GameName(game->gameNexusName()),
      m_Endorse(false), m_Track(false), m_Hash(QByteArray())
{}

ModworkshopInterface::mwsRequestInfo::mwsRequestInfo(
    int modID, int fileID, ModworkshopInterface::mwsRequestInfo::Type type, QVariant userData,
    const QString& subModule, MOBase::IPluginGame const* game)
    : m_ModID(modID), m_ModVersion("0"), m_FileID(fileID), m_Reply(nullptr),
      m_Type(type), m_UpdatePeriod(UpdatePeriod::NONE), m_UserData(userData),
      m_Timeout(nullptr), m_Reroute(false), m_ID(s_NextID.fetchAndAddAcquire(1)),
      m_URL(get_management_url()), m_SubModule(subModule),
      m_NexusGameID(game->nexusGameID()), m_GameName(game->gameNexusName()),
      m_Endorse(false), m_Track(false), m_Hash(QByteArray())
{}

ModworkshopInterface::mwsRequestInfo::mwsRequestInfo(Type type, QVariant userData,
                                               const QString& subModule)
    : m_ModID(0), m_ModVersion("0"), m_FileID(0), m_Reply(nullptr), m_Type(type),
      m_UpdatePeriod(UpdatePeriod::NONE), m_UserData(userData), m_Timeout(nullptr),
      m_Reroute(false), m_ID(s_NextID.fetchAndAddAcquire(1)),
      m_URL(get_management_url()), m_SubModule(subModule), m_NexusGameID(0),
      m_GameName(""), m_Endorse(false), m_Track(false), m_Hash(QByteArray())
{}

ModworkshopInterface::mwsRequestInfo::mwsRequestInfo(
    UpdatePeriod period, ModworkshopInterface::mwsRequestInfo::Type type, QVariant userData,
    const QString& subModule, MOBase::IPluginGame const* game)
    : m_ModID(0), m_ModVersion("0"), m_FileID(0), m_Reply(nullptr), m_Type(type),
      m_UpdatePeriod(period), m_UserData(userData), m_Timeout(nullptr),
      m_Reroute(false), m_ID(s_NextID.fetchAndAddAcquire(1)),
      m_URL(get_management_url()), m_SubModule(subModule),
      m_NexusGameID(game->nexusGameID()), m_GameName(game->gameNexusName()),
      m_Endorse(false), m_Track(false), m_Hash(QByteArray())
{}

ModworkshopInterface::mwsRequestInfo::mwsRequestInfo(
    QByteArray& hash, ModworkshopInterface::mwsRequestInfo::Type type, QVariant userData,
    const QString& subModule, MOBase::IPluginGame const* game)
    : m_ModID(0), m_ModVersion("0"), m_FileID(0), m_Reply(nullptr), m_Type(type),
      m_UpdatePeriod(UpdatePeriod::NONE), m_UserData(userData), m_Timeout(nullptr),
      m_Reroute(false), m_ID(s_NextID.fetchAndAddAcquire(1)),
      m_URL(get_management_url()), m_SubModule(subModule),
      m_NexusGameID(game->nexusGameID()), m_GameName(game->gameNexusName()),
      m_Endorse(false), m_Track(false), m_Hash(hash)
{}
