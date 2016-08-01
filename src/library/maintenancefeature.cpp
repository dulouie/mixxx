#include <QIcon>
#include <QTabWidget>

#include "library/dlghidden.h"
#include "library/dlgmissing.h"
#include "library/hiddentablemodel.h"
#include "library/maintenancefeature.h"
#include "library/missingtablemodel.h"

#include "widget/wtracktableview.h"

MaintenanceFeature::MaintenanceFeature(UserSettingsPointer pConfig,
                                       Library* pLibrary,
                                       QObject* parent,
                                       TrackCollection* pTrackCollection)
        : LibraryFeature(pConfig, pLibrary, pTrackCollection, parent),
          kMaintenanceTitle(tr("Maintenance")),
          kHiddenTitle(tr("Hidden Tracks")),
          kMissingTitle(tr("Missing Tracks")),
          m_pHiddenView(nullptr),
          m_pMissingView(nullptr),
          m_pTab(nullptr) {

}

QVariant MaintenanceFeature::title() {
    return kMaintenanceTitle;
}

QString MaintenanceFeature::getIconPath() {
    return ":/images/library/ic_library_maintenance.png";
}

TreeItemModel* MaintenanceFeature::getChildModel() {
    return nullptr;
}

void MaintenanceFeature::activate() {
    DEBUG_ASSERT_AND_HANDLE(!m_pTab.isNull()) {
        return;
    }

    slotTabIndexChanged(m_pTab->currentIndex());

    switchToFeature();
    emit(enableCoverArtDisplay(true));
}

void MaintenanceFeature::selectionChanged(const QItemSelection&,
                                          const QItemSelection&) {
    WTrackTableView* pTable = getFocusedTable();
    if (pTable == nullptr) {
        return;
    }

    auto it = m_idPaneCurrent.find(m_featureFocus);
    if (it == m_idPaneCurrent.end()) {
        return;
    }

    const QModelIndexList& selection = pTable->selectionModel()->selectedIndexes();
    if (*it == Pane::Hidden) {
        m_pHiddenView->setSelectedIndexes(selection);
    } else if (*it == Pane::Missing) {
        m_pMissingView->setSelectedIndexes(selection);
    }
}

void MaintenanceFeature::selectAll() {
    QPointer<WTrackTableView> pTable = getFocusedTable();
    if (!pTable.isNull()) {
        pTable->selectAll();
    }
}

QWidget* MaintenanceFeature::createInnerSidebarWidget(KeyboardEventFilter* pKeyboard) {
    // The inner widget is a tab with the hidden and the missing controls
    m_pTab = new QTabWidget(nullptr);
    m_pTab->installEventFilter(pKeyboard);
    connect(m_pTab, SIGNAL(currentChanged(int)),
            this, SLOT(slotTabIndexChanged(int)));

    m_pHiddenView = new DlgHidden(m_pTab);
    m_pHiddenView->setTableModel(getHiddenTableModel());
    m_pHiddenView->installEventFilter(pKeyboard);
    connect(m_pHiddenView, SIGNAL(unhide()), this, SLOT(slotUnhideHidden()));
    connect(m_pHiddenView, SIGNAL(purge()), this, SLOT(slotPurge()));
    connect(m_pHiddenView, SIGNAL(selectAll()), this, SLOT(selectAll()));

    m_pMissingView = new DlgMissing(m_pTab);
    m_pMissingView->setTableModel(getMissingTableModel());
    m_pMissingView->installEventFilter(pKeyboard);
    connect(m_pMissingView, SIGNAL(purge()), this, SLOT(slotPurge()));
    connect(m_pMissingView, SIGNAL(selectAll()), this, SLOT(selectAll()));

    m_idExpandedHidden = m_pTab->addTab(m_pHiddenView, kHiddenTitle);
    m_idExpandedMissing = m_pTab->addTab(m_pMissingView, kMissingTitle);

    return m_pTab;
}

QWidget* MaintenanceFeature::createPaneWidget(KeyboardEventFilter* pKeyboard,
                                              int paneId) {
    WTrackTableView* pTable = LibraryFeature::createTableWidget(pKeyboard, paneId);

    return pTable;
}

void MaintenanceFeature::slotTabIndexChanged(int index) {
    QPointer<WTrackTableView> pTable = getFocusedTable();

    DEBUG_ASSERT_AND_HANDLE(!pTable.isNull()) {
        return;
    }
    pTable->setSortingEnabled(false);
    const QString* title;

    if (index == m_idExpandedHidden) {
        DEBUG_ASSERT_AND_HANDLE(!m_pHiddenView.isNull()) {
            return;
        }
        m_idPaneCurrent[m_featureFocus] = Pane::Hidden;
        pTable->loadTrackModel(getHiddenTableModel());

        title = &kHiddenTitle;
        m_pHiddenView->onShow();
    } else if (index == m_idExpandedMissing) {
        DEBUG_ASSERT_AND_HANDLE(!m_pMissingView.isNull()) {
            return;
        }

        m_idPaneCurrent[m_featureFocus] = Pane::Missing;
        pTable->loadTrackModel(getMissingTableModel());

        title = &kMissingTitle;
        m_pMissingView->onShow();
    } else {
        return;
    }

    // This is the only way to get the selection signal changing the track
    // models, every time the model changes the selection model changes too
    // so we need to reconnect
    connect(pTable->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
            this,
            SLOT(selectionChanged(const QItemSelection &, const QItemSelection &)));

    switchToFeature();
    restoreSearch("");
    showBreadCrumb(kMaintenanceTitle % " > " % (*title));
    emit(enableCoverArtDisplay(true));
}

void MaintenanceFeature::slotUnhideHidden() {
    QPointer<WTrackTableView> pTable = getFocusedTable();
    if (pTable.isNull()) {
        return;
    }
    
    pTable->slotUnhide();
}

void MaintenanceFeature::slotPurge() {
    QPointer<WTrackTableView> pTable = getFocusedTable();
    if (pTable.isNull()) {
        return;
    }
    pTable->slotPurge();
}

HiddenTableModel* MaintenanceFeature::getHiddenTableModel() {
    if (m_pHiddenTableModel.isNull()) {
        m_pHiddenTableModel = new HiddenTableModel(this, m_pTrackCollection);
    }
    return m_pHiddenTableModel;
}

MissingTableModel* MaintenanceFeature::getMissingTableModel() {
    if (m_pMissingTableModel.isNull()) {
        m_pMissingTableModel = new MissingTableModel(this, m_pTrackCollection);
    }
    return m_pMissingTableModel;
}

