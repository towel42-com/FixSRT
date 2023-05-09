
#include "MainWindow.h"
#include "AdjustTime.h"

#include "ui_MainWindow.h"
#include "SABUtils/DelayLineEdit.h"
#include "SABUtils/BackupFile.h"

#include <QStandardItemModel>
#include <QSettings>
#include <QFileDialog>
#include <QSortFilterProxyModel>
#include <QMessageBox>
#include <QCompleter>
#include <QFileSystemModel>
#include <QTextStream>
#include <QDebug>
#include <optional>

QString stringFromTime( const QTime &time )
{
    auto retVal = QString( "%1:%2:%3,%4" )   //
                      .arg( time.hour(), 2, 10, QChar( '0' ) )   //
                      .arg( time.minute(), 2, 10, QChar( '0' ) )   //
                      .arg( time.second(), 2, 10, QChar( '0' ) )   //
                      .arg( time.msec(), 3, 10, QChar( '0' ) )   //
        ;
    return retVal;
}

QTime timeFromString( const QString &string )
{
    auto pos1 = string.indexOf( ':' );
    if ( pos1 == -1 )
        return {};
    auto tmp = string.left( pos1 );
    bool aOK = false;
    auto hours = tmp.toInt( &aOK );
    if ( !aOK )
        return {};

    auto pos2 = string.indexOf( ':', pos1 + 1 );
    if ( pos2 == -1 )
        return {};
    tmp = string.mid( pos1 + 1, pos2 - pos1 - 1 );
    auto mins = tmp.toInt( &aOK );
    if ( !aOK )
        return {};

    auto pos3 = string.indexOf( ',', pos2 + 1 );
    if ( pos3 == -1 )
        return {};
    tmp = string.mid( pos2 + 1, pos3 - pos2 - 1 );
    auto secs = tmp.toInt( &aOK );
    if ( !aOK )
        return {};

    tmp = string.mid( pos3 + 1 ).trimmed();
    auto ms = tmp.toInt( &aOK );
    if ( !aOK )
        return {};

    auto retVal = QTime( hours, mins, secs, ms );
    return retVal;
}

QTime getOffsetTime( const QTime &currValue, const std::pair< uint64_t, bool > &offset )
{
    auto newValue = currValue.addMSecs( offset.first * ( offset.second ? 1 : -1 ) );
    return newValue;
}

class CProxyModel : public QSortFilterProxyModel
{
public:
    CProxyModel( QObject *owner ) :
        QSortFilterProxyModel( owner )
    {
    }

    QTime getOffsetTime( const QModelIndex &idx ) const
    {
        auto currValue = idx.data( Qt::UserRole + 1 ).toTime();
        return ::getOffsetTime( currValue, fOffset );
    }

    QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override
    {
        if ( role == Qt::DisplayRole && ( ( index.column() == 1 ) || ( index.column() == 2 ) ) )
        {
            return stringFromTime( getOffsetTime( index ) );
        }
        return QSortFilterProxyModel::data( index, role );
    }

    void setOffset( uint64_t offset, bool add )
    {
        fOffset = { offset, add };
        invalidate();
    }

    std::pair< uint64_t, bool > fOffset;
};

