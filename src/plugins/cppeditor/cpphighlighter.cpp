/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cpphighlighter.h"

#include "cppdoxygen.h"
#include "cpptoolsreuse.h"

#include <texteditor/textdocumentlayout.h>

#include <cplusplus/SimpleLexer.h>
#include <cplusplus/Lexer.h>

#include <utils/porting.h>

#include <QTextDocument>

using namespace CppEditor;
using namespace TextEditor;
using namespace CPlusPlus;

CppHighlighter::CppHighlighter(QTextDocument *document) :
    SyntaxHighlighter(document)
{
    setDefaultTextFormatCategories();
}

void CppHighlighter::highlightBlock(const QString &text)
{
    const int previousBlockState_ = previousBlockState();
    int lexerState = 0, initialBraceDepth = 0;
    if (previousBlockState_ != -1) {
        lexerState = previousBlockState_ & 0xff;
        initialBraceDepth = previousBlockState_ >> 8;
    }

    int braceDepth = initialBraceDepth;

    SimpleLexer tokenize;
    tokenize.setLanguageFeatures(m_languageFeatures);

    int initialLexerState = lexerState;
    const Tokens tokens = tokenize(text, initialLexerState);
    lexerState = tokenize.state(); // refresh lexer state

    initialLexerState &= ~0x80; // discard newline expected bit
    int foldingIndent = initialBraceDepth;
    if (TextBlockUserData *userData = TextDocumentLayout::textUserData(currentBlock())) {
        userData->setFoldingIndent(0);
        userData->setFoldingStartIncluded(false);
        userData->setFoldingEndIncluded(false);
    }

    if (tokens.isEmpty()) {
        setCurrentBlockState((braceDepth << 8) | lexerState);
        TextDocumentLayout::clearParentheses(currentBlock());
        if (!text.isEmpty())  {// the empty line can still contain whitespace
            if (initialLexerState == T_COMMENT)
                setFormatWithSpaces(text, 0, text.length(), formatForCategory(C_COMMENT));
            else if (initialLexerState == T_DOXY_COMMENT)
                setFormatWithSpaces(text, 0, text.length(), formatForCategory(C_DOXYGEN_COMMENT));
            else
                setFormat(0, text.length(), formatForCategory(C_VISUAL_WHITESPACE));
        }
        TextDocumentLayout::setFoldingIndent(currentBlock(), foldingIndent);
        return;
    }

    const int firstNonSpace = tokens.first().utf16charsBegin();

    Parentheses parentheses;
    parentheses.reserve(5);

    bool expectPreprocessorKeyword = false;
    bool onlyHighlightComments = false;

    for (int i = 0; i < tokens.size(); ++i) {
        const Token &tk = tokens.at(i);

        int previousTokenEnd = 0;
        if (i != 0) {
            // mark the whitespaces
            previousTokenEnd = tokens.at(i - 1).utf16charsBegin() +
                               tokens.at(i - 1).utf16chars();
        }

        if (previousTokenEnd != tk.utf16charsBegin()) {
            setFormat(previousTokenEnd,
                      tk.utf16charsBegin() - previousTokenEnd,
                      formatForCategory(C_VISUAL_WHITESPACE));
        }

        if (tk.is(T_LPAREN) || tk.is(T_LBRACE) || tk.is(T_LBRACKET)) {
            const QChar c = text.at(tk.utf16charsBegin());
            parentheses.append(Parenthesis(Parenthesis::Opened, c, tk.utf16charsBegin()));
            if (tk.is(T_LBRACE)) {
                ++braceDepth;

                // if a folding block opens at the beginning of a line, treat the entire line
                // as if it were inside the folding block
                if (tk.utf16charsBegin() == firstNonSpace) {
                    ++foldingIndent;
                    TextDocumentLayout::userData(currentBlock())->setFoldingStartIncluded(true);
                }
            }
        } else if (tk.is(T_RPAREN) || tk.is(T_RBRACE) || tk.is(T_RBRACKET)) {
            const QChar c = text.at(tk.utf16charsBegin());
            parentheses.append(Parenthesis(Parenthesis::Closed, c, tk.utf16charsBegin()));
            if (tk.is(T_RBRACE)) {
                --braceDepth;
                if (braceDepth < foldingIndent) {
                    // unless we are at the end of the block, we reduce the folding indent
                    if (i == tokens.size()-1 || tokens.at(i+1).is(T_SEMICOLON))
                        TextDocumentLayout::userData(currentBlock())->setFoldingEndIncluded(true);
                    else
                        foldingIndent = qMin(braceDepth, foldingIndent);
                }
            }
        }

        bool highlightCurrentWordAsPreprocessor = expectPreprocessorKeyword;

        if (expectPreprocessorKeyword)
            expectPreprocessorKeyword = false;

        if (onlyHighlightComments && !tk.isComment())
            continue;

        if (i == 0 && tk.is(T_POUND)) {
            setFormatWithSpaces(text, tk.utf16charsBegin(), tk.utf16chars(),
                          formatForCategory(C_PREPROCESSOR));
            expectPreprocessorKeyword = true;
        } else if (highlightCurrentWordAsPreprocessor && (tk.isKeyword() || tk.is(T_IDENTIFIER))
                   && isPPKeyword(Utils::midView(text, tk.utf16charsBegin(), tk.utf16chars()))) {
            setFormat(tk.utf16charsBegin(), tk.utf16chars(), formatForCategory(C_PREPROCESSOR));
            const QStringView ppKeyword = Utils::midView(text, tk.utf16charsBegin(), tk.utf16chars());
            if (ppKeyword == QLatin1String("error")
                    || ppKeyword == QLatin1String("warning")
                    || ppKeyword == QLatin1String("pragma")) {
                onlyHighlightComments = true;
            }

        } else if (tk.is(T_NUMERIC_LITERAL)) {
            setFormat(tk.utf16charsBegin(), tk.utf16chars(), formatForCategory(C_NUMBER));
        } else if (tk.isStringLiteral() || tk.isCharLiteral()) {
            if (!highlightRawStringLiteral(text, tk)) {
                setFormatWithSpaces(text, tk.utf16charsBegin(), tk.utf16chars(),
                                    formatForCategory(C_STRING));
            }
        } else if (tk.isComment()) {
            const int startPosition = initialLexerState ? previousTokenEnd : tk.utf16charsBegin();
            if (tk.is(T_COMMENT) || tk.is(T_CPP_COMMENT)) {
                setFormatWithSpaces(text, startPosition, tk.utf16charsEnd() - startPosition,
                              formatForCategory(C_COMMENT));
            }

            else // a doxygen comment
                highlightDoxygenComment(text, startPosition, tk.utf16charsEnd() - startPosition);

            // we need to insert a close comment parenthesis, if
            //  - the line starts in a C Comment (initalState != 0)
            //  - the first token of the line is a T_COMMENT (i == 0 && tk.is(T_COMMENT))
            //  - is not a continuation line (tokens.size() > 1 || !state)
            if (initialLexerState && i == 0 && (tk.is(T_COMMENT) || tk.is(T_DOXY_COMMENT))
                && (tokens.size() > 1 || !lexerState)) {
                --braceDepth;
                // unless we are at the end of the block, we reduce the folding indent
                if (i == tokens.size()-1)
                    TextDocumentLayout::userData(currentBlock())->setFoldingEndIncluded(true);
                else
                    foldingIndent = qMin(braceDepth, foldingIndent);
                const int tokenEnd = tk.utf16charsBegin() + tk.utf16chars() - 1;
                parentheses.append(Parenthesis(Parenthesis::Closed, QLatin1Char('-'), tokenEnd));

                // clear the initial state.
                initialLexerState = 0;
            }

        } else if (tk.isKeyword()
                   || (m_languageFeatures.qtKeywordsEnabled
                       && isQtKeyword(
                           QStringView{text}.mid(tk.utf16charsBegin(), tk.utf16chars())))
                   || (m_languageFeatures.objCEnabled && tk.isObjCAtKeyword())) {
            setFormat(tk.utf16charsBegin(), tk.utf16chars(), formatForCategory(C_KEYWORD));
        } else if (tk.isPrimitiveType()) {
            setFormat(tk.utf16charsBegin(), tk.utf16chars(),
                      formatForCategory(C_PRIMITIVE_TYPE));
        } else if (tk.isOperator()) {
            setFormat(tk.utf16charsBegin(), tk.utf16chars(), formatForCategory(C_OPERATOR));
        } else if (tk.isPunctuation()) {
            setFormat(tk.utf16charsBegin(), tk.utf16chars(), formatForCategory(C_PUNCTUATION));
        } else if (i == 0 && tokens.size() > 1 && tk.is(T_IDENTIFIER) && tokens.at(1).is(T_COLON)) {
            setFormat(tk.utf16charsBegin(), tk.utf16chars(), formatForCategory(C_LABEL));
        } else if (tk.is(T_IDENTIFIER)) {
            highlightWord(Utils::midView(text, tk.utf16charsBegin(), tk.utf16chars()),
                          tk.utf16charsBegin(),
                          tk.utf16chars());
        }
    }

    // mark the trailing white spaces
    const int lastTokenEnd = tokens.last().utf16charsEnd();
    if (text.length() > lastTokenEnd)
        formatSpaces(text, lastTokenEnd, text.length() - lastTokenEnd);

    if (!initialLexerState && lexerState && !tokens.isEmpty()) {
        const Token &lastToken = tokens.last();
        if (lastToken.is(T_COMMENT) || lastToken.is(T_DOXY_COMMENT)) {
            parentheses.append(Parenthesis(Parenthesis::Opened, QLatin1Char('+'),
                                           lastToken.utf16charsBegin()));
            ++braceDepth;
        }
    }

    TextDocumentLayout::setParentheses(currentBlock(), parentheses);

    // if the block is ifdefed out, we only store the parentheses, but

    // do not adjust the brace depth.
    if (TextDocumentLayout::ifdefedOut(currentBlock())) {
        braceDepth = initialBraceDepth;
        foldingIndent = initialBraceDepth;
    }

    TextDocumentLayout::setFoldingIndent(currentBlock(), foldingIndent);

    // optimization: if only the brace depth changes, we adjust subsequent blocks
    // to have QSyntaxHighlighter stop the rehighlighting
    int currentState = currentBlockState();
    if (currentState != -1) {
        int oldState = currentState & 0xff;
        int oldBraceDepth = currentState >> 8;
        if (oldState == tokenize.state() && oldBraceDepth != braceDepth) {
            TextDocumentLayout::FoldValidator foldValidor;
            foldValidor.setup(qobject_cast<TextDocumentLayout *>(document()->documentLayout()));
            int delta = braceDepth - oldBraceDepth;
            QTextBlock block = currentBlock().next();
            while (block.isValid() && block.userState() != -1) {
                TextDocumentLayout::changeBraceDepth(block, delta);
                TextDocumentLayout::changeFoldingIndent(block, delta);
                foldValidor.process(block);
                block = block.next();
            }
            foldValidor.finalize();
        }
    }

    setCurrentBlockState((braceDepth << 8) | tokenize.state());
}

