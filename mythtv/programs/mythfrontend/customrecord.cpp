#include <qlayout.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qcursor.h>
#include <qlistview.h>
#include <qdatetime.h>
#include <qapplication.h>
#include <qimage.h>
#include <qpainter.h>
#include <qheader.h>
#include <qsqldatabase.h>
#include <qhbox.h>

#include <unistd.h>

#include <iostream>
using namespace std;

#include "customrecord.h"

#include "mythcontext.h"
#include "dialogbox.h"
#include "programinfo.h"
#include "proglist.h"
#include "scheduledrecording.h"
#include "recordingtypes.h"
#include "mythdbcon.h"

CustomRecord::CustomRecord(MythMainWindow *parent, const char *name)
              : MythDialog(parent, name)
{
    QVBoxLayout *vbox = new QVBoxLayout(this, (int)(20 * wmult));

    // Window title
    QString message = tr("Power Search Recording Rule Editor");
    QLabel *label = new QLabel(message, this);
    label->setBackgroundOrigin(WindowOrigin);
    label->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    vbox->addWidget(label);

    QVBoxLayout *vkbox = new QVBoxLayout(vbox, (int)(1 * wmult));
    QHBoxLayout *hbox = new QHBoxLayout(vkbox, (int)(1 * wmult));

    // Title edit box
    hbox = new QHBoxLayout(vbox, (int)(10 * wmult));

    message = tr("Rule Name") + ": ";
    label = new QLabel(message, this);
    label->setBackgroundOrigin(WindowOrigin);
    label->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    hbox->addWidget(label);

    m_title = new MythRemoteLineEdit( this, "title" );
    m_title->setBackgroundOrigin(WindowOrigin);
    hbox->addWidget(m_title);

    m_clause = new MythComboBox( false, this, "clause");
    m_clause->setBackgroundOrigin(WindowOrigin);

    m_clause->insertItem(tr("Match an exact title"));
    m_csql << "AND program.title = \"Nova\" ";

    m_clause->insertItem(tr("Match an exact episode"));
    m_csql << QString("AND program.title = \"Seinfeld\" \n"
                      "AND program.subtitle = \"The Soup\" ");

    m_clause->insertItem(tr("Match words in the title"));
    m_csql << "AND program.title LIKE \"%Junkyard%\" ";

    m_clause->insertItem(tr("Match in any descriptive field"));
    m_csql << QString("AND (program.title LIKE \"%Japan%\" \n"
                      "     OR program.subtitle LIKE \"%Japan%\" \n"
                      "     OR program.description LIKE \"%Japan%\") ");

    m_clause->insertItem(tr("Limit by category"));
    m_csql << "AND program.category = \"Reality\" ";

    m_clause->insertItem(tr("New episodes only"));
    m_csql << "AND program.previouslyshown = 0 ";

    m_clause->insertItem(tr("Exclude unidentified episodes (Data Direct)"));
    m_csql << QString("AND NOT (program.category_type = \"series\" \n"
                      "AND program.programid LIKE \"%0000\") ");

    m_clause->insertItem(tr("Exclude unidentified episodes (XMLTV)"));
    m_csql << "AND NOT (program.subtitle = \"\" AND program.description = \"\") ";

    m_clause->insertItem(QString(tr("Category type") +
            " (\"movie\", \"series\", \"sports\" " + tr("or") + " \"tvshow\")"));
    m_csql << "AND program.category_type = \"sports\" ";

    m_clause->insertItem(tr("Limit movies by the year of release"));
    m_csql << "AND program.category_type = \"movie\" AND airdate >= 2000 ";

    m_clause->insertItem(tr("Minimum star rating (0.0 to 1.0 for movies only)"));
    m_csql << "AND program.stars >= 0.75 ";

    m_clause->insertItem(tr("Exclude one station"));
    m_csql << "AND channel.callsign != \"GOLF\" ";

    m_clause->insertItem(tr("Match related callsigns"));
    m_csql << "AND channel.callsign LIKE \"HBO%\" ";

    m_clause->insertItem(tr("Only channels from a specific video source"));
    m_csql << "AND channel.sourceid = 2 ";

    m_clause->insertItem(tr("Only channels marked as commercial free"));
    m_csql << "AND channel.commfree > 0 ";

    m_clause->insertItem(tr("Anytime on a specific day of the week"));
    m_csql << "AND DAYNAME(program.starttime) = \"Tuesday\" ";

    m_clause->insertItem(tr("Only on weekdays (Monday through Friday)"));
    m_csql << "AND WEEKDAY(program.starttime) < 5 ";

    m_clause->insertItem(tr("Only on weekends"));
    m_csql << "AND WEEKDAY(program.starttime) >= 5 ";

    m_clause->insertItem(tr("Only in primetime"));
    m_csql << QString("AND HOUR(program.starttime) >= 19 \n"
                      "AND HOUR(program.starttime) < 23 ");

    m_clause->insertItem(tr("Not in primetime"));
    m_csql << QString("AND (HOUR(program.starttime) < 19 \n"
                      "     OR HOUR(program.starttime) >= 23) ");

    m_clause->insertItem(tr("Multiple sports teams (complete example)"));
    m_csql << QString("AND program.title LIKE \"NBA B%\" \n"
              "AND program.subtitle REGEXP \"(Rockets|Cavaliers|Lakers)\" ");

    m_clause->insertItem(tr("Sci-fi B-movies (complete example)"));
    m_csql << QString("AND program.category_type=\"movie\" \n"
              "AND program.category=\"Science fiction\" \n"
              "AND program.stars <= 0.5 AND airdate < 1970 ");

    vbox->addWidget(m_clause);

    // Preview box
    m_preview = new MythRemoteLineEdit( this, "preview" );
    m_preview->setBackgroundOrigin(WindowOrigin);
    m_preview->setEnabled(false);
    vbox->addWidget(m_preview);

    //  Add Button
    m_addButton = new MythPushButton( this, "add" );
    m_addButton->setBackgroundOrigin(WindowOrigin);
    m_addButton->setText( tr( "Add this example clause" ) );
    m_addButton->setEnabled(true);

    vbox->addWidget(m_addButton);

    // Description edit box
    m_description = new MythRemoteLineEdit(5, this, "description" );
    m_description->setBackgroundOrigin(WindowOrigin);
    vbox->addWidget(m_description);

    //  Test Button
    hbox = new QHBoxLayout(vbox, (int)(10 * wmult));

    m_testButton = new MythPushButton( this, "test" );
    m_testButton->setBackgroundOrigin(WindowOrigin);
    m_testButton->setText( tr( "Test" ) );
    m_testButton->setEnabled(false);

    hbox->addWidget(m_testButton);

    //  Record Button
    m_recordButton = new MythPushButton( this, "record" );
    m_recordButton->setBackgroundOrigin(WindowOrigin);
    m_recordButton->setText( tr( "Record" ) );
    m_recordButton->setEnabled(false);

    hbox->addWidget(m_recordButton);

    //  Cancel Button
    m_cancelButton = new MythPushButton( this, "cancel" );
    m_cancelButton->setBackgroundOrigin(WindowOrigin);
    m_cancelButton->setText( tr( "Cancel" ) );
    m_cancelButton->setEnabled(true);

    hbox->addWidget(m_cancelButton);

    connect(this, SIGNAL(dismissWindow()), this, SLOT(accept()));
     
    connect(m_title, SIGNAL(textChanged(void)), this, SLOT(textChanged(void)));
    connect(m_addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(m_clause, SIGNAL(activated(int)), this, SLOT(clauseChanged(void)));
    connect(m_clause, SIGNAL(highlighted(int)), this, SLOT(clauseChanged(void)));
    connect(m_description, SIGNAL(textChanged(void)), this,
            SLOT(textChanged(void)));
    connect(m_testButton, SIGNAL(clicked()), this, SLOT(testClicked()));
    connect(m_recordButton, SIGNAL(clicked()), this, SLOT(recordClicked()));
    connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));

    gContext->addListener(this);

    if (m_title->text().isEmpty())
        m_title->setFocus();
    else 
        m_clause->setFocus();

    clauseChanged();
}

