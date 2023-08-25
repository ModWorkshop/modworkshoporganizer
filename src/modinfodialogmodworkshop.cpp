#include "modinfodialognexus.h"
#include "bbcode.h"
#include "iplugingame.h"
#include "organizercore.h"
#include "settings.h"
#include "ui_modinfodialog.h"
#include <log.h>
#include <utility.h>
#include <versioninfo.h>

using namespace MOBase;

bool isValidModID(int id)
{
  return (id > 0);
}

ModworkshopTab::ModworkshopTab(ModInfoDialogTabContext cx)
    : ModInfoDialogTab(std::move(cx)), m_requestStarted(false), m_loading(false)
{
  ui->modID->setValidator(new QIntValidator(ui->modID));
  //Currently no user login
  //ui->endorse->setVisible(core().settings().modworkshop().endorsementIntegration());
  //Curently no tracking
  //ui->track->setVisible(core().settings().modworkshop().trackedIntegration());

  connect(ui->modID, &QLineEdit::editingFinished, [&] {
    onModIDChanged();
  });
  connect(ui->sourceGame,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [&] {
            onSourceGameChanged();
          });
  connect(ui->version, &QLineEdit::editingFinished, [&] {
    onVersionChanged();
  });

  connect(ui->refresh, &QPushButton::clicked, [&] {
    onRefreshBrowser();
  });
  connect(ui->visitModworkshop, &QPushButton::clicked, [&] {
    onVisitModworkshop();
  });
  //Currently no user login
  //connect(ui->endorse, &QPushButton::clicked, [&] {
  //  onEndorse();
  //});
  //connect(ui->track, &QPushButton::clicked, [&] {
  //  onTrack();
  //});

  connect(ui->hasCustomURL, &QCheckBox::toggled, [&] {
    onCustomURLToggled();
  });
  connect(ui->customURL, &QLineEdit::editingFinished, [&] {
    onCustomURLChanged();
  });
  connect(ui->visitCustomURL, &QPushButton::clicked, [&] {
    onVisitCustomURL();
  });
}

ModworkshopTab::~ModworkshopTab()
{
  cleanup();
}

void ModworkshopTab::cleanup()
{
  if (m_modConnection) {
    disconnect(m_modConnection);
    m_modConnection = {};
  }
}

void ModworkshopTab::clear()
{
  ui->modID->clear();
  ui->sourceGame->clear();
  ui->version->clear();
  ui->browser->setPage(new ModworkshopTabWebpage(ui->browser));
  ui->hasCustomURL->setChecked(false);
  ui->customURL->clear();
  setHasData(false);
}

void ModworkshopTab::update()
{
  QScopedValueRollback loading(m_loading, true);

  clear();

  ui->modID->setText(QString("%1").arg(mod().modworkshopId()));

  QString gameName = mod().gameName();
  ui->sourceGame->addItem(core().managedGame()->gameName(),
                          core().managedGame()->gameShortName());

  if (core().managedGame()->validShortNames().size() == 0) {
    ui->sourceGame->setDisabled(true);
  } else {
    for (auto game : plugin().plugins<MOBase::IPluginGame>()) {
      for (QString gameName : core().managedGame()->validShortNames()) {
        if (game->gameShortName().compare(gameName, Qt::CaseInsensitive) == 0) {
          ui->sourceGame->addItem(game->gameName(), game->gameShortName());
          break;
        }
      }
    }
  }

  ui->sourceGame->setCurrentIndex(ui->sourceGame->findData(gameName));

  auto* page = new ModworkshopTabWebpage(ui->browser);
  ui->browser->setPage(page);

  connect(page, &ModworkshopTabWebpage::linkClicked, [&](const QUrl& url) {
    shell::Open(url);
  });

  //ui->endorse->setEnabled((mod().endorsedState() == EndorsedState::ENDORSED_FALSE) ||
  //                        (mod().endorsedState() == EndorsedState::ENDORSED_NEVER));

  setHasData(mod().modworkshopId() >= 0);
}

void ModworkshopTab::firstActivation()
{
  updateWebpage();
}

void ModworkshopTab::setMod(ModInfoPtr mod, MOShared::FilesOrigin* origin)
{
  cleanup();

  ModInfoDialogTab::setMod(mod, origin);

  m_modConnection = connect(mod.data(), &ModInfo::modDetailsUpdated, [&] {
    onModChanged();
  });
}

bool ModworkshopTab::usesOriginFiles() const
{
  return false;
}

void ModworkshopTab::updateVersionColor()
{
  if (mod().version() != mod().newestVersion()) {
    ui->version->setStyleSheet("color: red");
    ui->version->setToolTip(
        tr("Current Version: %1").arg(mod().newestVersion().canonicalString()));
  } else {
    ui->version->setStyleSheet("color: green");
    ui->version->setToolTip(tr("No update available"));
  }
}

