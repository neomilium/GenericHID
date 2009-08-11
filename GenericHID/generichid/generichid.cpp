#include "stdafx.h"
#include "generichid.h"
#include "programdlg.h"
#include "dragtoolbutton.h"

const char * const CONFIGDATA_FILE = "config.xml";
const int HELP_WINDOW_HEIGHT = 30;


GenericHID::GenericHID(QWidget *parent, Qt::WFlags flags)
: QMainWindow(parent, flags)
, m_pShapes( NULL )
, m_pScene( NULL )
, m_pLastSelectedShape( NULL )
{
    ui.setupUi(this);

    m_MRU.setMenu( ui.menuFile );
    connect( &m_MRU, SIGNAL(MRUSelected(const QString &)), this, SLOT(onMRUSelected(const QString &)) );

    m_pShapes = ShapeCollection::LoadShapeCollection( CONFIGDATA_FILE );

    // Load the shapes into the tool box
    foreach ( const Shape *pShape, m_pShapes->shapes() )
    {
	DragToolButton *pButton = new DragToolButton( this, pShape );
	pButton->setIcon( QIcon(pShape->iconFile()) );
	pButton->setToolTip( pShape->description() );
	ui.toolToolBar->addWidget(  pButton );
    }

    // Create graphics scene
    m_pScene = new ShapeScene( this, -1000, -1000, 2000, 2000, this );
    ui.graphicsView->setScene( m_pScene );
    ui.graphicsView->setCacheMode(QGraphicsView::CacheBackground);
    ui.graphicsView->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    ui.graphicsView->setRenderHint(QPainter::Antialiasing);
    ui.graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    ui.graphicsView->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    ui.graphicsView->setDragMode( QGraphicsView::RubberBandDrag );
    ui.graphicsView->setBackgroundBrush( QBrush(QColor(255,255,236)) );

    // The action group
    QActionGroup *pActionGroup = new QActionGroup(this);
    pActionGroup->addAction( ui.actionMirrorTool );
    pActionGroup->addAction( ui.actionPointerTool );
    pActionGroup->addAction( ui.actionRotateTool );
    pActionGroup->addAction( ui.actionWireLinkTool );
    ui.actionPointerTool->setChecked( true );

    // Cursors
    m_curMirror = new QCursor( QPixmap(":/GenericHID/MirrorCursor"),0,0);
    m_curMirrorOff = new QCursor( QPixmap(":/GenericHID/MirrorCursorOff"),0,0);
    m_curRotate = new QCursor( QPixmap(":/GenericHID/RotateCursor"),0,0);
    m_curRotateOff = new QCursor( QPixmap(":/GenericHID/RotateCursorOff"),0,0);
    m_curWire = new QCursor( QPixmap(":/GenericHID/WireCursor"),0,0);
    m_curWireOff = new QCursor( QPixmap(":/GenericHID/WireCursorOff"),0,0);
    m_curWireNot = new QCursor( QPixmap(":/GenericHID/WireCursorNot"),0,0);
    m_curPointer = new QCursor( QPixmap(":/GenericHID/Cursor"),0,0);
    onPointerTool();

    connect( ui.graphicsView, SIGNAL(dropShapeEvent( const ::Shape *, QPointF) ), this, SLOT(onDropShapeEvent( const ::Shape *, QPointF) ) );
    connect( m_pScene, SIGNAL(selectionChanged() ), this, SLOT(onSelectionChanged() ) );


    ui.listView->setPropertiesWithoutValueMarked(false);
    ui.listView->setResizeMode(QtTreePropertyBrowser::Interactive);
    ui.listView->setRootIsDecorated(false);
    ShapeProperty::SetBrowserFactory( ui.listView );
    connect( ui.listView, SIGNAL(currentItemChanged(QtBrowserItem *)), this, SLOT(onPropertiesCurrentItemChanged( QtBrowserItem *)) );
    ui.listView->setSplitterPosition( ui.listView->width()/2 );

    ui.txtPropertyHelp->setBackgroundRole( QPalette::Window );
    QPalette helpPalette( QApplication::palette() );
    helpPalette.setColor( QPalette::Base, QApplication::palette().color(QPalette::Window) );
    ui.txtPropertyHelp->setPalette( helpPalette );
    QList<int> sizes;
    sizes.push_back( ui.splitter->height() - ui.splitter->handleWidth() - HELP_WINDOW_HEIGHT );
    sizes.push_back( HELP_WINDOW_HEIGHT );
    ui.splitter->setSizes( sizes );

    readSettings();
}