CustomRecord::~CustomRecord(void)
{
    gContext->removeListener(this);
}

void CustomRecord::textChanged(void)
{
    bool hastitle = !m_title->text().isEmpty();
    bool hasdesc = !m_description->text().isEmpty();

    m_testButton->setEnabled(hasdesc);
    m_recordButton->setEnabled(hastitle && hasdesc);
}

void CustomRecord::clauseChanged(void)
{
    QString msg = m_csql[m_clause->currentItem()];
    msg.replace("\n", " ");
    msg.replace(QRegExp(" [ ]*"), " ");
    m_preview->setText(msg);
}

void CustomRecord::addClicked(void)
{
    m_description->append(m_csql[m_clause->currentItem()]);
}

void CustomRecord::testClicked(void)
{
    if (!checkSyntax())
    {
        m_testButton->setFocus();
        return;
    }

    ProgLister *pl = new ProgLister(plSQLSearch, m_description->text(),
                                    gContext->GetMainWindow(), "proglist");
    pl->exec();
    delete pl;

    m_testButton->setFocus();
}

void CustomRecord::recordClicked(void)
{
    if (!checkSyntax())
    {
        m_recordButton->setFocus();
        return;
    }

    ScheduledRecording record;
    record.loadBySearch(kPowerSearch, m_title->text(), m_description->text());
    record.exec();

    if (record.getRecordID())
        accept();
    else
        m_recordButton->setFocus();
}

void CustomRecord::cancelClicked(void)
{
    accept();
}

bool CustomRecord::checkSyntax(void)
{
    bool ret = false;
    QString msg = "";

    QString desc = m_description->text();
    desc.replace("\'", "\\\'");

    MSqlQuery query(MSqlQuery::InitCon());

    query.prepare(QString("SELECT NULL FROM program, channel "
                          "WHERE NULL \n%1").arg(desc));

    if (query.exec() && query.isActive())
    {
        ret = true;
    }
    else
    {
        msg = tr("An error was found when checking") + ":\n\n";
        msg += query.executedQuery();
        msg += "\n\n" + tr("The database error was") + ":\n";
        msg += query.lastError().databaseText();

        DialogBox *errdiag = new DialogBox(gContext->GetMainWindow(), msg);
        errdiag->AddButton(QObject::tr("OK"));
        errdiag->exec();

        delete errdiag;

        ret = false;
    }
    return ret;
}