CMainWindow::CMainWindow( QWidget *parent ) :
    QMainWindow( parent ),
    fImpl( new Ui::CMainWindow )
{
    fImpl->setupUi( this );

    fImpl->srtFile->setDelay( 1000 );

    auto delayLE = new NSABUtils::CPathBasedDelayLineEdit;
    delayLE->setCheckExists( true );
    delayLE->setCheckIsFile( true );
    fImpl->srtFile->setDelayLineEdit( delayLE );

    auto completer = new QCompleter( this );
    auto dirModel = new QFileSystemModel( completer );
    dirModel->setRootPath( "/" );
    completer->setModel( dirModel );
    completer->setCompletionMode( QCompleter::PopupCompletion );
    completer->setCaseSensitivity( Qt::CaseInsensitive );

    setWindowIcon( QIcon( ":/resources/fixsrt.png" ) );
    setAttribute( Qt::WA_DeleteOnClose );

    fModel = new QStandardItemModel( this );
    initModel();
    fProxyModel = new CProxyModel( this );
    fProxyModel->setSourceModel( fModel );
    fImpl->srtData->setModel( fProxyModel );
    fImpl->srtData->setContextMenuPolicy( Qt::CustomContextMenu );

    connect( fImpl->srtData, &QTreeView::customContextMenuRequested, this, &CMainWindow::slotSRTContextMenu );
    connect( fImpl->openSrtBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectSrtFile );
    connect( fImpl->saveSrtBtn, &QPushButton::clicked, this, &CMainWindow::slotSave );
    connect( fImpl->renumberBtn, &QToolButton::clicked, this, &CMainWindow::slotRenumber );

    connect( fImpl->srtFile, &NSABUtils::CDelayComboBox::sigEditTextChangedAfterDelay, this, &CMainWindow::slotSrtFileChanged );
    connect( fImpl->srtFile, &NSABUtils::CDelayComboBox::editTextChanged, this, &CMainWindow::slotSrtFileChanged );

    connect( fImpl->addBtn, &QRadioButton::clicked, this, &CMainWindow::slotTimeAdjustChanged );
    connect( fImpl->subBtn, &QRadioButton::clicked, this, &CMainWindow::slotTimeAdjustChanged );
    connect( fImpl->timeOffset, &QTimeEdit::timeChanged, this, &CMainWindow::slotTimeAdjustChanged );

    QSettings settings;
    fImpl->srtFile->clear();

    QStringList dirs;
    if ( settings.contains( "SrtFiles" ) )
        dirs << settings.value( "SrtFiles", QString() ).toString();
    dirs << settings.value( "SrtFiles" ).toStringList();
    dirs.removeDuplicates();
    dirs.removeAll( QString() );

    fImpl->srtFile->addItems( dirs );
    fImpl->addBtn->setChecked( true );
    fImpl->subBtn->setChecked( false );
    fImpl->timeOffset->setTime( QTime( 0, 0, 0, 0 ) );
}

void CMainWindow::initModel()
{
    fModel->clear();
    fModel->setHorizontalHeaderLabels( QStringList() << tr( "Number" ) << tr( "Start Time" ) << tr( "End Time" ) << tr( "Text" ) );
}

CMainWindow::~CMainWindow()
{
    QSettings settings;
    settings.setValue( "SrtFiles", fImpl->srtFile->getAllText() );
}

void CMainWindow::slotSelectSrtFile()
{
    auto file = QFileDialog::getOpenFileName( this, "Select SRT File", fImpl->srtFile->currentText(), "SRT Files (*.srt);;All Files (*.*)" );
    if ( file.isEmpty() )
        return;

    if ( fImpl->srtFile->currentText() == file )
        loadSRT();
    else
        fImpl->srtFile->setCurrentText( file );
}

void CMainWindow::slotSave()
{
    auto file = QFileDialog::getSaveFileName( this, "Select New SRT File", fImpl->srtFile->currentText(), "SRT Files (*.srt);;All Files (*.*)" );
    if ( file.isEmpty() )
        return;

    QString msg;
    if ( !NSABUtils::NFileUtils::backup( file, &msg ) )
    {
        QMessageBox::critical( this, QString( "Error backing up '%1'" ).arg( file ), msg );
        return;
    }

    QFile fi( file );
    fi.open( QFile::WriteOnly | QFile::Text );
    if ( !fi.isOpen() )
    {
        QMessageBox::critical( this, QString( "Error Saving '%1'" ).arg( file ), fi.errorString() );
        return;
    }

    QTextStream ts( &fi );
    for ( int ii = 0; ii < fModel->rowCount(); ++ii )
    {
        saveSRTRow( ts, ii );
    }
}

void CMainWindow::saveSRTRow( QTextStream &ts, int rowNum ) const
{
    auto rowNumValue = fModel->item( rowNum, 0 )->data().toInt();
    auto startTime = fModel->item( rowNum, 1 )->data().toTime();
    auto endTime = fModel->item( rowNum, 2 )->data().toTime();
    auto subTitle = fModel->item( rowNum, 3 )->data().toStringList();

    auto offset = std::make_pair( fImpl->timeOffset->time().msecsSinceStartOfDay(), fImpl->addBtn->isChecked() );
    ts << rowNumValue << Qt::endl   //
       << stringFromTime( getOffsetTime( startTime, offset ) ) << " --> " << stringFromTime( getOffsetTime( endTime, offset ) ) << Qt::endl   //
        ;
    for ( auto &&ii : subTitle )
    {
        ts << ii << Qt::endl;
    }
    ts << Qt::endl;
}

void CMainWindow::slotSrtFileChanged()
{
    initModel();
    QFileInfo fi( fImpl->srtFile->currentText() );
    QString msg;
    if ( !fi.exists() )
        msg = QString( "'%1' does not exist" ).arg( fImpl->srtFile->currentText() );
    else if ( !fi.isFile() )
        msg = QString( "'%1' is not a file" ).arg( fImpl->srtFile->currentText() );
    else if ( !fi.isReadable() )
        msg = QString( "'%1' is not a readable file (please check permissions)" ).arg( fImpl->srtFile->currentText() );
    if ( !msg.isEmpty() )
    {
        QMessageBox::critical( this, tr( "Error Loading SRT File" ), msg );
        return;
    }
    loadSRT();
}

