#include "debugger.h"

static const QByteArray StartCommand = QByteArray("> ");
static const QByteArray PauseMessage = QByteArray("Paused:");


/*
	Lua debugger class

	This debugger can be used remotely, whitout lua interpreter,
	to debbug embedded lua applications.
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
    // start RemDebug controller
    remdebug->runFile("controller.lua");
}

void Debugger::stop()
{
    if (status != Off) remdebug->kill();
}

void Debugger::giveCommand(QByteArray command)
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

void Debugger::parseInput(QByteArray remdebugOutput)
{
    output.append(remdebugOutput);

    // Return if RemDebug didn't write whole status
    if (!output.endsWith(StartCommand)) return;

    if (output.startsWith(PauseMessage)) {
        QRegExp rx(QString("^").append(PauseMessage)
                   .append("( line ){0,1}(\\d*)( watch ){0,1}(\\d*) file (.*)\n.*"));

        rx.indexIn(output);

        int line = rx.cap(2).toInt();   // get line number
        int watch = rx.cap(4).toInt();  // get watch id
        QString file = rx.cap(5);

        editor->debugLine(file, line);

    }

    if (output.endsWith(StartCommand)) {

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
                giveCommand(QString("setb %1 %2\n").arg(br->file).arg(br->line).toAscii());
            }

            if (autoRun) giveCommand(RunCommand);
        }

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

    output.clear();
}

void Debugger::stateChange(bool running)
{
    if (running) { // controller started
        status = On;
        emit started();
    } else { // controller ended
        status = Off;
        editor->debugClear();
        editor->unlock();
        output.clear();
        input.clear();
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
