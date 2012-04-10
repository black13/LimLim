#include "debugger.h"
#include "global.h"

#include <QTreeWidgetItem>
#include <QDebug>
#include <QDir>

static const QByteArray StartCommand = QByteArray("> ");
static const QByteArray PauseMessage = QByteArray("Paused:");
static const QByteArray EvaluateMessage = QByteArray("Evaluate:");
static const QByteArray TableMessage = QByteArray("Table:");


/*
	Lua debugger class

	This debugger can be used remotely, whitout lua interpreter,
	to debbug embedded lua applications.

        TODO in final version of program remove console for better performance
*/
Debugger::Debugger(Editor *editor, Console *console, QObject *parent) :
    QObject(parent)
{
    remdebug = new Interpreter(console);
    this->editor = editor;

    autoRun = false;
    status = Off;

    connect(console, SIGNAL(emitOutput(QByteArray)), this, SLOT(parseInput(QByteArray)));
    connect(remdebug, SIGNAL(changedRunningState(bool)), this, SLOT(stateChange(bool)));
    connect(editor, SIGNAL(breakpointSet(int,QString)), this, SLOT(breakpointSet(int,QString)));
    connect(editor, SIGNAL(breakpointDeleted(int,QString)), this, SLOT(breakpointDeleted(int,QString)));

    // set up console
    this->console = console;
}

void Debugger::start()
{
    // Start RemDebug controller
    QString contPath = QString(APP_DIR_PATH)
                       .append("remdebug")
                       .append(QDir::separator())
                       .append("controller.lua");
    remdebug->runFile(contPath);
}

void Debugger::stop()
{
    if (status != Off) remdebug->kill();
}

/*inline*/ void Debugger::giveCommand(const QByteArray& command)
{
    switch (status) {
    case Waiting:
        status = Running;
        console->writeInput(command);
        emit waitingForCommand(false);
        break;
    case Running:
        input.append(command);
        break;
    case On:
        break;
    case Off:
        break;
    }
}

void Debugger::parseInput(const QByteArray& remdebugOutput)
{
    output.append(remdebugOutput);

    // Return if RemDebug didn't write whole status
    if (!output.endsWith(StartCommand)) return;

    // Controller initialization
    if (status == On) {
        status = Running;

        // Lock editor for editing
        editor->lock();

        // Read breakpoints
        QList<Breakpoint*> breakpoints = editor->getBreakpoints();
        QList<Breakpoint*>::iterator iter = breakpoints.begin();

        for (; iter != breakpoints.end(); iter++) {
            Breakpoint *br = static_cast<Breakpoint*> (*iter);
            breakpointSet(br->line, br->file);
        }

        if (autoRun) giveCommand(RunCommand);
    }

    // Parse status messages from output
    if (output.startsWith(PauseMessage)) {
        QRegExp rx(QString("^").append(PauseMessage)
                   .append("( line )?(\\d*)( watch )?(\\d*) file (.*)\n")
                   .append(StartCommand)
                   .append('$'));
        rx.indexIn(output);

        int line = rx.cap(2).toInt();   // get line number
        int watch = rx.cap(4).toInt();  // get watch id
        Q_UNUSED(watch);
        QString file = rx.cap(5);

        editor->debugLine(file, line);

        // Update all watched expressions
        emit luaStateChanged();

    // Parse value of watched expressions
    } else if (output.startsWith(EvaluateMessage)) {
        output.chop(StartCommand.length());

        // TODO add string formatting and whitespace
        QRegExp rx("\\s*([^\t.]+)\t([^\t.]+)");

        int pos = EvaluateMessage.length();
        while ((pos = rx.indexIn(output, pos)) != -1) {

            // TODO format to original values
            const QString &type = rx.cap(1);
            const QString &val = rx.cap(2);

            if (!watches.isEmpty()) {
                // take first watch from list
                QTreeWidgetItem *item = watches.takeFirst();
                // set watch value
                item->setText(1, val.trimmed());
                // set watch type
                item->setText(2, type.trimmed());
            }
            pos += rx.matchedLength();
        }
    } else if(output.startsWith(TableMessage)) {
        output.chop(StartCommand.length());

        // take first table from list
        QTreeWidgetItem *table = tables.takeFirst();
        table->takeChildren();

        QRegExp rx("(\\d+)\t");

        int pos = TableMessage.length();
        while ((pos = rx.indexIn(output, pos)) != -1) {
            QString fieldSize = rx.cap(1);
            QRegExp fieldRx(QString("(\\w+)\t(\\w+)\t(.{")
                            .append(fieldSize)
                            .append("})\n"));
            pos += rx.matchedLength();

            pos = fieldRx.indexIn(output, pos);
            QTreeWidgetItem* field = new QTreeWidgetItem(table,
                QStringList() << fieldRx.cap(1)   // field key
                              << fieldRx.cap(3)   // field value
                              << fieldRx.cap(2)); // field type
            table->addChild(field);
            pos += fieldRx.matchedLength();
        }
    }

    output.clear();

    // Check if there is input waiting for controller
    if (input.isEmpty()) {
        status = Waiting;
        emit waitingForCommand(true);
    } else { // There is input, write as command to console.
        status = Running;

        static QBuffer buffer(&input);
        if (!buffer.isOpen()) buffer.open(QIODevice::ReadOnly | QIODevice::Text);

        console->writeInput(buffer.readLine());

        if (buffer.atEnd()) {
            buffer.close();
            input.clear();
        }
    }
}

void Debugger::stateChange(bool running)
{
    if (running) { // controller started
        status = On;
        emit started();
    } else { // controller ended
        status = Off;
        output.clear();
        input.clear();
        watches.clear();
        editor->debugClear();
        editor->unlock();
        emit waitingForCommand(false);
        emit finished();
    }
}

void Debugger::breakpointSet(int line, QString file)
{
    giveCommand(QString("setb %1 %2\n").arg(file).arg(line).toAscii());
}

void Debugger::breakpointDeleted(int line, QString file)
{
    giveCommand(QString("delb %1 %2\n").arg(file).arg(line).toAscii());
}

/** Adds new expression to watch list. Singal watchesUpdated is emmited
  * when expressions are newly evaluated.
  * When expression is already in the map, expression is not added but
  * updated.
  */
void Debugger::updateWatch(QTreeWidgetItem *watch)
{
    if (status == Off || status == On) return;

    watches.append(watch);

    // evaluate watch
    giveCommand(QByteArray(EvaluateCommand)
        .append("type(")
        .append(watch->text(0)) // type
        .append("), (")
        .append(watch->text(0)) // value
        .append(")\n")
    );
}

void Debugger::updateWatches(QList<QTreeWidgetItem*> *watches)
{
    if (status == Off || status == On) return;

    this->watches.append(*watches);

    QStringList eval(EvaluateCommand);

    foreach (QTreeWidgetItem *watch, *watches) {
        eval.append(QString("type(").append(watch->text(0)).append(')'));
        eval.append(", ");
        eval.append(QString("(").append(watch->text(0)).append(')'));
        eval.append(", ");
    }
    eval.removeLast();
    eval.append("\n");
    giveCommand(eval.join("").toAscii());
}

void Debugger::updateTable(QTreeWidgetItem *table)
{
    if (status == Off || status == On) return;

    tables.append(table);
    giveCommand(QByteArray(TableCommand).append(table->text(0)).append("\n"));
}