void CMainWindow::loadSRT()
{
    QFileInfo fileInfo( fImpl->srtFile->currentText() );
    if ( !fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable() )
        return;

    QFile fi( fImpl->srtFile->currentText() );
    fi.open( QFile::ReadOnly | QFile::Text );
    if ( !fi.isOpen() )
        return;
    QTextStream ts( &fi );
    QString line;
    bool inSubTitle = false;

    std::optional< uint64_t > currNum;
    QStringList currSubTitle;
    QTime startTime;
    QTime endTime;

    while ( ts.readLineInto( &line ) )
    {
        if ( line.isEmpty() )
        {
            if ( inSubTitle && currNum.has_value() && startTime.isValid() && endTime.isValid() )
            {
                createSubtitle( currNum.value(), startTime, endTime, currSubTitle );
                currNum.reset();
                inSubTitle = false;
                currSubTitle.clear();
                startTime = QTime();
                endTime = QTime();
            }
            continue;
        }

        if ( inSubTitle )
        {
            auto pos = line.indexOf( "-->" );
            if ( pos == -1 )
                currSubTitle << line;
            else
            {
                startTime = timeFromString( line.left( pos ) );
                endTime = timeFromString( line.mid( pos + 3 ) );

                qDebug() << startTime << endTime;
            }
        }
        else
        {
            bool aOK = false;
            auto tmp = line.toULongLong( &aOK );
            if ( aOK )
            {
                currNum = tmp;
                currSubTitle.clear();
                startTime = QTime();
                endTime = QTime();
                inSubTitle = true;
            }
        }
    }
    if ( currNum.has_value() && startTime.isValid() && endTime.isValid() && !currSubTitle.isEmpty() )
        createSubtitle( currNum.value(), startTime, endTime, currSubTitle );
    slotTimeAdjustChanged();
}

void CMainWindow::slotTimeAdjustChanged()
{
    fProxyModel->setOffset( fImpl->timeOffset->time().msecsSinceStartOfDay(), fImpl->addBtn->isChecked() );
}

void CMainWindow::slotRenumber()
{
    for ( int ii = 0; ii < fModel->rowCount(); ++ii )
    {
        auto item = fModel->item( ii, 0 );
        item->setData( ii + 1 );
        item->setText( QString::number( ii + 1 ) );
    }
}

void CMainWindow::createSubtitle( int num, const QTime &start, const QTime &end, const QStringList &text )
{
    auto numItem = new QStandardItem( QString::number( num ) );
    numItem->setData( num );
    auto startTimeItem = new QStandardItem( stringFromTime( start ) );
    startTimeItem->setData( start );
    auto endTimeItem = new QStandardItem( stringFromTime( end ) );
    endTimeItem->setData( end );
    auto textItem = new QStandardItem( text.join( "\n" ) );
    textItem->setData( text );

    fModel->appendRow( QList< QStandardItem * >() << numItem << startTimeItem << endTimeItem << textItem );
}

void CMainWindow::slotSRTContextMenu( const QPoint &pos )
{
    auto idx = fImpl->srtData->indexAt( pos );
    if ( !idx.isValid() )
        return;

    auto sourceIdx = fProxyModel->mapToSource( idx );
    if ( !sourceIdx.isValid() )
        return;

    auto item = fModel->itemFromIndex( sourceIdx );
    if ( !item )
        return;

    if ( item->parent() )
        item = item->parent();

    QMenu menu;
    menu.addAction(
        "Set Offset Time Based on this Subtitle",
        [ this, sourceIdx ]()
        {
            auto startTime = fModel->item( sourceIdx.row(), 1 )->data().toTime();

            CAdjustTime dlg( startTime, { fImpl->timeOffset->time().msecsSinceStartOfDay(), fImpl->addBtn->isChecked() }, this );
            if ( dlg.exec() == QDialog::Accepted )
            {
                auto newOffset = dlg.newOffset();
                fImpl->addBtn->setChecked( newOffset.second );
                fImpl->subBtn->setChecked( !newOffset.second );
                fImpl->timeOffset->setTime( QTime::fromMSecsSinceStartOfDay( newOffset.first ) );
            }
        } );
    menu.exec( fImpl->srtData->viewport()->mapToGlobal( pos ) );
}