void CppHighlighter::setLanguageFeatures(const LanguageFeatures &languageFeatures)
{
    if (languageFeatures != m_languageFeatures) {
        m_languageFeatures = languageFeatures;
        rehighlight();
    }
}

bool CppHighlighter::isPPKeyword(QStringView text) const
{
    switch (text.length())
    {
    case 2:
        if (text.at(0) == QLatin1Char('i') && text.at(1) == QLatin1Char('f'))
            return true;
        break;

    case 4:
        if (text.at(0) == QLatin1Char('e')
            && (text == QLatin1String("elif") || text == QLatin1String("else")))
            return true;
        break;

    case 5:
        switch (text.at(0).toLatin1()) {
        case 'i':
            if (text == QLatin1String("ifdef"))
                return true;
            break;
          case 'u':
            if (text == QLatin1String("undef"))
                return true;
            break;
        case 'e':
            if (text == QLatin1String("endif") || text == QLatin1String("error"))
                return true;
            break;
        }
        break;

    case 6:
        switch (text.at(0).toLatin1()) {
        case 'i':
            if (text == QLatin1String("ifndef") || text == QLatin1String("import"))
                return true;
            break;
        case 'd':
            if (text == QLatin1String("define"))
                return true;
            break;
        case 'p':
            if (text == QLatin1String("pragma"))
                return true;
            break;
        }
        break;

    case 7:
        switch (text.at(0).toLatin1()) {
        case 'i':
            if (text == QLatin1String("include"))
                return true;
            break;
        case 'w':
            if (text == QLatin1String("warning"))
                return true;
            break;
        }
        break;

    case 12:
        if (text.at(0) == QLatin1Char('i') && text == QLatin1String("include_next"))
            return true;
        break;

    default:
        break;
    }

    return false;
}

