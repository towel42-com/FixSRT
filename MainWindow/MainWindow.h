#ifndef CMAINWINDOW_H
#define CMAINWINDOW_H

#include <QMainWindow>
#include "SABUtils/HashUtils.h"

class CProgressDlg;
class QStandardItem;
class CProxyModel;
class QStandardItemModel;
class QTime;
class QTextStream;

namespace Ui
{
    class CMainWindow;
}

class CMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    CMainWindow( QWidget *parent = 0 );
    ~CMainWindow();

Q_SIGNALS:

public Q_SLOTS:
    void slotSelectSrtFile();
    void slotSrtFileChanged();
    void slotRenumber();
    void slotTimeAdjustChanged();
    void slotSave();
    void slotSRTContextMenu( const QPoint &pos );

private:
    void initModel();
    void loadSRT();
    void createSubtitle( int num, const QTime &start, const QTime &end, const QStringList &text );
    void saveSRTRow( QTextStream &fi, int rowNum ) const;

    QStandardItemModel *fModel{ nullptr };
    CProxyModel *fProxyModel{ nullptr };
    std::unique_ptr< Ui::CMainWindow > fImpl;
};

#endif
