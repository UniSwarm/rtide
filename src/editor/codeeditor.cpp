#include "codeeditor.h"

#include <QApplication>
#include <QBoxLayout>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QSaveFile>

#include "edbee/edbee.h"
#include "edbee/texteditorwidget.h"
#include "edbee/io/textdocumentserializer.h"
#include "edbee/models/textdocument.h"
#include "edbee/models/textgrammar.h"
#include "edbee/models/texteditorconfig.h"
#include "edbee/texteditorcontroller.h"
#include "edbee/util/lineending.h"
#include "edbee/models/textsearcher.h"

#include "edbee/models/chardocument/chartextdocument.h"

bool CodeEditor::initialized = false;

CodeEditor::CodeEditor(Project *project, QWidget *parent)
    : Editor(project, parent)
{
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setMargin(0);

    if (!initialized)
    {
        // get the edbee instance
        edbee::Edbee* tm = edbee::Edbee::instance();

        // configure your paths
        tm->setKeyMapPath( QApplication::applicationDirPath()+"/../contrib/edbee-data/keymaps/");
        tm->setGrammarPath(  QApplication::applicationDirPath()+"/../contrib/edbee-data/syntaxfiles/" );
        tm->setThemePath( QApplication::applicationDirPath()+"/../contrib/edbee-data/themes/" );

        // initialize the library
        tm->init();
        tm->autoShutDownOnAppExit();

        initialized = true;
    }

    _editorWidget =  new edbee::TextEditorWidget();
    QFont font = _editorWidget->config()->font();
    font.setFamily("monospace");
    font.setStyleHint(QFont::Monospace);
    font.setPixelSize(13);
    _editorWidget->config()->setFont(font);
    _editorWidget->config()->setThemeName("RtIDE");
    //_editorWidget->config()->setThemeName("IDLE");
    //_editorWidget->config()->setShowWhitespaceMode(edbee::TextEditorConfig::ShowWhitespaces);
    _editorWidget->textDocument()->setLineEnding(edbee::LineEnding::unixType());

    connect(_editorWidget->textDocument(), &edbee::TextDocument::persistedChanged, this, &CodeEditor::modificationAppend);

    layout->addWidget(_editorWidget);
    setLayout(layout);
}

bool CodeEditor::isModified() const
{
    return !_editorWidget->textDocument()->isPersisted();
}

int CodeEditor::openFileData(const QString &filePath)
{
    QFile file (filePath);
    QFileInfo info(file);
    if (!info.isReadable() || !info.isFile())
        return -1;
    if (!file.open(QIODevice::ReadOnly))
        return -1;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    if (_editorWidget->textDocument()->length()>1)
    {
        edbee::CharTextDocument document;
        edbee::TextDocumentSerializer serializer( &document );
        if( !serializer.loadWithoutOpening( &file ) )
        {
            QApplication::restoreOverrideCursor();
            return -1;
        }

        _editorWidget->controller()->replace( 0, _editorWidget->textDocument()->length(), document.text(), 0 );
    }
    else
    {
        edbee::TextDocumentSerializer serializer( _editorWidget->textDocument() );
        if( !serializer.loadWithoutOpening( &file ) )
        {
            QApplication::restoreOverrideCursor();
            return -1;
        }

        edbee::TextGrammarManager* grammarManager = edbee::Edbee::instance()->grammarManager();
        edbee::TextGrammar* grammar = grammarManager->detectGrammarWithFilename( filePath );
        _editorWidget->textDocument()->setLanguageGrammar( grammar );
    }
    QApplication::restoreOverrideCursor();

    _editorWidget->textDocument()->setPersisted(true);

    setFilePath(filePath);
    emit modified(false);

    // TODO replace this section with .editorconfig
    if (info.suffix()=="mk" || info.fileName()=="Makefile")
        _editorWidget->config()->setUseTabChar(true);
    else
        _editorWidget->config()->setUseTabChar(false);

    return 0;
}

int CodeEditor::saveFileData(const QString &filePath)
{
    QString path = filePath.isEmpty() ? _filePath : filePath;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return -1;

    edbee::TextDocumentSerializer serializer( _editorWidget->textDocument() );
    if(!serializer.saveWithoutOpening(&file))
        return -1;

    if (!file.commit())
        return -1;

    _editorWidget->textDocument()->setPersisted(true);
    emit modified(false);

    return 0;
}

void CodeEditor::modificationAppend()
{
    emit modified(isModified());
}

Editor::SearchCaps CodeEditor::searchCap() const
{
    return Editor::SearchCaps(Editor::HasSearch | Editor::HasRegexp | Editor::HasReplace);
}

int CodeEditor::search(const QVariant &searchTerm, SearchFlags flags)
{
    edbee::TextEditorController* controller = _editorWidget->controller();
    edbee::TextSearcher* searcher = controller->textSearcher();

    if (flags.testFlag(RegExpMode))
    {
        searcher->setSyntax(edbee::TextSearcher::SyntaxRegExp);
        searcher->setSearchTerm(searchTerm.toString());
    }
    else
    {
        searcher->setSyntax(edbee::TextSearcher::SyntaxPlainString);
        searcher->setSearchTerm(QRegExp::escape(searchTerm.toString()));
    }
    searcher->markAll(controller->borderedTextRanges());

    controller->update();

    return controller->borderedTextRanges()->rangeCount(); // return the number of occurences found
}

void CodeEditor::searchNext()
{
    edbee::TextEditorController* controller = _editorWidget->controller();
    edbee::TextSearcher* searcher = controller->textSearcher();

    searcher->findNext(_editorWidget);

    controller->update();
}

void CodeEditor::searchPrev()
{
    edbee::TextEditorController* controller = _editorWidget->controller();
    edbee::TextSearcher* searcher = controller->textSearcher();

    searcher->findPrev(_editorWidget);

    controller->update();
}

void CodeEditor::searchSelectAll()
{
    edbee::TextEditorController* controller = _editorWidget->controller();
    edbee::TextSearcher* searcher = controller->textSearcher();

    searcher->selectAll(_editorWidget);

    controller->update();
}