void GenericHID::writeSettings()
{
    m_Settings.setValue("window/size", size());
    m_Settings.setValue("window/pos", pos());

    for ( int i = 0; i < MAX_MRU; i++ )
	if ( i < m_MRU.count() )
	    m_Settings.setValue( QString("application/mru%1").arg(i), m_MRU[i] );
	else
	    m_Settings.setValue( QString("application/mru%1").arg(i), "" );

    QList<int> sizes = ui.splitter->sizes();
    m_Settings.setValue( "window/property-size", sizes[0] );
    m_Settings.setValue( "window/help-size", sizes[1] );

    m_Settings.setValue( "window/property-column", ui.listView->splitterPosition() );

    m_Settings.setValue( "window/layout", this->saveState() );
}

void GenericHID::readSettings()
{
    resize(m_Settings.value("window/size", size()).toSize());
    move(m_Settings.value("window/pos", pos()).toPoint());

    for ( int i = MAX_MRU-1; i >= 0; i-- )
    {
	QString sMRU = m_Settings.value(QString("application/mru%1").arg(i), "" ).toString();
	if ( sMRU.length() > 0 )
	    m_MRU.append( sMRU );
    }

    if ( m_Settings.contains( "window/layout" ) )
	this->restoreState( m_Settings.value( "window/layout", QByteArray()).toByteArray() );

    QList<int> sizes = ui.splitter->sizes();
    sizes[0] = m_Settings.value( "window/property-size", sizes[0] ).toInt();
    sizes[1] = m_Settings.value( "window/help-size", sizes[1] ).toInt();
    ui.splitter->setSizes( sizes );

    ui.listView->setSplitterPosition( m_Settings.value("window/property-column", ui.listView->splitterPosition()).toInt() );
}

GenericHID::~GenericHID()
{

}

void GenericHID::Clear()
{
    m_pScene->clear();
}

