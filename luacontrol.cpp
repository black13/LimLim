#include <QtGui>

#include "console.h"
#include "luacontrol.h"
#include "interpreter.h"
#include "editor.h"
#include "variablewatcher.h"
#include "debugger.h"

LuaControl::LuaControl()
{
    luaConsole = new Console(this);
    luaEditor = new Editor(this);
    luaInterpret = new Interpreter(luaEditor, luaConsole, this);
    luaDebugger = new Debugger(this);
    watcheslist = new VariableWatcher(luaDebugger);
    setCentralWidget(luaEditor);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createDockWindows();

    readSettings();

    setWindowIcon(QIcon(":/images/lua.png"));

    setAttribute(Qt::WA_DeleteOnClose);
}

// TODO add disabling of buttons when launching program and
//      enabling buttons when program has finished

void LuaControl::run()
{
    luaInterpret->run();
}

void LuaControl::debug()
{
    luaDebugger->start();
    luaInterpret->debug();
}

void LuaControl::stop()
{
    luaDebugger->stop();
    luaInterpret->kill();
}


void LuaControl::createActions()
{
    // NEW FILE ACTIION
	newAction = new QAction(tr("&New"), this);
	newAction->setIcon(QIcon(":/images/edit/new.png"));
	newAction->setShortcut(QKeySequence::New);
	newAction->setStatusTip(tr("Create a new file"));
        connect(newAction, SIGNAL(triggered()), luaEditor, SLOT(newSource()));

    // OPEN RECENT FILE ACTIONS
	for (int i = 0; i < MaxRecentFiles; i++) {
		recentFileActions[i] = new QAction(this);
		recentFileActions[i]->setVisible(false);;
		connect(recentFileActions[i], SIGNAL(triggered()),
				this, SLOT(openRecentFile()));
	}

    // OPEN FILE ACTION
	openAction = new QAction(tr("&Open"), this);
	openAction->setIcon(QIcon(":/images/edit/open.png"));
	openAction->setShortcut(QKeySequence::Open);
	openAction->setStatusTip(tr("Open new file"));
        connect(openAction, SIGNAL(triggered()), luaEditor, SLOT(openSource()));

    // SAVE FILE ACTION
	saveAction = new QAction(tr("&Save"), this);
	saveAction->setIcon(QIcon(":/images/edit/save.png"));
	saveAction->setShortcut(QKeySequence::Save);
	saveAction->setStatusTip(tr("Save the Lua file to disk"));
        connect(saveAction, SIGNAL(triggered()), luaEditor, SLOT(saveCurrentSource()));

    // SAVE AS FILE ACTION
	saveAsAction = new QAction(tr("Save &As..."), this);
	saveAsAction->setStatusTip(tr("Save the Lua file under a new name"));
        connect(saveAsAction, SIGNAL(triggered()), luaEditor, SLOT(saveCurrentSourceAs()));

    // EXIT ACTION
	exitAction = new QAction(tr("E&xit"), this);
	exitAction->setShortcut(tr("Ctrl+Q"));
	exitAction->setStatusTip(tr("Exit the application"));
	connect(exitAction, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

    // CUT ACTION
	cutAction = new QAction(tr("Cu&t"), this);
	cutAction->setIcon(QIcon(":/images/edit/cut.png"));
	cutAction->setShortcut(QKeySequence::Cut);
        cutAction->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
//	connect(cutAction, SIGNAL(triggered()), editor, SLOT(cut()));

    // COPY ACTION
	copyAction = new QAction(tr("&Copy"), this);
	copyAction->setIcon(QIcon(":/images/edit/copy.png"));
	copyAction->setShortcut(QKeySequence::Copy);
        copyAction->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
//	connect(copyAction, SIGNAL(triggered()), editor, SLOT(copy()));

    // PASTE ACTION
	pasteAction = new QAction(tr("&Paste"), this);
	pasteAction->setIcon(QIcon(":/images/edit/paste.png"));
	pasteAction->setShortcut(QKeySequence::Paste);
        pasteAction->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
//	connect(pasteAction, SIGNAL(triggered()), editor, SLOT(paste()));

    // DELETE ACTION
	deleteAction = new QAction(tr("&Delete"), this);
	deleteAction->setShortcut(QKeySequence::Delete);
	deleteAction->setStatusTip(tr("Delete the current selection's contents"));
//	connect(deleteAction, SIGNAL(triggered()), editor, SLOT(removeSelectedText()));

    // ABOUT ACTION
	aboutAction = new QAction(tr("&About"), this);
	aboutAction->setStatusTip(tr("Show the application's About box"));
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

    // ABOUT QT ACTION
	aboutQtAction = new QAction(tr("About &Qt"), this);
	aboutQtAction->setStatusTip(tr("Show the Qt library's About box"));
	connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    // RUN ACTION
	runAction = new QAction(tr("&Run"), this);
	runAction->setShortcut(Qt::Key_F5);
	runAction->setIcon(QIcon(":/images/run.png"));
	runAction->setStatusTip(tr("Run current chunk of Lua code"));
	connect(runAction, SIGNAL(triggered()), this, SLOT(run()));

    // DEBUG ACTION
	debugAction = new QAction(tr("&Debug"), this);
        debugAction->setIcon(QIcon(":/images/compile.png"));
	debugAction->setStatusTip(tr("Debug current chunk of Lua code"));
	connect(debugAction, SIGNAL(triggered()), this, SLOT(debug()));

    // STOP ACTION
	stopAction = new QAction(tr("&Stop"), this);
	stopAction->setIcon(QIcon(":/images/process-stop.png"));
	stopAction->setStatusTip(tr("Stop current running program"));
        connect(stopAction, SIGNAL(triggered()), this, SLOT(stop()));

    // CONTINUE ACTION
        continueAction = new QAction(tr("&Continue"), this);
        continueAction->setIcon(QIcon(":/images/debug/run.png"));
        continueAction->setStatusTip(tr("Continue running program"));
        connect(continueAction, SIGNAL(triggered()), luaDebugger, SLOT(run()));

    // STEP INTO ACTION
        stepIntoAction = new QAction(tr("&Step into"), this);
        stepIntoAction->setIcon(QIcon(":/images/debug/step-into.png"));
        stepIntoAction->setStatusTip(tr("Step into function"));
        connect(stepIntoAction, SIGNAL(triggered()), luaDebugger, SLOT(stepIn()));

    // STEP OVER ACTION
        stepOverAction = new QAction(tr("Step &over"), this);
        stepOverAction->setIcon(QIcon(":/images/debug/step-over.png"));
        stepOverAction->setStatusTip(tr("Step over function"));
        connect(stepOverAction, SIGNAL(triggered()), luaDebugger, SLOT(stepOver()));
}

void LuaControl::createMenus()
{
	fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(newAction);
	fileMenu->addAction(openAction);
	fileMenu->addAction(saveAction);
	fileMenu->addAction(saveAsAction);
	separatorAction = fileMenu->addSeparator();
	for (int i = 0; i < MaxRecentFiles; ++i)
		fileMenu->addAction(recentFileActions[i]);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAction);

	editMenu = menuBar()->addMenu(tr("&Edit"));
	editMenu->addAction(cutAction);
	editMenu->addAction(copyAction);
	editMenu->addAction(pasteAction);
	editMenu->addAction(deleteAction);

	projectMenu = menuBar()->addMenu(tr("&Project"));
	projectMenu->addAction(runAction);
	projectMenu->addAction(debugAction);
	projectMenu->addAction(stopAction);

	optionsMenu = menuBar()->addMenu(tr("&Options"));

	menuBar()->addSeparator();

	helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(aboutAction);
	helpMenu->addAction(aboutQtAction);
}