void CppHighlighter::highlightWord(QStringView word, int position, int length)
{
    // try to highlight Qt 'identifiers' like QObject and Q_PROPERTY

    if (word.length() > 2 && word.at(0) == QLatin1Char('Q')) {
        if (word.at(1) == QLatin1Char('_') // Q_
            || (word.at(1) == QLatin1Char('T') && word.at(2) == QLatin1Char('_'))) { // QT_
            for (int i = 1; i < word.length(); ++i) {
                const QChar &ch = word.at(i);
                if (!(ch.isUpper() || ch == QLatin1Char('_')))
                    return;
            }

            setFormat(position, length, formatForCategory(C_TYPE));
        }
    }
}

bool CppHighlighter::highlightRawStringLiteral(QStringView _text, const Token &tk)
{
    // Step one: Does the lexer think this is a raw string literal?
    switch (tk.kind()) {
    case T_RAW_STRING_LITERAL:
    case T_RAW_WIDE_STRING_LITERAL:
    case T_RAW_UTF8_STRING_LITERAL:
    case T_RAW_UTF16_STRING_LITERAL:
    case T_RAW_UTF32_STRING_LITERAL:
        break;
    default:
        return false;
    }

    // TODO: Remove on upgrade to Qt >= 5.14.
    const QString text = _text.toString();

    // Step two: Find all the components. Bail out if we don't have a complete,
    //           well-formed raw string literal.
    const int rOffset = text.indexOf(QLatin1String("R\""), tk.utf16charsBegin());
    if (rOffset == -1)
        return false;
    const int delimiterOffset = rOffset + 2;
    const int openParenOffset = text.indexOf('(', delimiterOffset);
    if (openParenOffset == -1)
        return false;
    const QStringView delimiter = text.mid(delimiterOffset, openParenOffset - delimiterOffset);
    if (text.at(tk.utf16charsEnd() - 1) != '"')
        return false;
    const int endDelimiterOffset = tk.utf16charsEnd() - 1 - delimiter.length();
    if (endDelimiterOffset <= delimiterOffset)
        return false;
    if (text.mid(endDelimiterOffset, delimiter.length()) != delimiter)
        return false;
    if (text.at(endDelimiterOffset - 1) != ')')
        return false;

    // Step three: Do the actual formatting. For clarity, we display only the actual content as
    //             a string, and the rest (including the delimiter) as a keyword.
    const QTextCharFormat delimiterFormat = formatForCategory(C_KEYWORD);
    const int stringOffset = delimiterOffset + delimiter.length() + 1;
    setFormat(tk.utf16charsBegin(), stringOffset, delimiterFormat);
    setFormatWithSpaces(text, stringOffset, endDelimiterOffset - stringOffset - 1,
                        formatForCategory(C_STRING));
    setFormat(endDelimiterOffset - 1, delimiter.length() + 2, delimiterFormat);
    return true;
}

void CppHighlighter::highlightDoxygenComment(const QString &text, int position, int)
{
    int initial = position;

    const QChar *uc = text.unicode();
    const QChar *it = uc + position;

    const QTextCharFormat &format = formatForCategory(C_DOXYGEN_COMMENT);
    const QTextCharFormat &kwFormat = formatForCategory(C_DOXYGEN_TAG);

    while (!it->isNull()) {
        if (it->unicode() == QLatin1Char('\\') ||
            it->unicode() == QLatin1Char('@')) {
            ++it;

            const QChar *start = it;
            while (isValidAsciiIdentifierChar(*it))
                ++it;

            int k = classifyDoxygenTag(start, it - start);
            if (k != T_DOXY_IDENTIFIER) {
                setFormatWithSpaces(text, initial, start - uc - initial, format);
                setFormat(start - uc - 1, it - start + 1, kwFormat);
                initial = it - uc;
            }
        } else
            ++it;
    }

    setFormatWithSpaces(text, initial, it - uc - initial, format);
}