bool GenericHID::CheckDataChanged()
{
    QString s = m_pScene->makeXML();
    if ( s != m_sLastFileContents )
    {
	QMessageBox::StandardButton nRet = QMessageBox::warning( this, "Save Changes?", "Do you wish to save changes first?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );
	if ( nRet == QMessageBox::No )
	    return true;
	else if ( nRet == QMessageBox::Cancel )
	    return false;
	else 
	{
	    if ( m_sLastFile.isEmpty() )
		return DoSaveAs();
	    else
		return DoSave();
	}
    }
    return true;
}

void GenericHID::onFileNew()
{
    // check for changes
    if ( !CheckDataChanged() )
	return;

    // Clear existing 
    Clear();

    m_sLastFile.clear();
    m_sLastFileContents = m_pScene->makeXML();
}

void GenericHID::onMRUSelected(const QString &sFile)
{
    // check for changes
    if ( !CheckDataChanged() )
	return;

    // Clear existing 
    Clear();

    DoOpen( sFile );
}

void GenericHID::onFileOpen()
{
    // check for changes
    if ( !CheckDataChanged() )
	return;

    // open file
    QString sFile = QFileDialog::getOpenFileName( this, "Open GenericHID device", m_sLastFile, QString("Generic HID device file (*.ghd);;All files (*)") );
    if ( sFile.isNull() || sFile.isEmpty() )
	return;

    // Clear existing 
    Clear();

    DoOpen( sFile );
}

bool GenericHID::DoOpen( const QString &sFile )
{
    m_sLastFile = sFile;

    QFile file( sFile );
    if ( !file.open(QIODevice::ReadOnly) )
	return false;
    QString sXML;
    {
	QTextStream in(&file);
	sXML = in.readAll();
    }
    file.close();

    m_MRU.append( m_sLastFile );
    QDomDocument doc("GenericHID");
    QString sError;
    int nLine, nCol;
    bool bOK = doc.setContent( sXML, false, &sError, &nLine, &nCol );
    if ( !bOK )
    {
	QMessageBox::critical( this, "Error reading file", QString("Error reading input file: '%1', line:%2, col:%3").arg(sError).arg(nLine).arg(nCol) );
	return false;
    }
    m_sLastFileContents = sXML;

    m_pScene->loadXML( doc, m_pShapes );

    updateWindowTitle();
    return true;
}


void GenericHID::onFileSave()
{
    if ( m_pLastSelectedShape != NULL )
    {
	// write back properties if this shape before trying to save
        const ShapeProperties &pProps = m_pLastSelectedShape->shapeData()->properties();
        pProps.retreive(m_pLastSelectedShape->values());
    }

    if ( m_sLastFile.isEmpty() )
	DoSaveAs();
    else
	DoSave();
}

bool GenericHID::DoSaveAs()
{
    // Validate everything
    // Make the xml file to save
    QString s = m_pScene->makeXML();
    if ( s.length() > 0 )
    {
	for (;;)
	{
	    QString sFilename = QFileDialog::getSaveFileName( this, "Save GenericHID device", m_sLastFile, QString("Generic HID device file (*.ghd);;All files (*)") );
	    if ( sFilename.isEmpty() || sFilename.isNull() )
		return false;

	    QFileInfo fi(sFilename);
	    if ( fi.suffix().isEmpty() )
		sFilename.append( ".ghd" );

	    QFile file(sFilename.toLatin1().constData());
	    if ( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	    {
		QMessageBox::critical( this, "Can't save", QString("Failed to save file '%1':%2").arg(sFilename).arg(file.errorString()) );
	    }
	    else
	    {
		m_sLastFile = sFilename;
		file.write( s.toLatin1() );
		file.close();
		break;
	    }
	}
	m_MRU.append( m_sLastFile );
	m_sLastFileContents = s;
	updateWindowTitle();
    }
    return true;
}

bool GenericHID::DoSave()
{
    // Make the xml file to save
    QString s = m_pScene->makeXML();
    if ( s.length() > 0 )
    {
	QFile file(m_sLastFile.toLatin1().constData());
	if ( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
	    QMessageBox::critical( this, "Can't save", QString("Failed to save file '%1':%2").arg(m_sLastFile).arg(file.errorString()) );
	}
	else
	{
	    file.write( s.toLatin1() );
	    file.close();
	    m_sLastFileContents = s;
	}
    }
    return true;
}

void GenericHID::updateWindowTitle()
{
    QString s = QCoreApplication::applicationName();
    if ( !m_sLastFile.isEmpty() )
    {
	s += " - ";
	s += m_sLastFile;
    }
    setWindowTitle( s );
}

void GenericHID::onFileSaveAs()
{
    if ( m_pLastSelectedShape != NULL )
    {
	// write back properties if this shape before trying to save
        const ShapeProperties &pProps = m_pLastSelectedShape->shapeData()->properties();
        pProps.retreive(m_pLastSelectedShape->values());
    }

    DoSaveAs();
}

void GenericHID::onFileExit()
{
    close();
}

void GenericHID::closeEvent( QCloseEvent * event )
{
    // check for changes
    if ( !CheckDataChanged() )
    {
	event->ignore();
	return;
    }

    writeSettings();
}

void GenericHID::onPropertiesCurrentItemChanged( QtBrowserItem * current )
{
    if ( current == NULL || current->property() == NULL )
	ui.txtPropertyHelp->setText( QString() );
    else
    {
	ui.txtPropertyHelp->setText( QString("<b>%1</b><br>\n%2").arg(current->property()->propertyName()).arg(current->property()->toolTip()) );
    }
}

void GenericHID::onMicrocontrollerProgram()
{
}

void GenericHID::onMicrocontrollerExport()
{
}

void GenericHID::onMicrocontrollerImportAndProgram()
{
    ProgramDlg dlg(this);
    dlg.exec();
}

void GenericHID::onDropShapeEvent( const ::Shape *pShape, QPointF pos )
{
    // Create a new shape
    QString sError;
    if ( !m_pScene->CanAdd(pShape,sError) )
    {
	QMessageBox::critical( this, "Can't add", sError );
	return;
    }

    ShapeItem *pItem = m_pScene->CreateNewShape( pShape, this, pos );

    ui.listView->clear();
    if ( pItem != NULL )
    {
	// unselect existing items, and select ours
	foreach ( QGraphicsItem *item, m_pScene->selectedItems() )
	    item->setSelected( false );
	pItem->setSelected( true );
    }
}

void GenericHID::onSelectionChanged()
{
    if ( m_pLastSelectedShape != NULL && !m_pLastSelectedShape->isSelected() )
    {
	// write back properties if this shape is no longer selected
        const ShapeProperties &pProps = m_pLastSelectedShape->shapeData()->properties();
        pProps.retreive(m_pLastSelectedShape->values());
    }

    QList<ShapeItem *> selectedShapes;
    foreach ( QGraphicsItem *pItem, m_pScene->selectedItems() )
	if ( pItem->type() == ShapeItem::Type )
	    selectedShapes.append( qgraphicsitem_cast<ShapeItem *>(pItem) );

    if ( selectedShapes.count() == 1 )
    {
        ui.listView->clear();
	m_pLastSelectedShape = selectedShapes[0];

        const ShapeProperties &pProps = m_pLastSelectedShape->shapeData()->properties();
        ShapeProperty::SetBrowserFactory( ui.listView );
        ui.listView->addProperty(pProps.topItem());
        pProps.populate(m_pLastSelectedShape->values());
    }
    else
    {
	m_pLastSelectedShape = NULL;
        ui.listView->clear();
    }
}


void GenericHID::onRotateTool()
{
    //ui.graphicsView->unsetCursor();
    SetCursor( *m_curRotateOff, *m_curRotate );
    m_eEditMode = EditMode::Rotate;
}

void GenericHID::onMirrorTool()
{
    //ui.graphicsView->unsetCursor();
    SetCursor( *m_curMirrorOff, *m_curMirror );
    m_eEditMode = EditMode::Mirror;
}

void GenericHID::onPointerTool()
{
    //ui.graphicsView->unsetCursor();
    SetCursor( *m_curPointer, *m_curPointer );
    m_eEditMode = EditMode::Pointer;
}

void GenericHID::onWireLinkTool()
{
    //ui.graphicsView->unsetCursor();
    SetCursor( *m_curWireOff, *m_curWire );
    m_eEditMode = EditMode::Wiring;
}

void GenericHID::SetCursor( QCursor &cur1, QCursor &cur2 )
{
//    ui.graphicsView->setCursor( cur1 );
//    ui.graphicsView->viewport()->setCursor( cur1 );
//    foreach( ShapeInstance *pShape, m_pShapeInstances.instances() )
//	pShape->item()->setCursor( cur2 );
}

void GenericHID::ProcessCommandline()
{
    QStringList args = QCoreApplication::instance()->arguments();

    if ( args.count() > 1 )
    {
        QString sFile = args[1];
        QFileInfo info( sFile );
        if ( !info.exists() )
            QMessageBox::critical( this, "File not found", QString("File '%1' does not exist").arg(sFile) );
        else
            DoOpen( sFile );
    }
}


/*
 UI => XML => EEPROM Binary => Program
 Independant Test panel

 new
  - add remove pins as we need them 
    - binary coded switch
    - LCD 4/8 bit
    - Key matrix  rows x cols
  - PWM
    - mcu attribute
	- timer 1,2,3
	- frequency shared between outputs a/b/c on a single timer
  - counter
  - watch dog

todo
    - check changes
    - check validity - pre generate eeprom
 */