void LuaControl::createToolBars()
{
	fileToolBar = addToolBar(tr("&File"));
	fileToolBar->addAction(newAction);
	fileToolBar->addAction(openAction);
	fileToolBar->addAction(saveAction);

        runToolBar = addToolBar(tr("&Run"));
	runToolBar->addAction(runAction);
	runToolBar->addAction(debugAction);
	runToolBar->addAction(stopAction);

        debugToolBar = addToolBar(tr("&Debug"));
        debugToolBar->addAction(continueAction);
        debugToolBar->addAction(stepIntoAction);
        debugToolBar->addAction(stepOverAction);
}

void LuaControl::createStatusBar()
{
	statusLabel = new QLabel("Welcome to Lua IDE");
	statusBar()->addWidget(statusLabel);
	updateStatusBar();
}
void LuaControl::updateStatusBar()
{
}

void LuaControl::createDockWindows()
{
	// Lua interpreter dock widget
	QDockWidget *dock = new QDockWidget(tr("Output"), this);
	dock->setAllowedAreas(Qt::AllDockWidgetAreas);
        dock->setWidget(luaConsole);
        addDockWidget(Qt::BottomDockWidgetArea, dock);

	// Locals dock widget

	// Globals dock widget

	// Breakpoints dock widget

	// Watches dock widget
        dock = new QDockWidget(tr("Watches"), this);
        dock->setAllowedAreas(Qt::AllDockWidgetAreas);
        dock->setWidget(watcheslist);
        addDockWidget(Qt::BottomDockWidgetArea, dock);

        // Controller dock widget for debug
        dock = new QDockWidget(tr("RemDebug"), this);
        dock->setAllowedAreas(Qt::AllDockWidgetAreas);
        Console *console = new Console(this);
        luaDebugger->setConsole(console);
        dock->setWidget(console);
        addDockWidget(Qt::BottomDockWidgetArea, dock);
}

void LuaControl::closeEvent(QCloseEvent *event)
{
        if (luaEditor->closeAllSources()) {
		writeSettings();
		event->accept();
	} else {
		event->ignore();
	}
}

void LuaControl::updateRecentFileActions()
{/*
	QMutableStringListIterator i(recentFiles);
	while (i.hasNext()) {
		if (!QFile::exists(i.next()))
			i.remove();
	}

	for (int j = 0; j < MaxRecentFiles; ++j) {
		if (j < recentFiles.count()) {
			QString text = tr("&%1 %2")
					.arg(j + 1)
					.arg(strippedName(recentFiles[j]));
			recentFileActions[j]->setText(text);
			recentFileActions[j]->setData(recentFiles[j]);
			recentFileActions[j]->setVisible(true);
		} else {
			recentFileActions[j]->setVisible(false);
		}
	}
        separatorAction->setVisible(!recentFiles.isEmpty());*/
}

void LuaControl::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) luaEditor->openSource(action->data().toString());
}

void LuaControl::about()
{
	QMessageBox::about(this, tr("About Lua IDE"),
			tr("<h2>Lua IDE 0.1</h2>"
			   "<p>Copyright &copy; 2011 STU FIIT"
			   "<p>Lua IDE is a small application "
			   "for debugging and programming "
			   "in Lua language."));
}

void LuaControl::writeSettings()
{
        QSettings settings("STU FIIT", "Lua IDE");
	settings.setValue("geometry", saveGeometry());
	settings.setValue("recentFiles", recentFiles);
}

void LuaControl::readSettings()
{
        QSettings settings("STU FIIT", "Lua IDE");
	restoreGeometry(settings.value("geometry").toByteArray());
	recentFiles = settings.value("recentFiles").toStringList();
	updateRecentFileActions();
}