void ModworkshopTab::updateWebpage()
{
  const int modID = mod().modworkshopId();

  if (isValidModID(modID)) {
    const QString modworkshopLink =
        ModworkshopInterface::instance().getModURL(modID, mod().gameName());

    ui->visitModworkshop->setToolTip(modworkshopLink);
    refreshData(modID);
  } else {
    onModChanged();
  }

  ui->version->setText(mod().version().displayString());
  ui->hasCustomURL->setChecked(mod().hasCustomURL());
  ui->customURL->setText(mod().url());
  ui->customURL->setEnabled(mod().hasCustomURL());
  ui->visitCustomURL->setEnabled(mod().hasCustomURL());
  ui->visitCustomURL->setToolTip(mod().parseCustomURL().toString());

  //updateTracking();
}

//void ModworkshopTab::updateTracking()
//{
//  if (mod().trackedState() == TrackedState::TRACKED_TRUE) {
//    ui->track->setChecked(true);
//    ui->track->setText(tr("Tracked"));
//  } else {
//    ui->track->setChecked(false);
//    ui->track->setText(tr("Untracked"));
//  }
//}

void ModworkshopTab::refreshData(int modID)
{
  if (tryRefreshData(modID)) {
    m_requestStarted = true;
  } else {
    onModChanged();
  }
}

bool ModworkshopTab::tryRefreshData(int modID)
{
  if (isValidModID(modID) && !m_requestStarted) {
    if (mod().updateMWSInfo()) {
      ui->browser->setHtml("");
      return true;
    }
  }

  return false;
}

void ModworkshopTab::onModChanged()
{
  m_requestStarted = false;

  const QString modworkshopDescription = mod().getModworkshopDescription();

  QString descriptionAsHTML = R"(
<html>
  <head>
    <style class="modworkshop-description">
    body
    {
      font-family: sans-serif;
      font-size: 14px;
      background: #404040;
      color: #f1f1f1;
      max-width: 1060px;
      margin-left: auto;
      margin-right: auto;
      padding-right: 7px;
      padding-left: 7px;
      padding-top: 20px;
      padding-bottom: 20px;
    }

    img {
      max-width: 100%;
    }

    figure.quote {
      position: relative;
      padding: 24px;
      margin: 10px 20px 10px 10px;
      color: #e1e1e1;
      line-height: 1.5;
      font-style: italic;
      border-left: 6px solid #57a5cc;
      border-left-color: rgb(87, 165, 204);
      background: #383838 url(data:image/svg+xml;base64,PHN2ZyBjbGFzcz0iaWNvbi1xdW90ZSIgdmVyc2lvbj0iMS4xIiB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHN0eWxlPSJmaWxsOnJnYig2OSwgNjksIDcwKTtoZWlnaHQ6MjlweDtsZWZ0OjE1cHg7cG9zaXRpb246YWJzb2x1dGU7dG9wOjE1cHg7d2lkdGg6MzhweDsiPjxwYXRoIGNsYXNzPSJwYXRoMSIgZD0iTTAgMjAuNjc0YzAgNy4yMjUgNC42NjggMTEuMzM3IDkuODkyIDExLjMzNyA0LjgyNC0wLjA2MiA4LjcxOS0zLjk1NiA4Ljc4MS04Ljc3NSAwLTQuNzg1LTMuMzM0LTguMDA5LTcuNTU4LTguMDA5LTAuMDc4LTAuMDA0LTAuMTctMC4wMDYtMC4yNjItMC4wMDYtMC43MDMgMC0xLjM3NyAwLjEyNC0yLjAwMSAwLjM1MiAxLjA0MS00LjAxNCA1LjE1My04LjY4MyA4LjcxLTEwLjU3MmwtNi4xMTMtNS4wMDJjLTYuODkxIDQuODkxLTExLjQ0OCAxMi4zMzgtMTEuNDQ4IDIwLjY3NHpNMjIuNjc1IDIwLjY3NGMwIDcuMjI1IDQuNjY4IDExLjMzNyA5Ljg5MiAxMS4zMzcgNC44LTAuMDU2IDguNjctMy45NjEgOC42Ny04Ljc2OSAwLTAuMDA0IDAtMC4wMDggMC0wLjAxMiAwLTQuNzc5LTMuMjIzLTguMDAyLTcuNDQ3LTguMDAyLTAuMDk1LTAuMDA2LTAuMjA2LTAuMDA5LTAuMzE4LTAuMDA5LTAuNjg0IDAtMS4zMzkgMC4xMjYtMS45NDMgMC4zNTUgMC45MjctNC4wMTQgNS4xNS04LjY4MiA4LjcwNy0xMC41NzJsLTYuMTI0LTUuMDAyYy02Ljg5MSA0Ljg5MS0xMS40MzcgMTIuMzM4LTExLjQzNyAyMC42NzR6IiBzdHlsZT0iZmlsbDpyZ2IoNjksIDY5LCA3MCk7aGVpZ2h0OmF1dG87d2lkdGg6YXV0bzsiLz48L3N2Zz4=) no-repeat;
    }

    figure.quote blockquote {
      position: relative;
      margin: 0;
      padding: 0;
    }

    div.spoiler_content {
      background: #262626;
      border: 1px dashed #3b3b3b;
      padding: 5px;
      margin: 5px;
    }

    div.bbc_spoiler_show{
      border: 1px solid black;
      background-color: #454545;
      font-size: 11px;
      padding: 3px;
      color: #E6E6E6;
      border-radius: 3px;
      display: inline-block;
      cursor: pointer;
    }

    details summary::marker {
      display:none;
    }

    summary:focus {
      outline: 0;
    }

    a
    {
      /*should avoid overflow with long links forcing wordwrap regardless of spaces*/
      overflow-wrap: break-word;
      word-wrap: break-word;

      color: #8197ec;
      text-decoration: none;
    }
    </style>
  </head>
  <body>%1</body>
</html>)";

  if (modworkshopDescription.isEmpty()) {
    descriptionAsHTML = descriptionAsHTML.arg(tr(R"(
      <div style="text-align: center;">
      <p>This mod does not have a valid Modworkshop ID. You can add a custom web
      page for it in the "Custom URL" box below.</p>
      </div>)"));
  } else {
    descriptionAsHTML = descriptionAsHTML.arg(BBCode::convertToHTML(modworkshopDescription));
  }

  ui->browser->page()->setHtml(descriptionAsHTML);
  updateVersionColor();
  updateTracking();
}

