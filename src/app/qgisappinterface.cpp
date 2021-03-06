/***************************************************************************
                          qgisappinterface.cpp
                          Interface class for accessing exposed functions
                          in QgisApp
                             -------------------
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QFileInfo>
#include <QString>
#include <QMenu>
#include <QDialog>
#include <QAbstractButton>
#include <QSignalMapper>
#include <QTimer>
#include <QUiLoader>

#include "qgisappinterface.h"
#include "qgisappstylesheet.h"
#include "qgsclassicapp.h"
#include "qgscomposer.h"
#include "qgscomposerview.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptooladvanceddigitizing.h"
#include "qgsmapcanvas.h"
#include "qgsproject.h"
#include "qgsribbonapp.h"
#include "qgslayertreeview.h"
#include "qgsshortcutsmanager.h"
#include "qgsattributedialog.h"
#include "qgsfield.h"
#include "qgsvectordataprovider.h"
#include "qgsfeatureaction.h"
#include "qgsattributeaction.h"
#include "qgsattributetabledialog.h"


QgisAppInterface::QgisAppInterface( QgisApp *_qgis )
    : qgis( _qgis )
    , qgisc( qobject_cast<QgsClassicApp*>( _qgis ) )
    , qgisr( qobject_cast<QgsRibbonApp*>( _qgis ) )
    , mTimer( NULL )
    , legendIface( _qgis->layerTreeView() )
    , pluginManagerIface( _qgis->pluginManager() )
{
  // connect signals
  connect( qgis->layerTreeView(), SIGNAL( currentLayerChanged( QgsMapLayer * ) ),
           this, SIGNAL( currentLayerChanged( QgsMapLayer * ) ) );
  connect( qgis, SIGNAL( currentThemeChanged( QString ) ),
           this, SIGNAL( currentThemeChanged( QString ) ) );
  connect( qgis, SIGNAL( compositionAdded( QgsComposition* ) ),
           this, SIGNAL( compositionAdded( QgsComposition* ) ) );
  connect( qgis, SIGNAL( compositionWillBeRemoved( QgsComposition* ) ),
           this, SIGNAL( compositionWillBeRemoved( QgsComposition* ) ) );
  connect( qgis, SIGNAL( initializationCompleted() ),
           this, SIGNAL( initializationCompleted() ) );
  connect( qgis, SIGNAL( newProject() ),
           this, SIGNAL( newProjectCreated() ) );
  connect( qgis, SIGNAL( projectRead() ),
           this, SIGNAL( projectRead() ) );
  connect( qgis, SIGNAL( layerSavedAs( QgsMapLayer*, QString ) ),
           this, SIGNAL( layerSavedAs( QgsMapLayer*, QString ) ) );
}

QgisAppInterface::~QgisAppInterface()
{
}

QgsLegendInterface* QgisAppInterface::legendInterface()
{
  return &legendIface;
}

QgsPluginManagerInterface* QgisAppInterface::pluginManagerInterface()
{
  return &pluginManagerIface;
}

QgsLayerTreeView*QgisAppInterface::layerTreeView()
{
  return qgis->layerTreeView();
}

void QgisAppInterface::zoomFull()
{
  qgis->zoomFull();
}

void QgisAppInterface::zoomToPrevious()
{
  qgis->zoomToPrevious();
}

void QgisAppInterface::zoomToNext()
{
  qgis->zoomToNext();
}

void QgisAppInterface::zoomToActiveLayer()
{
  qgis->zoomToLayerExtent();
}

QgsVectorLayer* QgisAppInterface::addVectorLayer( QString vectorLayerPath, QString baseName, QString providerKey )
{
  if ( baseName.isEmpty() )
  {
    QFileInfo fi( vectorLayerPath );
    baseName = fi.completeBaseName();
  }
  return qgis->addVectorLayer( vectorLayerPath, baseName, providerKey );
}

QgsRasterLayer* QgisAppInterface::addRasterLayer( QString rasterLayerPath, QString baseName )
{
  if ( baseName.isEmpty() )
  {
    QFileInfo fi( rasterLayerPath );
    baseName = fi.completeBaseName();
  }
  return qgis->addRasterLayer( rasterLayerPath, baseName );
}

QgsRasterLayer* QgisAppInterface::addRasterLayer( const QString& url, const QString& baseName, const QString& providerKey )
{
  return qgis->addRasterLayer( url, baseName, providerKey );
}

bool QgisAppInterface::addProject( QString theProjectName )
{
  return qgis->addProject( theProjectName );
}

void QgisAppInterface::newProject( bool thePromptToSaveFlag )
{
  qgis->fileNew( thePromptToSaveFlag );
}

bool QgisAppInterface::fileSave()
{
  return qgis->fileSave();
}

QgsRedliningLayer* QgisAppInterface::redliningLayer()
{
  return qgis->redliningLayer();
}

QgsMapLayer *QgisAppInterface::activeLayer()
{
  return qgis->activeLayer();
}

bool QgisAppInterface::setActiveLayer( QgsMapLayer *layer )
{
  return qgis->setActiveLayer( layer );
}

void QgisAppInterface::addPluginToMenu( QString name, QAction* action )
{
  if ( qgisc ) qgisc->addPluginToMenu( name, action );
}

void QgisAppInterface::insertAddLayerAction( QAction *action )
{
  if ( qgisc ) qgisc->insertAddLayerAction( action );
}

void QgisAppInterface::removeAddLayerAction( QAction *action )
{
  if ( qgisc ) qgisc->removeAddLayerAction( action );
}

void QgisAppInterface::removePluginMenu( QString name, QAction* action )
{
  if ( qgisc ) qgisc->removePluginMenu( name, action );
}

void QgisAppInterface::addPluginToDatabaseMenu( QString name, QAction* action )
{
  if ( qgisc ) qgisc->addPluginToDatabaseMenu( name, action );
}

void QgisAppInterface::removePluginDatabaseMenu( QString name, QAction* action )
{
  if ( qgisc ) qgisc->removePluginDatabaseMenu( name, action );
}

void QgisAppInterface::addPluginToRasterMenu( QString name, QAction* action )
{
  if ( qgisc ) qgisc->addPluginToRasterMenu( name, action );
}

void QgisAppInterface::removePluginRasterMenu( QString name, QAction* action )
{
  if ( qgisc ) qgisc->removePluginRasterMenu( name, action );
}

void QgisAppInterface::addPluginToVectorMenu( QString name, QAction* action )
{
  if ( qgisc ) qgisc->addPluginToVectorMenu( name, action );
}

void QgisAppInterface::removePluginVectorMenu( QString name, QAction* action )
{
  if ( qgisc ) qgisc->removePluginVectorMenu( name, action );
}

void QgisAppInterface::addPluginToWebMenu( QString name, QAction* action )
{
  if ( qgisc ) qgisc->addPluginToWebMenu( name, action );
}

void QgisAppInterface::removePluginWebMenu( QString name, QAction* action )
{
  if ( qgisc ) qgisc->removePluginWebMenu( name, action );
}

int QgisAppInterface::addToolBarIcon( QAction * qAction )
{
  return qgisc ? qgisc->addPluginToolBarIcon( qAction ) : -1;
}

QAction *QgisAppInterface::addToolBarWidget( QWidget* widget )
{
  return qgisc ? qgisc->addPluginToolBarWidget( widget ) : 0;
}

void QgisAppInterface::removeToolBarIcon( QAction *qAction )
{
  if ( qgisc ) qgisc->removePluginToolBarIcon( qAction );
}

int QgisAppInterface::addRasterToolBarIcon( QAction * qAction )
{
  return qgisc ? qgisc->addRasterToolBarIcon( qAction ) : -1;
}

QAction *QgisAppInterface::addRasterToolBarWidget( QWidget* widget )
{
  return qgisc ? qgisc->addRasterToolBarWidget( widget ) : 0;
}

void QgisAppInterface::removeRasterToolBarIcon( QAction *qAction )
{
  if ( qgisc ) qgisc->removeRasterToolBarIcon( qAction );
}

int QgisAppInterface::addVectorToolBarIcon( QAction * qAction )
{
  return qgisc ? qgisc->addVectorToolBarIcon( qAction ) : -1;
}

QAction *QgisAppInterface::addVectorToolBarWidget( QWidget* widget )
{
  return qgisc ? qgisc->addVectorToolBarWidget( widget ) : 0;
}

void QgisAppInterface::removeVectorToolBarIcon( QAction *qAction )
{
  if ( qgisc ) qgisc->removeVectorToolBarIcon( qAction );
}

int QgisAppInterface::addDatabaseToolBarIcon( QAction * qAction )
{
  return qgisc ? qgisc->addDatabaseToolBarIcon( qAction ) : -1;
}

QAction *QgisAppInterface::addDatabaseToolBarWidget( QWidget* widget )
{
  return qgisc ? qgisc->addDatabaseToolBarWidget( widget ) : 0;
}

void QgisAppInterface::removeDatabaseToolBarIcon( QAction *qAction )
{
  if ( qgisc ) qgisc->removeDatabaseToolBarIcon( qAction );
}

int QgisAppInterface::addWebToolBarIcon( QAction * qAction )
{
  return qgisc ? qgisc->addWebToolBarIcon( qAction ) : -1;
}

QAction *QgisAppInterface::addWebToolBarWidget( QWidget* widget )
{
  return qgisc ? qgisc->addWebToolBarWidget( widget ) : 0;
}

void QgisAppInterface::removeWebToolBarIcon( QAction *qAction )
{
  if ( qgisc ) qgisc->removeWebToolBarIcon( qAction );
}

QToolBar* QgisAppInterface::addToolBar( QString name )
{
  return qgisc ? qgisc->addToolBar( name ) : 0;
}

void QgisAppInterface::addToolBar( QToolBar *toolbar, Qt::ToolBarArea area )
{
  if ( qgisc ) qgisc->addToolBar( toolbar, area );
}

void QgisAppInterface::openURL( QString url, bool useQgisDocDirectory )
{
  qgis->openURL( url, useQgisDocDirectory );
}

QgsMapCanvas * QgisAppInterface::mapCanvas()
{
  return qgis->mapCanvas();
}

QWidget * QgisAppInterface::mainWindow()
{
  return qgis;
}

QgsMessageBar * QgisAppInterface::messageBar()
{
  return qgis->messageBar();
}

const QList<QgsComposition*>& QgisAppInterface::printCompositions()
{
  return qgis->printCompositions();
}

QgsComposition* QgisAppInterface::createNewComposition( QString title )
{
  return qgis->createNewComposition( title );
}

QgsComposition* QgisAppInterface::duplicateComposition( QgsComposition* composition, QString title )
{
  return qgis->duplicateComposition( composition, title );
}

QMainWindow* QgisAppInterface::showComposer( QgsComposition* composition )
{
  return qgis->showComposer( composition );
}

void QgisAppInterface::deleteComposition( QgsComposition* composition )
{
  return qgis->deleteComposition( composition );
}

QMap<QString, QVariant> QgisAppInterface::defaultStyleSheetOptions()
{
  return qgis->styleSheetBuilder()->defaultOptions();
}

void QgisAppInterface::buildStyleSheet( const QMap<QString, QVariant>& opts )
{
  qgis->styleSheetBuilder()->buildStyleSheet( opts );
}

void QgisAppInterface::saveStyleSheetOptions( const QMap<QString, QVariant>& opts )
{
  qgis->styleSheetBuilder()->saveToSettings( opts );
}

QFont QgisAppInterface::defaultStyleSheetFont()
{
  return qgis->styleSheetBuilder()->defaultFont();
}

void QgisAppInterface::addDockWidget( Qt::DockWidgetArea area, QDockWidget * dockwidget )
{
  qgis->addDockWidget( area, dockwidget );
}

void QgisAppInterface::removeDockWidget( QDockWidget * dockwidget )
{
  qgis->removeDockWidget( dockwidget );
}

void QgisAppInterface::showLayerProperties( QgsMapLayer *l )
{
  if ( l && qgis )
  {
    qgis->showLayerProperties( l );
  }
}

QDialog* QgisAppInterface::showAttributeTable( QgsVectorLayer *l, const QString& filterExpression )
{
  if ( l )
  {
    QgsAttributeTableDialog *dialog = new QgsAttributeTableDialog( l );
    dialog->setFilterExpression( filterExpression );
    dialog->show();
    return dialog;
  }
  return 0;
}

void QgisAppInterface::addWindow( QAction *action )
{
  qgis->addWindow( action );
}

void QgisAppInterface::removeWindow( QAction *action )
{
  qgis->removeWindow( action );
}

bool QgisAppInterface::registerMainWindowAction( QAction* action, QString defaultShortcut )
{
  return QgsShortcutsManager::instance()->registerAction( action, defaultShortcut );
}

bool QgisAppInterface::unregisterMainWindowAction( QAction* action )
{
  return QgsShortcutsManager::instance()->unregisterAction( action );
}

//! Menus
Q_DECL_DEPRECATED QMenu *QgisAppInterface::fileMenu() { return qgis->projectMenu(); }
QMenu *QgisAppInterface::projectMenu() { return qgis->projectMenu(); }
QMenu *QgisAppInterface::editMenu() { return qgis->editMenu(); }
QMenu *QgisAppInterface::viewMenu() { return qgis->viewMenu(); }
QMenu *QgisAppInterface::layerMenu() { return qgis->layerMenu(); }
QMenu *QgisAppInterface::newLayerMenu() { return qgis->newLayerMenu(); }
QMenu *QgisAppInterface::addLayerMenu() { return qgis->addLayerMenu(); }
QMenu *QgisAppInterface::settingsMenu() { return qgis->settingsMenu(); }
QMenu *QgisAppInterface::pluginMenu() { return qgis->pluginMenu(); }
QMenu *QgisAppInterface::rasterMenu() { return qgis->rasterMenu(); }
QMenu *QgisAppInterface::vectorMenu() { return qgis->vectorMenu(); }
QMenu *QgisAppInterface::databaseMenu() { return qgis->databaseMenu(); }
QMenu *QgisAppInterface::webMenu() { return qgis->webMenu(); }
QMenu *QgisAppInterface::firstRightStandardMenu() { return qgis->firstRightStandardMenu(); }
QMenu *QgisAppInterface::windowMenu() { return qgis->windowMenu(); }
QMenu *QgisAppInterface::helpMenu() { return qgis->helpMenu(); }

//! ToolBars
QToolBar *QgisAppInterface::fileToolBar() { return qgisc ? qgisc->fileToolBar() : 0; }
QToolBar *QgisAppInterface::layerToolBar() { return qgisc ? qgisc->layerToolBar() : 0; }
QToolBar *QgisAppInterface::mapNavToolToolBar() { return qgisc ? qgisc->mapNavToolToolBar() : 0; }
QToolBar *QgisAppInterface::digitizeToolBar() { return qgisc ? qgisc->digitizeToolBar() : 0; }
QToolBar *QgisAppInterface::advancedDigitizeToolBar() { return qgisc ? qgisc->advancedDigitizeToolBar() : 0; }
QToolBar *QgisAppInterface::attributesToolBar() { return qgisc ? qgisc->attributesToolBar() : 0; }
QToolBar *QgisAppInterface::pluginToolBar() { return qgisc ? qgisc->pluginToolBar() : 0; }
QToolBar *QgisAppInterface::helpToolBar() { return qgisc ? qgisc->helpToolBar() : 0; }
QToolBar *QgisAppInterface::rasterToolBar() { return qgisc ? qgisc->rasterToolBar() : 0; }
QToolBar *QgisAppInterface::vectorToolBar() { return qgisc ? qgisc->vectorToolBar() : 0; }
QToolBar *QgisAppInterface::databaseToolBar() { return qgisc ? qgisc->databaseToolBar() : 0; }
QToolBar *QgisAppInterface::webToolBar() { return qgisc ? qgisc->webToolBar() : 0; }

//! Generic action finder
QAction *QgisAppInterface::findAction( const QString& name ) { return qgis->findAction( name ); }

QMenu* QgisAppInterface::getClassicMenu( ActionClassicMenuLocation classicMenuLocation, const QString& customName )
{
  QMenu* targetMenu = nullptr;
  switch ( classicMenuLocation )
  {
    case NO_MENU:
      break;
    case PROJECT_MENU:
      targetMenu = projectMenu(); break;
    case EDIT_MENU:
      targetMenu = editMenu(); break;
    case VIEW_MENU:
      targetMenu = viewMenu(); break;
    case LAYER_MENU:
      targetMenu = layerMenu(); break;
    case NEWLAYER_MENU:
      targetMenu = newLayerMenu(); break;
    case ADDLAYER_MENU:
      targetMenu = addLayerMenu(); break;
    case SETTINGS_MENU:
      targetMenu = settingsMenu(); break;
    case PLUGIN_MENU:
      targetMenu = pluginMenu(); break;
    case RASTER_MENU:
      targetMenu = rasterMenu(); break;
    case DATABASE_MENU:
      targetMenu = databaseMenu(); break;
    case VECTOR_MENU:
      targetMenu = vectorMenu(); break;
    case WEB_MENU:
      targetMenu = webMenu(); break;
    case FIRST_RIGHT_STANDARD_MENU:
      targetMenu = firstRightStandardMenu(); break;
    case WINDOW_MENU:
      targetMenu = windowMenu(); break;
    case HELP_MENU:
      targetMenu = helpMenu(); break;
    case CUSTOM_MENU:
    for ( const QAction* action : qgis->menuBar()->actions() )
      {
        if ( action->menu()->title() == customName )
        {
          targetMenu = action->menu();
          break;
        }
      }
      if ( !targetMenu )
      {
        targetMenu = new QMenu( qgis->menuBar() );
        qgis->menuBar()->addAction( targetMenu->menuAction() );
        targetMenu->setTitle( customName );
      }
      break;
  }
  return targetMenu;
}

QToolBar* QgisAppInterface::getClassicToolbar( ActionClassicToolbarLocation classicToolbarLocation, const QString &customName )
{
  QToolBar* targetToolbar = nullptr;
  switch ( classicToolbarLocation )
  {
    case NO_TOOLBAR: break;
    case FILE_TOOLBAR:
      targetToolbar = fileToolBar(); break;
    case LAYER_TOOLBAR:
      targetToolbar = layerToolBar(); break;
    case MAP_NAV_TOOL_TOOLBAR:
      targetToolbar = mapNavToolToolBar(); break;
    case ADVANCED_DIGITIZE_TOOLBAR:
      targetToolbar = advancedDigitizeToolBar(); break;
    case ATTRIBUTES_TOOLBAR:
      targetToolbar = attributesToolBar(); break;
    case PLUGIN_TOOLBAR:
      targetToolbar = pluginToolBar(); break;
    case HELP_TOOLBAR:
      targetToolbar = helpToolBar(); break;
    case RASTER_TOOLBAR:
      targetToolbar = rasterToolBar(); break;
    case VECTOR_TOOLBAR:
      targetToolbar = vectorToolBar(); break;
    case DATABASE_TOOLBAR:
      targetToolbar = databaseToolBar(); break;
    case WEB_TOOLBAR:
      targetToolbar = webToolBar(); break;
    case CUSTOM_TOOLBAR:
    for ( QToolBar* toolBar : qgis->findChildren<QToolBar*>() )
      {
        if ( toolBar->windowTitle() == customName )
        {
          targetToolbar = toolBar;
          break;
        }
      }
      if ( !targetToolbar )
      {
        targetToolbar = new QToolBar( qgis );
        targetToolbar->setWindowTitle( customName );
        qgis->addToolBar( targetToolbar );
      }
      break;
  }
  return targetToolbar;
}

QWidget* QgisAppInterface::getRibbonTabWidget( ActionRibbonTabLocation ribbonTabLocation, const QString &customName )
{
  QWidget* targetTabWidget = nullptr;
  switch ( ribbonTabLocation )
  {
    case NO_TAB:
      break;
    case MAPS_TAB:
      targetTabWidget = qgisr->mapsTab(); break;
    case VIEW_TAB:
      targetTabWidget = qgisr->viewTab(); break;
    case ANALYSIS_TAB:
      targetTabWidget = qgisr->analysisTab(); break;
    case DRAW_TAB:
      targetTabWidget = qgisr->drawTab(); break;
    case GPS_TAB:
      targetTabWidget = qgisr->gpsTab(); break;
    case MSS_TAB:
      targetTabWidget = qgisr->mssTab(); break;
    case SETTINGS_TAB:
      targetTabWidget = qgisr->settingsTab(); break;
    case HELP_TAB:
      targetTabWidget = qgisr->helpTab(); break;
    case CUSTOM_TAB:
      for ( int i = 0, n = qgisr->ribbonTabWidget()->count(); i < n; ++i )
      {
        if ( qgisr->ribbonTabWidget()->tabText( i ) == customName )
        {
          targetTabWidget = qgisr->ribbonTabWidget()->widget( i );
          break;
        }
      }
      if ( !targetTabWidget )
      {
        targetTabWidget = qgisr->addRibbonTab( customName );
      }
      break;
  }
  return targetTabWidget;
}

void QgisAppInterface::addAction( QAction* action, ActionClassicMenuLocation classicMenuLocation, ActionClassicToolbarLocation classicToolbarLocation, ActionRibbonTabLocation ribbonTabLocation, const QString& customName, QgsMapTool *associatedMapTool )
{
  if ( qgisc )
  {
    // Add action to classic app menu
    QMenu* targetMenu = getClassicMenu( classicMenuLocation, customName );
    if ( targetMenu )
    {
      targetMenu->addAction( action );
    }

    // Add action to classic app toolbar
    QToolBar* targetToolbar = getClassicToolbar( classicToolbarLocation, customName );
    if ( targetToolbar )
    {
      targetToolbar->addAction( action );
    }
  }

  // Add action to ribbon tabs
  if ( qgisr )
  {
    QWidget* targetTabWidget = getRibbonTabWidget( ribbonTabLocation, customName );
    if ( targetTabWidget )
    {
      qgisr->addActionToTab( action, targetTabWidget, associatedMapTool );
    }
  }
  if ( associatedMapTool )
  {
    associatedMapTool->setAction( action );
  }
}

void QgisAppInterface::addActionMenu( const QString &text, const QIcon &icon, QMenu* menu, ActionClassicMenuLocation classicMenuLocation, ActionClassicToolbarLocation classicToolbarLocation, ActionRibbonTabLocation ribbonTabLocation, const QString& customName )
{
  if ( qgisc )
  {
    // Add action to classic app menu
    QMenu* targetMenu = getClassicMenu( classicMenuLocation, customName );
    if ( targetMenu )
    {
      QAction* menuAction = new QAction( icon, text );
      menuAction->setMenu( menu );
      targetMenu->addAction( menuAction );
    }

    // Add action to classic app toolbar
    QToolBar* targetToolbar = getClassicToolbar( classicToolbarLocation, customName );
    if ( targetToolbar )
    {
      QToolButton* menuButton = new QToolButton();
      menuButton->setText( text );
      menuButton->setIcon( icon );
      menuButton->setMenu( menu );
      menuButton->setPopupMode( QToolButton::InstantPopup );
      targetToolbar->addWidget( menuButton );
    }
  }

  // Add action to ribbon tabs
  if ( qgisr )
  {
    QWidget* targetTabWidget = getRibbonTabWidget( ribbonTabLocation, customName );
    if ( targetTabWidget )
    {
      qgisr->addMenuButtonToTab( text, icon, menu, targetTabWidget );
    }
  }
}

//! Generic object finder
QObject* QgisAppInterface::findObject( const QString& name ) { return qgis->findChild<QObject*>( name ); }

//! Project menu actions
QAction *QgisAppInterface::actionNewProject() { return qgisc ? qgisc->actionNewProject() : 0; }
QAction *QgisAppInterface::actionOpenProject() { return qgisc ? qgisc->actionOpenProject() : 0; }
QAction *QgisAppInterface::actionSaveProject() { return qgisc ? qgisc->actionSaveProject() : 0; }
QAction *QgisAppInterface::actionSaveProjectAs() { return qgisc ? qgisc->actionSaveProjectAs() : 0; }
QAction *QgisAppInterface::actionSaveMapAsImage() { return qgisc ? qgisc->actionSaveMapAsImage() : 0; }
QAction *QgisAppInterface::actionProjectProperties() { return qgisc ? qgisc->actionProjectProperties() : 0; }
QAction *QgisAppInterface::actionPrintComposer() { return qgisc ? qgisc->actionNewPrintComposer() : 0; }
QAction *QgisAppInterface::actionShowComposerManager() { return qgisc ? qgisc->actionShowComposerManager() : 0; }
QAction *QgisAppInterface::actionExit() { return qgisc ? qgisc->actionExit() : 0; }

//! Edit menu actions
QAction *QgisAppInterface::actionCutFeatures() { return qgisc ? qgisc->actionCutFeatures() : 0; }
QAction *QgisAppInterface::actionCopyFeatures() { return qgisc ? qgisc->actionCopyFeatures() : 0; }
QAction *QgisAppInterface::actionPasteFeatures() { return qgisc ? qgisc->actionPasteFeatures() : 0; }
QAction *QgisAppInterface::actionAddFeature() { return qgisc ? qgisc->actionAddFeature() : 0; }
QAction *QgisAppInterface::actionDeleteSelected() { return qgisc ? qgisc->actionDeleteSelected() : 0; }
QAction *QgisAppInterface::actionMoveFeature() { return qgisc ? qgisc->actionMoveFeature() : 0; }
QAction *QgisAppInterface::actionSplitFeatures() { return qgisc ? qgisc->actionSplitFeatures() : 0; }
QAction *QgisAppInterface::actionSplitParts() { return qgisc ? qgisc->actionSplitParts() : 0; }
QAction *QgisAppInterface::actionAddRing() { return qgisc ? qgisc->actionAddRing() : 0; }
QAction *QgisAppInterface::actionAddPart() { return qgisc ? qgisc->actionAddPart() : 0; }
QAction *QgisAppInterface::actionSimplifyFeature() { return qgisc ? qgisc->actionSimplifyFeature() : 0; }
QAction *QgisAppInterface::actionDeleteRing() { return qgisc ? qgisc->actionDeleteRing() : 0; }
QAction *QgisAppInterface::actionDeletePart() { return qgisc ? qgisc->actionDeletePart() : 0; }
QAction *QgisAppInterface::actionNodeTool() { return qgisc ? qgisc->actionNodeTool() : 0; }

//! View menu actions
QAction *QgisAppInterface::actionPan() { return qgisc ? qgisc->actionPan() : 0; }
QAction *QgisAppInterface::actionPanToSelected() { return qgisc ? qgisc->actionPanToSelected() : 0; }
QAction *QgisAppInterface::actionZoomIn() { return qgisc ? qgisc->actionZoomIn() : 0; }
QAction *QgisAppInterface::actionZoomOut() { return qgisc ? qgisc->actionZoomOut() : 0; }
QAction *QgisAppInterface::actionSelect() { return qgisc ? qgisc->actionSelect() : 0; }
QAction *QgisAppInterface::actionSelectRectangle() { return qgisc ? qgisc->actionSelectRectangle() : 0; }
QAction *QgisAppInterface::actionSelectPolygon() { return qgisc ? qgisc->actionSelectPolygon() : 0; }
QAction *QgisAppInterface::actionSelectFreehand() { return qgisc ? qgisc->actionSelectFreehand() : 0; }
QAction *QgisAppInterface::actionSelectRadius() { return qgisc ? qgisc->actionSelectRadius() : 0; }
QAction *QgisAppInterface::actionIdentify() { return qgisc ? qgisc->actionIdentify() : 0; }
QAction *QgisAppInterface::actionFeatureAction() { return qgisc ? qgisc->actionFeatureAction() : 0; }
QAction *QgisAppInterface::actionMeasure() { return qgisc ? qgisc->actionMeasure() : 0; }
QAction *QgisAppInterface::actionMeasureArea() { return qgisc ? qgisc->actionMeasureArea() : 0; }
QAction *QgisAppInterface::actionZoomFullExtent() { return qgisc ? qgisc->actionZoomFullExtent() : 0; }
QAction *QgisAppInterface::actionZoomToLayer() { return qgisc ? qgisc->actionZoomToLayer() : 0; }
QAction *QgisAppInterface::actionZoomToSelected() { return qgisc ? qgisc->actionZoomToSelected() : 0; }
QAction *QgisAppInterface::actionZoomLast() { return qgisc ? qgisc->actionZoomLast() : 0; }
QAction *QgisAppInterface::actionZoomNext() { return qgisc ? qgisc->actionZoomNext() : 0; }
QAction *QgisAppInterface::actionZoomActualSize() { return qgisc ? qgisc->actionZoomActualSize() : 0; }
QAction *QgisAppInterface::actionMapTips() { return qgisc ? qgisc->actionMapTips() : 0; }
QAction *QgisAppInterface::actionNewBookmark() { return qgisc ? qgisc->actionNewBookmark() : 0; }
QAction *QgisAppInterface::actionShowBookmarks() { return qgisc ? qgisc->actionShowBookmarks() : 0; }
QAction *QgisAppInterface::actionDraw() { return qgisc ? qgisc->actionDraw() : 0; }

//! Layer menu actions
QAction *QgisAppInterface::actionNewVectorLayer() { return qgisc ? qgisc->actionNewVectorLayer() : 0; }
QAction *QgisAppInterface::actionAddOgrLayer() { return qgisc ? qgisc->actionAddOgrLayer() : 0; }
QAction *QgisAppInterface::actionAddRasterLayer() { return qgisc ? qgisc->actionAddRasterLayer() : 0; }
QAction *QgisAppInterface::actionAddPgLayer() { return qgisc ? qgisc->actionAddPgLayer() : 0; }
QAction *QgisAppInterface::actionAddWmsLayer() { return qgisc ? qgisc->actionAddWmsLayer() : 0; }
QAction *QgisAppInterface::actionAddAfsLayer() { return qgisc ? qgisc->actionAddAfsLayer() : 0; }
QAction *QgisAppInterface::actionAddAmsLayer() { return qgisc ? qgisc->actionAddAmsLayer() : 0; }
QAction *QgisAppInterface::actionCopyLayerStyle() { return qgisc ? qgisc->actionCopyLayerStyle() : 0; }
QAction *QgisAppInterface::actionPasteLayerStyle() { return qgisc ? qgisc->actionPasteLayerStyle() : 0; }
QAction *QgisAppInterface::actionOpenTable() { return qgisc ? qgisc->actionOpenTable() : 0; }
QAction *QgisAppInterface::actionOpenFieldCalculator() { return qgisc ? qgisc->actionOpenFieldCalculator() : 0; }
QAction *QgisAppInterface::actionToggleEditing() { return qgisc ? qgisc->actionToggleEditing() : 0; }
QAction *QgisAppInterface::actionSaveActiveLayerEdits() { return qgisc ? qgisc->actionSaveActiveLayerEdits() : 0; }
QAction *QgisAppInterface::actionAllEdits() { return qgisc ? qgisc->actionAllEdits() : 0; }
QAction *QgisAppInterface::actionSaveEdits() { return qgisc ? qgisc->actionSaveEdits() : 0; }
QAction *QgisAppInterface::actionSaveAllEdits() { return qgisc ? qgisc->actionSaveAllEdits() : 0; }
QAction *QgisAppInterface::actionRollbackEdits() { return qgisc ? qgisc->actionRollbackEdits() : 0; }
QAction *QgisAppInterface::actionRollbackAllEdits() { return qgisc ? qgisc->actionRollbackAllEdits() : 0; }
QAction *QgisAppInterface::actionCancelEdits() { return qgisc ? qgisc->actionCancelEdits() : 0; }
QAction *QgisAppInterface::actionCancelAllEdits() { return qgisc ? qgisc->actionCancelAllEdits() : 0; }
QAction *QgisAppInterface::actionLayerSaveAs() { return qgisc ? qgisc->actionLayerSaveAs() : 0; }
QAction *QgisAppInterface::actionLayerSelectionSaveAs() { return 0; }
QAction *QgisAppInterface::actionRemoveLayer() { return qgisc ? qgisc->actionRemoveLayer() : 0; }
QAction *QgisAppInterface::actionDuplicateLayer() { return qgisc ? qgisc->actionDuplicateLayer() : 0; }
QAction *QgisAppInterface::actionLayerProperties() { return qgisc ? qgisc->actionLayerProperties() : 0; }
QAction *QgisAppInterface::actionAddToOverview() { return qgisc ? qgisc->actionAddToOverview() : 0; }
QAction *QgisAppInterface::actionAddAllToOverview() { return qgisc ? qgisc->actionAddAllToOverview() : 0; }
QAction *QgisAppInterface::actionRemoveAllFromOverview() { return qgisc ? qgisc->actionRemoveAllFromOverview() : 0; }
QAction *QgisAppInterface::actionHideAllLayers() { return qgisc ? qgisc->actionHideAllLayers() : 0; }
QAction *QgisAppInterface::actionShowAllLayers() { return qgisc ? qgisc->actionShowAllLayers() : 0; }
QAction *QgisAppInterface::actionHideSelectedLayers() { return qgisc ? qgisc->actionHideSelectedLayers() : 0; }
QAction *QgisAppInterface::actionShowSelectedLayers() { return qgisc ? qgisc->actionShowSelectedLayers() : 0; }

//! Plugin menu actions
QAction *QgisAppInterface::actionManagePlugins() { return qgisc ? qgisc->actionManagePlugins() : 0; }
QAction *QgisAppInterface::actionPluginListSeparator() { return qgisc ? qgisc->actionPluginListSeparator() : 0; }
QAction *QgisAppInterface::actionShowPythonDialog() { return qgisc ? qgisc->actionShowPythonDialog() : 0; }

//! Settings menu actions
QAction *QgisAppInterface::actionToggleFullScreen() { return qgisc ? qgisc->actionToggleFullScreen() : 0; }
QAction *QgisAppInterface::actionOptions() { return qgisc ? qgisc->actionOptions() : 0; }
QAction *QgisAppInterface::actionCustomProjection() { return qgisc ? qgisc->actionCustomProjection() : 0; }

//! Help menu actions
QAction *QgisAppInterface::actionHelpContents() { return qgisc ? qgisc->actionHelpContents() : 0; }
QAction *QgisAppInterface::actionQgisHomePage() { return qgisc ? qgisc->actionQgisHomePage() : 0; }
QAction *QgisAppInterface::actionCheckQgisVersion() { return qgisc ? qgisc->actionCheckQgisVersion() : 0; }
QAction *QgisAppInterface::actionAbout() { return qgisc ? qgisc->actionAbout() : 0; }

bool QgisAppInterface::openFeatureForm( QgsVectorLayer *vlayer, QgsFeature &f, bool updateFeatureOnly, bool showModal )
{
  Q_UNUSED( updateFeatureOnly );
  if ( !vlayer )
    return false;

  QgsFeatureAction action( tr( "Attributes changed" ), f, vlayer, -1, -1, QgisApp::instance() );
  if ( vlayer->isEditable() )
  {
    return action.editFeature( showModal );
  }
  else
  {
    action.viewFeatureForm();
    return true;
  }
}

void QgisAppInterface::preloadForm( QString uifile )
{
  QSignalMapper* signalMapper = new QSignalMapper( this );
  mTimer = new QTimer( this );

  connect( mTimer, SIGNAL( timeout() ), signalMapper, SLOT( map() ) );
  connect( signalMapper, SIGNAL( mapped( QString ) ), mTimer, SLOT( stop() ) );
  connect( signalMapper, SIGNAL( mapped( QString ) ), this, SLOT( cacheloadForm( QString ) ) );

  signalMapper->setMapping( mTimer, uifile );

  mTimer->start( 0 );
}

void QgisAppInterface::cacheloadForm( QString uifile )
{
  QFile file( uifile );

  if ( file.open( QFile::ReadOnly ) )
  {
    QUiLoader loader;

    QFileInfo fi( uifile );
    loader.setWorkingDirectory( fi.dir() );
    QWidget *myWidget = loader.load( &file );
    file.close();
    delete myWidget;
  }
}

QgsAttributeDialog* QgisAppInterface::getFeatureForm( QgsVectorLayer *l, QgsFeature &feature )
{
  QgsDistanceArea myDa;

  myDa.setSourceCrs( l->crs().srsid() );
  myDa.setEllipsoidalMode( QgisApp::instance()->mapCanvas()->mapSettings().hasCrsTransformEnabled() );
  myDa.setEllipsoid( QgsProject::instance()->readEntry( "Measure", "/Ellipsoid", GEO_NONE ) );

  QgsAttributeEditorContext context;
  context.setDistanceArea( myDa );
  context.setVectorLayerTools( qgis->vectorLayerTools() );
  QgsAttributeDialog *dialog = new QgsAttributeDialog( l, &feature, false, NULL, true, context );
  if ( !feature.isValid() )
  {
    dialog->setIsAddDialog( true );
  }
  return dialog;
}

QgsVectorLayerTools* QgisAppInterface::vectorLayerTools()
{
  return qgis->vectorLayerTools();
}

QList<QgsMapLayer *> QgisAppInterface::editableLayers( bool modified ) const
{
  return qgis->editableLayers( modified );
}

int QgisAppInterface::messageTimeout()
{
  return qgis->messageTimeout();
}

void QgisAppInterface::addPasteHandler( const QString& mimeType, QgsPasteHandler* handler )
{
  qgis->addPasteHandler( mimeType, handler );
}

void QgisAppInterface::removePasteHandler( const QString& mimeType, QgsPasteHandler* handler )
{
  qgis->removePasteHandler( mimeType, handler );
}

void QgisAppInterface::registerMapLayerPropertiesFactory( QgsMapLayerPropertiesFactory* factory )
{
  qgis->registerMapLayerPropertiesFactory( factory );
}

void QgisAppInterface::unregisterMapLayerPropertiesFactory( QgsMapLayerPropertiesFactory* factory )
{
  qgis->unregisterMapLayerPropertiesFactory( factory );
}


QgsPluginInterface* QgisAppInterface::pluginInterface( const QString& pluginName )
{
  return qgis->pluginInterface( pluginName );
}

QgsClipboard* QgisAppInterface::clipboard()
{
  return qgis->clipboard();
}
