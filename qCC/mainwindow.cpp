//##########################################################################
//#                                                                        #
//#                            CLOUDCOMPARE                                #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#          COPYRIGHT: EDF R&D / TELECOM ParisTech (ENST-TSI)             #
//#                                                                        #
//##########################################################################

#include "mainwindow.h"

//CCLib Includes
#include <GenericChunkedArray.h>
#include <CloudSamplingTools.h>
#include <MeshSamplingTools.h>
#include <ScalarFieldTools.h>
#include <StatisticalTestingTools.h>
#include <WeibullDistribution.h>
#include <NormalDistribution.h>
#include <GenericIndexedCloud.h>
#include <Neighbourhood.h>
#include <AutoSegmentationTools.h>
#include <DistanceComputationTools.h>
#include <PointProjectionTools.h>
#include <GeometricalAnalysisTools.h>
#include <SimpleCloud.h>
#include <RegistrationTools.h> //Aurelien BEY

//qCC_db
#include <ccHObjectCaster.h>
#include <ccPointCloud.h>
#include <ccMesh.h>
#include <ccPolyline.h>
#include <ccSubMesh.h>
#include <ccOctree.h>
#include <ccKdTree.h>
#include <ccGBLSensor.h>
#include <ccCameraSensor.h>
#include <ccNormalVectors.h>
#include <ccProgressDialog.h>
#include <ccPlane.h>
#include <ccImage.h>
#include <cc2DLabel.h>
#include <cc2DViewportObject.h>
#include <ccColorScale.h>
#include <ccColorScalesManager.h>
#include <ccFacet.h>
#include <ccQuadric.h>
#include <ccExternalFactory.h>

//qCC includes
#include "ccHeightGridGeneration.h"
#include "ccRenderingTools.h"
#include "ccFastMarchingForNormsDirection.h"
#include "ccMinimumSpanningTreeForNormsDirection.h"
#include <ccInnerRect2DFinder.h>
#include "ccCommon.h"

//sub-windows
#include "ccGLWindow.h"
#include "ccConsole.h"
#include "ccHistogramWindow.h"

//plugins handling
#include <ccPluginInterface.h>
#include <ccStdPluginInterface.h>
#include <ccGLFilterPluginInterface.h>
#include "ccPluginDlg.h"

//shaders & Filters
#include <ccShader.h>
#include <ccGlFilter.h>

//dialogs
#include "ccDisplayOptionsDlg.h"
#include "ccGraphicalSegmentationTool.h"
#include "ccGraphicalTransformationTool.h"
#include "ccClippingBoxTool.h"
#include "ccOrderChoiceDlg.h"
#include "ccComparisonDlg.h"
#include "ccColorGradientDlg.h"
#include "ccAskTwoDoubleValuesDlg.h"
#include "ccAskThreeDoubleValuesDlg.h"
#include "ccPtsSamplingDlg.h"
#include "ccPickOneElementDlg.h"
#include "ccStatisticalTestDlg.h"
#include "ccLabelingDlg.h"
#include "ccSensorProjectionDlg.h"
#include "ccHeightGridGenerationDlg.h"
#include "ccUnrollDlg.h"
#include "ccAlignDlg.h" //Aurelien BEY
#include "ccRegistrationDlg.h" //Aurelien BEY
#include "ccSubsamplingDlg.h" //Aurelien BEY
#include "ccRenderToFileDlg.h"
#include "ccPointPropertiesDlg.h" //Aurelien BEY
#include "ccPointListPickingDlg.h"
#include "ccNormalComputationDlg.h"
#include "ccCameraParamEditDlg.h"
#include "ccScalarFieldArithmeticsDlg.h"
#include "ccScalarFieldFromColorDlg.h"
#include "ccSensorComputeDistancesDlg.h"
#include "ccSensorComputeScatteringAnglesDlg.h"
#include "ccCurvatureDlg.h"
#include "ccApplyTransformationDlg.h"
#include "ccCoordinatesShiftManager.h"
#include "ccPointPairRegistrationDlg.h"
#include "ccExportCoordToSFDlg.h"
#include "ccPrimitiveFactoryDlg.h"
#include "ccMouse3DContextMenu.h"
#include "ccColorScaleEditorDlg.h"
#include "ccComputeOctreeDlg.h"
#include "ccAdjustZoomDlg.h"
#include "ccBoundingBoxEditorDlg.h"
#include "ccColorLevelsDlg.h"
#include <ui_aboutDlg.h>

//other
#include "ccRegistrationTools.h"

//3D mouse handler
#ifdef CC_3DXWARE_SUPPORT
#include "devices/3dConnexion/Mouse3DInput.h"
#endif

//Qt Includes
#include <QtGui>
#include <QMdiArea>
#include <QSignalMapper>
#include <QMdiSubWindow>
#include <QLCDNumber>
#include <QFileDialog>
#include <QActionGroup>
#include <QProcess>
#include <QSettings>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QInputDialog>
#include <QTextStream>
#include <QColorDialog>

//System
#include <string.h>
#include <math.h>
#include <assert.h>
#include <cfloat>
#include <iostream>

//global static pointer (as there should only be one instance of MainWindow!)
static MainWindow* s_instance = 0;

//persistent settings key (to be used with QSettings)
static const QString s_psLoadFile				("LoadFile");
static const QString s_psSaveFile				("SaveFile");
static const QString s_psMainWinGeom			("mainWindowGeometry");
static const QString s_psMainWinState			("mainWindowState");
static const QString s_psCurrentPath			("currentPath");
static const QString s_psSelectedFilter			("selectedFilter");
static const QString s_psSelectedFilterCloud	("selectedFilterCloud");
static const QString s_psSelectedFilterMesh		("selectedFilterMesh");
static const QString s_psDuplicatePointsGroup	("duplicatePoints");
static const QString s_psDuplicatePointsMinDist	("minDist");

//standard message in case of locked vertices
void DisplayLockedVerticesWarning()
{
	ccConsole::Error("Mesh vertices are 'locked' (they may be shared by multiple meshes for instance).\nYou should call this method directly on the vertices cloud (but all meshes will be impacted!).");
}

MainWindow::MainWindow()
	: m_ccRoot(0)
	, m_uiFrozen(false)
	, m_3dMouseInput(0)
	, m_viewModePopupButton(0)
	, m_pivotVisibilityPopupButton(0)
	, m_cpeDlg(0)
	, m_gsTool(0)
	, m_transTool(0)
	, m_clipTool(0)
	, m_compDlg(0)
	, m_ppDlg(0)
	, m_plpDlg(0)
	, m_pprDlg(0)
	, m_pfDlg(0)
	, m_glFilterActions(this)
{
	//Dialog "auto-construction"
	setupUi(this);
	QSettings settings;
	restoreGeometry(settings.value(s_psMainWinGeom).toByteArray());

	setWindowTitle(QString("CloudCompare v")+ccCommon::GetCCVersion(false));

	//advanced widgets not handled by QDesigner
	{
		//view mode pop-up menu
		{
			m_viewModePopupButton = new QToolButton();
			QMenu* menu = new QMenu(m_viewModePopupButton);
			menu->addAction(actionSetOrthoView);
			menu->addAction(actionSetCenteredPerspectiveView);
			menu->addAction(actionSetViewerPerspectiveView);

			m_viewModePopupButton->setMenu(menu);
			m_viewModePopupButton->setPopupMode(QToolButton::InstantPopup);
			m_viewModePopupButton->setToolTip("Set current view mode");
			m_viewModePopupButton->setStatusTip(m_viewModePopupButton->toolTip());
			toolBarView->insertWidget(actionZoomAndCenter,m_viewModePopupButton);
			m_viewModePopupButton->setEnabled(false);
		}

		//pivot center pop-up menu
		{
			m_pivotVisibilityPopupButton = new QToolButton();
			QMenu* menu = new QMenu(m_pivotVisibilityPopupButton);
			menu->addAction(actionSetPivotAlwaysOn);
			menu->addAction(actionSetPivotRotationOnly);
			menu->addAction(actionSetPivotOff);

			m_pivotVisibilityPopupButton->setMenu(menu);
			m_pivotVisibilityPopupButton->setPopupMode(QToolButton::InstantPopup);
			m_pivotVisibilityPopupButton->setToolTip("Set pivot visibility");
			m_pivotVisibilityPopupButton->setStatusTip(m_pivotVisibilityPopupButton->toolTip());
			toolBarView->insertWidget(actionZoomAndCenter,m_pivotVisibilityPopupButton);
			m_pivotVisibilityPopupButton->setEnabled(false);
		}
	}

	//tabifyDockWidget(DockableDBTree,DockableProperties);

	//Console
	ccConsole::Init(consoleWidget,this,this);

	//db-tree link
	m_ccRoot = new ccDBRoot(dbTreeView, propertiesTreeView, this);
	connect(m_ccRoot, SIGNAL(selectionChanged()), this, SLOT(updateUIWithSelection()));

	//MDI Area
	m_mdiArea = new QMdiArea(this);
	setCentralWidget(m_mdiArea);
	connect(m_mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(updateMenus()));
	connect(m_mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(on3DViewActivated(QMdiSubWindow*)));

	//Window Mapper
	m_windowMapper = new QSignalMapper(this);
	connect(m_windowMapper, SIGNAL(mapped(QWidget*)), this, SLOT(setActiveSubWindow(QWidget*)));

	//Keyboard shortcuts
	connect(actionToggleVisibility,	SIGNAL(triggered()), this, SLOT(toggleSelectedEntitiesVisibility()));	//'V': toggles selected items visibility
	connect(actionToggleNormals,	SIGNAL(triggered()), this, SLOT(toggleSelectedEntitiesNormals()));		//'N': toggles selected items normals visibility
	connect(actionToggleColors,		SIGNAL(triggered()), this, SLOT(toggleSelectedEntitiesColors()));		//'C': toggles selected items colors visibility
	connect(actionToggleSF,			SIGNAL(triggered()), this, SLOT(toggleSelectedEntitiesSF()));			//'S': toggles selected items SF visibility
	connect(actionToggleShowName,	SIGNAL(triggered()), this, SLOT(toggleSelectedEntities3DName()));		//'D': toggles selected items '3D name' visibility
	connect(actionToggleMaterials,	SIGNAL(triggered()), this, SLOT(toggleSelectedEntitiesMaterials()));	//'M': toggles selected items materials/textures visibility

	connectActions();

	loadPlugins();

#ifdef CC_3DXWARE_SUPPORT
	enable3DMouse(true,true);
#else
	actionEnable3DMouse->setEnabled(false);
#endif

	new3DView();

	freezeUI(false);

	//updateMenus(); //the calls to 'new3DView' and 'freezeUI' already did that
	updateUIWithSelection();

	showMaximized();

	QMainWindow::statusBar()->showMessage(QString("Ready"));
	ccConsole::Print("CloudCompare started!");

	restoreState(settings.value(s_psMainWinState).toByteArray());
}

MainWindow::~MainWindow()
{
	release3DMouse();

	assert(m_ccRoot && m_mdiArea && m_windowMapper);
	m_ccRoot->disconnect();
	m_mdiArea->disconnect();
	m_windowMapper->disconnect();

	//we don't want any other dialog/function to use the following structures
	ccDBRoot* ccRoot = m_ccRoot;
	m_ccRoot = 0;
	if (m_mdiArea)
	{
		QList<QMdiSubWindow*> subWindowList = m_mdiArea->subWindowList();
		for (int i=0;i<subWindowList.size();++i)
			static_cast<ccGLWindow*>(subWindowList[i]->widget())->setSceneDB(0);
	}
	m_cpeDlg = 0;
	m_gsTool = 0;
	m_transTool = 0;
	m_clipTool = 0;
	m_compDlg=0;
	m_ppDlg = 0;
	m_plpDlg = 0;
	m_pprDlg = 0;
	m_pfDlg = 0;

	//release all 'overlay' dialogs
	while (!m_mdiDialogs.empty())
	{
		m_mdiDialogs.back().dialog->disconnect();
		m_mdiDialogs.back().dialog->stop(false);
		m_mdiDialogs.back().dialog->setParent(0);
		delete m_mdiDialogs.back().dialog;
		m_mdiDialogs.pop_back();
	}
	//m_mdiDialogs.clear();
	m_mdiArea->closeAllSubWindows();

	if (ccRoot)
		delete ccRoot;

	ccConsole::ReleaseInstance();
}

ccPluginInterface* MainWindow::getValidPlugin(QObject* plugin)
{
	if (plugin)
	{
		//standard plugin?
		ccStdPluginInterface* ccStdPlugin = qobject_cast<ccStdPluginInterface*>(plugin);
		if (ccStdPlugin)
			return static_cast<ccPluginInterface*>(ccStdPlugin);

		ccGLFilterPluginInterface* ccGLPlugin = qobject_cast<ccGLFilterPluginInterface*>(plugin);
		if (ccGLPlugin)
			return static_cast<ccPluginInterface*>(ccGLPlugin);
	}

	return 0;
}

void MainWindow::loadPlugins()
{
	menuPlugins->setEnabled(false);
	menuShadersAndFilters->setEnabled(false);
	toolBarPluginTools->setVisible(false);
	toolBarGLFilters->setVisible(false);

	//"static" plugins
	foreach (QObject *plugin, QPluginLoader::staticInstances())
		dispatchPlugin(plugin);

	ccConsole::Print(QString("Application path: ")+QCoreApplication::applicationDirPath());

#if defined(Q_OS_MAC)
	// plugins are in the bundle
	QString path = QCoreApplication::applicationDirPath();
	path.remove( "MacOS" );
	m_pluginsPath = path + "Plugins/ccPlugins";
#else
	//plugins are in bin/plugins
	m_pluginsPath = QCoreApplication::applicationDirPath()+QString("/plugins");
#endif

	ccConsole::Print(QString("Plugins lookup dir.: %1").arg(m_pluginsPath));

	QStringList filters;
#if defined(Q_OS_WIN)
	filters << "*.dll";
#elif defined(Q_OS_LINUX)
	filters << "*.so";
#elif defined(Q_OS_MAC)
	filters << "*.dylib";
#endif
	QDir pluginsDir(m_pluginsPath);
	pluginsDir.setNameFilters(filters);
	foreach (QString filename, pluginsDir.entryList(filters))
	{
		QPluginLoader loader(pluginsDir.absoluteFilePath(filename));
		QObject* plugin = loader.instance();
		if (plugin)
		{
			ccConsole::Print(QString("Found new plugin! ('%1')").arg(filename));
			if (dispatchPlugin(plugin))
			{
				m_pluginFileNames += filename;
			}
			else
			{
				ccConsole::Warning("Unsupported or invalid plugin type");
			}
		}
		else
		{
			ccConsole::Warning(QString("[Plugin] %1")/*.arg(pluginsDir.absoluteFilePath(filename))*/.arg(loader.errorString()));
		}
	}

	if (toolBarPluginTools->isEnabled())
	{
		actionDisplayPluginTools->setEnabled(true);
		actionDisplayPluginTools->setChecked(true);
	}
	else
	{
		//DGM: doesn't work :(
		//actionDisplayPluginTools->setChecked(false);
	}

	if (toolBarGLFilters->isEnabled())
	{
		actionDisplayGLFiltersTools->setEnabled(true);
		actionDisplayGLFiltersTools->setChecked(true);
	}
	else
	{
		//DGM: doesn't work :(
		//actionDisplayGLFiltersTools->setChecked(false);
	}
}

bool MainWindow::dispatchPlugin(QObject *plugin)
{
	ccPluginInterface* ccPlugin = getValidPlugin(plugin);
	if (!ccPlugin)
		return false;
	plugin->setParent(this);

	QString pluginName = ccPlugin->getName();
	if (pluginName.isEmpty())
	{
		ccLog::Warning("Plugin has an invalid (empty) name!");
		return false;
	}
	ccConsole::Print("Plugin name: [%s]",qPrintable(pluginName));

	switch(ccPlugin->getType())
	{

	case CC_STD_PLUGIN: //standard plugin
		{
			ccStdPluginInterface* stdPlugin = static_cast<ccStdPluginInterface*>(ccPlugin);
			stdPlugin->setMainAppInterface(this);

			QMenu* destMenu=0;
			QToolBar* destToolBar=0;

			QActionGroup actions(this);
			stdPlugin->getActions(actions);
			if (actions.actions().size() > 1) //more than one action? We create it's own menu and toolbar
			{
				destMenu = (menuPlugins ? menuPlugins->addMenu(pluginName) : 0);
				if (destMenu)
					destMenu->setIcon(stdPlugin->getIcon());
				destToolBar = addToolBar(pluginName+QString(" toolbar"));

				//not sure why but it seems that we must specifically set the object name.
				//if not the QSettings thing will complain about a not-setted name
				//when saving settings of qCC mainwindow
				destToolBar->setObjectName(pluginName);
			}
			else //default destination
			{
				destMenu = menuPlugins;
				destToolBar = toolBarPluginTools;
			}

			//add actions
			foreach(QAction* action,actions.actions())
			{
				//add to menu (if any)
				if (destMenu)
				{
					destMenu->addAction(action);
					destMenu->setEnabled(true);
				}
				//add to toolbar
				if (destToolBar)
				{
					destToolBar->addAction(action);
					destToolBar->setVisible(true);
					destToolBar->setEnabled(true);
				}
			}

			//add to std. plugins list
			m_stdPlugins.push_back(stdPlugin);

			// see if this plugin can give back an additional factory for objects
			ccExternalFactory* factory = stdPlugin->getCustomObjectsFactory();
			if (factory) // if it is valid add to the plugin_factories
			{
				assert(ccExternalFactory::Container::GetUniqueInstance());
				ccExternalFactory::Container::GetUniqueInstance()->addFactory(factory);
			}
		}
		break;

	case CC_GL_FILTER_PLUGIN: //GL filter
		{
			//(auto)create action
			QAction* action = new QAction(pluginName,plugin);
			action->setToolTip(ccPlugin->getDescription());
			action->setIcon(ccPlugin->getIcon());
			//connect default signal
			connect(action, SIGNAL(triggered()), this, SLOT(doEnableGLFilter()));

			menuShadersAndFilters->addAction(action);
			menuShadersAndFilters->setEnabled(true);
			toolBarGLFilters->addAction(action);
			toolBarGLFilters->setVisible(true);
			toolBarGLFilters->setEnabled(true);

			//add to GL filter (actions) list
			m_glFilterActions.addAction(action);

		}
		break;

	default:
		assert(false);
		ccLog::Print("Unhandled plugin type!");
		return false;
	}

	return true;
}

void MainWindow::aboutPlugins()
{
	ccPluginDlg ccpDlg(m_pluginsPath, m_pluginFileNames, this);
	ccpDlg.exec();
}

void MainWindow::doEnableGLFilter()
{
	ccGLWindow* win = getActiveGLWindow();
	if (!win)
	{
		ccLog::Warning("[GL filter] No active 3D view!");
		return;
	}

	QAction *action = qobject_cast<QAction*>(sender());
	ccPluginInterface *ccPlugin = getValidPlugin(action ? action->parent() : 0);
	if (!ccPlugin)
		return;

	if (ccPlugin->getType() != CC_GL_FILTER_PLUGIN)
		return;

	ccGlFilter* filter = static_cast<ccGLFilterPluginInterface*>(ccPlugin)->getFilter();
	if (filter)
	{
		if (win->areGLFiltersEnabled())
		{
			win->setGlFilter(filter);
			ccConsole::Print("Note: go to << Display > Shaders & Filters > No filter >> to disable GL filter");
		}
		else
			ccConsole::Error("GL filters not supported!");
	}
	else
	{
		ccConsole::Error("Can't load GL filter (an error occurred)!");
	}
}

void MainWindow::release3DMouse()
{
#ifdef CC_3DXWARE_SUPPORT
	if (m_3dMouseInput)
	{
		disconnect(m_3dMouseInput);
		delete m_3dMouseInput;
		m_3dMouseInput=0;
	}
#endif
}

void MainWindow::setup3DMouse(bool state)
{
	enable3DMouse(state,false);
}

void MainWindow::enable3DMouse(bool state, bool silent)
{
#ifdef CC_3DXWARE_SUPPORT
	if (m_3dMouseInput)
		release3DMouse();

	if (state)
	{
		if (Mouse3DInput::DeviceAvailable())
		{
			ccLog::Warning("[3D Mouse] Device has been detected!");
			m_3dMouseInput = new Mouse3DInput(this);
			QObject::connect(m_3dMouseInput, SIGNAL(sigMove3d(std::vector<float>&)), this, SLOT(on3DMouseMove(std::vector<float>&)));
			QObject::connect(m_3dMouseInput, SIGNAL(sigOn3dmouseKeyDown(int)), this, SLOT(on3DMouseKeyDown(int)));
			QObject::connect(m_3dMouseInput, SIGNAL(sigOn3dmouseKeyUp(int)), this, SLOT(on3DMouseKeyUp(int)));
		}
		else
		{
			if (silent)
				ccLog::Print("[3D Mouse] No device found");
			else
				ccLog::Error("[3D Mouse] No device found");
			state = false;
		}
	}
	else
	{
		ccLog::Warning("[3D Mouse] Device has been disabled");
	}
#else
	state = false;
#endif

	actionEnable3DMouse->blockSignals(true);
	actionEnable3DMouse->setChecked(state);
	actionEnable3DMouse->blockSignals(false);
}

void MainWindow::on3DMouseKeyUp(int)
{
	//nothing right now
}

#ifdef CC_3DXWARE_SUPPORT
static bool s_3dMouseContextMenuAlreadyShown = false; //DGM: to prevent multiple instances at once
#endif
// ANY CHANGE/BUG FIX SHOULD BE REFLECTED TO THE EQUIVALENT METHODS IN QCC "MainWindow.cpp" FILE!
void MainWindow::on3DMouseKeyDown(int key)
{
	if (!m_3dMouseInput)
		return;

#ifdef CC_3DXWARE_SUPPORT

	switch(key)
	{
	case Mouse3DInput::V3DK_MENU:
		if (!s_3dMouseContextMenuAlreadyShown)
		{
				s_3dMouseContextMenuAlreadyShown = true;

				//is there a currently active window?
				ccGLWindow* activeWin = getActiveGLWindow();
				if (activeWin)
				{
					ccMouse3DContextMenu(&m_3dMouseInput->getMouseParams(),activeWin,this).exec(QCursor::pos());
					//in case of...
					updateViewModePopUpMenu(activeWin);
					updatePivotVisibilityPopUpMenu(activeWin);
				}
				else
				{
					ccLog::Error("No active 3D view! Select a 3D view first...");
					return;
				}

				s_3dMouseContextMenuAlreadyShown = false;
		}
		break;
	case Mouse3DInput::V3DK_FIT:
		{
			if (m_selectedEntities.empty())
				setGlobalZoom();
			else
				zoomOnSelectedEntities();
		}
		break;
	case Mouse3DInput::V3DK_TOP:
		setTopView();
		break;
	case Mouse3DInput::V3DK_LEFT:
		setLeftView();
		break;
	case Mouse3DInput::V3DK_RIGHT:
		setRightView();
		break;
	case Mouse3DInput::V3DK_FRONT:
		setFrontView();
		break;
	case Mouse3DInput::V3DK_BOTTOM:
		setBottomView();
		break;
	case Mouse3DInput::V3DK_BACK:
		setBackView();
		break;
	case Mouse3DInput::V3DK_ROTATE:
		m_3dMouseInput->getMouseParams().enableRotation(!m_3dMouseInput->getMouseParams().rotationEnabled());
		break;
	case Mouse3DInput::V3DK_PANZOOM:
		m_3dMouseInput->getMouseParams().enablePanZoom(!m_3dMouseInput->getMouseParams().panZoomEnabled());
		break;
	case Mouse3DInput::V3DK_ISO1:
		setIsoView1();
		break;
	case Mouse3DInput::V3DK_ISO2:
		setIsoView2();
		break;
	case Mouse3DInput::V3DK_PLUS:
		m_3dMouseInput->getMouseParams().accelerate();
		break;
	case Mouse3DInput::V3DK_MINUS:
		m_3dMouseInput->getMouseParams().slowDown();
		break;
	case Mouse3DInput::V3DK_DOMINANT:
		m_3dMouseInput->getMouseParams().toggleDominantMode();
		break;
	case Mouse3DInput::V3DK_CW:
	case Mouse3DInput::V3DK_CCW:
		{
			ccGLWindow* activeWin = getActiveGLWindow();
			if (activeWin)
			{
				CCVector3d axis(0,0,-1);
				CCVector3d trans(0,0,0);
				ccGLMatrixd mat;
				double angle = M_PI/2;
				if (key == Mouse3DInput::V3DK_CCW)
					angle = -angle;
				mat.initFromParameters(angle,axis,trans);
				activeWin->rotateBaseViewMat(mat);
				activeWin->redraw();
			}
		}
		break;
	case Mouse3DInput::V3DK_ESC:
	case Mouse3DInput::V3DK_ALT:
	case Mouse3DInput::V3DK_SHIFT:
	case Mouse3DInput::V3DK_CTRL:
	default:
		ccLog::Warning("[3D mouse] This button is not handled (yet)");
		//TODO
		break;
	}

#endif
}

// ANY CHANGE/BUG FIX SHOULD BE REFLECTED TO THE EQUIVALENT METHODS IN QCC "MainWindow.cpp" FILE!
void MainWindow::on3DMouseMove(std::vector<float>& vec)
{
	//ccLog::PrintDebug(QString("[3D mouse] %1 %2 %3 %4 %5 %6").arg(vec[0]).arg(vec[1]).arg(vec[2]).arg(vec[3]).arg(vec[4]).arg(vec[5]));

#ifdef CC_3DXWARE_SUPPORT

	//no active device?
	if (!m_3dMouseInput)
		return;

	ccGLWindow* win = getActiveGLWindow();
	//no active window?
	if (!win)
		return;

	//mouse parameters
	const Mouse3DParameters& params = m_3dMouseInput->getMouseParams();
	bool panZoom = params.panZoomEnabled();
	bool rotate = params.rotationEnabled();
	if (!panZoom && !rotate)
		return;

	//view parameters
	bool objectMode = true;
	bool perspectiveView = win->getPerspectiveState(objectMode);

	//Viewer based perspective IS 'camera mode'
	{
		//Mouse3DParameters::NavigationMode navigationMode = objectMode ? Mouse3DParameters::ObjectMode : Mouse3DParameters::CameraMode;
		//if (params.navigationMode() != navigationMode)
		//{
		//	m_3dMouseInput->getMouseParams().setNavigationMode(navigationMode);
		//	ccLog::Warning("[3D mouse] Navigation mode has been changed to fit current viewing mode");
		//}
	}

	//dominant mode: dominant mode is intended to limit movement to a single direction
	if (params.dominantModeEnabled())
	{
		unsigned dominantDim = 0;
		for (unsigned i=1; i<6; ++i)
			if (fabs(vec[i]) > fabs(vec[dominantDim]))
				dominantDim = i;
		for (unsigned i=0; i<6; ++i)
			if (i != dominantDim)
				vec[i] = 0.0;
	}
	if (panZoom)
	{
		//Zoom: object moves closer/away (only for ortho. mode)
		if (!perspectiveView && fabs(vec[1])>ZERO_TOLERANCE)
		{
			win->updateZoom(1.0f + vec[1]);
			vec[1] = 0.0f;
		}

		//Zoom & Panning: camera moves right/left + up/down + backward/forward (only for perspective mode)
		if (fabs(vec[0])>ZERO_TOLERANCE || fabs(vec[1])>ZERO_TOLERANCE || fabs(vec[2])>ZERO_TOLERANCE)
		{
			const ccViewportParameters& viewParams = win->getViewportParameters();

			float scale = static_cast<float>(std::min(win->width(),win->height()) * viewParams.pixelSize);
			if (perspectiveView)
			{
				float tanFOV = tan(viewParams.fov*static_cast<float>(CC_DEG_TO_RAD)/*/2*/);
				vec[0] *= tanFOV;
				vec[2] *= tanFOV;
				scale /= win->computePerspectiveZoom();
			}
			else
			{
				scale /= win->getViewportParameters().zoom;
			}

			if (objectMode)
				scale = -scale;
			win->moveCamera(vec[0]*scale,-vec[2]*scale,vec[1]*scale);
		}
	}

	if (rotate)
	{
		if (	fabs(vec[3]) > ZERO_TOLERANCE
			||	fabs(vec[4]) > ZERO_TOLERANCE
			||	fabs(vec[5]) > ZERO_TOLERANCE)
		{
			//get corresponding quaternion
			float q[4];
			Mouse3DInput::GetQuaternion(vec,q);
			ccGLMatrixd rotMat = ccGLMatrixd::FromQuaternion(q);

			//horizon locked?
			if (params.horizonLocked())
			{
				rotMat = rotMat.yRotation();
			}

			win->rotateBaseViewMat(objectMode ? rotMat : rotMat.inverse());
			win->showPivotSymbol(true);
		}
		else
		{
			win->showPivotSymbol(false);
		}
	}

	win->redraw();

#endif
}

void MainWindow::connectActions()
{
	assert(m_ccRoot);
	assert(m_mdiArea);

	//TODO... but not ready yet ;)
	actionLoadShader->setVisible(false);
	actionKMeans->setVisible(false);
	actionFrontPropagation->setVisible(false);

	/*** MAIN MENU ***/

	//"File" menu
	connect(actionOpen,							SIGNAL(triggered()),	this,		SLOT(loadFile()));
	connect(actionSave,							SIGNAL(triggered()),	this,		SLOT(saveFile()));
	connect(actionPrimitiveFactory,				SIGNAL(triggered()),	this,		SLOT(doShowPrimitiveFactory()));
	connect(actionEnable3DMouse,				SIGNAL(toggled(bool)),	this,		SLOT(setup3DMouse(bool)));
	connect(actionCloseAll,						SIGNAL(triggered()),	this,		SLOT(closeAll()));
	connect(actionQuit,							SIGNAL(triggered()),	this,		SLOT(close()));

	//"Edit > Colors" menu
	connect(actionSetUniqueColor,				SIGNAL(triggered()),	this,		SLOT(doActionSetUniqueColor()));
	connect(actionSetColorGradient,				SIGNAL(triggered()),	this,		SLOT(doActionSetColorGradient()));
	connect(actionChangeColorLevels,			SIGNAL(triggered()),	this,		SLOT(doActionChangeColorLevels()));
	connect(actionColorize,						SIGNAL(triggered()),	this,		SLOT(doActionColorize()));
	connect(actionClearColor,					SIGNAL(triggered()),	this,		SLOT(doActionClearColor()));
	connect(actionInterpolateColors,			SIGNAL(triggered()),	this,		SLOT(doActionInterpolateColors()));

	//"Edit > Normals" menu
	connect(actionComputeNormals,				SIGNAL(triggered()),	this,		SLOT(doActionComputeNormals()));
	connect(actionInvertNormals,				SIGNAL(triggered()),	this,		SLOT(doActionInvertNormals()));
	connect(actionConvertNormalToHSV,			SIGNAL(triggered()),	this,		SLOT(doActionConvertNormalsToHSV()));
	connect(actionConvertNormalToDipDir,		SIGNAL(triggered()),	this,		SLOT(doActionConvertNormalsToDipDir()));
	connect(actionOrientNormalsMST,				SIGNAL(triggered()),	this,		SLOT(doActionOrientNormalsMST()));
	connect(actionOrientNormalsFM,				SIGNAL(triggered()),	this,		SLOT(doActionOrientNormalsFM()));
	connect(actionClearNormals,					SIGNAL(triggered()),	this,		SLOT(doActionClearNormals()));
	//"Edit > Octree" menu
	connect(actionComputeOctree,				SIGNAL(triggered()),	this,		SLOT(doActionComputeOctree()));
	connect(actionResampleWithOctree,			SIGNAL(triggered()),	this,		SLOT(doActionResampleWithOctree()));
	//"Edit > Mesh" menu
	connect(actionComputeMeshAA,				SIGNAL(triggered()),	this,		SLOT(doActionComputeMeshAA()));
	connect(actionComputeMeshLS,				SIGNAL(triggered()),	this,		SLOT(doActionComputeMeshLS()));
	connect(actionConvertTextureToColor,		SIGNAL(triggered()),	this,		SLOT(doActionConvertTextureToColor()));
	connect(actionSamplePoints,					SIGNAL(triggered()),	this,		SLOT(doActionSamplePoints()));
	connect(actionSmoothMeshLaplacian,			SIGNAL(triggered()),	this,		SLOT(doActionSmoothMeshLaplacian()));
	connect(actionSubdivideMesh,				SIGNAL(triggered()),	this,		SLOT(doActionSubdivideMesh()));
	connect(actionMeasureMeshSurface,			SIGNAL(triggered()),	this,		SLOT(doActionMeasureMeshSurface()));
	//"Edit > Mesh > Scalar Field" menu
	connect(actionSmoothMeshSF,					SIGNAL(triggered()),	this,		SLOT(doActionSmoothMeshSF()));
	connect(actionEnhanceMeshSF,				SIGNAL(triggered()),	this,		SLOT(doActionEnhanceMeshSF()));
	//"Edit > Sensor > Ground-Based lidar" menu
	connect(actionShowDepthBuffer,				SIGNAL(triggered()),	this,		SLOT(doActionShowDepthBuffer()));
	connect(actionExportDepthBuffer,			SIGNAL(triggered()),	this,		SLOT(doActionExportDepthBuffer()));
	//"Edit > Sensor" menu
	connect(actionCreateGBLSensor,				SIGNAL(triggered()),	this,		SLOT(doActionCreateGBLSensor()));
	connect(actionCreateCameraSensor,			SIGNAL(triggered()),	this,		SLOT(doActionCreateCameraSensor()));
	connect(actionModifySensor,					SIGNAL(triggered()),	this,		SLOT(doActionModifySensor()));
	connect(actionProjectUncertainty,			SIGNAL(triggered()),	this,		SLOT(doActionProjectUncertainty()));
	connect(actionCheckPointsInsideFrustrum,	SIGNAL(triggered()),	this,		SLOT(doActionCheckPointsInsideFrustrum()));
	connect(actionComputeDistancesFromSensor,	SIGNAL(triggered()),	this,		SLOT(doActionComputeDistancesFromSensor()));
	connect(actionComputeScatteringAngles,		SIGNAL(triggered()),	this,		SLOT(doActionComputeScatteringAngles()));
	connect(actionViewFromSensor,				SIGNAL(triggered()),	this,		SLOT(doActionSetViewFromSensor()));
	//"Edit > Scalar fields" menu
	connect(actionShowHistogram,				SIGNAL(triggered()),	this,		SLOT(showSelectedEntitiesHistogram()));
	connect(actionSFGradient,					SIGNAL(triggered()),	this,		SLOT(doActionSFGradient()));
	connect(actionGaussianFilter,				SIGNAL(triggered()),	this,		SLOT(doActionSFGaussianFilter()));
	connect(actionBilateralFilter,				SIGNAL(triggered()),	this,		SLOT(doActionSFBilateralFilter()));
	connect(actionFilterByValue,				SIGNAL(triggered()),	this,		SLOT(doActionFilterByValue()));
	connect(actionAddConstantSF,				SIGNAL(triggered()),	this,		SLOT(doActionAddConstantSF()));
	connect(actionScalarFieldArithmetic,		SIGNAL(triggered()),	this,		SLOT(doActionScalarFieldArithmetic()));
	connect(actionScalarFieldFromColor,			SIGNAL(triggered()),	this,		SLOT(doActionScalarFieldFromColor()));
	connect(actionConvertToRGB,					SIGNAL(triggered()),	this,		SLOT(doActionSFConvertToRGB()));
	connect(actionRenameSF,						SIGNAL(triggered()),	this,		SLOT(doActionRenameSF()));
	connect(actionOpenColorScalesManager,		SIGNAL(triggered()),	this,		SLOT(doActionOpenColorScalesManager()));
	connect(actionAddIdField,					SIGNAL(triggered()),	this,		SLOT(doActionAddIdField()));
	connect(actionSetSFAsCoord,					SIGNAL(triggered()),	this,		SLOT(doActionSetSFAsCoord()));
	connect(actionDeleteScalarField,			SIGNAL(triggered()),	this,		SLOT(doActionDeleteScalarField()));
	connect(actionDeleteAllSF,					SIGNAL(triggered()),	this,		SLOT(doActionDeleteAllSF()));
	//"Edit" menu
	connect(actionClone,						SIGNAL(triggered()),	this,		SLOT(doActionClone()));
	connect(actionMerge,						SIGNAL(triggered()),	this,		SLOT(doActionMerge()));
	connect(actionApplyTransformation,			SIGNAL(triggered()),	this,		SLOT(doActionApplyTransformation()));
	connect(actionApplyScale,					SIGNAL(triggered()),	this,		SLOT(doActionApplyScale()));
	connect(actionTranslateRotate,				SIGNAL(triggered()),	this,		SLOT(activateTranslateRotateMode()));
	connect(actionSegment,						SIGNAL(triggered()),	this,		SLOT(activateSegmentationMode()));
	connect(actionCrop,							SIGNAL(triggered()),	this,		SLOT(doActionCrop()));
	connect(actionEditGlobalShift,				SIGNAL(triggered()),	this,		SLOT(doActionEditGlobalShift()));
	connect(actionEditGlobalScale,				SIGNAL(triggered()),	this,		SLOT(doActionEditGlobalScale()));
	connect(actionSubsample,					SIGNAL(triggered()),	this,		SLOT(doActionSubsample()));
	connect(actionMatchBBCenters,				SIGNAL(triggered()),	this,		SLOT(doActionMatchBBCenters()));
	connect(actionDelete,						SIGNAL(triggered()),	m_ccRoot,	SLOT(deleteSelectedEntities()));

	//"Tools > Projection" menu
	connect(actionUnroll,						SIGNAL(triggered()),	this,		SLOT(doActionUnroll()));
	connect(actionHeightGridGeneration,			SIGNAL(triggered()),	this,		SLOT(doActionHeightGridGeneration()));
	connect(actionExportCoordToSF,				SIGNAL(triggered()),	this,		SLOT(doActionExportCoordToSF()));
	//"Tools > Registration" menu
	connect(actionRegister,						SIGNAL(triggered()),	this,		SLOT(doActionRegister()));
	connect(actionPointPairsAlign,				SIGNAL(triggered()),	this,		SLOT(activateRegisterPointPairTool()));
	//"Tools > Distances" menu
	connect(actionCloudCloudDist,				SIGNAL(triggered()),	this,		SLOT(doActionCloudCloudDist()));
	connect(actionCloudMeshDist,				SIGNAL(triggered()),	this,		SLOT(doActionCloudMeshDist()));
	connect(actionCPS,							SIGNAL(triggered()),	this,		SLOT(doActionComputeCPS()));
	//"Tools > Statistics" menu
	connect(actionComputeStatParams,			SIGNAL(triggered()),	this,		SLOT(doActionComputeStatParams()));
	connect(actionStatisticalTest,				SIGNAL(triggered()),	this,		SLOT(doActionStatisticalTest()));
	//"Tools > Segmentation" menu
	connect(actionLabelConnectedComponents,		SIGNAL(triggered()),	this,		SLOT(doActionLabelConnectedComponents()));
	connect(actionKMeans,						SIGNAL(triggered()),	this,		SLOT(doActionKMeans()));
	connect(actionFrontPropagation,				SIGNAL(triggered()),	this,		SLOT(doActionFrontPropagation()));
	connect(actionCrossSection,					SIGNAL(triggered()),	this,		SLOT(activateClippingBoxMode()));
	//"Tools > Fit" menu
	connect(actionFitPlane,						SIGNAL(triggered()),	this,		SLOT(doActionFitPlane()));
	connect(actionFitFacet,						SIGNAL(triggered()),	this,		SLOT(doActionFitFacet()));
	connect(actionFitQuadric,					SIGNAL(triggered()),	this,		SLOT(doActionFitQuadric()));
	//"Tools > Other" menu
	connect(actionApproximateDensity,			SIGNAL(triggered()),	this,		SLOT(doComputeApproximateDensity()));
	connect(actionAccurateDensity,				SIGNAL(triggered()),	this,		SLOT(doComputeAccurateDensity()));
	connect(actionCurvature,					SIGNAL(triggered()),	this,		SLOT(doComputeCurvature()));
	connect(actionRoughness,					SIGNAL(triggered()),	this,		SLOT(doComputeRoughness()));
	connect(actionRemoveDuplicatePoints,		SIGNAL(triggered()),	this,		SLOT(doRemoveDuplicatePoints()));
	//"Tools"
	connect(actionPointListPicking,				SIGNAL(triggered()),	this,		SLOT(activatePointListPickingMode()));
	connect(actionPointPicking,					SIGNAL(triggered()),	this,		SLOT(activatePointPickingMode()));

	//"Tools > Sand box (research)" menu
	connect(actionComputeKdTree,				SIGNAL(triggered()),	this,		SLOT(doActionComputeKdTree()));
	connect(actionDistanceToBestFitQuadric3D,	SIGNAL(triggered()),	this,		SLOT(doActionComputeDistToBestFitQuadric3D()));
	connect(actionComputeBestFitBB,				SIGNAL(triggered()),	this,		SLOT(doComputeBestFitBB()));
	connect(actionAlign,						SIGNAL(triggered()),	this,		SLOT(doAction4pcsRegister())); //Aurelien BEY le 13/11/2008
	connect(actionSNETest,						SIGNAL(triggered()),	this,		SLOT(doSphericalNeighbourhoodExtractionTest()));
	connect(actionCNETest,						SIGNAL(triggered()),	this,		SLOT(doCylindricalNeighbourhoodExtractionTest()));
	connect(actionFindBiggestInnerRectangle,	SIGNAL(triggered()),	this,		SLOT(doActionFindBiggestInnerRectangle()));
	connect(actionExportCloudsInfo,				SIGNAL(triggered()),	this,		SLOT(doActionExportCloudsInfo()));

	//"Display" menu
	connect(actionFullScreen,					SIGNAL(toggled(bool)),	this,		SLOT(toggleFullScreen(bool)));
	connect(actionRefresh,						SIGNAL(triggered()),	this,		SLOT(refreshAll()));
	connect(actionTestFrameRate,				SIGNAL(triggered()),	this,		SLOT(testFrameRate()));
	connect(actionToggleCenteredPerspective,	SIGNAL(triggered()),	this,		SLOT(toggleActiveWindowCenteredPerspective()));
	connect(actionToggleViewerBasedPerspective, SIGNAL(triggered()),	this,		SLOT(toggleActiveWindowViewerBasedPerspective()));
	connect(actionEditCamera,					SIGNAL(triggered()),	this,		SLOT(doActionEditCamera()));
	connect(actionAdjustZoom,					SIGNAL(triggered()),	this,		SLOT(doActionAdjustZoom()));
	connect(actionSaveViewportAsObject,			SIGNAL(triggered()),	this,		SLOT(doActionSaveViewportAsCamera()));

	//"Display > Lights & Materials" menu
	connect(actionDisplayOptions,				SIGNAL(triggered()),	this,		SLOT(setLightsAndMaterials()));
	connect(actionToggleSunLight,				SIGNAL(triggered()),	this,		SLOT(toggleActiveWindowSunLight()));
	connect(actionToggleCustomLight,			SIGNAL(triggered()),	this,		SLOT(toggleActiveWindowCustomLight()));
	connect(actionRenderToFile,					SIGNAL(triggered()),	this,		SLOT(doActionRenderToFile()));
	//"Display > Shaders & filters" menu
	connect(actionLoadShader,					SIGNAL(triggered()),	this,		SLOT(doActionLoadShader()));
	connect(actionDeleteShader,					SIGNAL(triggered()),	this,		SLOT(doActionDeleteShader()));
	connect(actionNoFilter,						SIGNAL(triggered()),	this,		SLOT(doDisableGLFilter()));

	//"Display > Active SF" menu
	connect(actionToggleActiveSFColorScale,		SIGNAL(triggered()),	this,		SLOT(doActionToggleActiveSFColorScale()));
	connect(actionShowActiveSFPrevious,			SIGNAL(triggered()),	this,		SLOT(doActionShowActiveSFPrevious()));
	connect(actionShowActiveSFNext,				SIGNAL(triggered()),	this,		SLOT(doActionShowActiveSFNext()));

	//"3D Views" menu
	connect(menu3DViews,						SIGNAL(aboutToShow()),	this,		SLOT(update3DViewsMenu()));
	connect(actionNew3DView,					SIGNAL(triggered()),	this,		SLOT(new3DView()));
	connect(actionClose3DView,					SIGNAL(triggered()),	m_mdiArea,	SLOT(closeActiveSubWindow()));
	connect(actionCloseAll3DViews,				SIGNAL(triggered()),	m_mdiArea,	SLOT(closeAllSubWindows()));
	connect(actionTile3DViews,					SIGNAL(triggered()),	m_mdiArea,	SLOT(tileSubWindows()));
	connect(actionCascade3DViews,				SIGNAL(triggered()),	m_mdiArea,	SLOT(cascadeSubWindows()));
	connect(actionNext3DView,					SIGNAL(triggered()),	m_mdiArea,	SLOT(activateNextSubWindow()));
	connect(actionPrevious3DView,				SIGNAL(triggered()),	m_mdiArea,	SLOT(activatePreviousSubWindow()));

	//"About" menu entry
	connect(actionHelp,							SIGNAL(triggered()),	this,		SLOT(help()));
	connect(actionAbout,						SIGNAL(triggered()),	this,		SLOT(about()));
	connect(actionAboutPlugins,					SIGNAL(triggered()),	this,		SLOT(aboutPlugins()));

	/*** Toolbars ***/

	//View toolbar
	connect(actionGlobalZoom,					SIGNAL(triggered()),	this,		SLOT(setGlobalZoom()));
	connect(actionPickRotationCenter,			SIGNAL(triggered()),	this,		SLOT(doPickRotationCenter()));
	connect(actionZoomAndCenter,				SIGNAL(triggered()),	this,		SLOT(zoomOnSelectedEntities()));
	connect(actionSetPivotAlwaysOn,				SIGNAL(triggered()),	this,		SLOT(setPivotAlwaysOn()));
	connect(actionSetPivotRotationOnly,			SIGNAL(triggered()),	this,		SLOT(setPivotRotationOnly()));
	connect(actionSetPivotOff,					SIGNAL(triggered()),	this,		SLOT(setPivotOff()));
	connect(actionSetOrthoView,					SIGNAL(triggered()),	this,		SLOT(setOrthoView()));
	connect(actionSetCenteredPerspectiveView,	SIGNAL(triggered()),	this,		SLOT(setCenteredPerspectiveView()));
	connect(actionSetViewerPerspectiveView,		SIGNAL(triggered()),	this,		SLOT(setViewerPerspectiveView()));
	connect(actionSetViewTop,					SIGNAL(triggered()),	this,		SLOT(setTopView()));
	connect(actionSetViewBottom,				SIGNAL(triggered()),	this,		SLOT(setBottomView()));
	connect(actionSetViewFront,					SIGNAL(triggered()),	this,		SLOT(setFrontView()));
	connect(actionSetViewBack,					SIGNAL(triggered()),	this,		SLOT(setBackView()));
	connect(actionSetViewLeft,					SIGNAL(triggered()),	this,		SLOT(setLeftView()));
	connect(actionSetViewRight,					SIGNAL(triggered()),	this,		SLOT(setRightView()));
	connect(actionSetViewIso1,					SIGNAL(triggered()),	this,		SLOT(setIsoView1()));
	connect(actionSetViewIso2,					SIGNAL(triggered()),	this,		SLOT(setIsoView2()));
}

void MainWindow::doActionColorize()
{
	doActionSetColor(true);
}

void MainWindow::doActionSetUniqueColor()
{
	doActionSetColor(false);
}

void MainWindow::doActionSetColor(bool colorize)
{
	//ask for color choice
	QColor newCol = QColorDialog::getColor(Qt::white, this);

	if (!newCol.isValid())
		return;

	ccHObject::Container selectedEntities = m_selectedEntities;
	while (!selectedEntities.empty())
	{
		ccHObject* ent = selectedEntities.back();
		selectedEntities.pop_back();
		if (ent->isA(CC_TYPES::HIERARCHY_OBJECT))
		{
			//automatically parse a group's children set
			for (unsigned i=0; i<ent->getChildrenNumber(); ++i)
				selectedEntities.push_back(ent->getChild(i));
		}
		else if (ent->isA(CC_TYPES::POINT_CLOUD) || ent->isA(CC_TYPES::MESH))
		{
			ccPointCloud* cloud = 0;
			if (ent->isA(CC_TYPES::POINT_CLOUD))
			{
				cloud = static_cast<ccPointCloud*>(ent);
			}
			else
			{
				ccMesh* mesh = static_cast<ccMesh*>(ent);
				ccGenericPointCloud* vertices = mesh->getAssociatedCloud();
				if (	!vertices
					||	!vertices->isA(CC_TYPES::POINT_CLOUD)
					||	(vertices->isLocked() && !mesh->isAncestorOf(vertices)) )
				{
					ccLog::Warning(QString("[SetColor] Can't set color for mesh '%1' (vertices are not accessible)").arg(ent->getName()));
					continue;
				}

				cloud = static_cast<ccPointCloud*>(vertices);
			}

			if (colorize)
			{
				cloud->colorize(static_cast<float>(newCol.redF()),
								static_cast<float>(newCol.greenF()),
								static_cast<float>(newCol.blueF()) );
			}
			else
			{
				cloud->setRGBColor(	static_cast<colorType>(newCol.red()),
									static_cast<colorType>(newCol.green()),
									static_cast<colorType>(newCol.blue()) );
			}
			cloud->showColors(true);
			cloud->prepareDisplayForRefresh();

			if (ent != cloud)
				ent->showColors(true);
			else if (cloud->getParent() && cloud->getParent()->isKindOf(CC_TYPES::MESH))
				cloud->getParent()->showColors(true);
		}
		else if (ent->isA(CC_TYPES::POLY_LINE))
		{
			ccPolyline * poly = ccHObjectCaster::ToPolyline(ent);
			colorType col[3] = {static_cast<colorType>(newCol.red()),
								static_cast<colorType>(newCol.green()),
								static_cast<colorType>(newCol.blue()) };
			poly->setColor(col);
			ent->showColors(true);
			ent->prepareDisplayForRefresh();
		}
		else if (ent->isA(CC_TYPES::FACET))
		{
			ccFacet* facet = ccHObjectCaster::ToFacet(ent);
			colorType col[3] = {static_cast<colorType>(newCol.red()),
								static_cast<colorType>(newCol.green()),
								static_cast<colorType>(newCol.blue()) };
			facet->setColor(col);
			ent->showColors(true);
			ent->prepareDisplayForRefresh();
		}
		else
		{
			ccLog::Warning(QString("[SetColor] Can't change color of entity '%1'").arg(ent->getName()));
		}
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionSetColorGradient()
{
	ccColorGradientDlg dlg(this);
	if (!dlg.exec())
		return;

	unsigned char dim = dlg.getDimension();
	ccColorGradientDlg::GradientType ramp = dlg.getType();

	ccColorScale::Shared colorScale(0);
	if (ramp == ccColorGradientDlg::Default)
	{
		colorScale = ccColorScalesManager::GetDefaultScale();
	}
	else if (ramp == ccColorGradientDlg::TwoColors)
	{
		colorScale = ccColorScale::Create("Temp scale");
		QColor first,second;
		dlg.getColors(first,second);
		colorScale->insert(ccColorScaleElement(0.0,first),false);
		colorScale->insert(ccColorScaleElement(1.0,second),true);
	}
	assert(colorScale || ramp == ccColorGradientDlg::Banding);

	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = m_selectedEntities[i];

		bool lockedVertices;
		ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent,&lockedVertices);
		if (lockedVertices)
		{
			DisplayLockedVerticesWarning();
			continue;
		}

		if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD)) // TODO
		{
			ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);

			bool success = false;
			if (ramp == ccColorGradientDlg::Banding)
				success = pc->setRGBColorByBanding(dim, dlg.getBandingFrequency());
			else
				success = pc->setRGBColorByHeight(dim, colorScale);

			if (success)
			{
				ent->showColors(true);
				ent->prepareDisplayForRefresh();
			}
		}
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionChangeColorLevels()
{
	if (m_selectedEntities.size() != 1)
	{
		ccConsole::Error("Select one and only one colored cloud or mesh!");
		return;
	}

	bool lockedVertices;
	ccPointCloud* pointCloud = ccHObjectCaster::ToPointCloud(m_selectedEntities[0],&lockedVertices);
	if (!pointCloud || lockedVertices)
	{
		if (lockedVertices)
			DisplayLockedVerticesWarning();
		return;
	}

	if (!pointCloud->hasColors())
	{
		ccConsole::Error("Selected entity has no colors!");
		return;
	}

	ccColorLevelsDlg dlg(this,pointCloud);
	dlg.exec();
}

void MainWindow::doActionInterpolateColors()
{
	if (m_selectedEntities.size() != 2)
	{
		ccConsole::Error("Select 2 entities (clouds or meshes)!");
		return;
	}

	ccHObject* ent1 = m_selectedEntities[0];
	ccHObject* ent2 = m_selectedEntities[1];

	ccGenericPointCloud* cloud1 = ccHObjectCaster::ToGenericPointCloud(ent1);
	ccGenericPointCloud* cloud2 = ccHObjectCaster::ToGenericPointCloud(ent2);

	if (!cloud1 || !cloud2)
	{
		ccConsole::Error("Select 2 entities (clouds or meshes)!");
		return;
	}

	if (!cloud1->hasColors() && !cloud2->hasColors())
	{
		ccConsole::Error("None of the selected entities has per-point or per-vertex colors!");
		return;
	}
	else if (cloud1->hasColors() && cloud2->hasColors())
	{
		ccConsole::Error("Both entities have colors! Remove the colors on the entity you wish to import the colors to!");
		return;
	}

	ccGenericPointCloud* source = cloud1;
	ccGenericPointCloud* dest = cloud2;

	if ( cloud2->hasColors())
	{
		std::swap(source,dest);
		std::swap(cloud1,cloud2);
		std::swap(ent1,ent2);
	}

	if (!dest->isA(CC_TYPES::POINT_CLOUD))
	{
		ccConsole::Error("Destination cloud (or vertices) must be a real point cloud!");
		return;
	}

	ccProgressDialog pDlg(true, this);
	if (static_cast<ccPointCloud*>(dest)->interpolateColorsFrom(source,&pDlg))
	{
		ent2->showColors(true);
	}
	else
	{
		ccConsole::Error("An error occurred! (see console)");
	}

	ent2->prepareDisplayForRefresh_recursive();
	refreshAll();
	updateUI();
}

void MainWindow::doActionInvertNormals()
{
	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = m_selectedEntities[i];
		bool lockedVertices;
		ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent,&lockedVertices);
		if (lockedVertices)
		{
			DisplayLockedVerticesWarning();
			continue;
		}

		if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD)) // TODO
		{
			ccPointCloud* ccCloud = static_cast<ccPointCloud*>(cloud);
			if (ccCloud->hasNormals())
			{
				ccCloud->invertNormals();
				ccCloud->showNormals(true);
				ccCloud->prepareDisplayForRefresh_recursive();
			}
		}
	}

	refreshAll();
}

void MainWindow::doActionConvertNormalsToDipDir()
{
	doActionConvertNormalsTo(DIP_DIR_SFS);
}

void MainWindow::doActionConvertNormalsToHSV()
{
	doActionConvertNormalsTo(HSV_COLORS);
}

void MainWindow::doActionConvertNormalsTo(NORMAL_CONVERSION_DEST dest)
{
	unsigned errorCount = 0;

	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = m_selectedEntities[i];
		bool lockedVertices;
		ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent,&lockedVertices);
		if (lockedVertices)
		{
			DisplayLockedVerticesWarning();
			continue;
		}

		if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD)) // TODO
		{
			ccPointCloud* ccCloud = static_cast<ccPointCloud*>(cloud);
			if (ccCloud->hasNormals())
			{
				bool success = true;
				switch(dest)
				{
				case HSV_COLORS:
					{
						success = ccCloud->convertNormalToRGB();
						if (success)
						{
							ccCloud->showSF(false);
							ccCloud->showNormals(false);
							ccCloud->showColors(true);
						}
					}
					break;
				case DIP_DIR_SFS:
					{
						//get/create 'dip' scalar field
						int dipSFIndex = ccCloud->getScalarFieldIndexByName(CC_DEFAULT_DIP_SF_NAME);
						if (dipSFIndex < 0)
							dipSFIndex = ccCloud->addScalarField(CC_DEFAULT_DIP_SF_NAME);
						if (dipSFIndex < 0)
						{
							ccLog::Warning("[MainWindow::doActionConvertNormalsTo] Not enough memory!");
							success = false;
							break;
						}

						//get/create 'dip direction' scalar field
						int dipDirSFIndex = ccCloud->getScalarFieldIndexByName(CC_DEFAULT_DIP_DIR_SF_NAME);
						if (dipDirSFIndex < 0)
							dipDirSFIndex = ccCloud->addScalarField(CC_DEFAULT_DIP_DIR_SF_NAME);
						if (dipDirSFIndex < 0)
						{
							ccCloud->deleteScalarField(dipSFIndex);
							ccLog::Warning("[MainWindow::doActionConvertNormalsTo] Not enough memory!");
							success = false;
							break;
						}

						ccScalarField* dipSF = static_cast<ccScalarField*>(ccCloud->getScalarField(dipSFIndex));
						ccScalarField* dipDirSF = static_cast<ccScalarField*>(ccCloud->getScalarField(dipDirSFIndex));
						assert(dipSF && dipDirSF);

						success = ccCloud->convertNormalToDipDirSFs(dipSF, dipDirSF);

						if (success)
						{
							//apply default 360 degrees color scale!
							ccColorScale::Shared scale = ccColorScalesManager::GetDefaultScale(ccColorScalesManager::HSV_360_DEG);
							dipSF->setColorScale(scale);
							dipDirSF->setColorScale(scale);
							ccCloud->setCurrentDisplayedScalarField(dipDirSFIndex); //dip dir. seems more interesting by default
							ccCloud->showSF(true);
						}
						else
						{
							ccCloud->deleteScalarField(dipSFIndex);
							ccCloud->deleteScalarField(dipDirSFIndex);
						}
					}
					break;
				default:
					assert(false);
					ccLog::Warning("[MainWindow::doActionConvertNormalsTo] Internal error: unhandled destination!");
					success = false;
					i = selNum; //no need to process the selected entities anymore!
					break;
				}

				if (success)
				{
					ccCloud->prepareDisplayForRefresh_recursive();
				}
				else
				{
					++errorCount;
				}
			}
		}
	}

	//errors should have been sent to console as warnings
	if (errorCount)
	{
		ccConsole::Error("Error(s) occurred! (see console)");
	}

	refreshAll();
	updateUI();
}

static double s_kdTreeMaxErrorPerCell = 0.1;
void MainWindow::doActionComputeKdTree()
{
	ccGenericPointCloud* cloud = 0;

	if (m_selectedEntities.size() == 1)
	{
		ccHObject* ent = m_selectedEntities.back();
		bool lockedVertices;
		cloud = ccHObjectCaster::ToGenericPointCloud(ent,&lockedVertices);
		if (lockedVertices)
		{
			DisplayLockedVerticesWarning();
			return;
		}
	}

	if (!cloud)
	{
		ccLog::Error("Selected one and only one point cloud or mesh!");
		return;
	}

	bool ok;
	s_kdTreeMaxErrorPerCell = QInputDialog::getDouble(this, "Compute Kd-tree", "Max error per leaf cell:", s_kdTreeMaxErrorPerCell, 1.0e-6, 1.0e6, 6, &ok);
	if (!ok)
		return;

	ccProgressDialog pDlg(true,this);

	//computation
	QElapsedTimer eTimer;
	eTimer.start();
	ccKdTree* kdtree = new ccKdTree(cloud);

	if (kdtree->build(s_kdTreeMaxErrorPerCell,CCLib::DistanceComputationTools::MAX_DIST_95_PERCENT,1000,&pDlg))
	{
		qint64 elapsedTime_ms = eTimer.elapsed();

		ccConsole::Print("[doActionComputeKdTree] Timing: %2.3f s",static_cast<double>(elapsedTime_ms)/1.0e3);
		cloud->setEnabled(true); //for mesh vertices!
		cloud->addChild(kdtree);
		kdtree->setDisplay(cloud->getDisplay());
		kdtree->setVisible(true);
		kdtree->prepareDisplayForRefresh();
#ifdef _DEBUG
		kdtree->convertCellIndexToSF();
#else
		kdtree->convertCellIndexToRandomColor();
#endif

		addToDB(kdtree);

		refreshAll();
		updateUI();
	}
	else
	{
		ccLog::Error("An error occurred!");
		delete kdtree;
		kdtree = 0;
	}
}

void MainWindow::doActionComputeOctree()
{
	ccBBox bbox;
	std::set<ccGenericPointCloud*> clouds;
	size_t selNum = m_selectedEntities.size();
	PointCoordinateType maxBoxSize = -1;
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = m_selectedEntities[i];

		//specific test for locked vertices
		bool lockedVertices;
		ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent,&lockedVertices);
		if (cloud && lockedVertices)
		{
			DisplayLockedVerticesWarning();
			continue;
		}
		clouds.insert(cloud);

		//we look for the biggest box so as to define the "minimum cell size"
		ccBBox thisBBox = cloud->getMyOwnBB();
		if (thisBBox.isValid())
		{
			CCVector3 dd = thisBBox.maxCorner()-thisBBox.minCorner();
			PointCoordinateType maxd = std::max(dd.x,std::max(dd.y,dd.z));
			if (maxBoxSize < 0.0 || maxd > maxBoxSize)
				maxBoxSize = maxd;
		}

		bbox += cloud->getBB();
	}

	if (clouds.empty() || maxBoxSize < 0.0)
	{
		ccLog::Warning("[doActionComputeOctree] No elligible entities in selection!");
		return;
	}

	//min(cellSize) = max(dim)/2^N with N = max subidivision level
	double minCellSize = static_cast<double>(maxBoxSize)/(1 << ccOctree::MAX_OCTREE_LEVEL);

	ccComputeOctreeDlg coDlg(bbox,minCellSize,this);
	if (!coDlg.exec())
		return;

	ccProgressDialog pDlg(true,this);

	//if we must use a custom bounding box, we update 'bbox'
	if (coDlg.getMode() == ccComputeOctreeDlg::CUSTOM_BBOX)
		bbox = coDlg.getCustomBBox();

	for (std::set<ccGenericPointCloud*>::iterator it = clouds.begin(); it != clouds.end(); ++it)
	{
		ccGenericPointCloud* cloud = *it;

		//we temporarily detach entity, as it may undergo
		//"severe" modifications (octree deletion, etc.) --> see ccPointCloud::computeOctree
		ccHObjectContext objContext = removeObjectTemporarilyFromDBTree(cloud);

		//computation
		QElapsedTimer eTimer;
		eTimer.start();
		ccOctree* octree = 0;
		switch(coDlg.getMode())
		{
		case ccComputeOctreeDlg::DEFAULT:
			octree = cloud->computeOctree(&pDlg);
			break;
		case ccComputeOctreeDlg::MIN_CELL_SIZE:
		case ccComputeOctreeDlg::CUSTOM_BBOX:
			{
				//for a cell-size based custom box, we must update it for each cloud!
				if (coDlg.getMode() == ccComputeOctreeDlg::MIN_CELL_SIZE)
				{
					double cellSize = coDlg.getMinCellSize();
					PointCoordinateType halfBoxWidth = (PointCoordinateType)(cellSize * (1 << ccOctree::MAX_OCTREE_LEVEL) / 2.0);
					CCVector3 C = cloud->getBB().getCenter();
					bbox = ccBBox(	C-CCVector3(halfBoxWidth,halfBoxWidth,halfBoxWidth),
									C+CCVector3(halfBoxWidth,halfBoxWidth,halfBoxWidth));
				}
				cloud->deleteOctree();
				octree = new ccOctree(cloud);
				if (octree->build(bbox.minCorner(),bbox.maxCorner(),0,0,&pDlg)>0)
				{
					octree->setDisplay(cloud->getDisplay());
					cloud->addChild(octree);
				}
				else
				{
					delete octree;
					octree = 0;
				}
			}
			break;
		default:
			assert(false);
			return;
		}
		qint64 elapsedTime_ms = eTimer.elapsed();

		//put object back in tree
		putObjectBackIntoDBTree(cloud,objContext);

		if (octree)
		{
			ccConsole::Print("[doActionComputeOctree] Timing: %2.3f s",static_cast<double>(elapsedTime_ms)/1.0e3);
			cloud->setEnabled(true); //for mesh vertices!
			octree->setVisible(true);
			octree->prepareDisplayForRefresh();
		}
		else
		{
			ccConsole::Warning(QString("Octree computation on cloud '%1' failed!").arg(cloud->getName()));
		}
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionResampleWithOctree()
{
	bool ok;
	int pointCount = QInputDialog::getInt(this,"Resample with octree", "Points (approx.)", 1000000, 1, INT_MAX, 100000, &ok);
	if (!ok)
		return;

	ccProgressDialog pDlg(false,this);
	assert(pointCount > 0);
	unsigned aimedPoints = static_cast<unsigned>(pointCount);

	ccHObject::Container selectedEntities = m_selectedEntities;
	size_t selNum = selectedEntities.size();
	bool errors = false;
	for (size_t i=0; i<selNum; ++i)
	{
		ccPointCloud* cloud = 0;
		ccHObject* ent = selectedEntities[i];
		/*if (ent->isKindOf(CC_TYPES::MESH)) //TODO
			cloud = ccHObjectCaster::ToGenericMesh(ent)->getAssociatedCloud();
		else */
		if (ent->isKindOf(CC_TYPES::POINT_CLOUD))
			cloud = static_cast<ccPointCloud*>(ent);

		if (cloud)
		{
			ccOctree* octree = cloud->getOctree();
			if (!octree)
			{
				octree = cloud->computeOctree(&pDlg);
				if (!octree)
				{
					ccConsole::Error(QString("Could not compute octree for cloud '%1'").arg(cloud->getName()));
					continue;
				}
			}

			cloud->setEnabled(false);
			QElapsedTimer eTimer;
			eTimer.start();
			CCLib::GenericIndexedCloud* result = CCLib::CloudSamplingTools::resampleCloudWithOctree(cloud,
				aimedPoints,
				CCLib::CloudSamplingTools::CELL_GRAVITY_CENTER,
				&pDlg,
				octree);

			if (result)
			{
				ccConsole::Print("[ResampleWithOctree] Timing: %3.2f s.",eTimer.elapsed()/1.0e3);
				ccPointCloud* newCloud = ccPointCloud::From(result);

				delete result;
				result = 0;

				if (newCloud)
				{
					newCloud->setGlobalShift(cloud->getGlobalShift());
					newCloud->setGlobalScale(cloud->getGlobalScale());
					addToDB(newCloud);
					newCloud->setDisplay(cloud->getDisplay());
					newCloud->prepareDisplayForRefresh();
				}
				else
				{
					errors = true;
				}
			}
		}
	}

	if (errors)
		ccLog::Error("[ResampleWithOctree] Errors occurred during the process! Result may be incomplete!");

	refreshAll();
}

void MainWindow::doActionApplyTransformation()
{
	ccApplyTransformationDlg dlg(this);
	if (!dlg.exec())
		return;

	ccGLMatrix transMat = dlg.getTransformation();
	CCVector3 T = transMat.getTranslationAsVec3D();

	//test if translation is very big
	const double maxCoord = ccCoordinatesShiftManager::MaxCoordinateAbsValue();
	bool coordsAreTooBig = (	fabs(T.x) > maxCoord
							||	fabs(T.y) > maxCoord
							||	fabs(T.z) > maxCoord );
	bool applyTranslationAsShift = false;

	//we must backup 'm_selectedEntities' as removeObjectTemporarilyFromDBTree can modify it!
	ccHObject::Container selectedEntities = m_selectedEntities;
	size_t selNum = selectedEntities.size();
	bool firstCloud = true;
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = selectedEntities[i];

		//specific test for locked vertices
		bool lockedVertices;
		ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent,&lockedVertices);
		if (cloud && lockedVertices)
		{
			DisplayLockedVerticesWarning();
			continue;
		}

		if (firstCloud)
		{
			if (coordsAreTooBig)
			{
				//if the translation is big, we must check that it's actually worsening the situation
				//(and not improving it - in which case we shouldn't rant ;)
				CCVector3 C = cloud->getBBCenter();
				CCVector3 C2 = C;
				transMat.apply(C2);

				if (C2.norm() > C.norm())
				{
					//TODO: what about the scale?
					applyTranslationAsShift = (QMessageBox::question(	this,
												"Big coordinates",
												"Translation is too big (original precision may be lost!). Do you wish to save it as 'global shift' instead?\n(global shift will only be applied at export time)",
												QMessageBox::Yes,
												QMessageBox::No) == QMessageBox::Yes);
					if (applyTranslationAsShift)
					{
						//clear transformation translation
						transMat.setTranslation(CCVector3(0,0,0));
					}
				}
			}

			firstCloud = false;
		}

		if (applyTranslationAsShift)
		{
			//apply translation as global shift
			cloud->setGlobalShift(cloud->getGlobalShift() - CCVector3d::fromArray(T.u));
		}

		//we temporarily detach entity, as it may undergo
		//"severe" modifications (octree deletion, etc.) --> see ccHObject::applyRigidTransformation
		ccHObjectContext objContext = removeObjectTemporarilyFromDBTree(ent);
		ent->setGLTransformation(transMat);
		ent->applyGLTransformation_recursive();
		ent->prepareDisplayForRefresh_recursive();
		putObjectBackIntoDBTree(ent,objContext);
	}

	refreshAll();
}

static double s_lastMultFactorX = 1.0;
static double s_lastMultFactorY = 1.0;
static double s_lastMultFactorZ = 1.0;
void MainWindow::doActionApplyScale()
{
	ccAskThreeDoubleValuesDlg dlg("fx","fy","fz",-1.0e6,1.0e6,s_lastMultFactorX,s_lastMultFactorY,s_lastMultFactorZ,8,"Scaling",this);
	if (!dlg.exec())
		return;

	//save values for next time
	double sX = s_lastMultFactorX = dlg.doubleSpinBox1->value();
	double sY = s_lastMultFactorY = dlg.doubleSpinBox2->value();
	double sZ = s_lastMultFactorZ = dlg.doubleSpinBox3->value();

	//we must backup 'm_selectedEntities' as removeObjectTemporarilyFromDBTree can modify it!
	ccHObject::Container selectedEntities = m_selectedEntities;
	size_t selNum = selectedEntities.size();
	size_t processNum = 0;
	bool firstCloud = true;
	bool applyScaleAsShift = false;

	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = selectedEntities[i];
		bool lockedVertices;
		ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent,&lockedVertices);
		if (lockedVertices)
		{
			DisplayLockedVerticesWarning();
			++processNum;
			continue;
		}

		if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD)) //TODO
		{

			if (firstCloud)
			{
				if (sX == sY && sX == sZ) //the following test only works for an 'isotropic' scale!
				{
					//we must check that the resulting cloud is not too big
					ccBBox bbox = cloud->getBB();
					double maxx = std::max(fabs(bbox.minCorner().x), fabs(bbox.maxCorner().x)) * sX;
					double maxy = std::max(fabs(bbox.minCorner().y), fabs(bbox.maxCorner().y)) * sY;
					double maxz = std::max(fabs(bbox.minCorner().z), fabs(bbox.maxCorner().z)) * sZ;

					const double maxCoord = ccCoordinatesShiftManager::MaxCoordinateAbsValue();
					bool coordsWereTooBig = (	maxx > maxCoord
											||	maxy > maxCoord
											||	maxz > maxCoord );
					bool coordsAreTooBig = (	maxx*sX > maxCoord
											||	maxy*sY > maxCoord
											||	maxz*sZ > maxCoord );

					if (!coordsWereTooBig && coordsAreTooBig)
					{
						applyScaleAsShift = (QMessageBox::question(	this,
							"Big coordinates",
							"Scale is too big (original precision may be lost!). Do you wish to save it as 'global scale' instead?\n(global scale will only be applied at export time)",
							QMessageBox::Yes,
							QMessageBox::No) == QMessageBox::Yes);
					}
				}

				firstCloud = false;
			}

			if (applyScaleAsShift)
			{
				assert(sX == sY && sX == sZ);
				const double& scale = cloud->getGlobalScale();
				cloud->setGlobalScale(scale*sX);
			}
			else
			{
				//we temporarily detach entity, as it may undergo
				//"severe" modifications (octree deletion, etc.) --> see ccPointCloud::multiply
				ccHObjectContext objContext = removeObjectTemporarilyFromDBTree(cloud);
				static_cast<ccPointCloud*>(cloud)->multiply(static_cast<PointCoordinateType>(sX),
															static_cast<PointCoordinateType>(sY),
															static_cast<PointCoordinateType>(sZ));
				putObjectBackIntoDBTree(cloud,objContext);
				cloud->prepareDisplayForRefresh_recursive();

				//don't forget the 'global shift'!
				const CCVector3d& shift = cloud->getGlobalShift();
				cloud->setGlobalShift( CCVector3d(	shift.x*sX,
													shift.y*sY,
													shift.z*sZ) );
				//DGM: nope! Not the global scale!
				//const double& scale = cloud->getGlobalScale();
				//cloud->setGlobalScale(scale*s_lastMultFactorX);
			}

			++processNum;
		}
	}

	if (processNum == 0)
	{
		ccConsole::Warning("No elligible entities (point clouds or meshes) were selected!");
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionEditGlobalShift()
{
	size_t selNum = m_selectedEntities.size();
	if (selNum != 1)
	{
		if (selNum > 1)
			ccConsole::Error("Select only one point cloud or mesh!");
		return;
	}
	ccHObject* ent = m_selectedEntities[0];

	bool lockedVertices;
	ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent,&lockedVertices);
	//for point clouds only
	if (!cloud)
		return;
	if (lockedVertices && !ent->isAncestorOf(cloud))
	{
		//see ccPropertiesTreeDelegate::fillWithMesh
		DisplayLockedVerticesWarning();
		return;
	}

	assert(cloud);
	const CCVector3d& shift = cloud->getGlobalShift();

	ccAskThreeDoubleValuesDlg dlg("x","y","z",-DBL_MAX,DBL_MAX,shift.x,shift.y,shift.z,2,"Global shift",this);
	if (!dlg.exec())
		return;

	double x = dlg.doubleSpinBox1->value();
	double y = dlg.doubleSpinBox2->value();
	double z = dlg.doubleSpinBox3->value();

	//apply new shift
	cloud->setGlobalShift(x,y,z);
	ccLog::Print("[doActionEditGlobalShift] New shift: (%f, %f, %f)",x,y,z);

	updateUI();
}

void MainWindow::doActionEditGlobalScale()
{
	size_t selNum = m_selectedEntities.size();
	if (selNum != 1)
	{
		if (selNum > 1)
			ccConsole::Error("Select only one point cloud or mesh!");
		return;
	}
	ccHObject* ent = m_selectedEntities[0];

	bool lockedVertices;
	ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent,&lockedVertices);
	//for point clouds only
	if (!cloud)
		return;
	if (lockedVertices && !ent->isAncestorOf(cloud))
	{
		//see ccPropertiesTreeDelegate::fillWithMesh
		DisplayLockedVerticesWarning();
		return;
	}

	assert(cloud);
	double scale = cloud->getGlobalScale();

	bool ok;
	scale = QInputDialog::getDouble(this, "Edit global scale", "Global scale", scale, 1e-6, 1e6, 6, &ok);
	if (!ok)
		return;

	//apply new scale
	cloud->setGlobalScale(scale);
	ccLog::Print("[doActionEditGlobalScale] New scale: %f",scale);

	updateUI();
}

void MainWindow::doComputeBestFitBB()
{
	if (QMessageBox::warning(	this,
								"This method is for test purpose only",
								"Cloud(s) are going to be rotated while still displayed in their previous position! Proceed?",
								QMessageBox::Yes | QMessageBox::No,
								QMessageBox::No ) != QMessageBox::Yes)
	{
		return;
	}

	//we must backup 'm_selectedEntities' as removeObjectTemporarilyFromDBTree can modify it!
	ccHObject::Container selectedEntities = m_selectedEntities;
	size_t selNum = selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = selectedEntities[i];
		ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent);

		if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD)) // TODO
		{
			CCLib::Neighbourhood Yk(cloud);

			CCLib::SquareMatrixd covMat = Yk.computeCovarianceMatrix();
			if (covMat.isValid())
			{
				CCLib::SquareMatrixd eig = covMat.computeJacobianEigenValuesAndVectors();
				if (eig.isValid())
				{
					eig.sortEigenValuesAndVectors();

					ccGLMatrix trans;
					GLfloat* rotMat = trans.data();
					for (unsigned j=0; j<3; ++j)
					{
						double u[3];
						eig.getEigenValueAndVector(j,u);
						CCVector3 v(static_cast<PointCoordinateType>(u[0]),
									static_cast<PointCoordinateType>(u[1]),
									static_cast<PointCoordinateType>(u[2]));
						v.normalize();
						rotMat[j*4]		= static_cast<float>(v.x);
						rotMat[j*4+1]	= static_cast<float>(v.y);
						rotMat[j*4+2]	= static_cast<float>(v.z);
					}

					const CCVector3* G = Yk.getGravityCenter();
					assert(G);
					trans.shiftRotationCenter(*G);

					cloud->setGLTransformation(trans);
					trans.invert();

					//we temporarily detach entity, as it may undergo
					//"severe" modifications (octree deletion, etc.) --> see ccPointCloud::applyRigidTransformation
					ccHObjectContext objContext = removeObjectTemporarilyFromDBTree(cloud);
					static_cast<ccPointCloud*>(cloud)->applyRigidTransformation(trans);
					putObjectBackIntoDBTree(cloud,objContext);

					ent->prepareDisplayForRefresh_recursive();
				}
			}
		}
	}

	refreshAll();
}

void MainWindow::doActionClearColor()
{
	doActionClearProperty(0);
}

void MainWindow::doActionClearNormals()
{
	doActionClearProperty(1);
}

void MainWindow::doActionDeleteScalarField()
{
	doActionClearProperty(2);
}

void MainWindow::doActionDeleteAllSF()
{
	doActionClearProperty(3);
}

void MainWindow::doActionClearProperty(int prop)
{
	//we must backup 'm_selectedEntities' as removeObjectTemporarilyFromDBTree can modify it!
	ccHObject::Container selectedEntities = m_selectedEntities;

	size_t selNum = selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = selectedEntities[i];

		//specific case: clear normals on a mesh
		if (prop == 1 && ( ent->isA(CC_TYPES::MESH) /*|| ent->isKindOf(CC_TYPES::PRIMITIVE)*/ )) //TODO
		{
			ccMesh* mesh = ccHObjectCaster::ToMesh(ent);
			if (mesh->hasTriNormals())
			{
				mesh->showNormals(false);
				ccHObjectContext objContext = removeObjectTemporarilyFromDBTree(mesh);
				mesh->clearTriNormals();
				putObjectBackIntoDBTree(mesh,objContext);
				ent->prepareDisplayForRefresh();
				continue;
			}
			else if (mesh->hasNormals()) //per-vertex normals?
			{
				if (mesh->getParent()
					&& (mesh->getParent()->isA(CC_TYPES::MESH)/*|| mesh->getParent()->isKindOf(CC_TYPES::PRIMITIVE)*/) //TODO
					&& ccHObjectCaster::ToMesh(mesh->getParent())->getAssociatedCloud() == mesh->getAssociatedCloud())
				{
					ccLog::Warning("[doActionClearNormals] Can't remove per-vertex normals on a sub mesh!");
				}
				else //mesh is alone, we can freely remove normals
				{
					if (mesh->getAssociatedCloud() && mesh->getAssociatedCloud()->isA(CC_TYPES::POINT_CLOUD))
					{
						mesh->showNormals(false);
						static_cast<ccPointCloud*>(mesh->getAssociatedCloud())->unallocateNorms();
						mesh->prepareDisplayForRefresh();
						continue;
					}
				}
			}
		}

		bool lockedVertices;
		ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent,&lockedVertices);
		if (lockedVertices)
		{
			DisplayLockedVerticesWarning();
			continue;
		}

		if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD)) // TODO
		{
			switch (prop)
			{
			case 0: //colors
				if (cloud->hasColors())
				{
					static_cast<ccPointCloud*>(cloud)->unallocateColors();
					ent->prepareDisplayForRefresh();
				}
				break;
			case 1: //normals
				if (cloud->hasNormals())
				{
					static_cast<ccPointCloud*>(cloud)->unallocateNorms();
					ent->prepareDisplayForRefresh();
				}
				break;
			case 2: //current sf
				if (cloud->hasDisplayedScalarField())
				{
					ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
					pc->deleteScalarField(pc->getCurrentDisplayedScalarFieldIndex());
					ent->prepareDisplayForRefresh();
				}
				break;
			case 3: //all sf
				if (cloud->hasScalarFields())
				{
					static_cast<ccPointCloud*>(cloud)->deleteAllScalarFields();
					ent->prepareDisplayForRefresh();
				}
				break;
			}
		}
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionMeasureMeshSurface()
{
	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = m_selectedEntities[i];
		if (ent->isKindOf(CC_TYPES::MESH))
		{
			ccGenericMesh* mesh = ccHObjectCaster::ToGenericMesh(ent);
			double S = CCLib::MeshSamplingTools::computeMeshArea(mesh);
			//we force the console to display itself
			forceConsoleDisplay();
			ccConsole::Print(QString("[Mesh Surface Measurer] Mesh %1: S=%2 (square units)").arg(ent->getName()).arg(S));
			if (mesh->size())
				ccConsole::Print(QString("[Mesh Surface Measurer] Mean triangle surface: %1 (square units)").arg(S/double(mesh->size())));
		}
	}
}

void displaySensorProjectErrorString(int errorCode)
{
	switch (errorCode)
	{
	case -1:
		ccConsole::Error("Internal error: bad input!");
		break;
	case -2:
		ccConsole::Error("Error: depth buffer is too big (try to reduce angular steps)");
		break;
	case -3:
		ccConsole::Error("Error: depth buffer is too small (try to increase angular steps)");
		break;
	case -4:
		ccConsole::Error("Error: not enough memory! (try to reduce angular steps)");
		break;
	default:
		ccConsole::Error("An unknown error occurred while creating sensor (code: %i)",errorCode);
	}
}

void MainWindow::doActionComputeDistancesFromSensor()
{
	//we support more than just one sensor in selection
	if (m_selectedEntities.empty())
	{
		ccConsole::Error("Select at least a sensor.");
		return;
	}

	//start dialog
	ccSensorComputeDistancesDlg cdDlg(this);
	if (!cdDlg.exec())
		return;

	for (int i = 0; i < m_selectedEntities.size(); ++i)
	{
		ccSensor* sensor = ccHObjectCaster::ToSensor(m_selectedEntities[i]);
		assert(sensor);
		if (!sensor)
			continue; //skip this entity

		//sensor must have a parent cloud -> this error probably could not happen
		//If in a future cc will permits to have not-cloud-associated sensors this will
		//ensure to not have bugs
		if (!sensor->getParent() || !sensor->getParent()->isKindOf(CC_TYPES::POINT_CLOUD))
		{
			ccConsole::Error("Sensor must be associated with a point cloud!");
			return;
		}

		//get associated cloud
		ccPointCloud * cloud = ccHObjectCaster::ToPointCloud(sensor->getParent());
		assert(cloud);

		//sensor center
		CCVector3 sensorCenter;
		if (!sensor->getActiveAbsoluteCenter(sensorCenter))
			return;

		//squared required?
		bool squared = cdDlg.computeSquaredDistances();

		//set up a new scalar field
		const char* defaultRangesSFname = squared ? CC_DEFAULT_SQUARED_RANGES_SF_NAME : CC_DEFAULT_RANGES_SF_NAME;
		int sfIdx = cloud->getScalarFieldIndexByName(defaultRangesSFname);
		if (sfIdx < 0)
		{
			sfIdx = cloud->addScalarField(defaultRangesSFname);
			if (sfIdx < 0)
			{
				ccConsole::Error("Not enough memory!");
				return;
			}
		}
		CCLib::ScalarField* distances = cloud->getScalarField(sfIdx);

		for (unsigned i=0; i<cloud->size(); ++i)
		{
			const CCVector3* P = cloud->getPoint(i);
			ScalarType s = static_cast<ScalarType>(squared ? (*P-sensorCenter).norm2() : (*P-sensorCenter).norm());
			distances->setValue(i, s);
		}

		distances->computeMinAndMax();
		cloud->setCurrentDisplayedScalarField(sfIdx);
		cloud->showSF(true);
		cloud->prepareDisplayForRefresh_recursive();
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionComputeScatteringAngles()
{
	//there should be only one sensor in current selection!
	if (m_selectedEntities.size() != 1 || !m_selectedEntities[0]->isKindOf(CC_TYPES::GBL_SENSOR))
	{
		ccConsole::Error("Select one and only one GBL sensor!");
		return;
	}

	ccSensor* sensor = ccHObjectCaster::ToSensor(m_selectedEntities[0]);
	assert(sensor);

	//sensor must have a parent cloud with normal
	if (!sensor->getParent() || !sensor->getParent()->isKindOf(CC_TYPES::POINT_CLOUD) || !sensor->getParent()->hasNormals())
	{
		ccConsole::Error("Sensor must be associated to a point cloud with normals! (compute normals first)");
		return;
	}

	//sensor center
	CCVector3 sensorCenter;
	if (!sensor->getActiveAbsoluteCenter(sensorCenter))
		return;

	//get associated cloud
	ccPointCloud * cloud = ccHObjectCaster::ToPointCloud(sensor->getParent());
	assert(cloud);
	if (!cloud)
		return;

	ccSensorComputeScatteringAnglesDlg cdDlg(this);
	if (!cdDlg.exec())
		return;

	bool toDegreeFlag = cdDlg.anglesInDegrees();

	//prepare a new scalar field
	const char* defaultScatAnglesSFname = toDegreeFlag ? CC_DEFAULT_DEG_SCATTERING_ANGLES_SF_NAME : CC_DEFAULT_RAD_SCATTERING_ANGLES_SF_NAME;
	int sfIdx = cloud->getScalarFieldIndexByName(defaultScatAnglesSFname);
	if (sfIdx < 0)
	{
		sfIdx = cloud->addScalarField(defaultScatAnglesSFname);
		if (sfIdx < 0)
		{
			ccConsole::Error("Not enough memory!");
			return;
		}
	}
	CCLib::ScalarField* angles = cloud->getScalarField(sfIdx);

	//perform computations
	for (unsigned i=0; i<cloud->size(); ++i)
	{
		//the point position
		const CCVector3* P = cloud->getPoint(i);

		//build the ray
		CCVector3 ray = *P - sensorCenter;
		ray.normalize();

		//get the current normal
		CCVector3 normal(cloud->getPointNormal(i));
		//normal.normalize(); //should already be the case!

		//compute the angle
		PointCoordinateType cosTheta = ray.dot(normal);
		ScalarType theta = static_cast<ScalarType>( acos(std::min<PointCoordinateType>(fabs(cosTheta),1)) );

		if (toDegreeFlag)
			theta *= static_cast<ScalarType>(CC_RAD_TO_DEG);

		angles->setValue(i,theta);
	}

	angles->computeMinAndMax();
	cloud->setCurrentDisplayedScalarField(sfIdx);
	cloud->showSF(true);
	cloud->prepareDisplayForRefresh_recursive();

	refreshAll();
	updateUI();
}

void MainWindow::doActionSetViewFromSensor()
{
	//there should be only one sensor in current selection!
	if (m_selectedEntities.size() != 1 || !m_selectedEntities[0]->isKindOf(CC_TYPES::SENSOR))
	{
		ccConsole::Error("Select one and only one sensor!");
		return;
	}

	ccSensor* sensor = ccHObjectCaster::ToSensor(m_selectedEntities[0]);
	assert(sensor);
	//sensor center
	ccIndexedTransformation trans;
	if (!sensor->getAbsoluteTransformation(trans, sensor->getActiveIndex()))
	{
		ccLog::Error("[doActionSetViewFromSensor] Failed to get a valid transformation for current index!");
		return;
	}
	CCVector3 sensorCenter = trans.getTranslationAsVec3D();

	//get associated cloud
	ccPointCloud * cloud = ccHObjectCaster::ToPointCloud(sensor->getParent());
	assert(cloud);

	ccGLWindow* win = 0;
	if (cloud)
		win = static_cast<ccGLWindow*>(cloud->getDisplay());
	else
		win = getActiveGLWindow();

	if (win)
	{
		//ccViewportParameters params = win->getViewportParameters();
		win->setPerspectiveState(true,false);
		win->setCameraPos(CCVector3d::fromArray(sensorCenter.u));
		win->setPivotPoint(CCVector3d::fromArray(sensorCenter.u));
		//FIXME: more complicated! Depends on the 'rotation order' for GBL sensors for instance
		win->setView(CC_FRONT_VIEW,false);
		win->rotateBaseViewMat(ccGLMatrixd(trans.data()));
		//TODO: can we set the right FOV?
		win->redraw();
	}
	else
	{
		ccLog::Warning("[doActionSetViewFromSensor] Failed to get a valid 3D view!");
	}
}

void MainWindow::doActionCreateGBLSensor()
{
	ccSensorProjectionDlg spDlg(this);
	if (!spDlg.exec())
		return;

	//We create the corresponding sensor for each input cloud (in a perfect world, there should be only one ;)
	ccHObject::Container selectedEntities = m_selectedEntities;
	size_t selNum = selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = selectedEntities[i];
		if (ent->isKindOf(CC_TYPES::POINT_CLOUD))
		{
			ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent);

			//we create a new sensor
			ccGBLSensor* sensor = new ccGBLSensor();

			//we init its parameters with dialog
			spDlg.updateGBLSensor(sensor);

			//we compute projection
			int errorCode;
			CCLib::GenericIndexedCloud* projectedPoints = sensor->project(cloud,errorCode,true);

			if (projectedPoints)
			{
				cloud->addChild(sensor);

				//we try to guess the sensor relative size (dirty)
				ccBBox bb = cloud->getBB();
				double diag = bb.getDiagNorm();
				if (diag < 1.0)
					sensor->setGraphicScale(static_cast<PointCoordinateType>(1.0e-3));
				else if (diag > 10000.0)
					sensor->setGraphicScale(static_cast<PointCoordinateType>(1.0e3));

				//we display depth buffer
				ccRenderingTools::ShowDepthBuffer(sensor,this);

				////DGM: test
				//{
				//	//add positions
				//	const unsigned count = 1000;
				//	const PointCoordinateType R = 100;
				//	const PointCoordinateType dh = 100;
				//	for (unsigned i=0; i<1000; ++i)
				//	{
				//		float angle = (float)i/(float)count * 6 * M_PI;
				//		float X = R * cos(angle);
				//		float Y = R * sin(angle);
				//		float Z = (float)i/(float)count * dh;

				//		ccIndexedTransformation trans;
				//		trans.initFromParameters(-angle,CCVector3(0,0,1),CCVector3(X,Y,Z));
				//		sensor->addPosition(trans,i);
				//	}
				//}

				//set position
				//ccIndexedTransformation trans;
				//sensor->addPosition(trans,0);

				ccGLWindow* win = static_cast<ccGLWindow*>(cloud->getDisplay());
				if (win)
				{
					sensor->setDisplay_recursive(win);
					sensor->setVisible(true);
					ccBBox box = cloud->getBB();
					win->updateConstellationCenterAndZoom(&box);
				}
				delete projectedPoints;
				projectedPoints = 0;

				addToDB(sensor);
			}
			else
			{
				displaySensorProjectErrorString(errorCode);
				delete sensor;
				sensor = 0;
			}
		}
	}

	updateUI();
}

void MainWindow::doActionCreateCameraSensor()
{
	//We create the corresponding sensor for each input cloud
	ccHObject::Container selectedEntities = m_selectedEntities;
	size_t selNum = selectedEntities.size();

	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = selectedEntities[i];

		if (ent->isKindOf(CC_TYPES::POINT_CLOUD))
		{
			ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent);

			//we create a new sensor
			ccCameraSensor* sensor = new ccCameraSensor();
			cloud->addChild(sensor);

			//we try to guess the sensor relative size (dirty)
			ccBBox bb = cloud->getBB();
			double diag = bb.getDiagNorm();
			if (diag < 1.0)
				sensor->setGraphicScale(static_cast<PointCoordinateType>(1.0e-3));
			else if (diag > 10000.0)
				sensor->setGraphicScale(static_cast<PointCoordinateType>(1.0e3));

			/*//we update sensor graphic representation
			sensor->updateGraphicRepresentation();*/

			//set position
			ccIndexedTransformation trans;
			sensor->addPosition(trans,0);

			ccGLWindow* win = static_cast<ccGLWindow*>(cloud->getDisplay());
			if (win)
			{
				sensor->setDisplay(win);
				sensor->setVisible(true);
				ccBBox box = cloud->getBB();
				win->updateConstellationCenterAndZoom(&box);
			}

			addToDB(sensor);
		}
	}

	updateUI();
}

void MainWindow::doActionModifySensor()
{
	//there should be only one point cloud with sensor in current selection!
	if (m_selectedEntities.empty() || m_selectedEntities.size()>1 || !m_selectedEntities[0]->isKindOf(CC_TYPES::SENSOR))
	{
		ccConsole::Error("Select one and only one sensor!");
		return;
	}

	ccSensor* sensor = static_cast<ccSensor*>(m_selectedEntities[0]);

	if (sensor->isA(CC_TYPES::GBL_SENSOR))
	{
		ccGBLSensor* gbl = static_cast<ccGBLSensor*>(sensor);
		ccSensorProjectionDlg spDlg(this);

		spDlg.initWithGBLSensor(gbl);
		if (spDlg.exec())
		{
			//we init its parameters with dialog
			spDlg.updateGBLSensor(gbl);

			//we re-project cloud
			if (gbl->getParent()->isKindOf(CC_TYPES::POINT_CLOUD))
			{
				int errorCode;
				ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(gbl->getParent());

				CCLib::GenericIndexedCloud* projectedPoints = gbl->project(cloud,errorCode,true);
				if (projectedPoints)
				{
					//we don't need the projected points anymore
					delete projectedPoints;
					projectedPoints = 0;

					//we display depth buffer
					ccRenderingTools::ShowDepthBuffer(gbl,this);

					//in case the sensor position has changed
					ccGLWindow* win = static_cast<ccGLWindow*>(cloud->getDisplay());
					if (win)
					{
						ccBBox box = cloud->getBB();
						win->updateConstellationCenterAndZoom(&box);
					}

					if (sensor->isVisible() && sensor->isEnabled())
					{
						sensor->prepareDisplayForRefresh();
						refreshAll();
					}

					updateUI();
				}
				else
				{
					displaySensorProjectErrorString(errorCode);
				}
			}
			else
			{
				ccConsole::Error(QString("Internal error: sensor ('%1') parent is not a point cloud!").arg(sensor->getName()));
			}
		}
	}
	else
	{
		ccConsole::Error("Can't modify this kind of sensor!");
	}
}

void MainWindow::doActionProjectUncertainty()
{
	//there should only be one sensor in the current selection!
	if (m_selectedEntities.size() != 1 || !m_selectedEntities[0]->isKindOf(CC_TYPES::CAMERA_SENSOR))
	{
		ccConsole::Error("Select one and only one camera (projective) sensor!");
		return;
	}

	ccCameraSensor* sensor = ccHObjectCaster::ToCameraSensor(m_selectedEntities[0]);
	assert(sensor);
	if (!sensor)
		return;

	//the sensor must be the child of a point cloud, or it is not possible to project anything
	if (!sensor->getParent() || !sensor->getParent()->isA(CC_TYPES::POINT_CLOUD))
	{
		ccConsole::Error("The sensor must be the child of a point cloud!");
		return;
	}

	bool lockedVertices;
	ccPointCloud* pointCloud = ccHObjectCaster::ToPointCloud(sensor->getParent(),&lockedVertices);
	if (!pointCloud || lockedVertices)
	{
		if (lockedVertices)
			DisplayLockedVerticesWarning();
		return;
	}

	CCLib::ReferenceCloud points(pointCloud);
	if (!points.reserve(pointCloud->size()))
	{
		ccConsole::Error("Not enough memory!");
		return;
	}
	points.setPointIndex(0,pointCloud->size());

	// compute uncertainty
	std::vector< Vector3Tpl<ScalarType> > accuracy;
	if (!sensor->computeUncertainty(&points, accuracy/*, false*/))
	{
		ccConsole::Error("Not enough memory!");
		return;
	}

	/////////////
	// SIGMA D //
	/////////////
	char dimChar[3] = {'x','y','z'};
	for (unsigned d=0; d<3; ++d)
	{
		// add scalar field
		QString sfName = QString("Sensor uncertainty (%1)").arg(dimChar[d]);
		int sfIdx = pointCloud->getScalarFieldIndexByName(qPrintable(sfName));
		if (sfIdx < 0)
			sfIdx = pointCloud->addScalarField(qPrintable(sfName));
		if (sfIdx < 0)
		{
			ccLog::Error("An error occured! (see console)");
			return;
		}

		// fill scalar field
		CCLib::ScalarField* sf = pointCloud->getScalarField(sfIdx);
		assert(sf);
		if (sf)
		{
			for (size_t i=0 ; i<accuracy.size() ; i++)
				sf->setValue(static_cast<unsigned>(i), accuracy[i].u[d]);
			sf->computeMinAndMax();
		}
	}

	/////////////////
	// SIGMA TOTAL //
	/////////////////

	// add scalar field
	{
		QString sfName = QString("Sensor uncertainty (3D)");
		int sfIdx = pointCloud->getScalarFieldIndexByName(qPrintable(sfName));
		if (sfIdx < 0)
			sfIdx = pointCloud->addScalarField(qPrintable(sfName));
		if (sfIdx <0)
		{
			ccLog::Error("An error occured! (see console)");
			return;
		}

		// fill scalar field
		CCLib::ScalarField* sf = pointCloud->getScalarField(sfIdx);
		assert(sf);
		if (sf)
		{
			for (size_t i=0 ; i<accuracy.size() ; i++)
				sf->setValue(static_cast<unsigned>(i), accuracy[i].norm());
			sf->computeMinAndMax();
		}

		pointCloud->showSF(true);
		pointCloud->setCurrentDisplayedScalarField(sfIdx);
		pointCloud->prepareDisplayForRefresh();
	}

	refreshAll();
}

void MainWindow::doActionCheckPointsInsideFrustrum()
{
	//there should be only one camera sensor in the current selection!
	if (m_selectedEntities.size() != 1 || !m_selectedEntities[0]->isKindOf(CC_TYPES::CAMERA_SENSOR))
	{
		ccConsole::Error("Select one and only one camera sensor!");
		return;
	}

	ccCameraSensor* sensor = ccHObjectCaster::ToCameraSensor(m_selectedEntities[0]);
	if (!sensor)
		return;

	//the sensor must be the child of a point cloud, otherwise it won't be possible to project anything
	if (!sensor->getParent() || !sensor->getParent()->isA(CC_TYPES::POINT_CLOUD))
	{
		ccConsole::Error("The sensor must be the child of a point cloud!");
		return;
	}

	ccPointCloud* pointCloud = ccHObjectCaster::ToPointCloud(sensor->getParent());
	if (!pointCloud)
		return;

	//comupte/get the point cloud's octree
	ccOctree* octree = pointCloud->getOctree();
	if (!octree)
	{
		octree = pointCloud->computeOctree();
		if (!octree)
		{
			ccConsole::Error("Failed to compute the octree!");
			return;
		}
	}
	assert(octree);

	// filter octree then project the points
	std::vector<unsigned> inCameraFrustrum;
	if (!octree->intersectWithFrustrum(sensor,inCameraFrustrum))
	{
		ccConsole::Error("Failed to intersect sensor frustrum with octree!");
	}
	else
	{
		// scalar field
		const char sfName[] = "Frustrum visibility";
		int sfIdx = pointCloud->getScalarFieldIndexByName(sfName);

		if (inCameraFrustrum.empty())
		{
			ccConsole::Error("No point fell inside the frustrum!");
			if (sfIdx >= 0)
				pointCloud->deleteScalarField(sfIdx);
		}
		else
		{
			if (sfIdx < 0)
				sfIdx = pointCloud->addScalarField(sfName);
			if (sfIdx < 0)
			{
				ccLog::Error("Failed to allocate memory for output scalar field!");
				return;
			}

			CCLib::ScalarField* sf = pointCloud->getScalarField(sfIdx);
			assert(sf);
			if (sf)
			{
				sf->fill(0);

				const ScalarType c_insideValue = static_cast<ScalarType>(1);

				for (size_t i=0; i<inCameraFrustrum.size(); i++)
					sf->setValue(inCameraFrustrum[i], c_insideValue);

				sf->computeMinAndMax();
				pointCloud->setCurrentDisplayedScalarField(sfIdx);
				pointCloud->showSF(true);

				pointCloud->refreshDisplay_recursive();
			}
		}
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionShowDepthBuffer()
{
	if (m_selectedEntities.empty())
		return;

	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = m_selectedEntities[i];
		if (ent->isKindOf(CC_TYPES::GBL_SENSOR))
		{
			ccGBLSensor* sensor = static_cast<ccGBLSensor*>(m_selectedEntities[0]);
			if (!sensor->getDepthBuffer().zBuff)
			{
				ccConsole::Error("[ShowDepthBuffer] Depth buffer not computed for this sensor.");
				return;
			}

			ccRenderingTools::ShowDepthBuffer(sensor,this);
		}
	}
}

static CC_FILE_TYPES currentDBSaveDlgFilter = DM_ASCII;
void MainWindow::doActionExportDepthBuffer()
{
	if (m_selectedEntities.empty())
		return;

	//we set up file filters
	QStringList filters;
	filters << CC_FILE_TYPE_FILTERS[DM_ASCII];

	QFileDialog dialog(this);
	dialog.setNameFilters(filters);
	dialog.setViewMode(QFileDialog::Detail);
	dialog.setConfirmOverwrite(true);
	dialog.setAcceptMode(QFileDialog::AcceptSave);

	dialog.selectNameFilter(CC_FILE_TYPE_FILTERS[currentDBSaveDlgFilter]);

	QString filename = m_selectedEntities[0]->getName()+QString(".")+QString(CC_FILE_TYPE_DEFAULT_EXTENSION[currentDBSaveDlgFilter]);
	dialog.selectFile(filename);

	if (dialog.exec())
	{
		QStringList fileNames = dialog.selectedFiles();
		if (fileNames.empty())
			return;

		assert(fileNames.size() == 1);

		//we try to find the selected file format
		QString filter = dialog.selectedNameFilter();
		CC_FILE_TYPES fType = UNKNOWN_FILE;
		for (unsigned i=0; i<static_cast<unsigned>(FILE_TYPES_COUNT); ++i)
		{
			if (filter == QString(CC_FILE_TYPE_FILTERS[i]))
			{
				fType = CC_FILE_TYPES_ENUMS[i];
				break;
			}
		}
		currentDBSaveDlgFilter = fType;

		ccHObject* toSave = 0;
		bool multEntities = false;
		if (m_selectedEntities.size() > 1)
		{
			toSave = new ccHObject("Temp Group");
			for (size_t i=0; i<m_selectedEntities.size(); ++i)
				toSave->addChild(m_selectedEntities[i],ccHObject::DP_NONE);
			multEntities = true;
		}
		else
		{
			toSave = m_selectedEntities[0];
		}

		CC_FILE_ERROR result = FileIOFilter::SaveToFile(toSave,
														qPrintable(fileNames.at(0)),
														fType);

		if (result!=CC_FERR_NO_ERROR)
			FileIOFilter::DisplayErrorMessage(result,"saving depth buffer",fileNames.at(0));
		else
			ccLog::Print(QString("[doActionExportDepthBuffer] File '%1' successfully exported").arg(fileNames.at(0)));

		if (multEntities)
			delete toSave;
	}
}

void MainWindow::doActionConvertTextureToColor()
{
	ccHObject::Container selectedEntities = m_selectedEntities;

	for (size_t i=0; i<selectedEntities.size(); ++i)
	{
		ccHObject* ent = selectedEntities[i];
		if (ent->isA(CC_TYPES::MESH)/*|| ent->isKindOf(CC_TYPES::PRIMITIVE)*/) //TODO
		{
			ccMesh* mesh = ccHObjectCaster::ToMesh(ent);
			assert(mesh);

			if (!mesh->hasMaterials())
			{
				ccLog::Warning(QString("[doActionConvertTextureToColor] Mesh '%1' has no material/texture!").arg(mesh->getName()));
				continue;
			}
			else
			{
				if (QMessageBox::warning(	this,
											"Mesh already has colors",
											QString("Mesh '%1' already has colors! Overwrite them?").arg(mesh->getName()),
											QMessageBox::Yes | QMessageBox::No,
											QMessageBox::No ) != QMessageBox::Yes)
				{
					continue;
				}


				//colorType C[3]={MAX_COLOR_COMP,MAX_COLOR_COMP,MAX_COLOR_COMP};
				//mesh->getColorFromMaterial(triIndex,*P,C,withRGB);
				//cloud->addRGBColor(C);
				if (mesh->convertMaterialsToVertexColors())
				{
					mesh->showColors(true);
					mesh->showMaterials(false);
					mesh->prepareDisplayForRefresh_recursive();
				}
				else
				{
					ccLog::Warning(QString("[doActionConvertTextureToColor] Failed to convert texture on mesh '%1'!").arg(mesh->getName()));
				}
			}
		}
	}

	refreshAll();
	updateUI();
}

static unsigned s_ptsSamplingCount = 1000000;
static double s_ptsSamplingDensity = 10.0;
void MainWindow::doActionSamplePoints()
{
	ccPtsSamplingDlg dlg(this);
	//restore last parameters
	dlg.setPointsNumber(s_ptsSamplingCount);
	dlg.setDensityValue(s_ptsSamplingDensity);
	if (!dlg.exec())
		return;

	ccProgressDialog pDlg(false,this);

	bool withNormals = dlg.generateNormals();
	bool withRGB = dlg.interpolateRGB();
	bool withTexture = dlg.interpolateTexture();
	bool useDensity = dlg.useDensity();
	assert(dlg.getPointsNumber() >= 0);
	s_ptsSamplingCount = static_cast<unsigned>(dlg.getPointsNumber());
	s_ptsSamplingDensity = dlg.getDensityValue();

	bool errors = false;

	ccHObject::Container selectedEntities = m_selectedEntities;

	for (size_t i=0; i<selectedEntities.size() ;++i)
	{
		ccHObject* ent = selectedEntities[i];
		if (ent->isKindOf(CC_TYPES::MESH))
		{
			ccGenericMesh* mesh = ccHObjectCaster::ToGenericMesh(ent);
			assert(mesh);

			ccPointCloud* cloud = mesh->samplePoints(	useDensity,
														useDensity ? s_ptsSamplingDensity : s_ptsSamplingCount,
														withNormals,
														withRGB,
														withTexture,
														&pDlg );

			if (cloud)
			{
				addToDB(cloud);
			}
			else
			{
				errors = true;
			}
		}
	}

	if (errors)
		ccLog::Error("[doActionSamplePoints] Errors occurred during the process! Result may be incomplete!");

	refreshAll();
}

void MainWindow::doRemoveDuplicatePoints()
{
	if (m_selectedEntities.empty())
		return;

	ccHObject::Container selectedEntities = m_selectedEntities;
	size_t selNum = selectedEntities.size();
	bool first = true;

	//persistent setting(s)
	QSettings settings;
	settings.beginGroup(s_psDuplicatePointsGroup);
	double minDistanceBetweenPoints = settings.value(s_psDuplicatePointsMinDist,1.0e-12).toDouble();

	bool ok;
	minDistanceBetweenPoints = QInputDialog::getDouble(this, "Remove duplicate points", "Min distance between points:", minDistanceBetweenPoints, 0, 1.0e8, 12, &ok);
	if (!ok)
		return;

	//save parameter
	settings.setValue(s_psDuplicatePointsMinDist,minDistanceBetweenPoints);

	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = selectedEntities[i];

		ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(ent);
		if (cloud)
		{
			//create temporary SF for 'duplicate flags'
			int sfIdx = cloud->getScalarFieldIndexByName("DuplicateFlags");
			if (sfIdx < 0)
				sfIdx = cloud->addScalarField("DuplicateFlags");
			if (sfIdx >= 0)
				cloud->setCurrentScalarField(sfIdx);
			else
			{
				ccConsole::Error("Couldn't create temporary scalar field! Not enough memory?");
				break;
			}

			ccOctree* octree = cloud->getOctree();
			ccProgressDialog pDlg(true,this);

			int result = CCLib::GeometricalAnalysisTools::flagDuplicatePoints(cloud,minDistanceBetweenPoints,&pDlg,octree);

			if (result >= 0)
			{
				//count the number of duplicate points!
				CCLib::ScalarField* flagSF = cloud->getScalarField(sfIdx);
				unsigned duplicateCount = 0;
				assert(flagSF);
				if (flagSF)
				{
					for (unsigned j=0; j<flagSF->currentSize(); ++j)
						if (flagSF->getValue(j) != 0)
							++duplicateCount;
				}

				if (duplicateCount == 0)
				{
					ccConsole::Print(QString("Cloud '%1' has no duplicate points").arg(cloud->getName()));
				}
				else
				{
					ccConsole::Warning(QString("Cloud '%1' has %2 duplicate point(s)").arg(cloud->getName()).arg(duplicateCount));

					ccPointCloud* filteredCloud = cloud->filterPointsByScalarValue(0,0);
					if (filteredCloud)
					{
						int sfIdx2 = filteredCloud->getScalarFieldIndexByName("DuplicateFlags");
						assert(sfIdx2 >= 0);
						filteredCloud->deleteScalarField(sfIdx2);
						filteredCloud->setName(QString("%1.clean").arg(cloud->getName()));
						filteredCloud->setDisplay(cloud->getDisplay());
						filteredCloud->prepareDisplayForRefresh();
						addToDB(filteredCloud);
						if (first)
						{
							m_ccRoot->unselectAllEntities();
							first = false;
						}
						m_ccRoot->selectEntity(filteredCloud,true);
					}
				}
			}
			else
			{
				ccConsole::Error("An error occurred! (Not enough memory?)");
			}

			cloud->deleteScalarField(sfIdx);
		}
	}

	refreshAll();
}

void MainWindow::doActionFilterByValue()
{
	ccHObject::Container selectedEntities = m_selectedEntities;
	size_t selNum = selectedEntities.size();

	typedef std::pair<ccHObject*,ccPointCloud*> entityAndVerticesType;
	std::vector<entityAndVerticesType> toFilter;
	{
		for (size_t i=0; i<selNum; ++i)
		{
			ccGenericPointCloud* cloud = 0;
			ccHObject* ent = selectedEntities[i];

			cloud = ccHObjectCaster::ToGenericPointCloud(ent);
			if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD)) // TODO
			{
				ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
				//la methode est activee sur le champ scalaire affiche
				CCLib::ScalarField* sf = pc->getCurrentDisplayedScalarField();
				if (sf)
				{
					toFilter.push_back(entityAndVerticesType(ent,pc));
				}
				else
				{
					ccConsole::Warning(QString("Entity [%1] has no active scalar field !").arg(ent->getName()));
				}
			}
		}
	}

	double minVald = 0.0;
	double maxVald = 1.0;

	if (toFilter.empty())
		return;

	//compute min and max "displayed" scalar values of currently selected
	//entities (the ones with an active scalar field only!)
	{
		for (size_t i=0; i<toFilter.size(); ++i)
		{
			ccScalarField* sf = toFilter[i].second->getCurrentDisplayedScalarField();
			assert(sf);

			if (i == 0)
			{
				minVald = static_cast<double>(sf->displayRange().start());
				maxVald = static_cast<double>(sf->displayRange().stop());
			}
			else
			{
				if (minVald > static_cast<double>(sf->displayRange().start()))
					minVald = static_cast<double>(sf->displayRange().start());
				if (maxVald < static_cast<double>(sf->displayRange().stop()))
					maxVald = static_cast<double>(sf->displayRange().stop());
			}
		}
	}

	ccAskTwoDoubleValuesDlg dlg("Min","Max",-DBL_MAX,DBL_MAX,minVald,maxVald,8,"Filter by scalar value",this);
	if (!dlg.exec())
		return;

	ScalarType minVal = (ScalarType)dlg.doubleSpinBox1->value();
	ScalarType maxVal = (ScalarType)dlg.doubleSpinBox2->value();

	ccHObject* firstResult = 0;
	{
		for (size_t i=0; i<toFilter.size(); ++i)
		{
			ccHObject* ent = toFilter[i].first;
			ccPointCloud* pc = toFilter[i].second;
			//CCLib::ScalarField* sf = pc->getCurrentDisplayedScalarField();
			//assert(sf);

			//we set as output (OUT) the currently displayed scalar field
			int outSfIdx = pc->getCurrentDisplayedScalarFieldIndex();
			assert(outSfIdx >= 0);
			pc->setCurrentOutScalarField(outSfIdx);
			//pc->setCurrentScalarField(outSfIdx);

			ccHObject* result = 0;
			if (ent->isKindOf(CC_TYPES::MESH))
			{
				pc->hidePointsByScalarValue(minVal,maxVal);
				if (ent->isA(CC_TYPES::MESH)/*|| ent->isKindOf(CC_TYPES::PRIMITIVE)*/) //TODO
					result = ccHObjectCaster::ToMesh(ent)->createNewMeshFromSelection(false);
				else if (ent->isA(CC_TYPES::SUB_MESH))
					result = ccHObjectCaster::ToSubMesh(ent)->createNewSubMeshFromSelection(false);
				pc->unallocateVisibilityArray();
			}
			else if (ent->isKindOf(CC_TYPES::POINT_CLOUD))
			{
				//pc->hidePointsByScalarValue(minVal,maxVal);
				//result = ccHObjectCaster::ToGenericPointCloud(ent)->hidePointsByScalarValue(false);
				//pc->unallocateVisibilityArray();

				//shortcut, as we know here that the point cloud is a "ccPointCloud"
				result = pc->filterPointsByScalarValue(minVal,maxVal);
			}

			if (result)
			{
				ent->setEnabled(false);
				result->setDisplay(ent->getDisplay());
				result->prepareDisplayForRefresh();
				addToDB(result);

				if (!firstResult)
					firstResult = result;
			}
			//*/
		}
	}

	if (firstResult)
	{
		ccConsole::Warning("Previously selected entities (sources) have been hidden!");
		if (m_ccRoot)
			m_ccRoot->selectEntity(firstResult);
	}

	refreshAll();
}

void MainWindow::doActionSFConvertToRGB()
{
	//we first ask the user if the SF colors should be mixed with existing colors
	bool mixWithExistingColors=false;
	{
		QMessageBox::StandardButton answer = QMessageBox::warning(	this,
																	"Scalar Field to RGB",
																	"Mix with existing colors (if relevant)?",
																	QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
																	QMessageBox::Yes );
		if (answer == QMessageBox::Yes)
			mixWithExistingColors = true;
		else if (answer == QMessageBox::Cancel)
			return;
	}

	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccGenericPointCloud* cloud = 0;
		ccHObject* ent = m_selectedEntities[i];

		bool lockedVertices;
		cloud = ccHObjectCaster::ToPointCloud(ent,&lockedVertices);
		if (lockedVertices)
		{
			DisplayLockedVerticesWarning();
			continue;
		}
		if (cloud) //TODO
		{
			ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
			//if there is no displayed SF --> nothing to do!
			if (pc->getCurrentDisplayedScalarField())
			{
				if (pc->setRGBColorWithCurrentScalarField(mixWithExistingColors))
				{
					ent->showColors(true);
					ent->showSF(false);
				}
			}

			cloud->prepareDisplayForRefresh_recursive();
		}
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionToggleActiveSFColorScale()
{
	doApplyActiveSFAction(0);
}

void MainWindow::doActionShowActiveSFPrevious()
{
	doApplyActiveSFAction(1);
}

void MainWindow::doActionShowActiveSFNext()
{
	doApplyActiveSFAction(2);
}

void MainWindow::doApplyActiveSFAction(int action)
{
	size_t selNum = m_selectedEntities.size();
	if (selNum != 1)
	{
		if (selNum > 1)
			ccConsole::Error("Select only one point cloud or mesh!");
		return;
	}
	ccHObject* ent = m_selectedEntities[0];

	bool lockedVertices;
	ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(ent,&lockedVertices);

	//for "real" point clouds only
	if (!cloud)
		return;
	if (lockedVertices && !ent->isAncestorOf(cloud))
	{
		//see ccPropertiesTreeDelegate::fillWithMesh
		DisplayLockedVerticesWarning();
		return;
	}

	assert(cloud);
	int sfIdx = cloud->getCurrentDisplayedScalarFieldIndex();
	switch (action)
	{
		case 0: //Toggle SF color scale
			if (sfIdx >= 0)
			{
				cloud->showSFColorsScale(!cloud->sfColorScaleShown());
				cloud->prepareDisplayForRefresh();
			}
			else
				ccConsole::Warning(QString("No active scalar field on entity '%1'").arg(ent->getName()));
			break;
		case 1: //Activate previous SF
			if (sfIdx >= 0)
			{
				cloud->setCurrentDisplayedScalarField(sfIdx-1);
				cloud->prepareDisplayForRefresh();
			}
			break;
		case 2: //Activate next SF
			if (sfIdx+1 < static_cast<int>(cloud->getNumberOfScalarFields()))
			{
				cloud->setCurrentDisplayedScalarField(sfIdx+1);
				cloud->prepareDisplayForRefresh();
			}
			break;
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionRenameSF()
{
	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccGenericPointCloud* cloud = ccHObjectCaster::ToPointCloud(m_selectedEntities[i]);
		if (cloud) //TODO
		{
			ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
			ccScalarField* sf = pc->getCurrentDisplayedScalarField();
			//if there is no displayed SF --> nothing to do!
			if (!sf)
			{
				ccConsole::Warning(QString("Cloud %1 has no displayed scalar field!").arg(pc->getName()));
			}
			else
			{
				const char* sfName = sf->getName();
				bool ok;
				QString newName = QInputDialog::getText(this,"SF name","name:",QLineEdit::Normal, QString(sfName ? sfName : "unknown"), &ok);
				if (ok)
					sf->setName(qPrintable(newName));
			}
		}
	}

	updateUI();
}

void MainWindow::doActionOpenColorScalesManager()
{
	ccColorScaleEditorDialog cseDlg(ccColorScalesManager::GetUniqueInstance(),ccColorScale::Shared(0), this);

	if (cseDlg.exec())
	{
		//save current scale manager state to persistent settings
		ccColorScalesManager::GetUniqueInstance()->toPersistentSettings();
	}

	updateUI();
}

void MainWindow::doActionAddIdField()
{
	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccGenericPointCloud* cloud = ccHObjectCaster::ToPointCloud(m_selectedEntities[i]);
		if (cloud) //TODO
		{
			ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);

			int sfIdx = pc->getScalarFieldIndexByName("Id");
			if (sfIdx < 0)
				sfIdx = pc->addScalarField("Id");
			if (sfIdx < 0)
			{
				ccLog::Warning("Not enough memory!");
				return;
			}

			CCLib::ScalarField* sf = pc->getScalarField(sfIdx);
			assert(sf->currentSize() == pc->size());

			for (unsigned j=0 ; j<cloud->size(); j++)
			{
				ScalarType idValue = static_cast<ScalarType>(j);
				sf->setValue(j, idValue);
			}

			sf->computeMinAndMax();
			pc->setCurrentDisplayedScalarField(sfIdx);
			pc->showSF(true);
		}
	}

	updateUI();
}

PointCoordinateType MainWindow::GetDefaultCloudKernelSize(const ccHObject::Container& entities)
{
	PointCoordinateType sigma = -1;

	size_t selNum = entities.size();
	//computation of a first sigma guess
	for (size_t i=0; i<selNum; ++i)
	{
		ccPointCloud* pc = ccHObjectCaster::ToPointCloud(entities[i]);
		if (pc && pc->size()>0)
		{
			//we get 1% of the cloud bounding box
			//and we divide by the number of points / 10e6 (so that the kernel for a 20 M. points cloud is half the one of a 10 M. cloud)
			PointCoordinateType sigmaCloud = pc->getBB().getDiagNorm() * static_cast<PointCoordinateType>(0.01/std::max(1.0,1.0e-7*static_cast<double>(pc->size())));

			//we keep the smallest value
			if (sigma < 0 || sigmaCloud < sigma)
				sigma = sigmaCloud;
		}
	}

	return sigma;
}

void MainWindow::doActionSFGaussianFilter()
{
	size_t selNum = m_selectedEntities.size();
	if (selNum == 0)
		return;

	double sigma = GetDefaultCloudKernelSize(m_selectedEntities);
	if (sigma < 0.0)
	{
		ccConsole::Error("No elligible point cloud in selection!");
		return;
	}

	bool ok;
	sigma = QInputDialog::getDouble(this,"Gaussian filter","sigma:",sigma,DBL_MIN,DBL_MAX,8,&ok);
	if (!ok)
		return;

	for (size_t i=0; i<selNum; ++i)
	{
		bool lockedVertices;
		ccPointCloud* pc = ccHObjectCaster::ToPointCloud(m_selectedEntities[i],&lockedVertices);
		if (!pc || lockedVertices)
		{
			DisplayLockedVerticesWarning();
			continue;
		}

		//la methode est activee sur le champ scalaire affiche
		CCLib::ScalarField* sf = pc->getCurrentDisplayedScalarField();
		if (sf)
		{
			//on met en lecture (OUT) le champ scalaire actuellement affiche
			int outSfIdx = pc->getCurrentDisplayedScalarFieldIndex();
			assert(outSfIdx >= 0);

			pc->setCurrentOutScalarField(outSfIdx);
			CCLib::ScalarField* outSF = pc->getCurrentOutScalarField();
			assert(sf);

			QString sfName = QString("%1.smooth(%2)").arg(outSF->getName()).arg(sigma);
			int sfIdx = pc->getScalarFieldIndexByName(qPrintable(sfName));
			if (sfIdx < 0)
				sfIdx = pc->addScalarField(qPrintable(sfName)); //output SF has same type as input SF
			if (sfIdx >= 0)
				pc->setCurrentInScalarField(sfIdx);
			else
			{
				ccConsole::Error(QString("Failed to create scalar field for cloud '%1' (not enough memory?)").arg(pc->getName()));
				continue;
			}

			ccOctree* octree = pc->getOctree();
			if (!octree)
			{
				ccProgressDialog pDlg(true,this);
				octree = pc->computeOctree(&pDlg);
				if (!octree)
				{
					ccConsole::Error(QString("Couldn't compute octree for cloud '%1'!").arg(pc->getName()));
					continue;
				}
			}

			if (octree)
			{
				ccProgressDialog pDlg(true,this);
				QElapsedTimer eTimer;
				eTimer.start();
				CCLib::ScalarFieldTools::applyScalarFieldGaussianFilter(static_cast<PointCoordinateType>(sigma),
																		pc,
																		-1,
																		&pDlg,
																		octree);
				ccConsole::Print("[GaussianFilter] Timing: %3.2f s.",static_cast<double>(eTimer.elapsed())/1.0e3);
				pc->setCurrentDisplayedScalarField(sfIdx);
				pc->showSF(sfIdx >= 0);
				sf = pc->getCurrentDisplayedScalarField();
				if (sf)
					sf->computeMinAndMax();
				pc->prepareDisplayForRefresh_recursive();
			}
			else
			{
				ccConsole::Error(QString("Failed to compute entity [%1] octree! (not enough memory?)").arg(pc->getName()));
			}
		}
		else
		{
			ccConsole::Warning(QString("Entity [%1] has no active scalar field!").arg(pc->getName()));
		}
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionSFBilateralFilter()
{
	size_t selNum = m_selectedEntities.size();
	if (selNum == 0)
		return;

	double sigma = GetDefaultCloudKernelSize(m_selectedEntities);
	if (sigma < 0.0)
	{
		ccConsole::Error("No elligible point cloud in selection!");
		return;
	}

	//estimate a good value for scalar field sigma, based on the first cloud
	//and its displayed scalar field
	ccPointCloud* pc_test = ccHObjectCaster::ToPointCloud(m_selectedEntities[0]);
	CCLib::ScalarField* sf_test = pc_test->getCurrentDisplayedScalarField();
	ScalarType range = sf_test->getMax() - sf_test->getMin();
	double scalarFieldSigma = range / 4; // using 1/4 of total range


	ccAskTwoDoubleValuesDlg dlg("Spatial sigma", "Scalar sigma", DBL_MIN, DBL_MAX, sigma, scalarFieldSigma , 8, 0, this);
	dlg.doubleSpinBox1->setStatusTip("3*sigma = 98% attenuation");
	dlg.doubleSpinBox2->setStatusTip("Scalar field's sigma controls how much the filter behaves as a Gaussian Filter\n sigma at +inf uses the whole range of scalars ");
	if (!dlg.exec())
		return;

	//get values
	sigma = dlg.doubleSpinBox1->value();
	scalarFieldSigma = dlg.doubleSpinBox2->value();

	for (size_t i=0; i<selNum; ++i)
	{
		bool lockedVertices;
		ccPointCloud* pc = ccHObjectCaster::ToPointCloud(m_selectedEntities[i],&lockedVertices);
		if (!pc || lockedVertices)
		{
			DisplayLockedVerticesWarning();
			continue;
		}

		//the algorithm will use the currently displayed SF
		CCLib::ScalarField* sf = pc->getCurrentDisplayedScalarField();
		if (sf)
		{
			//we set the displayed SF as "OUT" SF
			int outSfIdx = pc->getCurrentDisplayedScalarFieldIndex();
			assert(outSfIdx >= 0);

			pc->setCurrentOutScalarField(outSfIdx);
			CCLib::ScalarField* outSF = pc->getCurrentOutScalarField();
			assert(sf);

			QString sfName = QString("%1.bilsmooth(%2,%3)").arg(outSF->getName()).arg(sigma).arg(scalarFieldSigma);
			int sfIdx = pc->getScalarFieldIndexByName(qPrintable(sfName));
			if (sfIdx < 0)
				sfIdx = pc->addScalarField(qPrintable(sfName)); //output SF has same type as input SF
			if (sfIdx >= 0)
				pc->setCurrentInScalarField(sfIdx);
			else
			{
				ccConsole::Error(QString("Failed to create scalar field for cloud '%1' (not enough memory?)").arg(pc->getName()));
				continue;
			}

			ccOctree* octree = pc->getOctree();
			if (!octree)
			{
				ccProgressDialog pDlg(true,this);
				octree = pc->computeOctree(&pDlg);
				if (!octree)
				{
					ccConsole::Error(QString("Couldn't compute octree for cloud '%1'!").arg(pc->getName()));
					continue;
				}
			}

			assert(octree);
			{
				ccProgressDialog pDlg(true,this);
				QElapsedTimer eTimer;
				eTimer.start();

				CCLib::ScalarFieldTools::applyScalarFieldGaussianFilter(static_cast<PointCoordinateType>(sigma),
																		pc,
																		static_cast<PointCoordinateType>(scalarFieldSigma),
																		&pDlg,
																		octree);
				ccConsole::Print("[BilateralFilter] Timing: %3.2f s.",eTimer.elapsed()/1.0e3);
				pc->setCurrentDisplayedScalarField(sfIdx);
				pc->showSF(sfIdx >= 0);
				sf = pc->getCurrentDisplayedScalarField();
				if (sf)
					sf->computeMinAndMax();
				pc->prepareDisplayForRefresh_recursive();
			}
		}
		else
		{
			ccConsole::Warning(QString("Entity [%1] has no active scalar field!").arg(pc->getName()));
		}
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionSmoothMeshSF()
{
	doMeshSFAction(ccMesh::SMOOTH_MESH_SF);
}

void MainWindow::doActionEnhanceMeshSF()
{
	doMeshSFAction(ccMesh::ENHANCE_MESH_SF);
}

void MainWindow::doMeshSFAction(ccMesh::MESH_SCALAR_FIELD_PROCESS process)
{
	ccProgressDialog pDlg(false,this);

	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = m_selectedEntities[i];
		if (ent->isKindOf(CC_TYPES::MESH) || ent->isKindOf(CC_TYPES::PRIMITIVE)) //TODO
		{
			ccMesh* mesh = ccHObjectCaster::ToMesh(ent);
			if (mesh)
			{
				ccGenericPointCloud* cloud = mesh->getAssociatedCloud();

				if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD)) //TODO
				{
					ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);

					//on active le champ scalaire actuellement affiche
					int sfIdx = pc->getCurrentDisplayedScalarFieldIndex();
					if (sfIdx >= 0)
					{
						pc->setCurrentScalarField(sfIdx);
						mesh->processScalarField(process);
						pc->getCurrentInScalarField()->computeMinAndMax();
						mesh->prepareDisplayForRefresh_recursive();
					}
					else
					{
						ccConsole::Warning(QString("Mesh [%1] vertices have no activated scalar field!").arg(mesh->getName()));
					}
				}
			}
		}
	}

	refreshAll();
	updateUI();
}

static double s_subdivideMaxArea = 1.0;
void MainWindow::doActionSubdivideMesh()
{
	bool ok;
	s_subdivideMaxArea = QInputDialog::getDouble(this, "Subdivide mesh", "Max area per triangle:", s_subdivideMaxArea, 1e-6, 1e6, 8, &ok);
	if (!ok)
		return;

	//ccProgressDialog pDlg(true,this);

	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = m_selectedEntities[i];
		if (ent->isKindOf(CC_TYPES::MESH))
		{
			//single mesh?
			if (ent->isA(CC_TYPES::MESH))
			{
				ccMesh* mesh = static_cast<ccMesh*>(ent);

				ccMesh* subdividedMesh = 0;
				try
				{
					subdividedMesh = mesh->subdivide(static_cast<PointCoordinateType>(s_subdivideMaxArea));
				}
				catch(...)
				{
					ccLog::Error(QString("[Subdivide] An error occurred while trying to subdivide mesh '%1' (not enough memory?)").arg(mesh->getName()));
				}

				if (subdividedMesh)
				{
					subdividedMesh->setName(QString("%1.subdivided(S<%2)").arg(mesh->getName()).arg(s_subdivideMaxArea));
					subdividedMesh->setDisplay(mesh->getDisplay());
					mesh->refreshDisplay_recursive();
					mesh->setEnabled(false);
					addToDB(subdividedMesh);
				}
				else
				{
					ccConsole::Warning(QString("[Subdivide] Failed to subdivide mesh '%1' (not enough memory?)").arg(mesh->getName()));
				}
			}
			else
			{
				ccLog::Warning("[Subdivide] Works only on real meshes!");
			}
		}
	}

	refreshAll();
	updateUI();
}

static unsigned	s_laplacianSmooth_nbIter = 20;
static double	s_laplacianSmooth_factor = 0.2;
void MainWindow::doActionSmoothMeshLaplacian()
{
	bool ok;
	s_laplacianSmooth_nbIter = QInputDialog::getInt(this, "Smooth mesh", "Iterations:", s_laplacianSmooth_nbIter, 1, 1000, 1, &ok);
	if (!ok)
		return;
	s_laplacianSmooth_factor = QInputDialog::getDouble(this, "Smooth mesh", "Smoothing factor:", s_laplacianSmooth_factor, 0, 100, 3, &ok);
	if (!ok)
		return;

	ccProgressDialog pDlg(true,this);

	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = m_selectedEntities[i];
		if (ent->isA(CC_TYPES::MESH) || ent->isA(CC_TYPES::PRIMITIVE)) //FIXME: can we really do this with primitives?
		{
			ccMesh* mesh = ccHObjectCaster::ToMesh(ent);

			if (mesh->laplacianSmooth(	s_laplacianSmooth_nbIter,
										static_cast<PointCoordinateType>(s_laplacianSmooth_factor),
										&pDlg) )
			{
				mesh->prepareDisplayForRefresh_recursive();
			}
			else
			{
				ccConsole::Warning(QString("Failed to apply Laplacian smoothing to mesh '%1'").arg(mesh->getName()));
			}
		}
	}

	refreshAll();
	updateUI();
}

void MainWindow::RemoveSiblingsFromCCObjectList(ccHObject::Container& ccObjects)
{
	ccHObject::Container keptObjects;
	size_t count = ccObjects.size();
	keptObjects.reserve(count);

	for (size_t i=0; i<count; ++i)
	{
		ccHObject* obj = ccObjects[i];
		assert(obj);
		for (size_t j=0; j<count; ++j)
		{
			if (i != j)
			{
				if (ccObjects[j]->isAncestorOf(obj))
				{
					obj = 0;
					break;
				}
			}
		}

		if (obj)
			keptObjects.push_back(obj);
		else
			ccObjects[i] = 0;
	}

	ccObjects = keptObjects;
}

void MainWindow::doActionMerge()
{
	ccPointCloud* firstCloud = 0;
	ccHObjectContext firstCloudContext;

	//we deselect all selected entities (as they are going to disappear)
	ccHObject::Container _selectedEntities = m_selectedEntities;
	if (m_ccRoot)
		m_ccRoot->unselectAllEntities();

	//we will remove the useless clouds later
	ccHObject::Container toBeRemoved;

	size_t selNum = _selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = _selectedEntities[i];
		if (!ent)
			continue;

		//point clouds are simply added to the first selected ones
		//and then removed
		if (ent->isKindOf(CC_TYPES::POINT_CLOUD))
		{
			ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent);

			if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD)) //TODO
			{
				ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);

				if (!firstCloud)
				{
					firstCloud = pc;
					//we temporarily detach the first cloud, as it may undergo
					//"severe" modifications (octree deletion, etc.) --> see ccPointCloud::operator +=
					firstCloudContext = removeObjectTemporarilyFromDBTree(firstCloud);
				}
				else
				{
					unsigned beforePts = firstCloud->size();
					unsigned newPts = pc->size();
					*firstCloud += pc;

					//success?
					if (firstCloud->size() == beforePts + newPts)
					{
						firstCloud->prepareDisplayForRefresh_recursive();

						ccHObject* toRemove = 0;
						//if the entity to remove is a group with a unique child, we can remove it as well
						ccHObject* parent = pc->getParent();
						if (parent && parent->isA(CC_TYPES::HIERARCHY_OBJECT) && parent->getChildrenNumber() == 1 && parent!=firstCloudContext.parent)
							toRemove = parent;
						else
							toRemove = pc;

						//is a parent or sibling already in the "toBeRemoved" list?
						int j = 0;
						int count = static_cast<int>(toBeRemoved.size());
						while (j < count)
						{
							if (toBeRemoved[j]->isAncestorOf(toRemove))
							{
								toRemove = 0;
								break;
							}
							else if (toRemove->isAncestorOf(toBeRemoved[j]))
							{
								toBeRemoved[j] = toBeRemoved.back();
								toBeRemoved.pop_back();
								count--;
								j++;
							}
							else
							{
								//forward
								j++;
							}
						}
						if (toRemove)
							toBeRemoved.push_back(toRemove);
					}
					else
					{
						ccConsole::Error("Fusion failed! (not enough memory?)");
						break;
					}
					pc=0;
				}
			}
		}
		//meshes are placed in a common mesh group
		else if (ent->isKindOf(CC_TYPES::MESH))
		{
			ccConsole::Warning("Can't merge meshes yet! Sorry ...");
		}
		else
		{
			ccConsole::Warning(QString("Entity '%1' is neither a cloud nor a mesh, can't merge it!").arg(ent->getName()));
		}

		//security (we don't want to encounter it again)
		_selectedEntities[i]=0;
	}

	//something to remove?
	while (!toBeRemoved.empty())
	{
		if (toBeRemoved.back() && m_ccRoot)
			m_ccRoot->removeElement(toBeRemoved.back());
		toBeRemoved.pop_back();
	}

	if (firstCloud)
		putObjectBackIntoDBTree(firstCloud,firstCloudContext);

	refreshAll();
	updateUI();
}

void MainWindow::zoomOn(ccDrawableObject* object)
{
	ccGLWindow* win = static_cast<ccGLWindow*>(object->getDisplay());
	if (win)
	{
		ccBBox box = object->getBB(true,false,win);
		win->updateConstellationCenterAndZoom(&box);
	}
}

void MainWindow::doActionRegister()
{
	if (	m_selectedEntities.size() != 2
		||	(!m_selectedEntities[0]->isKindOf(CC_TYPES::POINT_CLOUD) && !m_selectedEntities[0]->isKindOf(CC_TYPES::MESH))
		||	(!m_selectedEntities[1]->isKindOf(CC_TYPES::POINT_CLOUD) && !m_selectedEntities[1]->isKindOf(CC_TYPES::MESH)) )
	{
		ccConsole::Error("Select 2 point clouds or meshes!");
		return;
	}

	ccHObject *data = static_cast<ccHObject*>(m_selectedEntities[1]);
	ccHObject *model = static_cast<ccHObject*>(m_selectedEntities[0]);

	ccRegistrationDlg rDlg(data,model,this);
	if (!rDlg.exec())
		return;

	//DGM (23/01/09): model and data order may have changed!
	model = rDlg.getModelEntity();
	data = rDlg.getDataEntity();

	double minErrorDecrease										= rDlg.getMinErrorDecrease();
	unsigned maxIterationCount									= rDlg.getMaxIterationCount();
	unsigned randomSamplingLimit								= rDlg.randomSamplingLimit();
	bool removeFarthestPoints									= rDlg.removeFarthestPoints();
	bool useDataSFAsWeights										= rDlg.useDataSFAsWeights();
	bool useModelSFAsWeights									= rDlg.useModelSFAsWeights();
	bool adjustScale											= rDlg.adjustScale();
	int transformationFilters									= rDlg.getTransformationFilters();
	CCLib::ICPRegistrationTools::CONVERGENCE_TYPE method		= rDlg.getConvergenceMethod();

	ccGLMatrix transMat;
	double finalError = 0.0;
	double finalScale = 1.0;

	if (ccRegistrationTools::ICP(	data,
									model,
									transMat,
									finalScale,
									finalError,
									minErrorDecrease,
									maxIterationCount,
									randomSamplingLimit,
									removeFarthestPoints,
									method,
									adjustScale,
									useDataSFAsWeights,
									useModelSFAsWeights,
									transformationFilters,
									this))
	{
		QString rmsString = QString("Final RMS: %1").arg(finalError);
		ccLog::Print(QString("[Register] ") + rmsString);

		QStringList summary;
		summary << rmsString;
		summary << "----------------";

		//transformation matrix
		{
			QString matString = transMat.toString();
			summary << QString("Transformation matrix");
			summary << transMat.toString(3,'\t'); //low precision, just for display
			summary << "----------------";

			ccLog::Print("[Register] Applied transformation matrix:");
			ccLog::Print(transMat.toString(12,' ')); //full precision
			ccLog::Print("Hint: copy it (CTRL+C) and apply it - or its inverse - on any entity with the 'Edit > Apply transformation' tool");
		}

		if (adjustScale)
		{
			QString scaleString = QString("Scale: %1 (already integrated in above matrix!)").arg(finalScale);
			ccLog::Warning(QString("[Register] ")+scaleString);
			summary << scaleString;
		}
		else
		{
			ccLog::Print(QString("[Register] Scale: fixed (1.0)"));
			summary << "Scale: fixed (1.0)";
		}

		summary << "----------------";
		summary << "Refer to Console (F8) for more details";

		//cloud to move
		ccGenericPointCloud* pc = 0;

		if (data->isKindOf(CC_TYPES::POINT_CLOUD))
		{
			pc = ccHObjectCaster::ToGenericPointCloud(data);
		}
		else if (data->isKindOf(CC_TYPES::MESH))
		{
			ccGenericMesh* mesh = ccHObjectCaster::ToGenericMesh(data);
			pc = mesh->getAssociatedCloud();

			//warning: point cloud is locked!
			if (pc->isLocked())
			{
				pc = 0;
				//we ask the user about cloning the 'data' mesh
				QMessageBox::StandardButton result = QMessageBox::question(	this,
																			"Registration",
																			"Data mesh vertices are locked (they may be shared with other meshes): Do you wish to clone this mesh to apply transformation?",
																			QMessageBox::Ok | QMessageBox::Cancel,
																			QMessageBox::Ok);

				//continue process?
				if (result == QMessageBox::Ok)
				{
					ccGenericMesh* newMesh = 0;
					if (mesh->isA(CC_TYPES::MESH))
						newMesh = static_cast<ccMesh*>(mesh)->clone();
					else
					{
						//FIXME TODO
						ccLog::Error("Doesn't work on sub-meshes yet!");
					}

					if (newMesh)
					{
						newMesh->setDisplay(data->getDisplay());
						addToDB(newMesh);
						data = newMesh;
						pc = newMesh->getAssociatedCloud();
					}
					else
					{
						ccLog::Error("Failed to clone 'data' mesh! (not enough memory?)");
					}
				}
			}
		}

		//if we managed to get a point cloud to move!
		if (pc)
		{
			//we temporarily detach cloud, as it may undergo
			//"severe" modifications (octree deletion, etc.) --> see ccPointCloud::applyRigidTransformation
			ccHObjectContext objContext = removeObjectTemporarilyFromDBTree(pc);
			pc->applyRigidTransformation(transMat);
			putObjectBackIntoDBTree(pc,objContext);

			//don't forget to update mesh bounding box also!
			if (data->isKindOf(CC_TYPES::MESH))
				ccHObjectCaster::ToGenericMesh(data)->refreshBB();

			//don't forget global shift
			ccGenericPointCloud* refPc = ccHObjectCaster::ToGenericPointCloud(model);
			if (refPc)
			{
				if (refPc->isShifted())
				{
					const CCVector3d& Pshift = refPc->getGlobalShift();
					const double& scale = refPc->getGlobalScale();
					pc->setGlobalShift(Pshift);
					pc->setGlobalScale(scale);
					ccLog::Warning(QString("[ICP] Aligned entity global shift has been updated to match the reference: (%1,%2,%3) [x%4]").arg(Pshift.x).arg(Pshift.y).arg(Pshift.z).arg(scale));
				}
				else if (pc->isShifted()) //we'll ask the user first before dropping the shift information on the aligned cloud
				{
					if (QMessageBox::question(this, "Drop shift information?", "Aligned entity is shifted but reference cloud is not: drop global shift information?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
					{
						pc->setGlobalShift(0,0,0);
						pc->setGlobalScale(1.0);
						ccLog::Warning(QString("[ICP] Aligned entity global shift has been reset to match the reference!"));
					}
				}
			}

			data->prepareDisplayForRefresh_recursive();
			data->setName(data->getName()+QString(".registered"));
			zoomOn(data);
		}

		//pop-up summary
		summary << "Refer to Console (F8) for more details";
		QMessageBox::information(this,"Register info",summary.join("\n"));
		forceConsoleDisplay();
	}

	refreshAll();
	updateUI();
}

//Aurelien BEY le 13/11/2008 : ajout de la fonction permettant de traiter la fonctionnalite de recalage grossier
void MainWindow::doAction4pcsRegister()
{
	if (QMessageBox::warning(	this,
								"Work in progress",
								"This method is still under development: are you sure you want to use it? (a crash may likely happen)",
								QMessageBox::Yes,QMessageBox::No) == QMessageBox::No )
								return;

	if (m_selectedEntities.size() != 2)
	{
		ccConsole::Error("Select 2 point clouds!");
		return;
	}

	if (!m_selectedEntities[0]->isKindOf(CC_TYPES::POINT_CLOUD) ||
		!m_selectedEntities[1]->isKindOf(CC_TYPES::POINT_CLOUD))
	{
		ccConsole::Error("Select 2 point clouds!");
		return;
	}

	ccGenericPointCloud *model = ccHObjectCaster::ToGenericPointCloud(m_selectedEntities[0]);
	ccGenericPointCloud *data = ccHObjectCaster::ToGenericPointCloud(m_selectedEntities[1]);

	ccAlignDlg aDlg(model, data);
	if (!aDlg.exec())
		return;

	model = aDlg.getModelObject();
	data = aDlg.getDataObject();

	//Take the correct number of points among the clouds
	CCLib::ReferenceCloud *subModel = aDlg.getSampledModel();
	CCLib::ReferenceCloud *subData = aDlg.getSampledData();

	unsigned nbMaxCandidates = aDlg.isNumberOfCandidatesLimited() ? aDlg.getMaxNumberOfCandidates() : 0;

	ccProgressDialog pDlg(true,this);

	CCLib::PointProjectionTools::Transformation transform;
	if (CCLib::FPCSRegistrationTools::RegisterClouds(	subModel,
														subData,
														transform,
														static_cast<ScalarType>(aDlg.getDelta()),
														static_cast<ScalarType>(aDlg.getDelta()/2),
														static_cast<PointCoordinateType>(aDlg.getOverlap()),
														aDlg.getNbTries(),
														5000,
														&pDlg,
														nbMaxCandidates))
	{
		//output resulting transformation matrix
		{
			ccGLMatrix transMat = FromCCLibMatrix<PointCoordinateType,float>(transform.R,transform.T);
			forceConsoleDisplay();
			ccConsole::Print("[Align] Resulting matrix:");
			ccConsole::Print(transMat.toString(12,' ')); //full precision
			ccConsole::Print("Hint: copy it (CTRL+C) and apply it - or its inverse - on any entity with the 'Edit > Apply transformation' tool");
		}

		ccPointCloud *newDataCloud=0;
		if (data->isA(CC_TYPES::POINT_CLOUD))
		{
			newDataCloud = static_cast<ccPointCloud*>(data)->cloneThis();
		}
		else
		{
			newDataCloud = ccPointCloud::From(data);
			newDataCloud->setGlobalShift(data->getGlobalShift());
			newDataCloud->setGlobalScale(data->getGlobalScale());
		}

		if (data->getParent())
			data->getParent()->addChild(newDataCloud);
		newDataCloud->setName(data->getName()+QString(".registered"));
		newDataCloud->applyTransformation(transform);
		newDataCloud->setDisplay(data->getDisplay());
		newDataCloud->prepareDisplayForRefresh();
		zoomOn(newDataCloud);
		addToDB(newDataCloud);

		data->setEnabled(false);
		data->prepareDisplayForRefresh_recursive();
	}
	else
	{
		ccConsole::Warning("[Align] Registration failed!");
	}

	if (subModel)
		delete subModel;
	if (subData)
		delete subData;

	refreshAll();
	updateUI();
}

void MainWindow::doActionSubsample()
{
	//find candidates
	std::vector<ccPointCloud*> clouds;
	unsigned maxPointCount = 0;
	double maxCloudRadius = 0;
	{
		for (size_t i=0; i<m_selectedEntities.size(); ++i)
		{
			if (m_selectedEntities[i]->isA(CC_TYPES::POINT_CLOUD))
			{
				ccPointCloud* cloud = static_cast<ccPointCloud*>(m_selectedEntities[i]);
				clouds.push_back(cloud);

				maxPointCount = std::max<unsigned>(maxPointCount, cloud->size());
				maxCloudRadius = std::max<double>(maxCloudRadius, cloud->getBB().getDiagNorm());
			}
		}
	}

	if (clouds.empty())
	{
		ccConsole::Error("Select at least one point cloud!");
		return;
	}

	//Display dialog
	ccSubsamplingDlg sDlg(maxPointCount, maxCloudRadius, this);
	if (!sDlg.exec())
		return;

	//process clouds
	{
		ccProgressDialog pDlg(false,this);
		pDlg.setMethodTitle("Subsampling");

		bool errors = false;

		QElapsedTimer eTimer;
		eTimer.start();

		for (size_t i=0; i<clouds.size(); ++i)
		{
			ccPointCloud* cloud = clouds[i];
			CCLib::ReferenceCloud *sampledCloud = sDlg.getSampledCloud(cloud,&pDlg);
			if (!sampledCloud)
			{
				ccConsole::Warning(QString("[Subsampling] Failed to subsample cloud '%1'!").arg(cloud->getName()));
				errors = true;
				continue;
			}

			int warnings = 0;
			ccPointCloud *newPointCloud = cloud->partialClone(sampledCloud,&warnings);
			
			delete sampledCloud;
			sampledCloud = 0;
			
			if (newPointCloud)
			{
				newPointCloud->setName(cloud->getName()+QString(".subsampled"));
				newPointCloud->setGlobalShift(cloud->getGlobalShift());
				newPointCloud->setGlobalScale(cloud->getGlobalScale());
				newPointCloud->setDisplay(cloud->getDisplay());
				newPointCloud->prepareDisplayForRefresh();
				if (cloud->getParent())
					cloud->getParent()->addChild(newPointCloud);
				cloud->setEnabled(false);
				addToDB(newPointCloud);

				newPointCloud->refreshDisplay();

				if (warnings)
				{
					ccLog::Warning("[Subsampling] Not enough memory: colors, normals or scalar fields may be missing!");
					errors = true;
				}
			}
			else
			{
				ccLog::Error("Not enough memory!");
				break;
			}
		}

		ccLog::Print("[Subsampling] Timing: %3.3f s.",eTimer.elapsed()/1000.0);

		if (errors)
		{
			ccLog::Error("Errors occurred (see console)");
		}
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionStatisticalTest()
{
	ccPickOneElementDlg pDlg("Distribution","Choose distribution",this);
	pDlg.addElement("Gauss");
	pDlg.addElement("Weibull");
	pDlg.setDefaultIndex(0);
	if (!pDlg.exec())
		return;

	int distribIndex = pDlg.getSelectedIndex();

	ccStatisticalTestDlg* sDlg = 0;
	switch (distribIndex)
	{
	case 0: //Gauss
		sDlg = new ccStatisticalTestDlg("mu","sigma",QString(),"Local Statistical Test (Gauss)",this);
		break;
	case 1: //Weibull
		sDlg = new ccStatisticalTestDlg("a","b","shift","Local Statistical Test (Weibull)",this);
		break;
	default:
		ccConsole::Error("Invalid distribution!");
		return;
	}

	if (sDlg->exec())
	{
		//build up corresponding distribution
		CCLib::GenericDistribution* distrib = 0;
		{
			ScalarType a = static_cast<ScalarType>(sDlg->getParam1());
			ScalarType b = static_cast<ScalarType>(sDlg->getParam2());
			ScalarType c = static_cast<ScalarType>(sDlg->getParam3());

			switch (distribIndex)
			{
			case 0: //Gauss
			{
				CCLib::NormalDistribution* N = new CCLib::NormalDistribution();
				N->setParameters(a,b*b); //warning: we input sigma2 here (not sigma)
				distrib = static_cast<CCLib::GenericDistribution*>(N);
				break;
			}
			case 1: //Weibull
				CCLib::WeibullDistribution* W = new CCLib::WeibullDistribution();
				W->setParameters(a,b,c);
				distrib = static_cast<CCLib::GenericDistribution*>(W);
				break;
			}
		}

		double pChi2 = sDlg->getProba();
		int nn = sDlg->getNeighborsNumber();

		size_t selNum = m_selectedEntities.size();
		for (size_t i=0; i<selNum; ++i)
		{
			ccPointCloud* pc = ccHObjectCaster::ToPointCloud(m_selectedEntities[i]); //TODO
			if (pc)
			{
				//we apply method on currently displayed SF
				ccScalarField* inSF = pc->getCurrentDisplayedScalarField();
				if (inSF)
				{
					assert(inSF->isAllocated());

					//force SF as 'OUT' field (in case of)
					int outSfIdx = pc->getCurrentDisplayedScalarFieldIndex();
					pc->setCurrentOutScalarField(outSfIdx);

					//force Chi2 Distances field as 'IN' field (create it by the way if necessary)
					int chi2SfIdx = pc->getScalarFieldIndexByName(CC_CHI2_DISTANCES_DEFAULT_SF_NAME);
					if (chi2SfIdx < 0)
						chi2SfIdx = pc->addScalarField(CC_CHI2_DISTANCES_DEFAULT_SF_NAME);
					if (chi2SfIdx < 0)
					{
						ccConsole::Error("Couldn't allocate a new scalar field for computing chi2 distances! Try to free some memory ...");
						break;
					}
					pc->setCurrentInScalarField(chi2SfIdx);

					//compute octree if necessary
					ccOctree* theOctree=pc->getOctree();
					if (!theOctree)
					{
						ccProgressDialog pDlg(true,this);
						theOctree = pc->computeOctree(&pDlg);
						if (!theOctree)
						{
							ccConsole::Error(QString("Couldn't compute octree for cloud '%1'!").arg(pc->getName()));
							break;
						}
					}

					ccProgressDialog pDlg(true,this);

					QElapsedTimer eTimer;
					eTimer.start();

					double chi2dist = CCLib::StatisticalTestingTools::testCloudWithStatisticalModel(distrib,pc,nn,pChi2,&pDlg,theOctree);

					ccConsole::Print("[Chi2 Test] Timing: %3.2f ms.",eTimer.elapsed()/1.0e3);
					ccConsole::Print("[Chi2 Test] %s test result = %f",distrib->getName(),chi2dist);

					//we set the theoretical Chi2 distance limit as the minimum displayed SF value so that all points below are grayed
					{
						ccScalarField* chi2SF = static_cast<ccScalarField*>(pc->getCurrentInScalarField());
						assert(chi2SF);
						chi2SF->computeMinAndMax();
						chi2dist *= chi2dist;
						chi2SF->setMinDisplayed(static_cast<ScalarType>(chi2dist));
						chi2SF->setSymmetricalScale(false);
						chi2SF->setSaturationStart(static_cast<ScalarType>(chi2dist));
						//chi2SF->setSaturationStop(chi2dist);
						pc->setCurrentDisplayedScalarField(chi2SfIdx);
						pc->showSF(true);
						pc->prepareDisplayForRefresh_recursive();
					}
				}
			}
		}

		delete distrib;
		distrib = 0;
	}

	delete sDlg;
	sDlg = 0;

	refreshAll();
	updateUI();
}

void MainWindow::doActionComputeStatParams()
{
	ccPickOneElementDlg pDlg("Distribution","Distribution Fitting",this);
	pDlg.addElement("Gauss");
	pDlg.addElement("Weibull");
	pDlg.setDefaultIndex(0);
	if (!pDlg.exec())
		return;

	CCLib::GenericDistribution* distrib = 0;
	{
		switch (pDlg.getSelectedIndex())
		{
		case 0: //GAUSS
			distrib = new CCLib::NormalDistribution();
			break;
		case 1: //WEIBULL
			distrib = new CCLib::WeibullDistribution();
			break;
		default:
			assert(false);
			return;
		}
	}
	assert(distrib);

	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccPointCloud* pc = ccHObjectCaster::ToPointCloud(m_selectedEntities[i]); //TODO
		if (pc)
		{
			//we apply method on currently displayed SF
			ccScalarField* sf = pc->getCurrentDisplayedScalarField();
			if (sf)
			{
				assert(sf->isAllocated());

				//force SF as 'OUT' field (in case of)
				int outSfIdx = pc->getCurrentDisplayedScalarFieldIndex();
				assert(outSfIdx >= 0);
				pc->setCurrentOutScalarField(outSfIdx);

				if (distrib->computeParameters(pc))
				{
					QString description;

					unsigned precision = ccGui::Parameters().displayedNumPrecision;
					switch (pDlg.getSelectedIndex())
					{
					case 0: //GAUSS
						{
							CCLib::NormalDistribution* normal = static_cast<CCLib::NormalDistribution*>(distrib);
							description = QString("mean = %1 / std.dev. = %2").arg(normal->getMu(),0,'f',precision).arg(sqrt(normal->getSigma2()),0,'f',precision);
						}
						break;
					case 1: //WEIBULL
						{
							CCLib::WeibullDistribution* weibull = static_cast<CCLib::WeibullDistribution*>(distrib);
							ScalarType a,b;
							weibull->getParameters(a,b);
							description = QString("a = %1 / b = %2 / shift = %3").arg(a,0,'f',precision).arg(b,0,'f',precision).arg(weibull->getValueShift(),0,'f',precision);
						}
						break;
					default:
						assert(false);
						return;
					}
					description.prepend(QString("%1: ").arg(distrib->getName()));
					ccConsole::Print(QString("[Distribution fitting] %1").arg(description));

					//Auto Chi2
					unsigned numberOfClasses = static_cast<unsigned>(ceil(sqrt(static_cast<double>(pc->size()))));
					std::vector<unsigned> histo;
					std::vector<double> npis;
					try
					{
						histo.resize(numberOfClasses,0);
						npis.resize(numberOfClasses,0.0);
					}
					catch(std::bad_alloc)
					{
						ccConsole::Warning("[Distribution fitting] Not enough memory!");
						continue;
					}

					unsigned finalNumberOfClasses = 0;
					double chi2dist = CCLib::StatisticalTestingTools::computeAdaptativeChi2Dist(distrib,pc,0,finalNumberOfClasses,false,0,0,&(histo[0]),&(npis[0]));

					if (chi2dist >= 0.0)
					{
						ccConsole::Print("[Distribution fitting] %s: Chi2 Distance = %f",distrib->getName(),chi2dist);
					}
					else
					{
						ccConsole::Warning("[Distribution fitting] Failed to compute Chi2 distance?!");
						continue;
					}

					//show histogram
					ccHistogramWindowDlg* hDlg = new ccHistogramWindowDlg(this);
					hDlg->setWindowTitle("[Distribution fitting]");
					{
						ccHistogramWindow* histogram = hDlg->window();
						histogram->fromBinArray(histo,sf->getMin(),sf->getMax());
						histo.clear();
						histogram->setCurveValues(npis);
						npis.clear();
						histogram->setTitle(description);
						histogram->setColorScheme(ccHistogramWindow::USE_CUSTOM_COLOR_SCALE);
						histogram->setColorScale(sf->getColorScale());
						histogram->setAxisLabels(sf->getName(),"Count");
						histogram->refresh();
					}
					hDlg->show();
				}
				else
				{
					ccConsole::Warning(QString("[Entity: %1]-[SF: %2] Couldn't compute distribution parameters!").arg(pc->getName()).arg(pc->getScalarFieldName(outSfIdx)));
				}
			}
		}
	}

	delete distrib;
	distrib = 0;
}

struct ComponentIndexAndSize
{
	unsigned index;
	unsigned size;

	ComponentIndexAndSize(unsigned i, unsigned s) : index(i), size(s) {}

	static bool DescendingCompOperator(const ComponentIndexAndSize& a, const ComponentIndexAndSize& b)
	{
		return a.size > b.size;
	}
};

void MainWindow::createComponentsClouds(ccGenericPointCloud* cloud,
										CCLib::ReferenceCloudContainer& components,
										unsigned minPointsPerComponent,
										bool randomColors,
										bool selectComponents,
										bool sortBysize/*=true*/)
{
	if (!cloud || components.empty())
		return;

	std::vector<ComponentIndexAndSize> sortedIndexes;
	std::vector<ComponentIndexAndSize>* _sortedIndexes = 0;
	if (sortBysize)
	{
		try
		{
			sortedIndexes.reserve(components.size());
		}
		catch (std::bad_alloc)
		{
			ccLog::Warning("[CreateComponentsClouds] Not enough memory to sort components by size!");
			sortBysize = false;
		}

		if (sortBysize) //still ok?
		{
			for (unsigned i=0; i<components.size(); ++i)
			{
				sortedIndexes.push_back(ComponentIndexAndSize(i,components[i]->size()));
			}

			std::sort(sortedIndexes.begin(), sortedIndexes.end(), ComponentIndexAndSize::DescendingCompOperator);
			_sortedIndexes = &sortedIndexes;
		}
	}

	//we create "real" point clouds for all input components
	{
		ccPointCloud* pc = cloud->isA(CC_TYPES::POINT_CLOUD) ? static_cast<ccPointCloud*>(cloud) : 0;

		//we create a new group to store all CCs
		ccHObject* ccGroup = new ccHObject(cloud->getName()+QString(" [CCs]"));

		//for each component
		for (unsigned i=0; i<components.size(); ++i)
		{
			CCLib::ReferenceCloud* compIndexes = _sortedIndexes ? components[_sortedIndexes->at(i).index] : components[i];

			//if it has enough points
			if (compIndexes->size() >= minPointsPerComponent)
			{
				//we create a new entity
				ccPointCloud* compCloud = (pc ? pc->partialClone(compIndexes) : ccPointCloud::From(compIndexes));
				if (compCloud)
				{
					//shall we colorize it with random color?
					if (randomColors)
					{
						colorType col[3];
						ccColor::Generator::Random(col);
						compCloud->setRGBColor(col);
						compCloud->showColors(true);
						compCloud->showSF(false);
					}

					//'shift on load' information
					if (pc)
					{
						compCloud->setGlobalShift(pc->getGlobalShift());
						compCloud->setGlobalScale(pc->getGlobalScale());
					}
					compCloud->setVisible(true);
					compCloud->setName(QString("CC#%1").arg(ccGroup->getChildrenNumber()));

					//we add new CC to group
					ccGroup->addChild(compCloud);

					if (selectComponents && m_ccRoot)
						m_ccRoot->selectEntity(compCloud,true);
				}
				else
				{
					ccConsole::Warning("[createComponentsClouds] Failed to create component #%i! (not enough memory)",ccGroup->getChildrenNumber()+1);
				}
			}

			delete compIndexes;
			compIndexes = 0;
		}

		components.clear();

		if (ccGroup->getChildrenNumber() == 0)
		{
			ccConsole::Error("No component was created! Check the minimum size...");
			delete ccGroup;
			ccGroup = 0;
		}
		else
		{
			ccGroup->setDisplay(cloud->getDisplay());
			addToDB(ccGroup);
		}

		ccConsole::Print(QString("[createComponentsClouds] %1 component(s) were created from cloud '%2'").arg(ccGroup->getChildrenNumber()).arg(cloud->getName()));

		cloud->prepareDisplayForRefresh();

		//auto-hide original cloud
		if (ccGroup)
		{
			cloud->setEnabled(false);
			ccConsole::Warning("[createComponentsClouds] Original cloud has been automatically hidden");
		}
	}
}

void MainWindow::doActionLabelConnectedComponents()
{
	ccHObject::Container selectedEntities = m_selectedEntities;
	//keep only the point clouds!
	std::vector<ccGenericPointCloud*> clouds;
	{
		size_t selNum = selectedEntities.size();
		for (size_t i=0; i<selNum; ++i)
		{
			ccHObject* ent = selectedEntities[i];
			if (ent->isKindOf(CC_TYPES::POINT_CLOUD))
				clouds.push_back(ccHObjectCaster::ToGenericPointCloud(ent));
		}
	}

	size_t count = clouds.size();
	if (count == 0)
		return;

	ccLabelingDlg dlg(this);
	if (count == 1)
		dlg.octreeLevelSpinBox->setCloud(clouds.front());
	if (!dlg.exec())
		return;

	int octreeLevel = dlg.getOctreeLevel();
	int minComponentSize = dlg.getMinPointsNb();
	bool randColors = dlg.randomColors();

	ccProgressDialog pDlg(false,this);

	//we unselect all entities as we are going to automatically select the created components
	//(otherwise the user won't percieve the change!)
	if (m_ccRoot)
		m_ccRoot->unselectAllEntities();

	for (size_t i=0; i<count; ++i)
	{
		ccGenericPointCloud* cloud = clouds[i];

		if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD)) //TODO
		{
			ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);

			ccOctree* theOctree = cloud->getOctree();
			if (!theOctree)
			{
				ccProgressDialog pDlg(true,this);
				theOctree = cloud->computeOctree(&pDlg);
				if (!theOctree)
				{
					ccConsole::Error(QString("Couldn't compute octree for cloud '%s'!").arg(cloud->getName()));
					break;
				}
			}

			//we create/activate CCs label scalar field
			int sfIdx = pc->getScalarFieldIndexByName(CC_CONNECTED_COMPONENTS_DEFAULT_LABEL_NAME);
			if (sfIdx < 0)
				sfIdx = pc->addScalarField(CC_CONNECTED_COMPONENTS_DEFAULT_LABEL_NAME);
			if (sfIdx < 0)
			{
				ccConsole::Error("Couldn't allocate a new scalar field for computing CC labels! Try to free some memory ...");
				break;
			}
			pc->setCurrentScalarField(sfIdx);

			//we try to label all CCs
			CCLib::ReferenceCloudContainer components;
			if (CCLib::AutoSegmentationTools::labelConnectedComponents(	cloud,
																		static_cast<uchar>(octreeLevel),
																		false,
																		&pDlg,
																		theOctree) >= 0)
			{
				//if successful, we extract each CC (stored in "components")
				pc->getCurrentInScalarField()->computeMinAndMax();
				if (!CCLib::AutoSegmentationTools::extractConnectedComponents(cloud,components))
				{
					ccConsole::Warning(QString("[doActionLabelConnectedComponents] Something went wrong while extracting CCs from cloud %1...").arg(cloud->getName()));
				}
			}
			else
			{
				ccConsole::Warning(QString("[doActionLabelConnectedComponents] Something went wrong while extracting CCs from cloud %1...").arg(cloud->getName()));
			}

			//we delete the CCs label scalar field (we don't need it anymore)
			pc->deleteScalarField(sfIdx);
			sfIdx = -1;

			//we create "real" point clouds for all CCs
			if (!components.empty())
			{
				createComponentsClouds(cloud, components, minComponentSize, randColors, true);
			}
		}
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionSetSFAsCoord()
{
	ccExportCoordToSFDlg ectsDlg(this);
	ectsDlg.warningLabel->setVisible(false);
	ectsDlg.setWindowTitle("Export SF to coordinate(s)");

	if (!ectsDlg.exec())
		return;

	bool exportDim[3] = {ectsDlg.exportX(), ectsDlg.exportY(), ectsDlg.exportZ()};
	if (!exportDim[0] && !exportDim[1] && !exportDim[2]) //nothing to do?!
		return;

	//for each selected cloud (or vertices set)
	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(m_selectedEntities[i]);
		if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD))
		{
			ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);

			ccScalarField* sf = pc->getCurrentDisplayedScalarField();
			if (sf)
			{
				unsigned ptsCount = pc->size();
				bool hasDefaultValueForNaN = false;
				ScalarType defaultValueForNaN = sf->getMin();

				for (unsigned i=0; i<ptsCount; ++i)
				{
					ScalarType s = sf->getValue(i);

					//handle NaN values
					if (!CCLib::ScalarField::ValidValue(s))
					{
						if (!hasDefaultValueForNaN)
						{
							bool ok;
							double out = QInputDialog::getDouble(this,"SF --> coordinate","Enter the coordinate equivalent for NaN values:",defaultValueForNaN,-DBL_MAX,DBL_MAX,6,&ok);
							if (ok)
								defaultValueForNaN = static_cast<ScalarType>(out);
							else
								ccLog::Warning("[SetSFAsCoord] By default the coordinate equivalent for NaN values will be the minimum SF value");
							hasDefaultValueForNaN = true;
						}
						s = defaultValueForNaN;
					}

					CCVector3* P = const_cast<CCVector3*>(pc->getPoint(i));

					//test each dimension
					for (unsigned d=0; d<3; ++d)
					{
						if (exportDim[d])
							P->u[d] = s;
					}
				}

				pc->invalidateBoundingBox();
			}
		}

	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionExportCoordToSF()
{
	ccExportCoordToSFDlg ectsDlg(this);

	if (!ectsDlg.exec())
		return;

	bool exportDim[3] = {ectsDlg.exportX(), ectsDlg.exportY(), ectsDlg.exportZ()};
	const QString defaultSFName[3] = {"Coord. X", "Coord. Y", "Coord. Z"};

	if (!exportDim[0] && !exportDim[1] && !exportDim[2]) //nothing to do?!
		return;

	//for each selected cloud (or vertices set)
	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(m_selectedEntities[i]);
		if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD))
		{
			ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
			unsigned ptsCount = pc->size();

			//test each dimension
			for (unsigned d=0; d<3; ++d)
			{
				if (exportDim[d])
				{
					int sfIndex = pc->getScalarFieldIndexByName(qPrintable(defaultSFName[d]));
					if (sfIndex < 0)
						sfIndex = pc->addScalarField(qPrintable(defaultSFName[d]));
					if (sfIndex < 0)
					{
						ccLog::Error("Not enough memory!");
						i = selNum;
						break;
					}

					CCLib::ScalarField* sf = pc->getScalarField(sfIndex);
					assert(sf && sf->currentSize() == ptsCount);
					if (sf)
					{
						for (unsigned k=0; k<ptsCount; ++k)
						{
							ScalarType s = static_cast<ScalarType>(pc->getPoint(k)->u[d]);
							sf->setValue(k,s);
						}
						sf->computeMinAndMax();
						pc->setCurrentDisplayedScalarField(sfIndex);
						m_selectedEntities[i]->showSF(true);
						m_selectedEntities[i]->refreshDisplay_recursive();
					}
				}
			}
		}

	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionHeightGridGeneration()
{
	size_t selNum = m_selectedEntities.size();
	if (selNum != 1)
	{
		ccConsole::Error("Select only one point cloud!");
		return;
	}

	ccHObject* ent = m_selectedEntities[0];
	if (!ent->isKindOf(CC_TYPES::POINT_CLOUD) )
	{
		ccConsole::Error("Select a point cloud!");
		return;
	}

	ccHeightGridGenerationDlg dlg(ent->getMyOwnBB(),this);
	if (!dlg.exec())
		return;

	bool generateCloud = dlg.generateCloud();
	bool generateCountSF = dlg.generateCountSF();
	bool resampleOriginalCloud = dlg.resampleOriginalCloud();
	bool generateImage = dlg.generateImage();
	bool generateASCII = dlg.generateASCII();
	bool generateRaster = dlg.generateRaster();

	if (!generateCloud && !generateImage && !generateASCII && !generateRaster)
	{
		ccConsole::Error("Nothing to do?! Mind the 'Generate' checkboxes...");
		return;
	}

	//Grid step must be > 0
	double gridStep = dlg.getGridStep();
	assert(gridStep > 0);
	//Custom bundig box
	ccBBox box = dlg.getCustomBBox();

	ccProgressDialog pDlg(true,this);
	ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent);

	//let's rock
	ccPointCloud* outputGrid = ccHeightGridGeneration::Compute(	cloud,
																gridStep,
																box,
																dlg.getProjectionDimension(),
																dlg.getTypeOfProjection(),
																dlg.getFillEmptyCellsStrategy(),
																dlg.getTypeOfSFInterpolation(),
																dlg.getCustomHeightForEmptyCells(),
																generateCloud,
																generateImage,
																generateRaster,
																generateASCII,
																generateCountSF,
																resampleOriginalCloud,
																&pDlg);

	//a cloud was demanded as output?
	if (outputGrid)
	{
		if (outputGrid->size() != 0)
		{
			if (cloud->getParent())
				cloud->getParent()->addChild(outputGrid);

			outputGrid->setName(QString("%1.heightGrid(%2)").arg(cloud->getName()).arg(gridStep,0,'g',3));
			outputGrid->setDisplay(cloud->getDisplay());
			outputGrid->prepareDisplayForRefresh();
			//zoomOn(outputGrid);
			addToDB(outputGrid);
			if (m_ccRoot)
				m_ccRoot->selectEntity(outputGrid);

			//don't forget original shift
			outputGrid->setGlobalShift(cloud->getGlobalShift());
			outputGrid->setGlobalScale(cloud->getGlobalScale());
			cloud->prepareDisplayForRefresh_recursive();
			cloud->setEnabled(false);
			ccConsole::Warning("Previously selected entity (source) has been hidden!");

			refreshAll();
			updateUI();
		}
		else
		{
			ccConsole::Warning("[doActionHeightGridGeneration] Output cloud was empty!");
			delete outputGrid;
			outputGrid = 0;
		}
	}
}

void MainWindow::doActionComputeMeshAA()
{
	doActionComputeMesh(GENERIC);
}

void MainWindow::doActionComputeMeshLS()
{
	doActionComputeMesh(GENERIC_BEST_LS_PLANE);
}

static double s_meshMaxEdgeLength = 0;
void MainWindow::doActionComputeMesh(CC_TRIANGULATION_TYPES type)
{
	bool ok = true;
	double maxEdgeLength = QInputDialog::getDouble(this,"Triangulate", "Max edge length (0 = no limit)", s_meshMaxEdgeLength, 0, DBL_MAX, 8, &ok);
	if (!ok)
		return;
	s_meshMaxEdgeLength = maxEdgeLength;

	//select candidates
	ccHObject::Container clouds;
	bool hadNormals = false;
	{
		for (size_t i=0; i<m_selectedEntities.size(); ++i)
		{
			ccHObject* ent = m_selectedEntities[i];
			if (ent->isKindOf(CC_TYPES::POINT_CLOUD))
			{
				clouds.push_back(ent);
				if (ent->isA(CC_TYPES::POINT_CLOUD))
					hadNormals |= static_cast<ccPointCloud*>(ent)->hasNormals();
			}
		}
	}

	//if the cloud(s) already had normals, ask the use if wants to update them or keep them as is (can look strange!)
	bool updateNormals = false;
	if (hadNormals)
	{
		updateNormals = (QMessageBox::question(	this,
												"Keep old normals?",
												"Cloud(s) already have normals. Do you want to update them (yes) or keep the old ones (no)?",
												QMessageBox::Yes,
												QMessageBox::No ) == QMessageBox::Yes);
	}

	QProgressDialog pDlg("Triangulation in progress...", QString(), 0, 0, this);
	pDlg.show();
	QApplication::processEvents();

	for (size_t i=0; i<clouds.size(); ++i)
	{
		ccHObject* ent = clouds[i];
		assert(ent->isKindOf(CC_TYPES::POINT_CLOUD));

		//compute mesh
		ccMesh* mesh = 0;
		{
			ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent);
			//compute raw mesh
			CCLib::GenericIndexedMesh* dummyMesh = CCLib::PointProjectionTools::computeTriangulation(	cloud,
																										type,
																										static_cast<PointCoordinateType>(maxEdgeLength));
			if (dummyMesh)
			{
				//convert raw mesh to ccMesh
				mesh = new ccMesh(dummyMesh, cloud);

				//don't need it anymore
				delete dummyMesh;
				dummyMesh = 0;

				if (mesh)
				{
					mesh->setName(cloud->getName()+QString(".mesh"));
					mesh->setDisplay(cloud->getDisplay());
					bool cloudHadNormals = cloud->hasNormals();
					if (!cloudHadNormals || (ent->isA(CC_TYPES::POINT_CLOUD) && updateNormals))
					{
						//compute per-vertex normals by default
						mesh->computeNormals(true);
					}
					mesh->showNormals(cloudHadNormals || !cloud->hasColors());
					cloud->setVisible(false);
					cloud->addChild(mesh);
					cloud->prepareDisplayForRefresh();
					if (mesh->getAssociatedCloud() && mesh->getAssociatedCloud() != cloud)
					{
						mesh->getAssociatedCloud()->setGlobalShift(cloud->getGlobalShift());
						mesh->getAssociatedCloud()->setGlobalScale(cloud->getGlobalScale());
					}
					addToDB(mesh);
					if (i == 0)
						m_ccRoot->selectEntity(mesh); //auto-select first element
				}
			}
		}

		if (!mesh)
		{
			ccConsole::Error("An error occurred while computing mesh! (not enough memory?)");
			break;
		}
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionFitQuadric()
{
	ccHObject::Container selectedEntities = m_selectedEntities;
	size_t selNum = selectedEntities.size();

	bool errors = false;
	//for all selected entities
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = selectedEntities[i];
		//look for clouds
		if (ent->isKindOf(CC_TYPES::POINT_CLOUD))
		{
			ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent);

			double rms = 0;
			ccQuadric* quadric = ccQuadric::Fit(cloud,&rms);
			if (quadric)
			{
				cloud->addChild(quadric);
				quadric->setName(QString("Quadric (%1)").arg(cloud->getName()));
				quadric->setDisplay(cloud->getDisplay());
				quadric->prepareDisplayForRefresh();
				addToDB(quadric);

				ccConsole::Print(QString("[doActionFitQuadric] Quadric equation: ") + quadric->getEquationString());
				ccConsole::Print(QString("[doActionFitQuadric] RMS: %1").arg(rms));

			}
			else
			{
				ccConsole::Warning(QString("Failed to compute quadric on cloud '%1'").arg(cloud->getName()));
				errors = true;
			}
		}
	}

	if (errors)
	{
		ccConsole::Error("Error(s) occurred: see console");
	}

	refreshAll();
}

void MainWindow::doActionComputeDistToBestFitQuadric3D()
{
	ccHObject::Container selectedEntities = m_selectedEntities;

	bool ok = true;
	int steps = QInputDialog::getInt(this,"Distance to best fit quadric (3D)","Steps (per dim.)",50,10,10000,10,&ok);
	if (!ok)
		return;

	size_t selNum = selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = selectedEntities[i];
		if (ent->isKindOf(CC_TYPES::POINT_CLOUD))
		{
			ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent);
			CCLib::Neighbourhood Yk(cloud);

			const double* Q = Yk.get3DQuadric();
			if (Q)
			{
				const double& a = Q[0];
				const double& b = Q[1];
				const double& c = Q[2];
				const double& e = Q[3];
				const double& f = Q[4];
				const double& g = Q[5];
				const double& l = Q[6];
				const double& m = Q[7];
				const double& n = Q[8];
				const double& d = Q[9];

				//gravity center
				const CCVector3* G = Yk.getGravityCenter();
				if (!G)
				{
					ccConsole::Warning(QString("Failed to get gravity center of cloud '%1'!").arg(cloud->getName()));
					continue;
				}

				const ccBBox bbox = cloud->getBB();
				PointCoordinateType maxDim = bbox.getMaxBoxDim();
				CCVector3 C = bbox.getCenter();

				//Sample points on a cube and compute for each of them the distance to the Quadric
				ccPointCloud* newCloud = new ccPointCloud();
				if (!newCloud->reserve(steps*steps*steps))
				{
					ccConsole::Error("Not enough memory!");
				}

				const char defaultSFName[] = "Dist. to 3D quadric";
				int sfIdx = newCloud->getScalarFieldIndexByName(defaultSFName);
				if (sfIdx < 0)
					sfIdx = newCloud->addScalarField(defaultSFName);
				if (sfIdx < 0)
				{
					ccConsole::Error("Couldn't allocate a new scalar field for computing distances! Try to free some memory ...");
					delete newCloud;
					continue;
				}

				ccScalarField* sf = static_cast<ccScalarField*>(newCloud->getScalarField(sfIdx));
				assert(sf);

				//FILE* fp = fopen("doActionComputeQuadric3D_trace.txt","wt");
				unsigned count = 0;
				for (int x=0; x<steps; ++x)
				{
					CCVector3 P;
					P.x = C.x + maxDim * (static_cast<PointCoordinateType>(x) / static_cast<PointCoordinateType>(steps-1) - PC_ONE/2);
					for (int y=0; y<steps; ++y)
					{
						P.y = C.y + maxDim * (static_cast<PointCoordinateType>(y) / static_cast<PointCoordinateType>(steps-1) - PC_ONE/2);
						for (int z=0; z<steps; ++z)
						{
							P.z = C.z + maxDim * (static_cast<PointCoordinateType>(z) / static_cast<PointCoordinateType>(steps-1) - PC_ONE/2);
							newCloud->addPoint(P);

							//compute distance to quadric
							CCVector3 Pc = P-*G;
							ScalarType dist = static_cast<ScalarType>(	a*Pc.x*Pc.x + b*Pc.y*Pc.y + c*Pc.z*Pc.z
																	+	e*Pc.x*Pc.y + f*Pc.y*Pc.z + g*Pc.x*Pc.z
																	+	l*Pc.x + m*Pc.y + n*Pc.z + d);

							sf->setValue(count++,dist);
							//fprintf(fp,"%f %f %f %f\n",Pc.x,Pc.y,Pc.z,dist);
						}
					}
				}
				//fclose(fp);

				if (sf)
				{
					sf->computeMinAndMax();
					newCloud->setCurrentDisplayedScalarField(sfIdx);
					newCloud->showSF(true);
				}
				newCloud->setName("Distance map to 3D quadric");
				newCloud->setDisplay(cloud->getDisplay());
				newCloud->prepareDisplayForRefresh();

				addToDB(newCloud);
			}
			else
			{
				ccConsole::Warning(QString("Failed to compute 3D quadric on cloud '%1'").arg(cloud->getName()));
			}
		}
	}

	refreshAll();
}

void MainWindow::doActionComputeCPS()
{
	size_t selNum = m_selectedEntities.size();
	if (selNum != 2)
	{
		ccConsole::Error("Select 2 point clouds!");
		return;
	}

	if (!m_selectedEntities[0]->isKindOf(CC_TYPES::POINT_CLOUD) ||
		!m_selectedEntities[1]->isKindOf(CC_TYPES::POINT_CLOUD))
	{
		ccConsole::Error("Select 2 point clouds!");
		return;
	}

	ccOrderChoiceDlg dlg(	m_selectedEntities[0], "Compared",
							m_selectedEntities[1], "Reference",
							this );
	if (!dlg.exec())
		return;

	ccGenericPointCloud* compCloud = ccHObjectCaster::ToGenericPointCloud(dlg.getFirstEntity());
	ccGenericPointCloud* srcCloud = ccHObjectCaster::ToGenericPointCloud(dlg.getSecondEntity());

	if (!compCloud->isA(CC_TYPES::POINT_CLOUD)) //TODO
	{
		ccConsole::Error("Compared cloud must be a real point cloud!");
		return;
	}
	ccPointCloud* cmpPC = static_cast<ccPointCloud*>(compCloud);

	int sfIdx = cmpPC->getScalarFieldIndexByName("tempScalarField");
	if (sfIdx < 0)
		sfIdx = cmpPC->addScalarField("tempScalarField");
	if (sfIdx < 0)
	{
		ccConsole::Error("Couldn't allocate a new scalar field for computing distances! Try to free some memory ...");
		return;
	}
	cmpPC->setCurrentScalarField(sfIdx);
	cmpPC->enableScalarField();
	cmpPC->forEach(CCLib::ScalarFieldTools::SetScalarValueToNaN);

	CCLib::ReferenceCloud CPSet(srcCloud);
	ccProgressDialog pDlg(true,this);
	CCLib::DistanceComputationTools::Cloud2CloudDistanceComputationParams params;
	params.CPSet = &CPSet;
	int result = CCLib::DistanceComputationTools::computeHausdorffDistance(compCloud,srcCloud,params,&pDlg);
	cmpPC->deleteScalarField(sfIdx);

	if (result >= 0)
	{
		ccPointCloud* newCloud = 0;
		//if the source cloud is a "true" cloud, the extracted CPS
		//will also get its attributes
		if (srcCloud->isA(CC_TYPES::POINT_CLOUD))
			newCloud = static_cast<ccPointCloud*>(srcCloud)->partialClone(&CPSet);
		else
		{
			newCloud = ccPointCloud::From(&CPSet);
			newCloud->setGlobalShift(srcCloud->getGlobalShift());
			newCloud->setGlobalScale(srcCloud->getGlobalScale());
		}

		newCloud->setName(QString("[%1]->CPSet(%2)").arg(srcCloud->getName()).arg(compCloud->getName()));
		newCloud->setDisplay(compCloud->getDisplay());
		newCloud->prepareDisplayForRefresh();
		addToDB(newCloud);

		//we hide the source cloud (for a clearer display)
		srcCloud->setEnabled(false);
		srcCloud->prepareDisplayForRefresh();
	}

	refreshAll();
}

void MainWindow::doActionComputeNormals()
{
	if (m_selectedEntities.empty())
	{
		ccConsole::Error("Select at least one point cloud");
		return;
	}

	size_t count = m_selectedEntities.size();
	PointCoordinateType defaultRadius = 0;
	bool onlyMeshes = true;
	bool hasMeshes = false;
	bool hasSubMeshes = false;
	for (size_t i=0; i<count; ++i)
	{
		if (!m_selectedEntities[i]->isKindOf(CC_TYPES::MESH))
		{
			if (defaultRadius == 0 && m_selectedEntities[i]->isA(CC_TYPES::POINT_CLOUD))
			{
				ccPointCloud* cloud = static_cast<ccPointCloud*>(m_selectedEntities[i]);
				defaultRadius = cloud->getBB().getMaxBoxDim() * static_cast<PointCoordinateType>(0.01); //diameter=1% of the bounding box max dim
			}
			onlyMeshes = false;
			break;
		}
		else
		{
			if (m_selectedEntities[i]->isA(CC_TYPES::MESH))
				hasMeshes = true;
			else
				hasSubMeshes = true;
		}
	}

	if (hasSubMeshes)
	{
		ccConsole::Error(QString("Can't compute normals on sub-meshes! Select the parent mesh instead"));
		return;
	}

	CC_LOCAL_MODEL_TYPES model = NO_MODEL;
	int preferedOrientation = -1;

	//We display dialog only for point clouds
	if (!onlyMeshes)
	{
		ccNormalComputationDlg ncDlg(this);
		ncDlg.setRadius(defaultRadius);

		if (!ncDlg.exec())
			return;

		model = ncDlg.getLocalModel();
		preferedOrientation = ncDlg.getPreferedOrientation();
		defaultRadius = ncDlg.getRadius();
	}

	bool perVertex = true;
	if (hasMeshes)
	{
		perVertex = (QMessageBox::question(	this,
											"Mesh normals",
											"Compute per-vertex normals (yes) or per-triangle (no)?",
											QMessageBox::Yes,
											QMessageBox::No ) == QMessageBox::Yes);
	}

	//Compute normals for each selected cloud
	for (size_t i=0; i<m_selectedEntities.size(); i++)
	{
		if (m_selectedEntities[i]->isA(CC_TYPES::POINT_CLOUD))
		{
			ccPointCloud* cloud = static_cast<ccPointCloud*>(m_selectedEntities[i]);

			ccProgressDialog pDlg(true,this);

			if (!cloud->getOctree())
				if (!cloud->computeOctree(&pDlg))
				{
					ccConsole::Error(QString("Could not compute octree for cloud '%1'").arg(cloud->getName()));
					continue;
				}

			//computes cloud normals
			QElapsedTimer eTimer;
			eTimer.start();
			NormsIndexesTableType* normsIndexes = new NormsIndexesTableType;
			if (!ccNormalVectors::ComputeCloudNormals(cloud, *normsIndexes, model, defaultRadius, preferedOrientation, (CCLib::GenericProgressCallback*)&pDlg, cloud->getOctree()))
			{
				ccConsole::Error(QString("Failed to compute normals on cloud '%1'").arg(cloud->getName()));
				continue;
			}
			ccConsole::Print("[ComputeCloudNormals] Timing: %3.2f s.",eTimer.elapsed()/1.0e3);

			if (!cloud->hasNormals())
			{
				if (!cloud->resizeTheNormsTable())
				{
					ccConsole::Error(QString("Failed to instantiate normals array on cloud '%1'").arg(cloud->getName()));
					continue;
				}
			}
			else
			{
				//we hide normals during process
				cloud->showNormals(false);
			}

			for (unsigned j=0; j<normsIndexes->currentSize(); j++)
				cloud->setPointNormalIndex(j, normsIndexes->getValue(j));

			normsIndexes->release();
			normsIndexes = 0;

			cloud->showNormals(true);
			cloud->prepareDisplayForRefresh();
		}
		else if (m_selectedEntities[i]->isA(CC_TYPES::MESH)/*|| m_selectedEntities[i]->isA(CC_TYPES::PRIMITIVE)*/) //TODO
		{
			ccMesh* mesh = ccHObjectCaster::ToMesh(m_selectedEntities[i]);
			mesh->clearTriNormals();
			mesh->showNormals(false);
			if (!mesh->computeNormals(perVertex))
			{
				ccConsole::Error(QString("Failed to compute normals on mesh '%1'").arg(mesh->getName()));
				continue;
			}
			mesh->prepareDisplayForRefresh_recursive();
		}
	}

	//ask the user if we wants to orient cloud normals (with MST)
	if (	!onlyMeshes
		&&	preferedOrientation < 0
		&&	QMessageBox::question(	this,
									"Orient normals",
									"Orient normals with Minimum Spanning Tree?",
									QMessageBox::Yes,QMessageBox::No) == QMessageBox::Yes)
	{
		doActionOrientNormalsMST();
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionOrientNormalsMST()
{
	if (m_selectedEntities.empty())
	{
		ccConsole::Error("Select at least one point cloud");
		return;
	}

	ccProgressDialog pDlg(true,this);

	bool success = false;
	for (size_t i=0; i<m_selectedEntities.size(); i++)
	{
		if (!m_selectedEntities[i]->isA(CC_TYPES::POINT_CLOUD))
			continue;

		ccPointCloud* cloud = static_cast<ccPointCloud*>(m_selectedEntities[i]);
		if (!cloud->hasNormals())
		{
			ccConsole::Warning(QString("Cloud '%1' has no normals!").arg(cloud->getName()));
			continue;
		}

		//use Minimum Spanning Tree to resolve normals direction
		if (ccMinimumSpanningTreeForNormsDirection::Process(cloud,&pDlg,cloud->getOctree()))
		{
			cloud->prepareDisplayForRefresh();
			success = true;
		}
		else
		{
			ccConsole::Error(QString("Process failed on cloud '%1'").arg(cloud->getName()));
			break;
		}
	}

	if (success)
		ccLog::Warning("Normals have been oriented: you may still have to globally invert the cloud normals however (Edit > Normals > Invert).");

	refreshAll();
	updateUI();
}

void MainWindow::doActionOrientNormalsFM()
{
	if (m_selectedEntities.empty())
	{
		ccConsole::Error("Select at least one point cloud");
		return;
	}

	bool ok;
	int value = QInputDialog::getInt(this,"Orient normals (FM)", "Octree level", 5, 1, CCLib::DgmOctree::MAX_OCTREE_LEVEL, 1, &ok);
	if (!ok)
		return;
	assert(value >= 0 && value <= 255);
	uchar level = static_cast<uchar>(value);

	ccProgressDialog pDlg(false,this);

	bool success = false;
	for (size_t i=0; i<m_selectedEntities.size(); i++)
	{
		if (!m_selectedEntities[i]->isA(CC_TYPES::POINT_CLOUD))
			continue;

		ccPointCloud* cloud = static_cast<ccPointCloud*>(m_selectedEntities[i]);
		if (!cloud->hasNormals())
		{
			ccConsole::Warning(QString("Cloud '%1' has no normals!").arg(cloud->getName()));
			continue;
		}

		if (!cloud->getOctree())
		{
			if (!cloud->computeOctree((CCLib::GenericProgressCallback*)&pDlg))
			{
				ccConsole::Error(QString("Could not compute octree for cloud '%1'").arg(cloud->getName()));
				continue;
			}
		}

		unsigned pointCount = cloud->size();

		NormsIndexesTableType* normsIndexes = new NormsIndexesTableType;
		if (!normsIndexes->reserve(pointCount))
		{
			ccConsole::Error(QString("Not engouh memory! (cloud '%1')").arg(cloud->getName()));
			continue;
		}

		//init array with current normals
		for (unsigned j=0; j<pointCount; j++)
		{
			const normsType& index = cloud->getPointNormalIndex(j);
			normsIndexes->addElement(index);
		}

		//apply algorithm
		ccFastMarchingForNormsDirection::ResolveNormsDirectionByFrontPropagation(cloud, normsIndexes, level, (CCLib::GenericProgressCallback*)&pDlg, cloud->getOctree());

		//compress resulting normals and transfer them to the cloud
		for (unsigned j=0; j<pointCount; j++)
			cloud->setPointNormalIndex(j, normsIndexes->getValue(j));

		normsIndexes->release();
		normsIndexes=0;

		cloud->prepareDisplayForRefresh();
		success = true;
	}

	if (success)
		ccLog::Warning("Normals have been oriented: you may still have to globally invert the cloud normals however (Edit > Normals > Invert).");

	refreshAll();
	updateUI();
}

static int s_innerRectDim = 2;
void MainWindow::doActionFindBiggestInnerRectangle()
{
	size_t selNum = m_selectedEntities.size();
	if (selNum == 0)
		return;

	ccHObject* entity = m_selectedEntities.size() == 1 ? m_selectedEntities[0] : 0;
	if (!entity || !entity->isKindOf(CC_TYPES::POINT_CLOUD))
	{
		ccConsole::Error("Select one point cloud!");
		return;
	}

	bool ok;
	int dim = QInputDialog::getInt(this,"Dimension","Orthogonal dim (X=0 / Y=1 / Z=2)",s_innerRectDim,0,2,1,&ok);
	if (!ok)
		return;
	s_innerRectDim = dim;

	ccGenericPointCloud* cloud = static_cast<ccGenericPointCloud*>(entity);
	ccBox* box = ccInnerRect2DFinder().process(cloud,static_cast<unsigned char>(dim));

	if (box)
	{
		cloud->addChild(box);
		box->setVisible(true);
		box->setDisplay(cloud->getDisplay());
		box->setDisplay(cloud->getDisplay());
		addToDB(box);
	}

	updateUI();
}

void MainWindow::doActionMatchBBCenters()
{
	size_t selNum = m_selectedEntities.size();

	//we need at least 2 entities
	if (selNum < 2)
		return;

	//we must backup 'm_selectedEntities' as removeObjectTemporarilyFromDBTree can modify it!
	ccHObject::Container selectedEntities = m_selectedEntities;

	//by default, we take the first entity as reference
	//TODO: maybe the user would like to select the reference himself ;)
	ccHObject* refEnt = selectedEntities[0];
	CCVector3 refCenter = refEnt->getBBCenter();

	for (size_t i=1; i<selNum; ++i)
	{
		ccHObject* ent = selectedEntities[i];
		CCVector3 center = ent->getBBCenter();

		CCVector3 T = refCenter-center;

		//transformation (used only for translation)
		ccGLMatrix glTrans;
		glTrans += T;

		forceConsoleDisplay();
		ccConsole::Print(QString("[Synchronize] Transformation matrix (%1 --> %2):").arg(ent->getName()).arg(selectedEntities[0]->getName()));
		ccConsole::Print(glTrans.toString(12,' ')); //full precision
		ccConsole::Print("Hint: copy it (CTRL+C) and apply it - or its inverse - on any entity with the 'Edit > Apply transformation' tool");

		//we temporarily detach entity, as it may undergo
		//"severe" modifications (octree deletion, etc.) --> see ccHObject::applyGLTransformation
		ccHObjectContext objContext = removeObjectTemporarilyFromDBTree(ent);
		ent->applyGLTransformation_recursive(&glTrans);
		putObjectBackIntoDBTree(ent,objContext);

		ent->prepareDisplayForRefresh_recursive();
	}

	zoomOnSelectedEntities();

	updateUI();
}

void MainWindow::doActionUnroll()
{
	//there should be only one point cloud with sensor in current selection!
	if (m_selectedEntities.empty() || m_selectedEntities.size()>1)
	{
		ccConsole::Error("Select one and only one entity!");
		return;
	}

	//if selected entity is a mesh, the method will be applied to its vertices
	bool lockedVertices;
	ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(m_selectedEntities[0],&lockedVertices);
	if (lockedVertices)
	{
		DisplayLockedVerticesWarning();
		return;
	}

	//for "real" point clouds only
	if (!cloud || !cloud->isA(CC_TYPES::POINT_CLOUD))
	{
		ccConsole::Error("Method can't be applied on locked vertices or virtual point clouds!");
		return;
	}
	ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);

	ccUnrollDlg unrollDlg(this);
	if (!unrollDlg.exec())
		return;

	int mode = unrollDlg.getType();
	PointCoordinateType radius = static_cast<PointCoordinateType>(unrollDlg.getRadius());
	double angle = unrollDlg.getAngle();
	unsigned char dim = (unsigned char)unrollDlg.getAxisDimension();
	CCVector3* pCenter = 0;
	CCVector3 center;
	if (mode == 1 || !unrollDlg.isAxisPositionAuto())
	{
		center = unrollDlg.getAxisPosition();
		pCenter = &center;
	}

	//We apply unrolling method
	ccProgressDialog pDlg(true,this);

	if (mode == 0)
		pc->unrollOnCylinder(radius,pCenter,dim,(CCLib::GenericProgressCallback*)&pDlg);
	else if (mode == 1)
		pc->unrollOnCone(radius,angle,center,dim,(CCLib::GenericProgressCallback*)&pDlg);
	else
		assert(false);

	ccGLWindow* win = static_cast<ccGLWindow*>(cloud->getDisplay());
	if (win)
		win->updateConstellationCenterAndZoom();
	updateUI();
}

ccGLWindow *MainWindow::getActiveGLWindow()
{
	if (!m_mdiArea)
		return 0;

	QMdiSubWindow *activeSubWindow = m_mdiArea->activeSubWindow();
	if (activeSubWindow)
		return static_cast<ccGLWindow*>(activeSubWindow->widget());
	else
	{
		QList<QMdiSubWindow*> subWindowList = m_mdiArea->subWindowList();
		if (!subWindowList.isEmpty())
			return static_cast<ccGLWindow*>(subWindowList[0]->widget());
	}

	return 0;
}

QMdiSubWindow* MainWindow::getMDISubWindow(ccGLWindow* win)
{
	QList<QMdiSubWindow*> subWindowList = m_mdiArea->subWindowList();
	for (int i=0; i<subWindowList.size(); ++i)
		if (static_cast<ccGLWindow*>(subWindowList[i]->widget()) == win)
			return subWindowList[i];

	//not found!
	return 0;
}

ccGLWindow* MainWindow::new3DView()
{
	assert(m_ccRoot && m_mdiArea);

	//already existing window?
	QList<QMdiSubWindow*> subWindowList = m_mdiArea->subWindowList();
	ccGLWindow* otherWin=0;
	if (!subWindowList.isEmpty())
		otherWin=static_cast<ccGLWindow*>(subWindowList[0]->widget());

	QGLFormat format = QGLFormat::defaultFormat();
	format.setStencil(false);
	format.setSwapInterval(0);
	ccGLWindow *view3D = new ccGLWindow(this,format,otherWin); //We share OpenGL contexts between windows!
	view3D->setMinimumSize(400,300);
	view3D->resize(500,400);

	m_mdiArea->addSubWindow(view3D);

	connect(view3D,	SIGNAL(entitySelectionChanged(int)),				m_ccRoot,	SLOT(selectEntity(int)));
	connect(view3D,	SIGNAL(entitiesSelectionChanged(std::set<int>)),	m_ccRoot,	SLOT(selectEntities(std::set<int>)));

	//'echo' mode
	connect(view3D,	SIGNAL(mouseWheelRotated(float)),					this,		SLOT(echoMouseWheelRotate(float)));
	connect(view3D,	SIGNAL(cameraDisplaced(float,float)),				this,		SLOT(echoCameraDisplaced(float,float)));
	connect(view3D,	SIGNAL(viewMatRotated(const ccGLMatrixd&)),			this,		SLOT(echoBaseViewMatRotation(const ccGLMatrixd&)));
	connect(view3D,	SIGNAL(cameraPosChanged(const CCVector3d&)),		this,		SLOT(echoCameraPosChanged(const CCVector3d&)));
	connect(view3D,	SIGNAL(pivotPointChanged(const CCVector3d&)),		this,		SLOT(echoPivotPointChanged(const CCVector3d&)));
	connect(view3D,	SIGNAL(pixelSizeChanged(float)),					this,		SLOT(echoPixelSizeChanged(float)));

	connect(view3D,	SIGNAL(destroyed(QObject*)),						this,		SLOT(prepareWindowDeletion(QObject*)));
	connect(view3D,	SIGNAL(filesDropped(const QStringList&)),			this,		SLOT(addToDBAuto(const QStringList&)));
	connect(view3D,	SIGNAL(newLabel(ccHObject*)),						this,		SLOT(handleNewLabel(ccHObject*)));

	view3D->setSceneDB(m_ccRoot->getRootEntity());
	view3D->setAttribute(Qt::WA_DeleteOnClose);
	m_ccRoot->updatePropertiesView();

	QMainWindow::statusBar()->showMessage(QString("New 3D View"), 2000);

	view3D->showMaximized();

	return view3D;
}

void MainWindow::prepareWindowDeletion(QObject* glWindow)
{
	if (!m_ccRoot)
		return;

	//we assume only ccGLWindow can be connected to this slot!
	ccGLWindow* win = qobject_cast<ccGLWindow*>(glWindow);

	m_ccRoot->hidePropertiesView();
	m_ccRoot->getRootEntity()->removeFromDisplay_recursive(win);
	m_ccRoot->updatePropertiesView();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	//if (m_uiFrozen)
	//{
	//	ccConsole::Error("Close current dialog/interactor first!");
	//	event->ignore();
	//}
	//else
	{
		if (m_ccRoot && m_ccRoot->getRootEntity()->getChildrenNumber() == 0
			|| QMessageBox::question(	this,
										"Quit",
										"Are you sure you want to quit?",
										QMessageBox::Ok,QMessageBox::Cancel ) != QMessageBox::Cancel)
		{
			event->accept();
		}
		else
		{
			event->ignore();
		}
	}

	//save the state as settings
	QSettings settings;
	settings.setValue(s_psMainWinGeom, saveGeometry());
	settings.setValue(s_psMainWinState, saveState());
}

void MainWindow::moveEvent(QMoveEvent* event)
{
	QMainWindow::moveEvent(event);

	updateMDIDialogsPlacement();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);

	updateMDIDialogsPlacement();
}

void MainWindow::registerMDIDialog(ccOverlayDialog* dlg, Qt::Corner pos)
{
	//check for existence
	for (size_t i=0; i<m_mdiDialogs.size(); ++i)
	{
		if (m_mdiDialogs[i].dialog == dlg)
		{
			//we only update position in this case
			m_mdiDialogs[i].position = pos;
			return;
		}
	}

	//otherwise we add it to DB
	m_mdiDialogs.push_back(ccMDIDialogs(dlg,pos));
}

void MainWindow::unregisterMDIDialog(ccOverlayDialog* dlg)
{
	if (dlg)
	{
		std::vector<ccMDIDialogs>::iterator it = m_mdiDialogs.begin();
		while (it != m_mdiDialogs.end())
			if (it->dialog == dlg)
				break;
		if (it != m_mdiDialogs.end())
			m_mdiDialogs.erase(it);
		dlg->disconnect();
		dlg->stop(false);
		dlg->deleteLater();
	}
}

void MainWindow::placeMDIDialog(ccMDIDialogs& mdiDlg)
{
	if (!mdiDlg.dialog || !mdiDlg.dialog->isVisible() || !m_mdiArea)
		return;

	int dx=0,dy=0;
	switch (mdiDlg.position)
	{
	case Qt::TopLeftCorner:
		dx = 5;
		dy = 5;
		break;
	case Qt::TopRightCorner:
		dx = std::max(5,m_mdiArea->width() - mdiDlg.dialog->width() - 5);
		dy = 5;
		break;
	case Qt::BottomLeftCorner:
		dx = 5;
		dy = std::max(5,m_mdiArea->height() - mdiDlg.dialog->height() - 5);
		break;
	case Qt::BottomRightCorner:
		dx = std::max(5,m_mdiArea->width() - mdiDlg.dialog->width() - 5);
		dy = std::max(5,m_mdiArea->height() - mdiDlg.dialog->height() - 5);
		break;
	}

	//show();
	mdiDlg.dialog->move(m_mdiArea->mapToGlobal(QPoint(dx,dy)));
	mdiDlg.dialog->raise();
}

void MainWindow::updateMDIDialogsPlacement()
{
	for (size_t i=0; i<m_mdiDialogs.size(); ++i)
		placeMDIDialog(m_mdiDialogs[i]);
}

void MainWindow::toggleFullScreen(bool state)
{
	if (state)
		showFullScreen();
	else
		showNormal();
}

void MainWindow::about()
{
	QDialog* aboutDialog = new QDialog(this);

	Ui::AboutDialog ui;
	ui.setupUi(aboutDialog);

	QString ccVer = ccCommon::GetCCVersion();
	QString htmlText = ui.textEdit->toHtml();
	QString enrichedHtmlText = htmlText.arg(ccVer);
	//ccLog::PrintDebug(htmlText);
	//ccLog::PrintDebug(ccVer);
	//ccLog::PrintDebug(enrichedHtmlText);

	ui.textEdit->setHtml(enrichedHtmlText);

	aboutDialog->exec();

	//delete aboutDialog; //Qt will take care of it? Anyway CC crash if we try to delete it now!
}

void MainWindow::help()
{
	QFile doc(QApplication::applicationDirPath()+QString("/user_guide_CloudCompare.pdf"));
	if (!doc.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(	this,
								QString("User guide not found"),
								QString("Goto http://www.cloudcompare.org/documentation.html") );
	}
	else
	{
		QString program = "AcroRd32.exe";
		QStringList arguments;
		arguments << "user_guide_CloudCompare.pdf";
		QProcess *myProcess = new QProcess();
		myProcess->start(program, arguments);
	}
}

void MainWindow::freezeUI(bool state)
{
	toolBarMainTools->setDisabled(state);
	toolBarSFTools->setDisabled(state);
	toolBarPluginTools->setDisabled(state);
	toolBarGLFilters->setDisabled(state);

	//toolBarView->setDisabled(state);
	DockableDBTree->setDisabled(state);
	menubar->setDisabled(state);

	if (state)
	{
		menuEdit->setDisabled(true);
		menuTools->setDisabled(true);
	}
	else
	{
		updateMenus();
	}

	m_uiFrozen = state;
}

void MainWindow::activateRegisterPointPairTool()
{
	size_t selNum = m_selectedEntities.size();
	if (selNum == 0 || selNum > 2)
	{
		ccConsole::Error("Select one or two entities (point cloud or mesh)!");
		return;
	}

	bool lockedVertices1 = false;
	ccGenericPointCloud* cloud1 = ccHObjectCaster::ToGenericPointCloud(m_selectedEntities[0],&lockedVertices1);
	bool lockedVertices2 = false;
	ccGenericPointCloud* cloud2 = (m_selectedEntities.size()>1 ? ccHObjectCaster::ToGenericPointCloud(m_selectedEntities[1],&lockedVertices2) : 0);
	if (!cloud1 || (m_selectedEntities.size()>1 && !cloud2))
	{
		ccConsole::Error("Select point clouds or meshes only!");
		return;
	}
	if (lockedVertices1 || lockedVertices2)
	{
		DisplayLockedVerticesWarning();
		//ccConsole::Error("At least one vertex set is locked (you should select the 'vertices' entity directly!)");
		return;
	}

	ccGenericPointCloud* aligned = cloud1;
	ccGenericPointCloud* reference = 0;

	//if we have 2 clouds, we must ask the user which one is the 'aligned' one and which one is the 'reference' one
	if (cloud2)
	{
		ccOrderChoiceDlg dlg(	cloud1, "Aligned",
								cloud2, "Reference",
								this );
		if (!dlg.exec())
			return;

		aligned = ccHObjectCaster::ToGenericPointCloud(dlg.getFirstEntity());
		reference = ccHObjectCaster::ToGenericPointCloud(dlg.getSecondEntity());
	}

	//we disable all windows
	disableAllBut(0);

	if (!m_pprDlg)
	{
		m_pprDlg = new ccPointPairRegistrationDlg(this);
		connect(m_pprDlg, SIGNAL(processFinished(bool)), this, SLOT(deactivateRegisterPointPairTool(bool)));
		registerMDIDialog(m_pprDlg,Qt::TopRightCorner);
	}

	ccGLWindow* win = new3DView();
	if (!win)
	{
		ccLog::Error("[PointPairRegistration] Failed to create dedicated 3D view!");
		return;
	}

	if (!m_pprDlg->init(win,aligned,reference))
		deactivateRegisterPointPairTool(false);

	freezeUI(true);

	if (!m_pprDlg->start())
		deactivateRegisterPointPairTool(false);
	else
		updateMDIDialogsPlacement();
}

void MainWindow::deactivateRegisterPointPairTool(bool state)
{
	if (m_pprDlg)
		m_pprDlg->clear();

	//we enable all GL windows
	enableAll();

	QList<QMdiSubWindow*> subWindowList = m_mdiArea->subWindowList();
	if (!subWindowList.isEmpty())
		subWindowList[0]->showMaximized();

	freezeUI(false);

	updateUI();

	ccGLWindow* win = getActiveGLWindow();
	if (win)
		win->zoomGlobal();
}

void MainWindow::activateSegmentationMode()
{
	ccGLWindow* win = getActiveGLWindow();
	if (!win)
		return;

	size_t selNum = m_selectedEntities.size();
	if (selNum == 0)
		return;

	if (!m_gsTool)
	{
		m_gsTool = new ccGraphicalSegmentationTool(this);
		connect(m_gsTool, SIGNAL(processFinished(bool)), this, SLOT(deactivateSegmentationMode(bool)));

		registerMDIDialog(m_gsTool,Qt::TopRightCorner);
	}

	m_gsTool->linkWith(win);

	for (size_t i=0; i<selNum; ++i)
		m_gsTool->addEntity(m_selectedEntities[i]);

	if (m_gsTool->getNumberOfValidEntities() == 0)
	{
		ccConsole::Error("No segmentable entity in active window!");
		return;
	}

	freezeUI(true);
	toolBarView->setDisabled(false);

	//we disable all other windows
	disableAllBut(win);

	if (!m_gsTool->start())
		deactivateSegmentationMode(false);
	else
		updateMDIDialogsPlacement();
}

void MainWindow::deactivateSegmentationMode(bool state)
{
	bool deleteHiddenParts = false;

	//shall we apply segmentation?
	if (state)
	{
		ccHObject* firstResult = 0;

		deleteHiddenParts = m_gsTool->deleteHiddenParts();

		//aditional vertices of which visibility array should be manually reset
		std::set<ccGenericPointCloud*> verticesToReset;

		for (std::set<ccHObject*>::const_iterator p = m_gsTool->entities().begin(); p != m_gsTool->entities().end(); ++p)
		{
			ccHObject* entity = *p;

			if (entity->isKindOf(CC_TYPES::POINT_CLOUD) || entity->isKindOf(CC_TYPES::MESH))
			{
				//Special case: labels (do this before temporarily removing 'entity' from DB!)
				bool lockedVertices;
				ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(entity,&lockedVertices);
				if (cloud)
				{
					//assert(!lockedVertices); //in some cases we accept to segment meshes with locked vertices!
					ccHObject::Container labels;
					if (m_ccRoot)
						m_ccRoot->getRootEntity()->filterChildren(labels,true,CC_TYPES::LABEL_2D);
					for (ccHObject::Container::iterator it=labels.begin(); it!=labels.end(); ++it)
						if ((*it)->isA(CC_TYPES::LABEL_2D)) //Warning: cc2DViewportLabel is also a kind of 'CC_TYPES::LABEL_2D'!
						{
							//we must check all dependent labels and remove them!!!
							//TODO: couldn't we be more clever and update the label instead?
							cc2DLabel* label = static_cast<cc2DLabel*>(*it);
							bool removeLabel = false;
							for (unsigned i=0; i<label->size(); ++i)
								if (label->getPoint(i).cloud == entity)
								{
									removeLabel = true;
									break;
								}

							if (removeLabel && label->getParent())
							{
								ccLog::Warning(QString("[Segmentation] Label %1 is dependent on cloud %2 and will be removed").arg(label->getName()).arg(cloud->getName()));
								ccHObject* labelParent = label->getParent();
								ccHObjectContext objContext = removeObjectTemporarilyFromDBTree(labelParent);
								labelParent->removeChild(label);
								label=0;
								putObjectBackIntoDBTree(labelParent,objContext);
							}
						}
				}

				//we temporarily detach entity, as it may undergo
				//"severe" modifications (octree deletion, etc.) --> see ccPointCloud::createNewCloudFromVisibilitySelection
				ccHObjectContext objContext = removeObjectTemporarilyFromDBTree(entity);

				//save origin entity display (for later)
				ccGenericGLDisplay* display = entity->getDisplay();

				ccHObject* segmentationResult = 0;
				if (entity->isKindOf(CC_TYPES::POINT_CLOUD))
				{
					ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(entity);
					segmentationResult = cloud->createNewCloudFromVisibilitySelection(!deleteHiddenParts);

					if (!deleteHiddenParts && cloud->size() == 0) //if 'deleteHiddenParts' it will be done afterwards anyway
					{
						delete entity;
						entity = 0;
					}
				}
				else if (entity->isKindOf(CC_TYPES::MESH)/*|| entity->isA(CC_TYPES::PRIMITIVE)*/) //TODO
				{
					if (entity->isA(CC_TYPES::MESH))
						segmentationResult = ccHObjectCaster::ToMesh(entity)->createNewMeshFromSelection(!deleteHiddenParts);
					else if (entity->isA(CC_TYPES::SUB_MESH))
						segmentationResult = ccHObjectCaster::ToSubMesh(entity)->createNewSubMeshFromSelection(!deleteHiddenParts);

					if (!deleteHiddenParts && ccHObjectCaster::ToGenericMesh(entity)->size() == 0) //if 'deleteHiddenParts' it will be done afterwards anyway
					{
						delete entity;
						entity = 0;
					}
				}

				if (segmentationResult)
				{
					if (!deleteHiddenParts) //no need to put it back if we delete it afterwards!
					{
						if (entity)
						{
							entity->setName(entity->getName()+QString(".remaining"));

							//we also need to check if there is a childrent that is a GBLsensor
							unsigned n = entity->getChildrenNumber();

							//we put a new sensor here if we find one
							ccGBLSensor* sensor = 0;
							for (unsigned i=0; i<n; ++i)
							{
								if (entity->getChild(i)->isA(CC_TYPES::GBL_SENSOR))
								{
									sensor = ccHObjectCaster::ToGBLSensor(entity->getChild(i));
									break;
								}
							}

							if (sensor)
							{
								//we create a copy of that
								ccGBLSensor * cloned_sensor = new ccGBLSensor(*sensor);

								ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(segmentationResult);

								int errorCode;
								CCLib::GenericIndexedCloud* projectedPoints = cloned_sensor->project(cloud,errorCode,true);

								// we need also to do the same for the original cloud
								sensor->project(ccHObjectCaster::ToGenericPointCloud(entity), errorCode, true);

								cloud->addChild(cloned_sensor);
							}
							putObjectBackIntoDBTree(entity,objContext);
						}
					}
					else
					{
						//keep original name(s)
						segmentationResult->setName(entity->getName());
						if (entity->isKindOf(CC_TYPES::MESH) && segmentationResult->isKindOf(CC_TYPES::MESH))
						{
							ccGenericMesh* meshEntity = ccHObjectCaster::ToGenericMesh(entity);
							ccHObjectCaster::ToGenericMesh(segmentationResult)->getAssociatedCloud()->setName(meshEntity->getAssociatedCloud()->getName());

							//specific case: if the sub mesh is deleted afterwards (see below)
							//then its associated vertices won't be 'reset' by the segmentation tool!
							if (deleteHiddenParts && meshEntity->isA(CC_TYPES::SUB_MESH))
								verticesToReset.insert(meshEntity->getAssociatedCloud());
						}

						delete entity;
						entity = 0;
					}

					if (segmentationResult->isA(CC_TYPES::SUB_MESH))
					{
						//for sub-meshes, we have no choice but to use its parent mesh!
						objContext.parent = static_cast<ccSubMesh*>(segmentationResult)->getAssociatedMesh();
					}
					else
					{
						//otherwise we look for first non-mesh or non-cloud parent
						while (objContext.parent && (objContext.parent->isKindOf(CC_TYPES::MESH) || objContext.parent->isKindOf(CC_TYPES::POINT_CLOUD)))
							objContext.parent = objContext.parent->getParent();
					}

					if (objContext.parent)
						objContext.parent->addChild(segmentationResult); //FiXME: objContext.parentFlags?

					segmentationResult->setDisplay(display);
					segmentationResult->prepareDisplayForRefresh_recursive();

					addToDB(segmentationResult);

					if (!firstResult)
						firstResult = segmentationResult;
				}
				else if (entity)
				{
					//ccConsole::Error("An error occurred! (not enough memory?)");
					putObjectBackIntoDBTree(entity,objContext);
				}
			}
		}

		//specific actions
		{
			for (std::set<ccGenericPointCloud*>::iterator p = verticesToReset.begin(); p != verticesToReset.end(); ++p)
			{
				(*p)->resetVisibilityArray();
			}
		}

		if (firstResult && m_ccRoot)
			m_ccRoot->selectEntity(firstResult);
	}

	if (m_gsTool)
		m_gsTool->removeAllEntities(!deleteHiddenParts);

	//we enable all GL windows
	enableAll();

	freezeUI(false);

	updateUI();

	ccGLWindow* win = getActiveGLWindow();
	if (win)
		win->redraw();
}

void MainWindow::activatePointListPickingMode()
{
	ccGLWindow* win = getActiveGLWindow();
	if (!win)
		return;

	//there should be only one point cloud in current selection!
	if (m_selectedEntities.empty() || m_selectedEntities.size()>1)
	{
		ccConsole::Error("Select one and only one entity!");
		return;
	}

	ccPointCloud* pc = ccHObjectCaster::ToPointCloud(m_selectedEntities[0]);
	if (!pc)
	{
		ccConsole::Error("Wrong type of entity");
		return;
	}

	if (!pc->isVisible() || !pc->isEnabled())
	{
		ccConsole::Error("Points must be visible!");
		return;
	}

	if (!m_plpDlg)
	{
		m_plpDlg = new ccPointListPickingDlg(this);
		connect(m_plpDlg, SIGNAL(processFinished(bool)), this, SLOT(deactivatePointListPickingMode(bool)));

		registerMDIDialog(m_plpDlg,Qt::TopRightCorner);
	}

	//DGM: we must update marker size spin box value (as it may have changed by the user with the "display dialog")
	m_plpDlg->markerSizeSpinBox->setValue(win->getDisplayParameters().pickedPointsSize);

	m_plpDlg->linkWith(win);
	m_plpDlg->linkWithCloud(pc);

	freezeUI(true);

	//we disable all other windows
	disableAllBut(win);

	if (!m_plpDlg->start())
		deactivatePointListPickingMode(false);
	else
		updateMDIDialogsPlacement();
}

void MainWindow::deactivatePointListPickingMode(bool state)
{
	if (m_plpDlg)
	{
		//m_plpDlg->linkWith(0);
		m_plpDlg->linkWithCloud(0);
	}

	//we enable all GL windows
	enableAll();

	freezeUI(false);

	updateUI();
}

void MainWindow::activatePointPickingMode()
{
	ccGLWindow* win = getActiveGLWindow();
	if (!win)
		return;

	if (m_ccRoot)
		m_ccRoot->unselectAllEntities(); //we don't want any entity selected (especially existing labels!)

	if (!m_ppDlg)
	{
		m_ppDlg = new ccPointPropertiesDlg(this);
		connect(m_ppDlg, SIGNAL(processFinished(bool)),	this, SLOT(deactivatePointPickingMode(bool)));
		connect(m_ppDlg, SIGNAL(newLabel(ccHObject*)),	this, SLOT(handleNewLabel(ccHObject*)));

		registerMDIDialog(m_ppDlg,Qt::TopRightCorner);
	}

	m_ppDlg->linkWith(win);

	freezeUI(true);

	//we disable all other windows
	disableAllBut(win);

	if (!m_ppDlg->start())
		deactivatePointPickingMode(false);
	else
		updateMDIDialogsPlacement();
}

void MainWindow::deactivatePointPickingMode(bool state)
{
	//if (m_ppDlg)
	//	m_ppDlg->linkWith(0);

	//we enable all GL windows
	enableAll();

	freezeUI(false);

	updateUI();
}

void MainWindow::activateClippingBoxMode()
{
	size_t selNum = m_selectedEntities.size();
	if (selNum == 0)
		return;

	ccGLWindow* win = getActiveGLWindow();
	if (!win)
		return;

	if (!m_clipTool)
		m_clipTool = new ccClippingBoxTool(this);
	m_clipTool->linkWith(win);

	ccHObject* entity = m_selectedEntities[0];
	if (!m_clipTool->setAssociatedEntity(entity))
	{
		ccConsole::Error("Select a point cloud!");
		return;
	}

	if (m_clipTool->start())
	{
		//automatically deselect the entity (to avoid seeing its bounding box ;)
		m_ccRoot->unselectEntity(entity);
		connect(m_clipTool, SIGNAL(processFinished(bool)), this, SLOT(deactivateClippingBoxMode(bool)));
		registerMDIDialog(m_clipTool,Qt::TopRightCorner);
		freezeUI(true);
		updateMDIDialogsPlacement();
		//deactivate all other GL windows
		disableAllBut(win);
	}
	else
	{
		ccConsole::Error("Unexpected error!"); //indeed...
	}
}

void MainWindow::deactivateClippingBoxMode(bool state)
{
	//we reactivate all GL windows
	enableAll();

	freezeUI(false);

	updateUI();
}

void MainWindow::activateTranslateRotateMode()
{
	size_t selNum = m_selectedEntities.size();
	if (selNum == 0)
		return;

	ccGLWindow* win = getActiveGLWindow();
	if (!win)
		return;

	if (!m_transTool)
		m_transTool = new ccGraphicalTransformationTool(this);
	assert(m_transTool->getNumberOfValidEntities() == 0);
	m_transTool->linkWith(win);

	bool rejectedEntities = false;
	for (size_t i=0; i<selNum;++i)
	{
		ccHObject* entity = m_selectedEntities[i];
		if (!m_transTool->addEntity(entity))
			rejectedEntities = true;
	}

	if (m_transTool->getNumberOfValidEntities() == 0)
	{
		ccConsole::Error("No entity elligible for manual transformation! (see console)");
		return;
	}
	else if (rejectedEntities)
	{
		ccConsole::Error("Some entities were ingored! (see console)");
	}

	//try to activate "moving mode" in current GL window
	if (m_transTool->start())
	{
		connect(m_transTool, SIGNAL(processFinished(bool)), this, SLOT(deactivateTranslateRotateMode(bool)));
		registerMDIDialog(m_transTool,Qt::TopRightCorner);
		freezeUI(true);
		updateMDIDialogsPlacement();
		//deactivate all other GL windows
		disableAllBut(win);
	}
	else
	{
		ccConsole::Error("Unexpected error!"); //indeed...
	}
}

void MainWindow::deactivateTranslateRotateMode(bool state)
{
	//if (m_transTool)
	//	m_transTool->close();

	//we reactivate all GL windows
	enableAll();

	freezeUI(false);

	updateUI();
}

void MainWindow::testFrameRate()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
		win->testFrameRate();
}

void MainWindow::setTopView()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
		win->setView(CC_TOP_VIEW);
}

void MainWindow::setBottomView()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
		win->setView(CC_BOTTOM_VIEW);
}

void MainWindow::setFrontView()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
		win->setView(CC_FRONT_VIEW);
}

void MainWindow::setBackView()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
		win->setView(CC_BACK_VIEW);
}

void MainWindow::setLeftView()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
		win->setView(CC_LEFT_VIEW);
}

void MainWindow::setRightView()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
		win->setView(CC_RIGHT_VIEW);
}

void MainWindow::setIsoView1()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
		win->setView(CC_ISO_VIEW_1);
}

void MainWindow::setIsoView2()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
		win->setView(CC_ISO_VIEW_2);
}

void MainWindow::setLightsAndMaterials()
{
	ccDisplayOptionsDlg colorsDlg(this);
	connect(&colorsDlg, SIGNAL(aspectHasChanged()), this, SLOT(redrawAll()));

	colorsDlg.exec();

	disconnect(&colorsDlg, 0, 0, 0);
}

void MainWindow::doActionRenderToFile()
{
	ccGLWindow* win = getActiveGLWindow();
	if (!win)
		return;

	ccRenderToFileDlg rtfDlg(win->width(),win->height(),this);

	if (rtfDlg.exec())
	{
		QApplication::processEvents();
		win->renderToFile(qPrintable(rtfDlg.getFilename()),rtfDlg.getZoom(),rtfDlg.dontScalePoints(),rtfDlg.renderOverlayItems());
	}
}

void MainWindow::doActionEditCamera()
{
	//current active MDI area
	QMdiSubWindow* qWin = m_mdiArea->activeSubWindow();
	if (!qWin)
		return;

	if (!m_cpeDlg)
	{
		m_cpeDlg = new ccCameraParamEditDlg(qWin);
		//m_cpeDlg->makeFrameless(); //does not work on linux
		m_cpeDlg->linkWith(qWin);
		connect(m_mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), m_cpeDlg, SLOT(linkWith(QMdiSubWindow*)));

		registerMDIDialog(m_cpeDlg,Qt::BottomLeftCorner);
	}

	m_cpeDlg->start();

	updateMDIDialogsPlacement();
}

void MainWindow::doActionAdjustZoom()
{
	//current active MDI area
	ccGLWindow* win = getActiveGLWindow();
	if (!win)
		return;

	const ccViewportParameters& params = win->getViewportParameters();
	if (params.perspectiveView)
	{
		ccConsole::Error("Orthographic mode only!");
		return;
	}

	ccAdjustZoomDlg azDlg(win,this);

	if (!azDlg.exec())
		return;

	//apply zoom
	double zoom = azDlg.getZoom();
	win->setZoom(static_cast<float>(zoom));
	win->redraw();
}

static unsigned s_viewportIndex = 0;
void MainWindow::doActionSaveViewportAsCamera()
{
	ccGLWindow* win = getActiveGLWindow();
	if (!win)
		return;

	cc2DViewportObject* viewportObject = new cc2DViewportObject(QString("Viewport #%1").arg(++s_viewportIndex));
	viewportObject->setParameters(win->getViewportParameters());
	viewportObject->setDisplay(win);

	addToDB(viewportObject);
}

void MainWindow::setGlobalZoom()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
		win->zoomGlobal();
}

void MainWindow::zoomOnSelectedEntities()
{
	ccGLWindow* win = 0;

	ccHObject tempGroup("TempGroup");
	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		if (i == 0 || !win)
		{
			//take the first valid window as reference
			win = static_cast<ccGLWindow*>(m_selectedEntities[i]->getDisplay());
		}

		if (win)
		{
			if (m_selectedEntities[i]->getDisplay() == win)
			{
				tempGroup.addChild(m_selectedEntities[i],ccHObject::DP_NONE);
			}
			else if (m_selectedEntities[i]->getDisplay() != 0)
			{
				ccLog::Error("All selected entities must be displayed in the same 3D view!");
				return;
			}
		}
	}

	if (tempGroup.getChildrenNumber() != 0)
	{
		ccBBox box = tempGroup.getBB(false, false, win);
		win->updateConstellationCenterAndZoom(&box);
	}

	refreshAll();
}

void MainWindow::setPivotAlwaysOn()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
	{
		win->setPivotVisibility(ccGLWindow::PIVOT_ALWAYS_SHOW);
		win->redraw();

		//update pop-up menu 'top' icon
		if (m_pivotVisibilityPopupButton)
			m_pivotVisibilityPopupButton->setIcon(actionSetPivotAlwaysOn->icon());
	}
}

void MainWindow::setPivotRotationOnly()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
	{
		win->setPivotVisibility(ccGLWindow::PIVOT_SHOW_ON_MOVE);
		win->redraw();

		//update pop-up menu 'top' icon
		if (m_pivotVisibilityPopupButton)
			m_pivotVisibilityPopupButton->setIcon(actionSetPivotRotationOnly->icon());
	}
}

void MainWindow::setPivotOff()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
	{
		win->setPivotVisibility(ccGLWindow::PIVOT_HIDE);
		win->redraw();

		//update pop-up menu 'top' icon
		if (m_pivotVisibilityPopupButton)
			m_pivotVisibilityPopupButton->setIcon(actionSetPivotOff->icon());
	}
}

void MainWindow::setOrthoView()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
	{
		win->setPerspectiveState(false,true);
		win->redraw();

		//update pop-up menu 'top' icon
		if (m_viewModePopupButton)
			m_viewModePopupButton->setIcon(actionSetOrthoView->icon());
	}
}

void MainWindow::setCenteredPerspectiveView()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
	{
		win->setPerspectiveState(true,true);
		win->redraw();

		//update pop-up menu 'top' icon
		if (m_viewModePopupButton)
			m_viewModePopupButton->setIcon(actionSetCenteredPerspectiveView->icon());
	}
}

void MainWindow::setViewerPerspectiveView()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
	{
		win->setPerspectiveState(true,false);
		win->redraw();

		//update pop-up menu 'top' icon
		if (m_viewModePopupButton)
			m_viewModePopupButton->setIcon(actionSetViewerPerspectiveView->icon());
	}
}

static ccGLWindow* s_pickingWindow = 0;
void MainWindow::doPickRotationCenter()
{
	if (s_pickingWindow)
	{
		s_pickingWindow->displayNewMessage(QString(),ccGLWindow::LOWER_LEFT_MESSAGE); //clear previous messages
		s_pickingWindow->displayNewMessage("Rotation center picking aborted",ccGLWindow::LOWER_LEFT_MESSAGE);
		s_pickingWindow->redraw();
		cancelPickRotationCenter();
		return;
	}

	ccGLWindow* win = getActiveGLWindow();
	if (!win)
	{
		ccConsole::Error("No active 3D view!");
		return;
	}

	bool objectCentered = true;
	bool perspectiveEnabled = win->getPerspectiveState(objectCentered);
	if (perspectiveEnabled && !objectCentered)
	{
		ccLog::Error("Perspective mode is viewer-centered: can't use a point as rotation center!");
		return;
	}

	//we need at least one cloud
	//bool atLeastOneCloudVisible = false;
	//{
	//	assert(m_ccRoot && m_ccRoot->getRootEntity());
	//	ccHObject::Container clouds;
	//	m_ccRoot->getRootEntity()->filterChildren(clouds,true,CC_TYPES::POINT_CLOUD);
	//	for (unsigned i=0;i<clouds.size();++i)
	//		if (clouds[i]->isVisible() && clouds[i]->isEnabled())
	//		{
	//			//we must check that the cloud is really visible: i.e. it's parent are not deactivated!
	//			atLeastOneCloudVisible = true;
	//			ccHObject* parent = clouds[i]->getParent();
	//			while (parent)
	//			{
	//				if (!parent->isEnabled())
	//				{
	//					atLeastOneCloudVisible = false;
	//					break;
	//				}
	//				parent = parent->getParent();
	//			}
	//			if (atLeastOneCloudVisible)
	//				break;
	//		}
	//}

	//if (!atLeastOneCloudVisible)
	//{
	//	ccConsole::Error("No visible cloud in active 3D view!");
	//	return;
	//}

	connect(win, SIGNAL(pointPicked(int, unsigned, int, int)), this, SLOT(processPickedRotationCenter(int, unsigned, int, int)));
	win->setPickingMode(ccGLWindow::POINT_PICKING);
	win->displayNewMessage("Pick a point to be used as rotation center (click on icon again to cancel)",ccGLWindow::LOWER_LEFT_MESSAGE,true,3600);
	win->redraw();
	s_pickingWindow = win;

	freezeUI(true);
}

void MainWindow::processPickedRotationCenter(int cloudUniqueID, unsigned pointIndex, int, int)
{
	if (s_pickingWindow)
	{
		ccHObject* obj = 0;
		ccHObject* db = s_pickingWindow->getSceneDB();
		if (db)
			obj = db->find(cloudUniqueID);
		if (obj && obj->isKindOf(CC_TYPES::POINT_CLOUD))
		{
			ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(obj);
			const CCVector3* P = cloud->getPoint(pointIndex);
			if (P)
			{
				const ccViewportParameters& params = s_pickingWindow->getViewportParameters();
				if (!params.perspectiveView || params.objectCenteredView)
				{
					CCVector3d newPivot = CCVector3d::fromArray(P->u);
					//compute the equivalent camera center
					CCVector3d dP = params.pivotPoint - newPivot;
					CCVector3d MdP = dP; params.viewMat.applyRotation(MdP);
					CCVector3d newCameraPos = params.cameraCenter + MdP - dP;
					s_pickingWindow->setCameraPos(newCameraPos);
					s_pickingWindow->setPivotPoint(newPivot);

					const unsigned& precision = s_pickingWindow->getDisplayParameters().displayedNumPrecision;
					s_pickingWindow->displayNewMessage(QString(),ccGLWindow::LOWER_LEFT_MESSAGE,false); //clear precedent message
					s_pickingWindow->displayNewMessage(QString("Point (%1,%2,%3) set as rotation center").arg(P->x,0,'f',precision).arg(P->y,0,'f',precision).arg(P->z,0,'f',precision),ccGLWindow::LOWER_LEFT_MESSAGE,true);
				}
				s_pickingWindow->redraw();
			}
		}
	}

	cancelPickRotationCenter();
}

void MainWindow::cancelPickRotationCenter()
{
	if (s_pickingWindow)
	{
		disconnect(s_pickingWindow, SIGNAL(pointPicked(int, unsigned, int, int)), this, SLOT(processPickedRotationCenter(int, unsigned, int, int)));
		s_pickingWindow->setPickingMode(ccGLWindow::DEFAULT_PICKING);
		s_pickingWindow=0;
	}

	freezeUI(false);
}

void MainWindow::toggleSelectedEntitiesVisibility()
{
	toggleSelectedEntitiesProp(0);
}

void MainWindow::toggleSelectedEntitiesColors()
{
	toggleSelectedEntitiesProp(1);
}

void MainWindow::toggleSelectedEntitiesNormals()
{
	toggleSelectedEntitiesProp(2);
}

void MainWindow::toggleSelectedEntitiesSF()
{
	toggleSelectedEntitiesProp(3);
}

void MainWindow::toggleSelectedEntitiesMaterials()
{
	toggleSelectedEntitiesProp(4);
}

void MainWindow::toggleSelectedEntities3DName()
{
	toggleSelectedEntitiesProp(5);
}

void MainWindow::toggleSelectedEntitiesProp(int prop)
{
	ccHObject::Container baseEntities;
	RemoveSiblings(m_selectedEntities,baseEntities);
	for (size_t i=0; i<baseEntities.size(); ++i)
	{
		switch(prop)
		{
		case 0: //visibility
			baseEntities[i]->toggleVisibility_recursive();
			break;
		case 1: //colors
			baseEntities[i]->toggleColors_recursive();
			break;
		case 2: //normals
			baseEntities[i]->toggleNormals_recursive();
			break;
		case 3: //sf
			baseEntities[i]->toggleSF_recursive();
			break;
		case 4: //material/texture
			baseEntities[i]->toggleMaterials_recursive();
			break;
		case 5: //name in 3D
			baseEntities[i]->toggleShowName_recursive();
			break;
		default:
			assert(false);
		}
		baseEntities[i]->prepareDisplayForRefresh_recursive();
	}

	refreshAll();
	updateUI();
}

void MainWindow::showSelectedEntitiesHistogram()
{
	size_t selNum = m_selectedEntities.size();
	for (size_t i=0; i<selNum; ++i)
	{
		//for "real" point clouds only
		ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(m_selectedEntities[i]);
		if (cloud)
		{
			//on affiche l'histogramme du champ scalaire courant
			ccScalarField* sf = static_cast<ccScalarField*>(cloud->getCurrentDisplayedScalarField());
			if (sf)
			{
				ccHistogramWindowDlg* hDlg = new ccHistogramWindowDlg(this);
				hDlg->setWindowTitle(QString("Histogram [%1]").arg(cloud->getName()));

				ccHistogramWindow* histogram = hDlg->window();
				{
					unsigned numberOfPoints = cloud->size();
					unsigned numberOfClasses = static_cast<unsigned>(sqrt(static_cast<double>(numberOfPoints)));
					//we take the 'nearest' multiple of 4
					numberOfClasses &= (~3);
					numberOfClasses = std::max<unsigned>(4,numberOfClasses);
					numberOfClasses = std::min<unsigned>(256,numberOfClasses);

					histogram->setTitle(QString("%1 (%2 values) ").arg(sf->getName()).arg(numberOfPoints));
					histogram->fromSF(sf,numberOfClasses);
					histogram->setAxisLabels(sf->getName(),"Count");
					histogram->refresh();
				}
				hDlg->show();
			}
		}
	}
}

void MainWindow::doActionCrop()
{
	ccHObject::Container selectedEntities = m_selectedEntities;
	size_t selNum = selectedEntities.size();

	//find candidates
	std::vector<ccPointCloud*> candidates;
	ccBBox baseBB;
	{
		for (size_t i=0; i<selNum; ++i)
		{
			ccHObject* ent = selectedEntities[i];
			if (ent->isA(CC_TYPES::POINT_CLOUD))
			{
				ccPointCloud* cloud = static_cast<ccPointCloud*>(ent);
				candidates.push_back(cloud);

				baseBB += cloud->getBB();
			}
		}
	}

	if (candidates.empty())
	{
		ccConsole::Warning("[Crop] No elligible candidate found!");
		return;
	}

	ccBoundingBoxEditorDlg bbeDlg(this);
	bbeDlg.setBaseBBox(baseBB,false);
	bbeDlg.showInclusionWarning(false);
	bbeDlg.setWindowTitle("Crop");

	if (!bbeDlg.exec())
		return;

	//deselect all entities
	if (m_ccRoot)
		m_ccRoot->unselectAllEntities();

	//process cloud/meshes
	{
		for (size_t i=0; i<candidates.size(); ++i)
		{
			ccPointCloud* cloud = candidates[i];

			CCLib::ReferenceCloud* selection = cloud->crop(bbeDlg.getBox(),true);
			if (selection)
			{
				if (selection->size() != 0)
				{
					//crop
					ccPointCloud* croppedEnt = cloud->partialClone(selection);
					if (croppedEnt)
					{
						croppedEnt->setName(cloud->getName()+QString(".cropped"));
						croppedEnt->setDisplay(cloud->getDisplay());
						addToDB(croppedEnt);
						m_ccRoot->selectEntity(croppedEnt,true);
					}
				}
				else
				{
					//no points fall inside selection!
					ccConsole::Warning(QString("[ccPointCloud::crop] No point of cloud '%1' falls inside the input box!").arg(cloud->getName()));
				}

				delete selection;
				selection = 0;
			}
		}
	}

	updateUI();
}

void MainWindow::doActionClone()
{
	ccHObject::Container selectedEntities = m_selectedEntities;
	size_t selNum = selectedEntities.size();

	ccHObject* lastClone = 0;
	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* clone = 0;
		if (selectedEntities[i]->isKindOf(CC_TYPES::POINT_CLOUD))
		{
			clone = ccHObjectCaster::ToGenericPointCloud(selectedEntities[i])->clone();
			if (!clone)
			{
				ccConsole::Error(QString("An error occurred while cloning cloud %1").arg(selectedEntities[i]->getName()));
			}
		}
		else if (selectedEntities[i]->isKindOf(CC_TYPES::PRIMITIVE))
		{
			clone = static_cast<ccGenericPrimitive*>(selectedEntities[i])->clone();
			if (!clone)
			{
				ccConsole::Error(QString("An error occurred while cloning primitive %1").arg(selectedEntities[i]->getName()));
			}
		}
		else if (selectedEntities[i]->isA(CC_TYPES::MESH))
		{
			clone = ccHObjectCaster::ToMesh(selectedEntities[i])->clone();
			if (!clone)
			{
				ccConsole::Error(QString("An error occurred while cloning mesh %1").arg(selectedEntities[i]->getName()));
			}
		}
		else if (selectedEntities[i]->isA(CC_TYPES::POLY_LINE))
		{
			ccPolyline* poly = ccHObjectCaster::ToPolyline(selectedEntities[i]);
			clone = (poly ? new ccPolyline(*poly) : 0);
			if (!clone)
			{
				ccConsole::Error(QString("An error occurred while cloning polyline %1").arg(selectedEntities[i]->getName()));
			}
		}
		else if (selectedEntities[i]->isA(CC_TYPES::FACET))
		{
			ccFacet* facet = ccHObjectCaster::ToFacet(selectedEntities[i]);
			clone = (facet ? facet->clone() : 0);
			if (!clone)
			{
				ccConsole::Error(QString("An error occurred while cloning facet %1").arg(selectedEntities[i]->getName()));
			}
		}
		else
		{
			ccLog::Warning(QString("Entity '%1' can't be cloned (type not supported yet!)").arg(selectedEntities[i]->getName()));
		}

		if (clone)
		{
			clone->setDisplay(selectedEntities[i]->getDisplay());
			addToDB(clone);
			lastClone = clone;
		}
	}

	if (lastClone && m_ccRoot)
		m_ccRoot->selectEntity(lastClone->getUniqueID());

	updateUI();
}

static double s_constantSFValue = 0.0;
void MainWindow::doActionAddConstantSF()
{
	size_t selNum = m_selectedEntities.size();
	if (selNum != 1)
	{
		if (selNum > 1)
			ccConsole::Error("Select only one point cloud or mesh!");
		return;
	}

	ccHObject* ent = m_selectedEntities[0];

	bool lockedVertices;
	ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(ent,&lockedVertices);

	//for "real" point clouds only
	if (!cloud)
		return;
	if (lockedVertices && !ent->isAncestorOf(cloud))
	{
		DisplayLockedVerticesWarning();
		return;
	}

	QString defaultName = "Constant";
	unsigned trys = 1;
	while (cloud->getScalarFieldIndexByName(qPrintable(defaultName)) >= 0 || trys > 99)
	{
		defaultName = QString("Constant #%1").arg(++trys);
	}

	//ask for a name
	bool ok;
	QString sfName = QInputDialog::getText(this,"New SF name", "SF name (must be unique)", QLineEdit::Normal, defaultName, &ok);
	if (!ok)
		return;
	if (sfName.isNull())
	{
		ccLog::Error("Invalid name");
		return;
	}
	if (cloud->getScalarFieldIndexByName(qPrintable(sfName)) >= 0)
	{
		ccLog::Error("Name already exists!");
		return;
	}

	ScalarType sfValue = static_cast<ScalarType>(QInputDialog::getDouble(this,"Add constant value", "value", s_constantSFValue, -DBL_MAX, DBL_MAX, 8, &ok));
	if (!ok)
		return;

	int sfIdx = cloud->getScalarFieldIndexByName(qPrintable(sfName));
	if (sfIdx < 0)
		sfIdx = cloud->addScalarField(qPrintable(sfName));
	if (sfIdx < 0)
	{
		ccLog::Error("An error occurred! (see console)");
		return;
	}

	CCLib::ScalarField* sf = cloud->getScalarField(sfIdx);
	assert(sf);
	if (sf)
	{
		sf->fill(sfValue);
		sf->computeMinAndMax();
		cloud->setCurrentDisplayedScalarField(sfIdx);
		cloud->showSF(true);
		updateUI();
		if (cloud->getDisplay())
			cloud->getDisplay()->redraw();
	}

	ccLog::Print(QString("New scalar field added to %1 (constant value: %2)").arg(cloud->getName()).arg(sfValue));
}

QString GetFirstAvailableSFName(ccPointCloud* cloud, const QString& baseName)
{
	if (!cloud)
	{
		assert(false);
		return QString();
	}

	QString name = baseName;
	unsigned trys = 0;
	while (cloud->getScalarFieldIndexByName(qPrintable(name)) >= 0 || trys > 99)
		name = QString("%1 #%2").arg(baseName).arg(++trys);

	if (trys > 99)
		return QString();
	return name;
}

void MainWindow::doActionScalarFieldFromColor()
{
	//candidates
	std::set<ccPointCloud*> clouds;
	{
		for (size_t i=0; i<m_selectedEntities.size(); ++i)
		{
			ccHObject* ent = m_selectedEntities[i];
			ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(ent);
			if (cloud && ent->hasColors()) //only for clouds (or vertices)
				clouds.insert( cloud );
		}
	}

	if (clouds.empty())
		return;

	ccScalarFieldFromColorDlg dialog(this);
	if (!dialog.exec())
		return;

	bool exportR = dialog.getRStatus();
	bool exportG = dialog.getGStatus();
	bool exportB = dialog.getBStatus();
	bool exportC = dialog.getCompositeStatus();

	for (std::set<ccPointCloud*>::const_iterator it = clouds.begin(); it != clouds.end(); ++it)
	{
		ccPointCloud* cloud = *it;

		std::vector<ccScalarField*> fields(4);
		fields[0] = (exportR ? new ccScalarField(qPrintable(GetFirstAvailableSFName(cloud,"R"))) : 0);
		fields[1] = (exportG ? new ccScalarField(qPrintable(GetFirstAvailableSFName(cloud,"G"))) : 0);
		fields[2] = (exportB ? new ccScalarField(qPrintable(GetFirstAvailableSFName(cloud,"B"))) : 0);
		fields[3] = (exportC ? new ccScalarField(qPrintable(GetFirstAvailableSFName(cloud,"Composite"))) : 0);

		//try to instantiate memory for each field
		{
			unsigned count = cloud->size();
			for (size_t i=0; i<fields.size(); ++i)
			{
				if (fields[i] && !fields[i]->reserve(count))
				{
					ccLog::Warning(QString("[doActionScalarFieldFromColor] Not enough memory to instantiate SF '%1' on cloud '%2'").arg(fields[i]->getName()).arg(cloud->getName()));
					fields[i]->release();
					fields[i] = 0;
				}
			}
		}

		//export points
		for (unsigned j=0; j<cloud->size(); ++j)
		{
			const colorType* rgb = cloud->getPointColor(j);

			if (fields[0])
				fields[0]->setValue(j, rgb[0]);
			if (fields[1])
				fields[1]->setValue(j, rgb[1]);
			if (fields[2])
				fields[2]->setValue(j, rgb[2]);
			if (fields[3])
				fields[3]->setValue(j, static_cast<ScalarType>(rgb[0] + rgb[1] + rgb[2])/3 );
		}

		QString fieldsStr;
		{
			for (size_t i=0; i<fields.size(); ++i)
			{
				if (fields[i])
				{
					fields[i]->computeMinAndMax();

					int sfIdx = cloud->getScalarFieldIndexByName(fields[i]->getName());
					if (sfIdx >= 0)
						cloud->deleteScalarField(sfIdx);
					sfIdx = cloud->addScalarField(fields[i]);
					assert(sfIdx >= 0);
					if (sfIdx >= 0)
					{
						cloud->setCurrentDisplayedScalarField(sfIdx);
						cloud->showSF(true);
						cloud->refreshDisplay();

						//mesh vertices?
						if (cloud->getParent() && cloud->getParent()->isKindOf(CC_TYPES::MESH))
						{
							cloud->getParent()->showSF(true);
							cloud->getParent()->refreshDisplay();
						}

						if (!fieldsStr.isEmpty())
							fieldsStr.append(", ");
						fieldsStr.append(fields[i]->getName());
					}
					else
					{
						ccConsole::Warning(QString("[doActionScalarFieldFromColor] Failed to add scalar field '%1' to cloud '%2'?!").arg(fields[i]->getName()).arg(cloud->getName()));
						fields[i]->release();
						fields[i] = 0;
					}
				}
			}
		}

		if (!fieldsStr.isEmpty())
			ccLog::Print(QString("[doActionScalarFieldFromColor] New scalar fields (%1) added to '%2'").arg(fieldsStr).arg(cloud->getName()));
	}

	refreshAll();
	updateUI();
}

void MainWindow::doActionScalarFieldArithmetic()
{
	assert(!m_selectedEntities.empty());

	ccHObject* entity = m_selectedEntities[0];
	bool lockedVertices;
	ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(entity,&lockedVertices);
	if (lockedVertices)
	{
		DisplayLockedVerticesWarning();
		return;
	}
	if (!cloud)
		return;

	ccScalarFieldArithmeticsDlg sfaDlg(cloud,this);

	if (!sfaDlg.exec())
		return;

	if (!sfaDlg.apply(cloud))
	{
		ccConsole::Error("An error occurred (see Console for more details)");
	}

	cloud->showSF(true);
	cloud->prepareDisplayForRefresh_recursive();

	refreshAll();
	updateUI();
}

void MainWindow::doActionFitPlane()
{
	doComputePlaneOrientation(false);
}

void MainWindow::doActionFitFacet()
{
	doComputePlaneOrientation(true);
}

static double s_polygonMaxEdgeLength = 0;
void MainWindow::doComputePlaneOrientation(bool fitFacet)
{
	ccHObject::Container selectedEntities = m_selectedEntities;
	size_t selNum = selectedEntities.size();
	if (selNum < 1)
		return;

	double maxEdgeLength = 0;
	if (fitFacet)
	{
		bool ok = true;
		maxEdgeLength = QInputDialog::getDouble(this,"Fit facet", "Max edge length (0 = no limit)", s_polygonMaxEdgeLength, 0, DBL_MAX, 8, &ok);
		if (!ok)
			return;
		s_polygonMaxEdgeLength = maxEdgeLength;
	}

	for (size_t i=0; i<selNum; ++i)
	{
		ccHObject* ent = selectedEntities[i];

		CCLib::GenericIndexedCloudPersist* cloud = 0;

		if (ent->isKindOf(CC_TYPES::POLY_LINE))
		{
			ccPolyline * pline = ccHObjectCaster::ToPolyline(ent);
			cloud = static_cast<CCLib::GenericIndexedCloudPersist*>(pline);
		}
		else
		{
			ccGenericPointCloud* gencloud = ccHObjectCaster::ToGenericPointCloud(ent);
			if (gencloud)
			{
				cloud = static_cast<CCLib::GenericIndexedCloudPersist*>(gencloud);
			}
		}

		if (cloud)
		{
			double rms = 0.0;
			CCVector3 C,N;

			ccHObject* plane = 0;
			if (fitFacet)
			{
				ccFacet* facet = ccFacet::Create(cloud, static_cast<PointCoordinateType>(maxEdgeLength));
				if (facet)
				{
					plane = static_cast<ccHObject*>(facet);
					N = facet->getNormal();
					C = facet->getCenter();
					rms = facet->getRMS();
				}
			}
			else
			{
				ccPlane* pPlane = ccPlane::Fit(cloud, &rms);
				if (pPlane)
				{
					plane = static_cast<ccHObject*>(pPlane);
					N = pPlane->getNormal();
					C = *CCLib::Neighbourhood(cloud).getGravityCenter();
					pPlane->enableStippling(true);
				}
			}

			//as all information appears in Console...
			forceConsoleDisplay();

			if (plane)
			{
				ccConsole::Print(QString("[Orientation] Entity '%1'").arg(ent->getName()));
				ccConsole::Print("\t- plane fitting RMS: %f",rms);

				//We always consider the normal with a positive 'Z' by default!
				if (N.z < 0.0)
					N *= -1.0;
				ccConsole::Print("\t- normal: (%f,%f,%f)",N.x,N.y,N.z);

				//we compute strike & dip by the way
				PointCoordinateType dip = 0, dipDir = 0;
				ccNormalVectors::ConvertNormalToDipAndDipDir(N,dip,dipDir);
				QString dipAndDipDirStr = ccNormalVectors::ConvertDipAndDipDirToString(dip,dipDir);
				ccConsole::Print(QString("\t- %1").arg(dipAndDipDirStr));

				//hack: output the transformation matrix that would make this normal points towards +Z
				ccGLMatrix makeZPosMatrix = ccGLMatrix::FromToRotation(N,CCVector3(0,0,1.0f));
				CCVector3 Gt = C;
				makeZPosMatrix.applyRotation(Gt);
				makeZPosMatrix.setTranslation(C-Gt);
				makeZPosMatrix.invert();
				ccConsole::Print("[Orientation] A matrix that would make this plane horizontal (normal towards Z+) is:");
				ccConsole::Print(makeZPosMatrix.toString(12,' ')); //full precision
				ccConsole::Print("[Orientation] You can copy this matrix values (CTRL+C) and paste them in the 'Apply transformation tool' dialog");

				plane->setName(dipAndDipDirStr);
				plane->applyGLTransformation_recursive(); //not yet in DB
				plane->setVisible(true);
				plane->setSelectionBehavior(ccHObject::SELECTION_FIT_BBOX);

				ent->addChild(plane);
				plane->setDisplay(ent->getDisplay());
				plane->prepareDisplayForRefresh_recursive();
				addToDB(plane);
			}
			else
			{
				ccConsole::Warning(QString("Failed to fit a plane/facet on entity '%1'").arg(ent->getName()));
			}
		}
	}

	refreshAll();
	updateUI();
}

void MainWindow::doShowPrimitiveFactory()
{
	if (!m_pfDlg)
		m_pfDlg = new ccPrimitiveFactoryDlg(this);

	m_pfDlg->setModal(false);
	m_pfDlg->setWindowModality(Qt::NonModal);
	m_pfDlg->show();
}

void MainWindow::doComputeApproximateDensity()
{
	if (!ApplyCCLibAlgortihm(CCLIB_ALGO_APPROX_DENSITY,m_selectedEntities,this))
		return;
	refreshAll();
	updateUI();
}

void MainWindow::doComputeAccurateDensity()
{
	if (!ApplyCCLibAlgortihm(CCLIB_ALGO_ACCURATE_DENSITY,m_selectedEntities,this))
		return;
	refreshAll();
	updateUI();
}

void MainWindow::doComputeCurvature()
{
	if (!ApplyCCLibAlgortihm(CCLIB_ALGO_CURVATURE,m_selectedEntities,this))
		return;
	refreshAll();
	updateUI();
}

void MainWindow::doActionSFGradient()
{
	if (!ApplyCCLibAlgortihm(CCLIB_ALGO_SF_GRADIENT,m_selectedEntities,this))
		return;
	refreshAll();
	updateUI();
}

void MainWindow::doComputeRoughness()
{
	if (!ApplyCCLibAlgortihm(CCLIB_ALGO_ROUGHNESS,m_selectedEntities,this))
		return;
	refreshAll();
	updateUI();
}

void MainWindow::doSphericalNeighbourhoodExtractionTest()
{
	if (!ApplyCCLibAlgortihm(CCLIB_SPHERICAL_NEIGHBOURHOOD_EXTRACTION_TEST,m_selectedEntities,this))
		return;
	refreshAll();
	updateUI();
}

void MainWindow::doCylindricalNeighbourhoodExtractionTest()
{
	bool ok;
	double radius = QInputDialog::getDouble(this,"CNE Test","radius",0.02,1.0e-6,1.0e6,6,&ok);
	if (!ok)
		return;

	double height = QInputDialog::getDouble(this,"CNE Test","height",0.05,1.0e-6,1.0e6,6,&ok);
	if (!ok)
		return;

	ccPointCloud* cloud = new ccPointCloud("cube");
	const unsigned ptsCount = 1000000;
	if (!cloud->reserve(ptsCount))
	{
		ccConsole::Error("Not enough memory!");
		delete cloud;
		return;
	}

	//fill a unit cube with random points
	{
		for (unsigned i=0; i<ptsCount; ++i)
		{
			CCVector3 P(	static_cast<PointCoordinateType>(rand())/static_cast<PointCoordinateType>(RAND_MAX),
							static_cast<PointCoordinateType>(rand())/static_cast<PointCoordinateType>(RAND_MAX),
							static_cast<PointCoordinateType>(rand())/static_cast<PointCoordinateType>(RAND_MAX) );

			cloud->addPoint(P);
		}
	}

	//get/Add scalar field
	int sfIdx = cloud->getScalarFieldIndexByName("CNE test");
	if (sfIdx < 0)
		sfIdx = cloud->addScalarField("CNE test");
	if (sfIdx < 0)
	{
		ccConsole::Error("Not enough memory!");
		delete cloud;
		return;
	}
	cloud->setCurrentScalarField(sfIdx);

	//reset scalar field
	cloud->getScalarField(sfIdx)->fill(NAN_VALUE);

	ccProgressDialog pDlg(true,this);
	ccOctree* octree = cloud->computeOctree(&pDlg);
	if (octree)
	{
		QElapsedTimer subTimer;
		subTimer.start();
		unsigned long long extractedPoints = 0;
		unsigned char level = octree->findBestLevelForAGivenNeighbourhoodSizeExtraction(static_cast<PointCoordinateType>(2.5*radius)); //2.5 = empirical
		const unsigned samples = 1000;
		for (unsigned j=0; j<samples; ++j)
		{
			//generate random normal vector
			CCVector3 dir(0,0,1);
			{
				ccGLMatrix rot;
				rot.initFromParameters(	static_cast<PointCoordinateType>( static_cast<double>(rand())/static_cast<double>(RAND_MAX) * 2.0*M_PI ),
										static_cast<PointCoordinateType>( static_cast<double>(rand())/static_cast<double>(RAND_MAX) * 2.0*M_PI ),
										static_cast<PointCoordinateType>( static_cast<double>(rand())/static_cast<double>(RAND_MAX) * 2.0*M_PI ),
										CCVector3(0,0,0) );
				rot.applyRotation(dir);
			}
			unsigned randIndex = (static_cast<unsigned>(static_cast<double>(rand())*static_cast<double>(ptsCount)/static_cast<double>(RAND_MAX)) % ptsCount);

			CCLib::DgmOctree::CylindricalNeighbourhood cn;
			cn.center = *cloud->getPoint(randIndex);
			cn.dir = dir;
			cn.level = level;
			cn.radius = static_cast<PointCoordinateType>(radius);
			cn.maxHalfLength = static_cast<PointCoordinateType>(height/2);

			octree->getPointsInCylindricalNeighbourhood(cn);
			//octree->getPointsInSphericalNeighbourhood(*cloud->getPoint(randIndex),radius,neighbours,level);
			size_t neihgboursCount = cn.neighbours.size();
			extractedPoints += static_cast<unsigned long long>(neihgboursCount);
			for (size_t k=0; k<neihgboursCount; ++k)
				cloud->setPointScalarValue(cn.neighbours[k].pointIndex,static_cast<ScalarType>(sqrt(cn.neighbours[k].squareDistd)));
		}
		ccConsole::Print("[CNE_TEST] Mean extraction time = %i ms (radius = %f, height = %f, mean(neighbours) = %3.1f)",subTimer.elapsed(),radius,height,static_cast<double>(extractedPoints)/static_cast<double>(samples));
	}
	else
	{
		ccConsole::Error("Failed to compute octree!");
	}

	ccScalarField* sf = static_cast<ccScalarField*>(cloud->getScalarField(sfIdx));
	sf->computeMinAndMax();
	sf->showNaNValuesInGrey(false);
	cloud->setCurrentDisplayedScalarField(sfIdx);
	cloud->showSF(true);

	addToDB(cloud);

	refreshAll();
	updateUI();
}

void MainWindow::doActionExportCloudsInfo()
{
	size_t selNum = m_selectedEntities.size();

	//look for clouds
	std::vector<ccPointCloud*> clouds;
	unsigned maxSFCount = 0;
	{
		for (size_t i=0; i<selNum; ++i)
		{
			ccHObject* ent = m_selectedEntities[i];
			ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(ent);
			if (cloud)
			{
				clouds.push_back(cloud);
				maxSFCount = std::max<unsigned>(maxSFCount,cloud->getNumberOfScalarFields());
			}
		}
	}

	if (clouds.empty())
	{
		ccConsole::Error("Select at least one point cloud!");
		return;
	}

	//persistent settings
	QSettings settings;
	settings.beginGroup(s_psSaveFile);
	QString currentPath = settings.value(s_psCurrentPath,QApplication::applicationDirPath()).toString();

	QString outputFilename = QFileDialog::getSaveFileName(this, "Select output file", currentPath, "*.csv");
	if (outputFilename.isEmpty())
		return;

	QFile csvFile(outputFilename);
	if (!csvFile.open(QFile::WriteOnly))
	{
		ccConsole::Error("Failed to open file for writing! (check file permissions)");
		return;
	}

	//write CSV header
	QTextStream csvStream(&csvFile);
	csvStream << "Name;";
	csvStream << "Points;";
	csvStream << "meanX;";
	csvStream << "meanY;";
	csvStream << "meanZ;";
	{
		for (unsigned i=0; i<maxSFCount; ++i)
		{
			QString sfIndex = QString("SF#%1").arg(i+1);
			csvStream << sfIndex << " name;";
			csvStream << sfIndex << " valid values;";
			csvStream << sfIndex << " mean;";
			csvStream << sfIndex << " std.dev.;";
			csvStream << sfIndex << " sum;";
		}
	}
	csvStream << endl;

	//write one line per cloud
	{
		for (size_t i=0; i<clouds.size(); ++i)
		{
			ccPointCloud* cloud = clouds[i];
			{
				CCVector3 G = *CCLib::Neighbourhood(cloud).getGravityCenter();
				csvStream << cloud->getName() << ";" /*"Name;"*/;
				csvStream << cloud->size() << ";" /*"Points;"*/;
				csvStream << G.x << ";" /*"meanX;"*/;
				csvStream << G.y << ";" /*"meanY;"*/;
				csvStream << G.z << ";" /*"meanZ;"*/;
				for (unsigned j=0; j<cloud->getNumberOfScalarFields(); ++j)
				{
					CCLib::ScalarField* sf = cloud->getScalarField(j);
					csvStream << sf->getName() << ";" /*"SF name;"*/;

					unsigned validCount = 0;
					double sfSum = 0;
					double sfSum2 = 0;
					for (unsigned k=0; k<sf->currentSize(); ++k)
					{
						const ScalarType& val = sf->getValue(k);
						if (CCLib::ScalarField::ValidValue(val))
						{
							++validCount;
							sfSum += val;
							sfSum2 += val*val;
						}
					}
					csvStream << validCount << ";" /*"SF valid values;"*/;
					double mean = sfSum/validCount;
					csvStream << mean << ";" /*"SF mean;"*/;
					csvStream << sqrt(fabs(sfSum2/validCount - mean*mean)) << ";" /*"SF std.dev.;"*/;
					csvStream << sfSum << ";" /*"SF sum;"*/;
				}
				csvStream << endl;
			}
		}
	}

	ccConsole::Print(QString("File '%1' successfully saved (%2 cloud(s))").arg(outputFilename).arg(clouds.size()));
	csvFile.close();
}

bool MainWindow::ApplyCCLibAlgortihm(CC_LIB_ALGORITHM algo, ccHObject::Container& entities, QWidget *parent/*=0*/, void** additionalParameters/*=0*/)
{
	size_t selNum = entities.size();
	if (selNum < 1)
		return false;

	//generic parameters
	QString sfName;

	//computeDensity parameters
	PointCoordinateType densityKernelSize = PC_ONE;

	//curvature parameters
	PointCoordinateType curvKernelSize = -PC_ONE;
	CCLib::Neighbourhood::CC_CURVATURE_TYPE curvType = CCLib::Neighbourhood::GAUSSIAN_CURV;

	//computeScalarFieldGradient parameters
	bool euclidian = false;

	//computeRoughness parameters
	PointCoordinateType roughnessKernelSize = PC_ONE;

	switch (algo)
	{
	case CCLIB_ALGO_APPROX_DENSITY:
		{
			sfName = CC_LOCAL_DENSITY_APPROX_FIELD_NAME;
		}
		break;

	case CCLIB_ALGO_ACCURATE_DENSITY:
		{
			//parameters already provided?
			if (additionalParameters)
			{
				densityKernelSize = *static_cast<PointCoordinateType*>(additionalParameters[0]);
			}
			else //ask the user!
			{
				densityKernelSize = GetDefaultCloudKernelSize(entities)/4;
				if (densityKernelSize < 0)
				{
					ccConsole::Error("Invalid kernel size!");
					return false;
				}
				bool ok;
				double val = QInputDialog::getDouble(parent,"Accurate density","Radius",static_cast<double>(densityKernelSize),DBL_MIN,DBL_MAX,8,&ok);
				if (!ok)
					return false;
				densityKernelSize = static_cast<PointCoordinateType>(val);
			}

			sfName = QString("%1 (r=%2)").arg(CC_LOCAL_DENSITY_FIELD_NAME).arg(densityKernelSize);
		}
		break;

	case CCLIB_ALGO_CURVATURE:
		{
			//parameters already provided?
			if (additionalParameters)
			{
				curvType = *static_cast<CCLib::Neighbourhood::CC_CURVATURE_TYPE*>(additionalParameters[0]);
				curvKernelSize = *static_cast<PointCoordinateType*>(additionalParameters[1]);
			}
			else //ask the user!
			{
				curvKernelSize = GetDefaultCloudKernelSize(entities);
				if (curvKernelSize < 0)
				{
					ccConsole::Error("Invalid kernel size!");
					return false;
				}
				ccCurvatureDlg curvDlg(0);
				if (selNum == 1)
					curvDlg.setKernelSize(curvKernelSize);
				if (!curvDlg.exec())
					return false;

				curvType = curvDlg.getCurvatureType();
				curvKernelSize = static_cast<PointCoordinateType>(curvDlg.getKernelSize());
			}

			sfName = QString("%1 (%2)").arg(curvType == CCLib::Neighbourhood::MEAN_CURV ? CC_MEAN_CURVATURE_FIELD_NAME : CC_GAUSSIAN_CURVATURE_FIELD_NAME).arg(curvKernelSize);
		}
		break;

	case CCLIB_ALGO_SF_GRADIENT:
		{
			sfName = CC_GRADIENT_NORMS_FIELD_NAME;
			//parameters already provided?
			if (additionalParameters)
			{
				euclidian = *static_cast<bool*>(additionalParameters[0]);
			}
			else //ask the user!
			{
				euclidian = (	QMessageBox::question(parent,
								"Gradient",
								"Is the scalar field composed of (euclidian) distances?",
								QMessageBox::Yes | QMessageBox::No,
								QMessageBox::No ) == QMessageBox::Yes );
			}
		}
		break;

	case CCLIB_ALGO_ROUGHNESS:
		case CCLIB_SPHERICAL_NEIGHBOURHOOD_EXTRACTION_TEST: //for tests: we'll use the roughness kernel for SNE
			{
				//parameters already provided?
				if (additionalParameters)
				{
					roughnessKernelSize = *static_cast<PointCoordinateType*>(additionalParameters[0]);
				}
				else //ask the user!
				{
					roughnessKernelSize = GetDefaultCloudKernelSize(entities);
					if (roughnessKernelSize < 0)
					{
						ccConsole::Error("Invalid kernel size!");
						return false;
					}
					bool ok;
					double val = QInputDialog::getDouble(parent, "Subdivide mesh", "Kernel size:", static_cast<double>(roughnessKernelSize), DBL_MIN, DBL_MAX, 8, &ok);
					if (!ok)
						return false;
					roughnessKernelSize = static_cast<PointCoordinateType>(val);
				}

				sfName = QString(CC_ROUGHNESS_FIELD_NAME)+QString("(%1)").arg(roughnessKernelSize);
			}
			break;

		default:
			assert(false);
			return false;
	}

	for (size_t i=0; i<selNum; ++i)
	{
		//is the ith selected data is elligible for processing?
		ccGenericPointCloud* cloud =0;
		switch(algo)
		{
		case CCLIB_ALGO_SF_GRADIENT:
			//for scalar field gradient, we can apply it directly on meshes
			bool lockedVertices;
			cloud = ccHObjectCaster::ToGenericPointCloud(entities[i],&lockedVertices);
			if (lockedVertices)
			{
				DisplayLockedVerticesWarning();
				cloud = 0;
			}
			if (cloud)
			{
				//but we need an already displayed SF!
				if (cloud->isA(CC_TYPES::POINT_CLOUD))
				{
					ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
					//on met en lecture (OUT) le champ scalaire actuellement affiche
					int outSfIdx = pc->getCurrentDisplayedScalarFieldIndex();
					if (outSfIdx < 0)
					{
						cloud = 0;
					}
					else
					{
						pc->setCurrentOutScalarField(outSfIdx);
						sfName = QString("%1(%2)").arg(CC_GRADIENT_NORMS_FIELD_NAME).arg(pc->getScalarFieldName(outSfIdx));
					}
				}
				else //if (!cloud->hasDisplayedScalarField()) //TODO: displayed but not necessarily set as OUTPUT!
				{
					cloud=0;
				}
			}
			break;

			//by default, we apply processings on clouds only
		default:
			if (entities[i]->isKindOf(CC_TYPES::POINT_CLOUD))
				cloud = ccHObjectCaster::ToGenericPointCloud(entities[i]);
			break;
		}

		if (cloud)
		{
			ccPointCloud* pc = 0;
			int sfIdx = -1;
			if (cloud->isA(CC_TYPES::POINT_CLOUD))
			{
				pc = static_cast<ccPointCloud*>(cloud);

				sfIdx = pc->getScalarFieldIndexByName(qPrintable(sfName));
				if (sfIdx < 0)
					sfIdx = pc->addScalarField(qPrintable(sfName));
				if (sfIdx >= 0)
					pc->setCurrentInScalarField(sfIdx);
				else
				{
					ccConsole::Error(QString("Failed to create scalar field on cloud '%1' (not enough memory?)").arg(pc->getName()));
					continue;
				}
			}

			ccProgressDialog pDlg(true,parent);

			ccOctree* octree = cloud->getOctree();
			if (!octree)
			{
				pDlg.show();
				octree = cloud->computeOctree(&pDlg);
				if (!octree)
				{
					ccConsole::Error(QString("Couldn't compute octree for cloud '%1'!").arg(cloud->getName()));
					break;
				}
			}

			int result = 0;
			QElapsedTimer eTimer;
			eTimer.start();
			switch(algo)
			{
			case CCLIB_ALGO_APPROX_DENSITY:
				result = CCLib::GeometricalAnalysisTools::computeLocalDensityApprox(cloud,
																					&pDlg,
																					octree);
				break;

			case CCLIB_ALGO_ACCURATE_DENSITY:
				result = CCLib::GeometricalAnalysisTools::computeLocalDensity(	cloud,
																				densityKernelSize,
																				&pDlg,
																				octree);
				break;

			case CCLIB_ALGO_CURVATURE:
				result = CCLib::GeometricalAnalysisTools::computeCurvature(	cloud,
																			curvType,
																			curvKernelSize,
																			&pDlg,
																			octree);
				break;

			case CCLIB_ALGO_SF_GRADIENT:
				result = CCLib::ScalarFieldTools::computeScalarFieldGradient(	cloud,
																				euclidian,
																				false,
																				&pDlg,
																				octree);

				//rename output scalar field
				if (result == 0)
				{
					int outSfIdx = pc->getCurrentDisplayedScalarFieldIndex();
					assert(outSfIdx >= 0);
					sfName = QString("%1.gradient").arg(pc->getScalarFieldName(outSfIdx));
					pc->renameScalarField(outSfIdx,qPrintable(sfName));
				}
				//*/
				break;

			case CCLIB_ALGO_ROUGHNESS:
				result = CCLib::GeometricalAnalysisTools::computeRoughness(	cloud,
																			roughnessKernelSize,
																			&pDlg,
																			octree);
				break;

				//TEST
			case CCLIB_SPHERICAL_NEIGHBOURHOOD_EXTRACTION_TEST:
				{
					unsigned count = cloud->size();
					cloud->enableScalarField();
					{
						for (unsigned j=0; j<count; ++j)
							cloud->setPointScalarValue(j,NAN_VALUE);
					}

					QElapsedTimer subTimer;
					subTimer.start();
					unsigned long long extractedPoints = 0;
					unsigned char level = octree->findBestLevelForAGivenNeighbourhoodSizeExtraction(roughnessKernelSize);;
					const unsigned samples = 1000;
					for (unsigned j=0; j<samples; ++j)
					{
						unsigned randIndex = (static_cast<unsigned>(static_cast<double>(rand())*static_cast<double>(count)/static_cast<double>(RAND_MAX)) % count);
						CCLib::DgmOctree::NeighboursSet neighbours;
						octree->getPointsInSphericalNeighbourhood(*cloud->getPoint(randIndex),roughnessKernelSize,neighbours,level);
						size_t neihgboursCount = neighbours.size();
						extractedPoints += static_cast<unsigned long long>(neihgboursCount);
						for (size_t k=0; k<neihgboursCount; ++k)
							cloud->setPointScalarValue(neighbours[k].pointIndex,static_cast<ScalarType>(sqrt(neighbours[k].squareDistd)));
					}
					ccConsole::Print("[SNE_TEST] Mean extraction time = %i ms (radius = %f, mean(neighbours) = %3.1f)",subTimer.elapsed(),roughnessKernelSize,static_cast<double>(extractedPoints)/static_cast<double>(samples));

					result = 0;
				}
				break;

			default:
				//missed something?
				assert(false);
			}
			qint64 elapsedTime_ms = eTimer.elapsed();

			if (result == 0)
			{
				if (pc && sfIdx >= 0)
				{
					pc->setCurrentDisplayedScalarField(sfIdx);
					pc->showSF(sfIdx >= 0);
					pc->getCurrentInScalarField()->computeMinAndMax();
				}
				cloud->prepareDisplayForRefresh();
				ccConsole::Print("[Algortihm] Timing: %3.2f s.",static_cast<double>(elapsedTime_ms)/1.0e3);
			}
			else
			{
				ccConsole::Warning(QString("Failed to apply processing to cloud '%1'").arg(cloud->getName()));
				if (pc && sfIdx >= 0)
				{
					pc->deleteScalarField(sfIdx);
					sfIdx = -1;
				}
			}
		}
	}

	return true;
}

void MainWindow::doActionCloudCloudDist()
{
	size_t selNum = m_selectedEntities.size();
	if (selNum != 2)
	{
		ccConsole::Error("Select 2 point clouds!");
		return;
	}

	if (!m_selectedEntities[0]->isKindOf(CC_TYPES::POINT_CLOUD) ||
		!m_selectedEntities[1]->isKindOf(CC_TYPES::POINT_CLOUD))
	{
		ccConsole::Error("Select 2 point clouds!");
		return;
	}

	ccOrderChoiceDlg dlg(	m_selectedEntities[0], "Compared",
							m_selectedEntities[1], "Reference",
							this );
	if (!dlg.exec())
		return;

	ccGenericPointCloud* compCloud = ccHObjectCaster::ToGenericPointCloud(dlg.getFirstEntity());
	ccGenericPointCloud* refCloud = ccHObjectCaster::ToGenericPointCloud(dlg.getSecondEntity());

	//assert(!m_compDlg);
	if (m_compDlg)
		delete m_compDlg;
	m_compDlg = new ccComparisonDlg(compCloud, refCloud, ccComparisonDlg::CLOUDCLOUD_DIST, this);
	connect(m_compDlg, SIGNAL(finished(int)), this, SLOT(deactivateComparisonMode(int)));
	m_compDlg->show();
	//cDlg.setModal(false);
	//cDlg.exec();
	freezeUI(true);
}

void MainWindow::doActionCloudMeshDist()
{
	size_t selNum = m_selectedEntities.size();
	if (selNum != 2)
	{
		ccConsole::Error("Select 2 entities!");
		return;
	}

	bool isMesh[2] = {false,false};
	unsigned meshNum = 0;
	unsigned cloudNum = 0;
	for (unsigned i=0; i<2; ++i)
	{
		if (m_selectedEntities[i]->isKindOf(CC_TYPES::MESH))
		{
			++meshNum;
			isMesh[i] = true;
		}
		else if (m_selectedEntities[i]->isKindOf(CC_TYPES::POINT_CLOUD))
		{
			++cloudNum;
		}
	}

	if (meshNum == 0)
	{
		ccConsole::Error("Select at least one mesh!");
		return;
	}
	else if (meshNum+cloudNum < 2)
	{
		ccConsole::Error("Select one mesh and one cloud or two meshes!");
		return;
	}

	ccHObject* compEnt = 0;
	ccGenericMesh* refMesh = 0;

	if (meshNum == 1)
	{
		compEnt = m_selectedEntities[isMesh[0] ? 1 : 0];
		refMesh = ccHObjectCaster::ToGenericMesh(m_selectedEntities[isMesh[0] ? 0 : 1]);
	}
	else
	{
		ccOrderChoiceDlg dlg(	m_selectedEntities[0], "Compared",
								m_selectedEntities[1], "Reference",
								this );
		if (!dlg.exec())
			return;

		compEnt = dlg.getFirstEntity();
		refMesh = ccHObjectCaster::ToGenericMesh(dlg.getSecondEntity());
	}

	//assert(!m_compDlg);
	if (m_compDlg)
		delete m_compDlg;
	m_compDlg = new ccComparisonDlg(compEnt, refMesh, ccComparisonDlg::CLOUDMESH_DIST, this);
	connect(m_compDlg, SIGNAL(finished(int)), this, SLOT(deactivateComparisonMode(int)));
	m_compDlg->show();

	freezeUI(true);
}

void MainWindow::deactivateComparisonMode(int)
{
	//DGM: a bug apperead with recent changes (from CC or QT?)
	//which prevent us from deleting the dialog right away...
	//(it seems that QT has not yet finished the dialog closing
	//when the 'finished' signal is sent).
	//if(m_compDlg)
	//	delete m_compDlg;
	//m_compDlg = 0;

	freezeUI(false);

	updateUI();
}

void MainWindow::toggleActiveWindowSunLight()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
	{
		win->toggleSunLight();
		win->redraw();
	}
}

void MainWindow::toggleActiveWindowCustomLight()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
	{
		win->toggleCustomLight();
		win->redraw();
	}
}

void MainWindow::toggleActiveWindowCenteredPerspective()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
	{
		win->togglePerspective(true);
		win->redraw();
		updateViewModePopUpMenu(win);
	}
}

void MainWindow::toggleActiveWindowViewerBasedPerspective()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
	{
		win->togglePerspective(false);
		win->redraw();
		updateViewModePopUpMenu(win);
	}
}

void MainWindow::doActionDeleteShader()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
		win->setShader(0);
}

void MainWindow::doDisableGLFilter()
{
	ccGLWindow* win = getActiveGLWindow();
	if (win)
	{
		win->setGlFilter(0);
		win->redraw();
	}
}

void MainWindow::removeFromDB(ccHObject* obj, bool autoDelete/*=true*/)
{
	if (!obj)
		return;

	//remove dependency to avoid deleting the object when removing it from DB tree
	if (!autoDelete && obj->getParent())
		obj->getParent()->removeDependencyWith(obj);

	if (m_ccRoot)
		m_ccRoot->removeElement(obj);
}

void MainWindow::setSelectedInDB(ccHObject* obj, bool selected)
{
	if (obj && m_ccRoot)
	{
		if (selected)
			m_ccRoot->selectEntity(obj);
		else
			m_ccRoot->unselectEntity(obj);
	}
}

void MainWindow::addToDB(	ccHObject* obj,
							bool updateZoom/*=true*/,
							bool autoExpandDBTree/*=true*/,
							bool checkDimensions/*=true*/ )
{
	//let's check that the new entity is not too big nor too far from scene center!
	if (checkDimensions)
	{
		//get entity bounding box
		ccBBox bBox = obj->getBB();

		CCVector3 center = bBox.getCenter();
		PointCoordinateType diag = bBox.getDiagNorm();

		double P[3] = {center[0],center[1],center[2]};
		CCVector3d Pshift(0,0,0);
		double scale = 1.0;
		bool applyAll = false;
		bool shiftAlreadyEnabled = false;
		//here we must test that coordinates are not too big whatever the case because OpenGL
		//really don't like big ones (even if we work with GLdoubles).
		if (ccCoordinatesShiftManager::Handle(P,diag,true,shiftAlreadyEnabled,Pshift,&scale,applyAll))
		{
			//apply global shift
			if (Pshift.norm2() > 0)
			{
				ccGLMatrix mat;
				mat.toIdentity();
				mat.data()[12] = static_cast<float>(Pshift.x);
				mat.data()[13] = static_cast<float>(Pshift.y);
				mat.data()[14] = static_cast<float>(Pshift.z);
				obj->applyGLTransformation_recursive(&mat);
				ccConsole::Warning(QString("Entity '%1' has been translated: (%2,%3,%4) [original position will be restored when saving]").arg(obj->getName()).arg(Pshift.x,0,'f',2).arg(Pshift.y,0,'f',2).arg(Pshift.z,0,'f',2));
			}

			//apply global scale
			if (scale != 1.0)
			{
				ccGLMatrix mat;
				mat.toIdentity();
				mat.data()[0] = mat.data()[5] = mat.data()[10] = static_cast<float>(scale);
				obj->applyGLTransformation_recursive(&mat);
				ccConsole::Warning(QString("Entity '%1' has been rescaled: X%2 [original scale will be restored when saving]").arg(obj->getName()).arg(scale,0,'f',6));
			}

			//update 'global shift' and 'global scale' for ALL clouds recursively
			//FIXME: why don't we do that all the time?!
			ccHObject::Container children;
			children.push_back(obj);
			while (!children.empty())
			{
				ccHObject* child = children.back();
				children.pop_back();

				if (child->isKindOf(CC_TYPES::POINT_CLOUD))
				{
					ccGenericPointCloud* pc = ccHObjectCaster::ToGenericPointCloud(child);
					pc->setGlobalShift(pc->getGlobalShift() + Pshift);
					pc->setGlobalScale(pc->getGlobalScale() * scale);
				}

				for (unsigned i=0; i<child->getChildrenNumber(); ++i)
					children.push_back(child->getChild(i));
			}
		}
	}

	//add object to DB root
	if (m_ccRoot)
	{
		//force a 'global zoom' if the DB was emtpy!
		if (!m_ccRoot->getRootEntity() || m_ccRoot->getRootEntity()->getChildrenNumber() == 0)
			updateZoom = true;
		m_ccRoot->addElement(obj,autoExpandDBTree);
	}
	else
	{
		ccLog::Warning("[MainWindow::addToDB] Internal error: no associated db?!");
		assert(false);
	}

	//we can now set destination display (if none already)
	if (!obj->getDisplay())
	{
		ccGLWindow* activeWin = getActiveGLWindow();
		if (!activeWin)
		{
			//no active GL window?!
			return;
		}
		obj->setDisplay_recursive(activeWin);
	}

	//eventually we update the corresponding display
	assert(obj->getDisplay());
	if (updateZoom)
	{
		static_cast<ccGLWindow*>(obj->getDisplay())->zoomGlobal(); //automatically calls ccGLWindow::redraw
	}
	else
	{
		obj->prepareDisplayForRefresh();
		refreshAll();
	}
}

void MainWindow::addToDBAuto(const QStringList& filenames)
{
	ccGLWindow* win = qobject_cast<ccGLWindow*>(QObject::sender());

	addToDB(filenames, UNKNOWN_FILE, win);
}

void MainWindow::addToDB(const QStringList& filenames, CC_FILE_TYPES fType, ccGLWindow* destWin/*=0*/)
{
	//to handle same 'shift on load' for multiple files
	CCVector3d loadCoordinatesShift(0,0,0);
	bool loadCoordinatesTransEnabled = false;

	//the same for 'addToDB' (if the first one is not supported, or if the scale remains too big)
	CCVector3d addCoordinatesShift(0,0,0);
	bool addCoordinatesTransEnabled = false;
	double addCoordinatesScale = 1.0;

	for (int i=0; i<filenames.size(); ++i)
	{
		ccHObject* newGroup = FileIOFilter::LoadFromFile(filenames[i],fType,true,&loadCoordinatesTransEnabled,&loadCoordinatesShift);

		if (newGroup)
		{
			if (destWin)
				newGroup->setDisplay_recursive(destWin);
			addToDB(newGroup,true,true,false);
		}
	}

	QMainWindow::statusBar()->showMessage(QString("%1 file(s) loaded").arg(filenames.size()),2000);
}

void MainWindow::handleNewLabel(ccHObject* entity)
{
	assert(entity);
	if (entity)
		addToDB(entity);
}

void MainWindow::forceConsoleDisplay()
{
	//if the console is hidden, we autoamtically display it!
	if (DockableConsole && DockableConsole->isHidden())
	{
		DockableConsole->show();
		QApplication::processEvents();
	}
}

ccColorScalesManager* MainWindow::getColorScalesManager()
{
	return ccColorScalesManager::GetUniqueInstance();
}

void MainWindow::closeAll()
{
	if (!m_ccRoot)
		return;

	if (QMessageBox::question(	this, "Close all", "Are you sure you want to remove all loaded entities?", QMessageBox::Yes, QMessageBox::No ) != QMessageBox::Yes)
		return;

	ccHObject* root = m_ccRoot->getRootEntity();
	if (root)
	{
		m_ccRoot->unloadAll();
	}

	redrawAll();
}

void MainWindow::loadFile()
{
	QSettings settings;
	settings.beginGroup(s_psLoadFile);
	QString currentPath = settings.value(s_psCurrentPath,QApplication::applicationDirPath()).toString();
	int currentOpenDlgFilter = settings.value(s_psSelectedFilter,BIN).toInt();

	// Add all available file extension filters to a single QString.
	// Each filter entry is separated by double semicolon ";;".
	QString filters;
	filters.append(QString(CC_FILE_TYPE_FILTERS[UNKNOWN_FILE]) + ";;");
	filters.append(QString(CC_FILE_TYPE_FILTERS[BIN]) + ";;");
	filters.append(QString(CC_FILE_TYPE_FILTERS[ASCII]) + ";;");
	filters.append(QString(CC_FILE_TYPE_FILTERS[PTX]) + ";;");
	filters.append(QString(CC_FILE_TYPE_FILTERS[PLY]) + ";;");
	filters.append(QString(CC_FILE_TYPE_FILTERS[OBJ]) + ";;");
	filters.append(QString(CC_FILE_TYPE_FILTERS[VTK]) + ";;");
	filters.append(QString(CC_FILE_TYPE_FILTERS[STL]) + ";;");
	filters.append(QString(CC_FILE_TYPE_FILTERS[OFF]) + ";;");
#ifdef CC_FBX_SUPPORT
	filters.append(QString(CC_FILE_TYPE_FILTERS[FBX]) + ";;");
#endif
	filters.append(QString(CC_FILE_TYPE_FILTERS[PCD]) + ";;");
#ifdef CC_X3D_SUPPORT
	filters.append(QString(CC_FILE_TYPE_FILTERS[X3D]) + ";;");
#endif

#ifdef CC_LAS_SUPPORT
	filters.append(QString(CC_FILE_TYPE_FILTERS[LAS]) + ";;");
#endif
#ifdef CC_E57_SUPPORT
	filters.append(QString(CC_FILE_TYPE_FILTERS[E57]) + ";;");
#endif
#ifdef CC_DXF_SUPPORT
	filters.append(QString(CC_FILE_TYPE_FILTERS[DXF]) + ";;");
#endif
#ifdef CC_PDMS_SUPPORT
	filters.append(QString(CC_FILE_TYPE_FILTERS[PDMS]) + ";;");
#endif
#ifdef CC_GDAL_SUPPORT
	filters.append(QString(CC_FILE_TYPE_FILTERS[RASTER]) + ";;");
#endif
	filters.append(QString(CC_FILE_TYPE_FILTERS[SOI]) + ";;");
	filters.append(QString(CC_FILE_TYPE_FILTERS[PN]) + ";;");
	filters.append(QString(CC_FILE_TYPE_FILTERS[PV]) + ";;");
	filters.append(QString(CC_FILE_TYPE_FILTERS[POV]) + ";;");
	filters.append(QString(CC_FILE_TYPE_FILTERS[ICM]) + ";;");
	filters.append(QString(CC_FILE_TYPE_FILTERS[BUNDLER])/*+ ";;"*/);

	//currently selected filter
	QString selectedFilter = CC_FILE_TYPE_FILTERS[currentOpenDlgFilter];

	//file choosing dialog
	QStringList selectedFiles = QFileDialog::getOpenFileNames(	this,
																tr("Open file(s)"),
																currentPath,
																filters,
																&selectedFilter
#ifdef _DEBUG
																,QFileDialog::DontUseNativeDialog
#endif
															);
	if (selectedFiles.isEmpty())
		return;

	CC_FILE_TYPES fType = UNKNOWN_FILE;
	for (unsigned i=0;i<static_cast<unsigned>(FILE_TYPES_COUNT);++i)
	{
		if (selectedFilter == QString(CC_FILE_TYPE_FILTERS[i]))
		{
			fType = CC_FILE_TYPES_ENUMS[i];
			break;
		}
	}

	//load files
	addToDB(selectedFiles,fType);

	//we update current file path
	currentPath = QFileInfo(selectedFiles[0]).absolutePath();
	currentOpenDlgFilter = fType;

	//save last loading location
	settings.setValue(s_psCurrentPath,currentPath);
	settings.setValue(s_psSelectedFilter,currentOpenDlgFilter);
	settings.endGroup();
}

//Helper: check for a filename validity
static bool IsValidFileName(QString filename)
{
#ifdef CC_WINDOWS
	QString sPattern("^(?!^(PRN|AUX|CLOCK\\$|NUL|CON|COM\\d|LPT\\d|\\..*)(\\..+)?$)[^\\x00-\\x1f\\\\?*:\\"";|/]+$");
#else
	QString sPattern("^(([a-zA-Z]:|\\\\)\\\\)?(((\\.)|(\\.\\.)|([^\\\\/:\\*\\?""\\|<>\\. ](([^\\\\/:\\*\\?""\\|<>\\. ])|([^\\\\/:\\*\\?""\\|<>]*[^\\\\/:\\*\\?""\\|<>\\. ]))?))\\\\)*[^\\\\/:\\*\\?""\\|<>\\. ](([^\\\\/:\\*\\?""\\|<>\\. ])|([^\\\\/:\\*\\?""\\|<>]*[^\\\\/:\\*\\?""\\|<>\\. ]))?$");
#endif

	return QRegExp(sPattern).exactMatch(filename);
}

void MainWindow::saveFile()
{
	size_t selNum = m_selectedEntities.size();
	if (selNum == 0)
		return;

	//persistent settings
	QSettings settings;
	settings.beginGroup(s_psSaveFile);
	QString currentPath = settings.value(s_psCurrentPath,QApplication::applicationDirPath()).toString();
	int currentCloudSaveDlgFilter = settings.value(s_psSelectedFilterCloud,BIN).toInt();
	int currentMeshSaveDlgFilter = settings.value(s_psSelectedFilterMesh,PLY).toInt();

	ccHObject clouds("clouds");
	ccHObject meshes("meshes");
	ccHObject images("images");
	ccHObject polylines("polylines");
	ccHObject other("other");
	ccHObject otherSerializable("serializable");
	ccHObject::Container entitiesToSave;
	entitiesToSave.insert(entitiesToSave.begin(),m_selectedEntities.begin(),m_selectedEntities.end());
	while (!entitiesToSave.empty())
	{
		ccHObject* child = entitiesToSave.back();
		entitiesToSave.pop_back();

		if (child->isA(CC_TYPES::HIERARCHY_OBJECT))
		{
			for (unsigned j=0;j<child->getChildrenNumber();++j)
				entitiesToSave.push_back(child->getChild(j));
		}
		else
		{
			//we put entity in the container corresponding to its type
			ccHObject* dest = 0;
			if (child->isA(CC_TYPES::POINT_CLOUD))
				dest = &clouds;
			else if (child->isKindOf(CC_TYPES::MESH))
				dest = &meshes;
			else if (child->isKindOf(CC_TYPES::IMAGE))
				dest = &images;
			else if (child->isKindOf(CC_TYPES::POLY_LINE))
				dest = &polylines;
			else if (child->isSerializable())
				dest = &otherSerializable;
			else
				dest = &other;

			assert(dest);

			//we don't want double insertions if the user has clicked both the father and child
			if (!dest->find(child->getUniqueID()))
				dest->addChild(child,ccHObject::DP_NONE);
		}
	}

	bool hasCloud = (clouds.getChildrenNumber() != 0);
	bool hasMesh = (meshes.getChildrenNumber() != 0);
	bool hasImage = (images.getChildrenNumber() != 0);
	bool hasPolylines = (polylines.getChildrenNumber() != 0);
	bool hasSerializable = (otherSerializable.getChildrenNumber() != 0);
	bool hasOther = (other.getChildrenNumber() != 0);

	int stdSaveTypes =		static_cast<int>(hasCloud)
						+	static_cast<int>(hasMesh)
						+	static_cast<int>(hasImage)
						+	static_cast<int>(hasPolylines)
						+	static_cast<int>(hasSerializable);
	if (stdSaveTypes == 0)
	{
		ccConsole::Error("Can't save selected entity(ies) this way!");
		return;
	}

	//we set up the right file filters, depending on the selected
	//entities shared type (cloud, mesh, etc.).
	QString filters;

	//From now on, BIN format handles about anyhting!
	filters.append(QString(CC_FILE_TYPE_FILTERS[BIN]) + ";;");

	ccHObject* toSave = 0;
	QString selectedFilter = CC_FILE_TYPE_FILTERS[BIN];

	//if we only have one type of entity selected, then we can let the user choose specific formats
	if (stdSaveTypes == 1)
	{
		if (hasCloud)
		{
			toSave = &clouds;
			selectedFilter = CC_FILE_TYPE_FILTERS[currentCloudSaveDlgFilter];

			//add cloud output file filters
			filters.append(QString(CC_FILE_TYPE_FILTERS[ASCII]) + ";;");
#ifdef CC_E57_SUPPORT
			filters.append(QString(CC_FILE_TYPE_FILTERS[E57]) + ";;");
#endif
			if (clouds.getChildrenNumber() == 1)
			{
				filters.append(QString(CC_FILE_TYPE_FILTERS[PLY]) + ";;");
				filters.append(QString(CC_FILE_TYPE_FILTERS[VTK]) + ";;");
#ifdef CC_LAS_SUPPORT
				filters.append(QString(CC_FILE_TYPE_FILTERS[LAS]) + ";;");
#endif
				filters.append(QString(CC_FILE_TYPE_FILTERS[PN]) + ";;");
				filters.append(QString(CC_FILE_TYPE_FILTERS[PV]) + ";;");
			}
			//TODO: POV files handling!
			//filters.append(CC_FILE_TYPE_FILTERS[POV]);
			//filters.append("\n");
		}
		else if (hasMesh)
		{
			if (meshes.getChildrenNumber() == 1)
			{
				toSave = &meshes;
				selectedFilter = CC_FILE_TYPE_FILTERS[currentMeshSaveDlgFilter];

				//add meshes output file filters
				filters.append(QString(CC_FILE_TYPE_FILTERS[OBJ])+";;");
				filters.append(QString(CC_FILE_TYPE_FILTERS[PLY])+";;");
				filters.append(QString(CC_FILE_TYPE_FILTERS[VTK])+";;");
				filters.append(QString(CC_FILE_TYPE_FILTERS[STL])+";;");
				filters.append(QString(CC_FILE_TYPE_FILTERS[OFF])+";;");
#ifdef CC_X3D_SUPPORT
				filters.append(QString(CC_FILE_TYPE_FILTERS[X3D])+";;");
#endif
				filters.append(QString(CC_FILE_TYPE_FILTERS[MA])+";;");
			}
#ifdef CC_FBX_SUPPORT
			filters.append(QString(CC_FILE_TYPE_FILTERS[FBX]) + ";;");
#endif
		}
		else if (hasPolylines)
		{
#ifdef CC_DXF_SUPPORT
			filters.append(QString(CC_FILE_TYPE_FILTERS[DXF])+";;");
#endif
			toSave = &polylines;
		}
		else if (hasImage)
		{
			if (images.getChildrenNumber()>1)
			{
				ccConsole::Warning("[MainWindow::saveFile] Only BIN format is able to store multiple images at once");
			}
			else
			{
				toSave = &images;

				//add images output file filters
				//we grab the list of supported image file formats (writing)
				QList<QByteArray> formats = QImageWriter::supportedImageFormats();
				//we convert this list into a proper "filters" string
				for (int i=0; i<formats.size(); ++i)
					filters.append(QString("%1 image (*.%2);;").arg(QString(formats[i].data()).toUpper()).arg(formats[i].data()));
			}
		}
	}

	QString dir = currentPath+QString("/");
	if (selNum == 1)
	{
		//hierarchy objects have generally as name: 'filename.ext (fullpath)'
		//so we must only take the first part! (otherwise this type of name
		//with a path inside perturbs the QFileDialog a lot ;))
		QString defaultFileName(m_selectedEntities[0]->getName());
		if (m_selectedEntities[0]->isA(CC_TYPES::HIERARCHY_OBJECT))
		{
			QStringList parts = defaultFileName.split(' ',QString::SkipEmptyParts);
			if (parts.size() > 0)
				defaultFileName = parts[0];
		}

		if (!QFileInfo(defaultFileName).suffix().isEmpty()) //we remove extension
			defaultFileName = QFileInfo(defaultFileName).baseName();

		if (!IsValidFileName(defaultFileName))
		{
			ccLog::Warning("[MainWindow::saveFile] First entity's name would make an invalid filename! Can't use it...");
			defaultFileName = "project";
		}

		dir += defaultFileName;
	}

	QString selectedFilename = QFileDialog::getSaveFileName(this,
															tr("Save file"),
															dir,
															filters,
															&selectedFilter);

	if (selectedFilename.isEmpty())
		return;

	//ignored items
	if (hasOther)
	{
		ccConsole::Warning("[MainWindow::saveFile] The following selected entites won't be saved:");
		for (unsigned i=0;i<other.getChildrenNumber();++i)
			ccConsole::Warning(QString("\t- %1s").arg(other.getChild(i)->getName()));
	}

	CC_FILE_ERROR result = CC_FERR_NO_ERROR;
	//bin format
	if (selectedFilter == QString(CC_FILE_TYPE_FILTERS[BIN]))
	{
		if (selNum == 1)
			result = FileIOFilter::SaveToFile(m_selectedEntities[0],qPrintable(selectedFilename),BIN);
		else
		{
			ccHObject::Container tempContainer;
			RemoveSiblings(m_selectedEntities,tempContainer);

			if (tempContainer.size())
			{
				ccHObject root;
				for (size_t i=0; i<tempContainer.size(); ++i)
					root.addChild(tempContainer[i],ccHObject::DP_NONE);
				result = FileIOFilter::SaveToFile(&root,qPrintable(selectedFilename),BIN);
			}
			else
			{
				ccLog::Warning("[MainWindow::saveFile] No selected entity could be saved!");
				result = CC_FERR_NO_SAVE;
			}
		}

		currentCloudSaveDlgFilter = BIN;
	}
	else if (toSave)
	{
		//ignored items
		if (hasSerializable)
		{
			if (!hasOther)
				ccConsole::Warning("[MainWindow::saveFile] The following selected entites won't be saved:"); //display this warning only if not already done
			for (unsigned i=0; i<otherSerializable.getChildrenNumber(); ++i)
				ccConsole::Warning(QString("\t- %1").arg(otherSerializable.getChild(i)->getName()));
		}

		if (hasCloud || hasMesh)
		{
			CC_FILE_TYPES fType = UNKNOWN_FILE;
			for (unsigned i=0; i<static_cast<unsigned>(FILE_TYPES_COUNT); ++i)
			{
				if (selectedFilter == QString(CC_FILE_TYPE_FILTERS[i]))
				{
					fType = CC_FILE_TYPES_ENUMS[i];
					break;
				}
			}

			if (hasCloud)
				currentCloudSaveDlgFilter = fType;
			else if (hasMesh)
				currentMeshSaveDlgFilter = fType;

			result = FileIOFilter::SaveToFile(	toSave->getChildrenNumber() > 1 ? toSave : toSave->getChild(0),
												qPrintable(selectedFilename),
												fType);
		}
		else if (hasPolylines)
		{
#ifdef CC_DXF_SUPPORT
			result = FileIOFilter::SaveToFile(	toSave,
												qPrintable(selectedFilename),
												DXF);
#endif
		}
		else if (hasImage)
		{
			assert(images.getChildrenNumber() == 1);
			ccImage* image = static_cast<ccImage*>(images.getChild(0));
			if (!image->data().save(selectedFilename))
				result = CC_FERR_WRITING;
		}
	}

	if (result != CC_FERR_NO_ERROR)
	{
		FileIOFilter::DisplayErrorMessage(result,"saving",selectedFilename);
	}
	else
	{
		ccLog::Print(QString("[I/O] File '%1' saved successfully").arg(selectedFilename));
	}

	//we update current file path
	currentPath = QFileInfo(selectedFilename).absolutePath();

	settings.setValue(s_psCurrentPath,currentPath);
	settings.setValue(s_psSelectedFilterCloud,static_cast<int>(currentCloudSaveDlgFilter));
	settings.setValue(s_psSelectedFilterMesh,static_cast<int>(currentMeshSaveDlgFilter));
	settings.endGroup();
}

void MainWindow::on3DViewActivated(QMdiSubWindow* mdiWin)
{
	ccGLWindow* win = mdiWin ? static_cast<ccGLWindow*>(mdiWin->widget()) : 0;
	if (win)
	{
		updateViewModePopUpMenu(win);
		updatePivotVisibilityPopUpMenu(win);


	}
	else
	{
		if (m_viewModePopupButton)
		{
		}
	}
}

void MainWindow::updateViewModePopUpMenu(ccGLWindow* win)
{
	if (!m_viewModePopupButton)
		return;

	//update the view mode pop-up 'top' icon
	if (win)
	{
		bool objectCentered = true;
		bool perspectiveEnabled = win->getPerspectiveState(objectCentered);

		QAction* currentModeAction = 0;
		if (!perspectiveEnabled)
			currentModeAction = actionSetOrthoView;
		else if (objectCentered)
			currentModeAction = actionSetCenteredPerspectiveView;
		else
			currentModeAction = actionSetViewerPerspectiveView;

		assert(currentModeAction);
		m_viewModePopupButton->setIcon(currentModeAction->icon());
		m_viewModePopupButton->setEnabled(true);
	}
	else
	{
		m_viewModePopupButton->setIcon(QIcon());
		m_viewModePopupButton->setEnabled(false);
	}
}

void MainWindow::updatePivotVisibilityPopUpMenu(ccGLWindow* win)
{
	if (!m_pivotVisibilityPopupButton)
		return;

	//update the pivot visibility pop-up 'top' icon
	if (win)
	{
		QAction* visibilityAction = 0;
		switch(win->getPivotVisibility())
		{
		case ccGLWindow::PIVOT_HIDE:
			visibilityAction = actionSetPivotOff;
			break;
		case ccGLWindow::PIVOT_SHOW_ON_MOVE:
			visibilityAction = actionSetPivotRotationOnly;
			break;
		case ccGLWindow::PIVOT_ALWAYS_SHOW:
			visibilityAction = actionSetPivotAlwaysOn;
			break;
		default:
			assert(false);
		}

		if (visibilityAction)
			m_pivotVisibilityPopupButton->setIcon(visibilityAction->icon());

		//pivot is not available in viewer-based perspective!
		bool objectCentered = true;
		win->getPerspectiveState(objectCentered);
		m_pivotVisibilityPopupButton->setEnabled(objectCentered);
	}
	else
	{
		m_pivotVisibilityPopupButton->setIcon(QIcon());
		m_pivotVisibilityPopupButton->setEnabled(false);
	}
}


void MainWindow::updateMenus()
{
	ccGLWindow* win = getActiveGLWindow();
	bool hasMdiChild = (win != 0);
	bool hasSelectedEntities = (m_ccRoot && m_ccRoot->countSelectedEntities()>0);

	//General Menu
	menuEdit->setEnabled(true/*hasSelectedEntities*/);
	menuTools->setEnabled(true/*hasSelectedEntities*/);

	//3D Views Menu
	actionClose3DView->setEnabled(hasMdiChild);
	actionCloseAll3DViews->setEnabled(hasMdiChild);
	actionTile3DViews->setEnabled(hasMdiChild);
	actionCascade3DViews->setEnabled(hasMdiChild);
	actionNext3DView->setEnabled(hasMdiChild);
	actionPrevious3DView->setEnabled(hasMdiChild);

	//Shaders & Filters display Menu
	bool shadersEnabled = (win ? win->areShadersEnabled() : false);
	actionLoadShader->setEnabled(shadersEnabled);
	actionDeleteShader->setEnabled(shadersEnabled);

	bool filtersEnabled = (win ? win->areGLFiltersEnabled() : false);
	actionNoFilter->setEnabled(filtersEnabled);

	//View Menu
	toolBarView->setEnabled(hasMdiChild);

	//oher actions
	actionSegment->setEnabled(hasMdiChild && hasSelectedEntities);
	actionTranslateRotate->setEnabled(hasMdiChild && hasSelectedEntities);
	actionPointPicking->setEnabled(hasMdiChild);
	//actionPointListPicking->setEnabled(hasMdiChild);
	actionTestFrameRate->setEnabled(hasMdiChild);
	actionRenderToFile->setEnabled(hasMdiChild);
	actionToggleSunLight->setEnabled(hasMdiChild);
	actionToggleCustomLight->setEnabled(hasMdiChild);
	actionToggleCenteredPerspective->setEnabled(hasMdiChild);
	actionToggleViewerBasedPerspective->setEnabled(hasMdiChild);

	//plugins
	foreach (QAction* act, m_glFilterActions.actions())
		act->setEnabled(hasMdiChild);
}

void MainWindow::update3DViewsMenu()
{
	menu3DViews->clear();
	menu3DViews->addAction(actionNew3DView);
	menu3DViews->addSeparator();
	menu3DViews->addAction(actionClose3DView);
	menu3DViews->addAction(actionCloseAll3DViews);
	menu3DViews->addSeparator();
	menu3DViews->addAction(actionTile3DViews);
	menu3DViews->addAction(actionCascade3DViews);
	menu3DViews->addSeparator();
	menu3DViews->addAction(actionNext3DView);
	menu3DViews->addAction(actionPrevious3DView);

	QList<QMdiSubWindow *> windows = m_mdiArea->subWindowList();
	if (!windows.isEmpty())
	{
		//Dynamic Separator
		QAction* separator = new QAction(this);
		separator->setSeparator(true);
		menu3DViews->addAction(separator);

		for (int i=0; i<windows.size(); ++i)
		{
			QWidget *child = windows.at(i)->widget();

			QString text = QString("&%1 %2").arg(i + 1).arg(child->windowTitle());
			QAction *action = menu3DViews->addAction(text);
			action->setCheckable(true);
			action ->setChecked(child == getActiveGLWindow());
			connect(action, SIGNAL(triggered()), m_windowMapper, SLOT(map()));
			m_windowMapper->setMapping(action, windows.at(i));
		}
	}
}

void MainWindow::setActiveSubWindow(QWidget *window)
{
	if (!window || !m_mdiArea)
		return;
	m_mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
}

void MainWindow::redrawAll()
{
	QList<QMdiSubWindow*> windows = m_mdiArea->subWindowList();
	for (int i=0; i<windows.size(); ++i)
		static_cast<ccGLWindow*>(windows.at(i)->widget())->redraw();
}

void MainWindow::refreshAll()
{
	QList<QMdiSubWindow*> windows = m_mdiArea->subWindowList();
	for (int i=0; i<windows.size(); ++i)
		static_cast<ccGLWindow*>(windows.at(i)->widget())->refresh();
}

void MainWindow::updateUI()
{
	updateUIWithSelection();
	updateMenus();
	if (m_ccRoot)
		m_ccRoot->updatePropertiesView();
}

void MainWindow::updateUIWithSelection()
{
	dbTreeSelectionInfo selInfo;

	m_selectedEntities.clear();

	if (m_ccRoot)
		m_ccRoot->getSelectedEntities(m_selectedEntities,CC_TYPES::OBJECT,&selInfo);
	//expandDBTreeWithSelection(m_selectedEntities);

	enableUIItems(selInfo);
}

void MainWindow::expandDBTreeWithSelection(ccHObject::Container& selection)
{
	if (!m_ccRoot)
		return;

	size_t selNum = selection.size();
	for (size_t i=0; i<selNum; ++i)
		m_ccRoot->expandElement(selection[i],true);
}

void MainWindow::enableAll()
{
	QList<QMdiSubWindow *> windows = m_mdiArea->subWindowList();
	for (int i=0; i<windows.size(); ++i)
		windows.at(i)->setEnabled(true);
}

void MainWindow::disableAll()
{
	QList<QMdiSubWindow *> windows = m_mdiArea->subWindowList();
	for (int i=0; i<windows.size(); ++i)
		windows.at(i)->setEnabled(false);
}

void MainWindow::disableAllBut(ccGLWindow* win)
{
	//we disable all other windows
	QList<QMdiSubWindow*> windows = m_mdiArea->subWindowList();
	for (int i=0; i<windows.size(); ++i)
		if (static_cast<ccGLWindow*>(windows.at(i)->widget()) != win)
			windows.at(i)->setEnabled(false);
}

void MainWindow::enableUIItems(dbTreeSelectionInfo& selInfo)
{
	//>0
	bool atLeastOneEntity = (selInfo.selCount>0);
	bool atLeastOneCloud = (selInfo.cloudCount>0);
	bool atLeastOneMesh = (selInfo.meshCount>0);
	//bool atLeastOneOctree = (selInfo.octreeCount>0);
	bool atLeastOneNormal = (selInfo.normalsCount>0);
	bool atLeastOneColor = (selInfo.colorCount>0);
	bool atLeastOneSF = (selInfo.sfCount>0);
	//bool atLeastOneSensor = (selInfo.sensorCount>0);
	bool atLeastOneGBLSensor = (selInfo.gblSensorCount>0);
	bool atLeastOneCameraSensor = (selInfo.cameraSensorCount>0);
	bool activeWindow = (getActiveGLWindow() != 0);

	//menuEdit->setEnabled(atLeastOneEntity);
	//menuTools->setEnabled(atLeastOneEntity);
	menuCreateSensor->setEnabled(atLeastOneCloud);
	menuGroundBasedLidar->setEnabled(atLeastOneGBLSensor);
	menuCameraSensor->setEnabled(atLeastOneCameraSensor);

	actionZoomAndCenter->setEnabled(atLeastOneEntity && activeWindow);
	actionSave->setEnabled(atLeastOneEntity);
	actionClone->setEnabled(atLeastOneEntity);
	actionDelete->setEnabled(atLeastOneEntity);
	actionExportCoordToSF->setEnabled(atLeastOneEntity);
	actionSegment->setEnabled(atLeastOneEntity && activeWindow);
	actionTranslateRotate->setEnabled(atLeastOneEntity && activeWindow);
	actionShowDepthBuffer->setEnabled(atLeastOneGBLSensor);
	actionExportDepthBuffer->setEnabled(atLeastOneGBLSensor);
	actionResampleWithOctree->setEnabled(atLeastOneCloud);
	actionApplyScale->setEnabled(atLeastOneCloud || atLeastOneMesh);
	actionApplyTransformation->setEnabled(atLeastOneEntity);
	actionComputeOctree->setEnabled(atLeastOneCloud || atLeastOneMesh);
	actionComputeNormals->setEnabled(atLeastOneCloud || atLeastOneMesh);
	actionSetColorGradient->setEnabled(atLeastOneCloud || atLeastOneMesh);
	actionChangeColorLevels->setEnabled(atLeastOneCloud || atLeastOneMesh);
	actionCrop->setEnabled(atLeastOneCloud || atLeastOneMesh);
	actionSetUniqueColor->setEnabled(atLeastOneEntity/*atLeastOneCloud || atLeastOneMesh*/); //DGM: we can set color to a group now!
	actionColorize->setEnabled(atLeastOneEntity/*atLeastOneCloud || atLeastOneMesh*/); //DGM: we can set color to a group now!
	actionScalarFieldFromColor->setEnabled(atLeastOneEntity && atLeastOneColor);
	actionComputeMeshAA->setEnabled(atLeastOneCloud);
	actionComputeMeshLS->setEnabled(atLeastOneCloud);
	//actionComputeQuadric3D->setEnabled(atLeastOneCloud);
	actionComputeBestFitBB->setEnabled(atLeastOneEntity);
	actionApproximateDensity->setEnabled(atLeastOneCloud);
	actionAccurateDensity->setEnabled(atLeastOneCloud);
	actionCurvature->setEnabled(atLeastOneCloud);
	actionRoughness->setEnabled(atLeastOneCloud);
	actionRemoveDuplicatePoints->setEnabled(atLeastOneCloud);
	actionFitPlane->setEnabled(atLeastOneEntity);
	actionFitFacet->setEnabled(atLeastOneEntity);
	actionFitQuadric->setEnabled(atLeastOneCloud);
	actionSubsample->setEnabled(atLeastOneCloud);

	actionSNETest->setEnabled(atLeastOneCloud);
	actionExportCloudsInfo->setEnabled(atLeastOneCloud);

	actionFilterByValue->setEnabled(atLeastOneSF);
	actionConvertToRGB->setEnabled(atLeastOneSF);
	actionRenameSF->setEnabled(atLeastOneSF);
	actionAddIdField->setEnabled(atLeastOneCloud);
	actionComputeStatParams->setEnabled(atLeastOneSF);
	actionShowHistogram->setEnabled(atLeastOneSF);
	actionGaussianFilter->setEnabled(atLeastOneSF);
	actionBilateralFilter->setEnabled(atLeastOneSF);
	actionDeleteScalarField->setEnabled(atLeastOneSF);
	actionDeleteAllSF->setEnabled(atLeastOneSF);
	actionMultiplySF->setEnabled(/*TODO: atLeastOneSF*/false);
	actionSFGradient->setEnabled(atLeastOneSF);
	actionSetSFAsCoord->setEnabled(atLeastOneSF && atLeastOneCloud);

	actionSamplePoints->setEnabled(atLeastOneMesh);
	actionMeasureMeshSurface->setEnabled(atLeastOneMesh);
	actionSmoothMeshLaplacian->setEnabled(atLeastOneMesh);
	actionConvertTextureToColor->setEnabled(atLeastOneMesh);
	actionSubdivideMesh->setEnabled(atLeastOneMesh);
	actionDistanceToBestFitQuadric3D->setEnabled(atLeastOneCloud);

	menuMeshScalarField->setEnabled(atLeastOneSF && atLeastOneMesh);
	//actionSmoothMeshSF->setEnabled(atLeastOneSF && atLeastOneMesh);
	//actionEnhanceMeshSF->setEnabled(atLeastOneSF && atLeastOneMesh);

	actionOrientNormalsMST->setEnabled(atLeastOneCloud && atLeastOneNormal);
	actionOrientNormalsFM->setEnabled(atLeastOneCloud && atLeastOneNormal);
	actionClearNormals->setEnabled(atLeastOneNormal);
	actionInvertNormals->setEnabled(atLeastOneNormal);
	actionConvertNormalToHSV->setEnabled(atLeastOneNormal);
	actionConvertNormalToDipDir->setEnabled(atLeastOneNormal);
	actionClearColor->setEnabled(atLeastOneColor);

	// == 1
	bool exactlyOneEntity = (selInfo.selCount == 1);
	bool exactlyOneCloud = (selInfo.cloudCount == 1);
	bool exactlyOneMesh = (selInfo.meshCount == 1);
	bool exactlyOneSF = (selInfo.sfCount == 1);
	bool exactlyOneSensor = (selInfo.sensorCount == 1);
	bool exactlyOneCameraSensor = (selInfo.cameraSensorCount == 1);

	actionModifySensor->setEnabled(exactlyOneSensor);
	actionComputeDistancesFromSensor->setEnabled(atLeastOneCameraSensor || atLeastOneGBLSensor);
	actionComputeScatteringAngles->setEnabled(exactlyOneSensor);
	actionViewFromSensor->setEnabled(exactlyOneSensor);
	actionCreateGBLSensor->setEnabled(atLeastOneCloud);
	actionCreateCameraSensor->setEnabled(atLeastOneCloud);
	actionProjectUncertainty->setEnabled(exactlyOneCameraSensor);
	actionCheckPointsInsideFrustrum->setEnabled(exactlyOneCameraSensor);
	actionLabelConnectedComponents->setEnabled(atLeastOneCloud);
	actionUnroll->setEnabled(exactlyOneEntity);
	actionStatisticalTest->setEnabled(exactlyOneEntity && exactlyOneSF);
	actionAddConstantSF->setEnabled(exactlyOneCloud || exactlyOneMesh);
	actionEditGlobalShift->setEnabled(exactlyOneCloud || exactlyOneMesh);
	actionEditGlobalScale->setEnabled(exactlyOneCloud || exactlyOneMesh);
	actionComputeKdTree->setEnabled(exactlyOneCloud || exactlyOneMesh);

	actionKMeans->setEnabled(/*TODO: exactlyOneEntity && exactlyOneSF*/false);
	actionFrontPropagation->setEnabled(/*TODO: exactlyOneEntity && exactlyOneSF*/false);

	actionFindBiggestInnerRectangle->setEnabled(exactlyOneCloud);

	menuActiveScalarField->setEnabled((exactlyOneCloud || exactlyOneMesh) && selInfo.sfCount>0);
	actionCrossSection->setEnabled(exactlyOneCloud);
	actionHeightGridGeneration->setEnabled(exactlyOneCloud);

	actionPointListPicking->setEnabled(exactlyOneEntity);

	// == 2
	bool exactlyTwoEntities = (selInfo.selCount == 2);
	bool exactlyTwoClouds = (selInfo.cloudCount == 2);
	//bool exactlyTwoSF = (selInfo.sfCount == 2);

	actionRegister->setEnabled(exactlyTwoEntities);
	actionInterpolateColors->setEnabled(exactlyTwoEntities && atLeastOneColor);
	actionPointPairsAlign->setEnabled(exactlyOneEntity || exactlyTwoEntities);
	actionAlign->setEnabled(exactlyTwoEntities); //Aurelien BEY le 13/11/2008
	actionCloudCloudDist->setEnabled(exactlyTwoClouds);
	actionCloudMeshDist->setEnabled(exactlyTwoEntities && atLeastOneMesh);
	actionCPS->setEnabled(exactlyTwoClouds);
	actionScalarFieldArithmetic->setEnabled(exactlyOneEntity && atLeastOneSF);

	//>1
	bool atLeastTwoEntities = (selInfo.selCount>1);

	actionMerge->setEnabled(atLeastTwoEntities);
	actionMatchBBCenters->setEnabled(atLeastTwoEntities);

	//standard plugins
	foreach (ccStdPluginInterface* plugin, m_stdPlugins)
		plugin->onNewSelection(m_selectedEntities);
}

void MainWindow::echoMouseWheelRotate(float wheelDelta_deg)
{
	if (checkBoxCameraLink->checkState() != Qt::Checked)
		return;

	ccGLWindow* sendingWindow = dynamic_cast<ccGLWindow*>(sender());
	if (!sendingWindow)
		return;

	QList<QMdiSubWindow *> windows = m_mdiArea->subWindowList();
	for (int i=0; i<windows.size(); ++i)
	{
		ccGLWindow *child = static_cast<ccGLWindow*>(windows.at(i)->widget());
		if (child != sendingWindow)
		{
			child->blockSignals(true);
			child->onWheelEvent(wheelDelta_deg);
			child->blockSignals(false);
			child->redraw();
		}
	}
}

void MainWindow::echoCameraDisplaced(float ddx, float ddy)
{
	if (checkBoxCameraLink->checkState() != Qt::Checked)
		return;

	ccGLWindow* sendingWindow = dynamic_cast<ccGLWindow*>(sender());
	if (!sendingWindow)
		return;

	QList<QMdiSubWindow *> windows = m_mdiArea->subWindowList();
	for (int i=0; i<windows.size(); ++i)
	{
		ccGLWindow *child = static_cast<ccGLWindow*>(windows.at(i)->widget());
		if (child != sendingWindow)
		{
			child->blockSignals(true);
			child->moveCamera(ddx,ddy,0.0f);
			child->blockSignals(false);
			child->redraw();
		}
	}
}

void MainWindow::echoBaseViewMatRotation(const ccGLMatrixd& rotMat)
{
	if (checkBoxCameraLink->checkState() != Qt::Checked)
		return;

	ccGLWindow* sendingWindow = dynamic_cast<ccGLWindow*>(sender());
	if (!sendingWindow)
		return;

	QList<QMdiSubWindow *> windows = m_mdiArea->subWindowList();
	for (int i=0; i<windows.size(); ++i)
	{
		ccGLWindow *child = static_cast<ccGLWindow*>(windows.at(i)->widget());
		if (child != sendingWindow)
		{
			child->blockSignals(true);
			child->rotateBaseViewMat(rotMat);
			child->blockSignals(false);
			child->redraw();
		}
	}
}

 void MainWindow::echoCameraPosChanged(const CCVector3d& P)
 {
	 if (checkBoxCameraLink->checkState() != Qt::Checked)
		 return;

	 ccGLWindow* sendingWindow = dynamic_cast<ccGLWindow*>(sender());
	 if (!sendingWindow)
		 return;

	 QList<QMdiSubWindow *> windows = m_mdiArea->subWindowList();
	 for (int i=0; i<windows.size(); ++i)
	 {
		 ccGLWindow *child = static_cast<ccGLWindow*>(windows.at(i)->widget());
		 if (child != sendingWindow)
		 {
			 child->blockSignals(true);
			 child->setCameraPos(P);
			 child->blockSignals(false);
			 child->redraw();
		 }
	 }
 }

 void MainWindow::echoPivotPointChanged(const CCVector3d& P)
 {
	 if (checkBoxCameraLink->checkState() != Qt::Checked)
		 return;

	 ccGLWindow* sendingWindow = dynamic_cast<ccGLWindow*>(sender());
	 if (!sendingWindow)
		 return;

	 QList<QMdiSubWindow *> windows = m_mdiArea->subWindowList();
	 for (int i=0; i<windows.size(); ++i)
	 {
		 ccGLWindow *child = static_cast<ccGLWindow*>(windows.at(i)->widget());
		 if (child != sendingWindow)
		 {
			 child->blockSignals(true);
			 child->setPivotPoint(P);
			 child->blockSignals(false);
			 child->redraw();
		 }
	 }
 }

 void MainWindow::echoPixelSizeChanged(float pixelSize)
 {
	 if (checkBoxCameraLink->checkState() != Qt::Checked)
		 return;

	 ccGLWindow* sendingWindow = dynamic_cast<ccGLWindow*>(sender());
	 if (!sendingWindow)
		 return;

	 QList<QMdiSubWindow *> windows = m_mdiArea->subWindowList();
	 for (int i=0; i<windows.size(); ++i)
	 {
		 ccGLWindow *child = static_cast<ccGLWindow*>(windows.at(i)->widget());
		 if (child != sendingWindow)
		 {
			 child->blockSignals(true);
			 child->setPixelSize(pixelSize);
			 child->blockSignals(false);
			 child->redraw();
		 }
	 }
 }

void MainWindow::dispToConsole(QString message, ConsoleMessageLevel level/*=STD_CONSOLE_MESSAGE*/)
{
	switch(level)
	{
	case STD_CONSOLE_MESSAGE:
		ccConsole::Print(message);
		break;
	case WRN_CONSOLE_MESSAGE:
		ccConsole::Warning(message);
		break;
	case ERR_CONSOLE_MESSAGE:
		ccConsole::Error(message);
		break;
	}
}

void MainWindow::doActionLoadShader() //TODO
{
	ccConsole::Error("Not yet implemented! Sorry ...");
}

void MainWindow::doActionKMeans()//TODO
{
	ccConsole::Error("Not yet implemented! Sorry ...");
}

void MainWindow::doActionFrontPropagation() //TODO
{
	ccConsole::Error("Not yet implemented! Sorry ...");
}

/************** STATIC METHODS ******************/

MainWindow* MainWindow::TheInstance()
{
	if (!s_instance)
		s_instance = new MainWindow();
	return s_instance;
}

void MainWindow::DestroyInstance()
{
	if (s_instance)
		delete s_instance;
	s_instance=0;
}

void MainWindow::GetGLWindows(std::vector<ccGLWindow*>& glWindows)
{
	QList<QMdiSubWindow*> windows = TheInstance()->m_mdiArea->subWindowList();
	int winNum = windows.size();

	if (winNum == 0)
		return;

	glWindows.clear();
	glWindows.reserve(winNum);

	for (int i=0; i<winNum; ++i)
		glWindows.push_back(static_cast<ccGLWindow*>(windows.at(i)->widget()));
}

ccGLWindow* MainWindow::GetActiveGLWindow()
{
	return TheInstance()->getActiveGLWindow();
}

ccGLWindow* MainWindow::GetGLWindow(const QString& title)
{
	QList<QMdiSubWindow *> windows = TheInstance()->m_mdiArea->subWindowList();
	int winNum = windows.size();

	if (winNum == 0)
		return 0;

	for (int i=0; i<winNum; ++i)
	{
		ccGLWindow* win = static_cast<ccGLWindow*>(windows.at(i)->widget());
		if (win->windowTitle() == title)
			return win;
	}

	return 0;
}

void MainWindow::RefreshAllGLWindow()
{
	TheInstance()->refreshAll();
}

void MainWindow::UpdateUI()
{
	TheInstance()->updateUI();
}

ccDBRoot* MainWindow::db()
{
	return m_ccRoot;
}

ccHObject* MainWindow::dbRootObject()
{
	return (m_ccRoot ? m_ccRoot->getRootEntity() : 0);
}

MainWindow::ccHObjectContext MainWindow::removeObjectTemporarilyFromDBTree(ccHObject* obj)
{
	ccHObjectContext context;

	assert(obj);
	if (!m_ccRoot || !obj)
		return context;

	//mandatory (to call putObjectBackIntoDBTree)
	context.parent = obj->getParent();

	//remove the object's dependency to its father (in case it undergoes "severe" modifications)
	if (context.parent)
	{
		context.parentFlags = context.parent->getDependencyFlagsWith(obj);
		context.childFlags = obj->getDependencyFlagsWith(context.parent);

		context.parent->removeDependencyWith(obj);
		obj->removeDependencyWith(context.parent);
	}

	m_ccRoot->removeElement(obj);

	return context;
}

void MainWindow::putObjectBackIntoDBTree(ccHObject* obj, const ccHObjectContext& context)
{
	assert(obj);
	if (!obj || !m_ccRoot)
		return;

	if (context.parent)
	{
		context.parent->addChild(obj,context.parentFlags);
		obj->addDependency(context.parent,context.childFlags);
	}

	//DGM: we must call 'notifyGeometryUpdate' as any call to this method
	//while the object was temporarily 'cut' from the DB tree were
	//ineffective!
	obj->notifyGeometryUpdate();

	m_ccRoot->addElement(obj,false);
}

//For primitives test
/*#include <ccBox.h>
#include <ccCone.h>
#include <ccCylinder.h>
#include <ccTorus.h>
#include <ccSphere.h>
#include <ccDish.h>
#include <ccExtru.h>

void doTestPrimitives()
{
	//PRIMITIVES TEST
	addToDB(new ccBox(CCVector3(10,20,30)));
	addToDB(new ccCone(10,20,30));
	addToDB(new ccCylinder(20,30));
	addToDB(new ccCone(10,20,30,5,10));
	addToDB(new ccTorus(50,60,M_PI/3,false));
	addToDB(new ccTorus(50,60,M_PI/3,true,20));
	addToDB(new ccSphere(35));
	addToDB(new ccDish(35,15,0));
	addToDB(new ccDish(35,25,0));
	addToDB(new ccDish(35,35,0));
	addToDB(new ccDish(35,15,15));

	std::vector<CCVector2> contour;
	contour.push_back(CCVector2(10,00));
	contour.push_back(CCVector2(00,20));
	contour.push_back(CCVector2(15,25));
	contour.push_back(CCVector2(20,10));
	contour.push_back(CCVector2(25,27));
	contour.push_back(CCVector2(18,35));
	contour.push_back(CCVector2(22,40));
	contour.push_back(CCVector2(30,30));
	contour.push_back(CCVector2(27,05));
	addToDB(new ccExtru(contour,10));
}
//*/