void ModworkshopTab::onModIDChanged()
{
  if (m_loading) {
    return;
  }

  const int oldID = mod().modworkshopId();
  const int newID = ui->modID->text().toInt();

  if (oldID != newID) {
    mod().setModworkshopID(newID);
    mod().setLastModworkshopQuery(QDateTime::fromSecsSinceEpoch(0));

    ui->browser->page()->setHtml("");

    if (isValidModID(newID)) {
      refreshData(newID);
    }
  }
}

void ModworkshopTab::onSourceGameChanged()
{
  if (m_loading) {
    return;
  }

  for (auto game : plugin().plugins<MOBase::IPluginGame>()) {
    if (game->gameName() == ui->sourceGame->currentText()) {
      mod().setGameName(game->gameShortName());
      mod().setLastModworkshopQuery(QDateTime::fromSecsSinceEpoch(0));
      refreshData(mod().modworkshopId());
      return;
    }
  }
}

void ModworkshopTab::onVersionChanged()
{
  if (m_loading) {
    return;
  }

  MOBase::VersionInfo version(ui->version->text());
  mod().setVersion(version);
  updateVersionColor();
}

void ModworkshopTab::onRefreshBrowser()
{
  const auto modID = mod().modworkshopId();

  if (isValidModID(modID)) {
    mod().setLastModworkshopQuery(QDateTime::fromSecsSinceEpoch(0));
    updateWebpage();
  } else {
    log::info("Mod has no valid Modworkshop ID, info can't be updated.");
  }
}

void ModworkshopTab::onVisitModworkshop()
{
  const int modID = mod().modworkshopId();

  if (isValidModID(modID)) {
    const QString modworkshopLink =
        ModworkshopInterface::instance().getModURL(modID, mod().gameName());

    shell::Open(QUrl(modworkshopLink));
  }
}

//void ModworkshopTab::onEndorse()
//{
  // use modPtr() instead of mod() or this because the callback may be
  // executed after the dialog is closed
//  core().loggedInAction(parentWidget(), [m = modPtr()] {
//    m->endorse(true);
//  });
//}

void ModworkshopTab::onTrack()
{
  // use modPtr() instead of mod() or this because the callback may be
  // executed after the dialog is closed
//  core().loggedInAction(parentWidget(), [m = modPtr()] {
//    if (m->trackedState() == TrackedState::TRACKED_TRUE) {
//      m->track(false);
//    } else {
//      m->track(true);
//    }
//  });
//}

void ModworkshopTab::onCustomURLToggled()
{
  if (m_loading) {
    return;
  }

  mod().setHasCustomURL(ui->hasCustomURL->isChecked());
  ui->customURL->setEnabled(mod().hasCustomURL());
  ui->visitCustomURL->setEnabled(mod().hasCustomURL());
}

void ModworkshopTab::onCustomURLChanged()
{
  if (m_loading) {
    return;
  }

  mod().setCustomURL(ui->customURL->text());
  ui->visitCustomURL->setToolTip(mod().parseCustomURL().toString());
}

void ModworkshopTab::onVisitCustomURL()
{
  const QUrl url = mod().parseCustomURL();
  if (url.isValid()) {
    shell::Open(url);
  }
}
