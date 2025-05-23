/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtextformat.h"
#include "qtextformat_p.h"

#include <qvariant.h>
#include <qdatastream.h>
#include <qdebug.h>
#include <qmap.h>
#include <qhashfunctions.h>

QT_BEGIN_NAMESPACE

/*!
    \class QTextLength
    \reentrant

    \brief The QTextLength class encapsulates the different types of length
    used in a QTextDocument.
    \inmodule QtGui

    \ingroup richtext-processing

    When we specify a value for the length of an element in a text document,
    we often need to provide some other information so that the length is
    used in the way we expect. For example, when we specify a table width,
    the value can represent a fixed number of pixels, or it can be a percentage
    value. This information changes both the meaning of the value and the way
    it is used.

    Generally, this class is used to specify table widths. These can be
    specified either as a fixed amount of pixels, as a percentage of the
    containing frame's width, or by a variable width that allows it to take
    up just the space it requires.

    \sa QTextTable
*/

/*!
    \fn explicit QTextLength::QTextLength()

    Constructs a new length object which represents a variable size.
*/

/*!
    \fn QTextLength::QTextLength(Type type, qreal value)

    Constructs a new length object of the given \a type and \a value.
*/

/*!
    \fn Type QTextLength::type() const

    Returns the type of this length object.

    \sa QTextLength::Type
*/

/*!
    \fn qreal QTextLength::value(qreal maximumLength) const

    Returns the effective length, constrained by the type of the length object
    and the specified \a maximumLength.

    \sa type()
*/

/*!
    \fn qreal QTextLength::rawValue() const

    Returns the constraint value that is specific for the type of the length.
    If the length is QTextLength::PercentageLength then the raw value is in
    percent, in the range of 0 to 100. If the length is QTextLength::FixedLength
    then that fixed amount is returned. For variable lengths, zero is returned.
*/

/*!
    \fn bool QTextLength::operator==(const QTextLength &other) const

    Returns \c true if this text length is the same as the \a other text
    length.
*/

/*!
    \fn bool QTextLength::operator!=(const QTextLength &other) const

    Returns \c true if this text length is different from the \a other text
    length.
*/

/*!
    \enum QTextLength::Type

    This enum describes the different types a length object can
    have.

    \value VariableLength The width of the object is variable
    \value FixedLength The width of the object is fixed
    \value PercentageLength The width of the object is in
                            percentage of the maximum width

    \sa type()
*/

/*!
   Returns the text length as a QVariant
*/
QTextLength::operator QVariant() const
{
    return QVariant(QMetaType::QTextLength, this);
}

#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &stream, const QTextLength &length)
{
    return stream << qint32(length.lengthType) << double(length.fixedValueOrPercentage);
}

QDataStream &operator>>(QDataStream &stream, QTextLength &length)
{
    qint32 type;
    double fixedValueOrPercentage;
    stream >> type >> fixedValueOrPercentage;
    length.fixedValueOrPercentage = fixedValueOrPercentage;
    length.lengthType = QTextLength::Type(type);
    return stream;
}
#endif // QT_NO_DATASTREAM

namespace {
struct Property
{
    inline Property(qint32 k, const QVariant &v) : key(k), value(v) {}
    inline Property() {}

    qint32 key = -1;
    QVariant value;

    inline bool operator==(const Property &other) const
    { return key == other.key && value == other.value; }
};
}
Q_DECLARE_TYPEINFO(Property, Q_MOVABLE_TYPE);

class QTextFormatPrivate : public QSharedData
{
public:
    QTextFormatPrivate() : hashDirty(true), fontDirty(true), hashValue(0) {}

    inline uint hash() const
    {
        if (!hashDirty)
            return hashValue;
        return recalcHash();
    }

    inline bool operator==(const QTextFormatPrivate &rhs) const {
        if (hash() != rhs.hash())
            return false;

        return props == rhs.props;
    }

    inline void insertProperty(qint32 key, const QVariant &value)
    {
        hashDirty = true;
        if ((key >= QTextFormat::FirstFontProperty && key <= QTextFormat::LastFontProperty)
                || key == QTextFormat::FontLetterSpacingType) {
            fontDirty = true;
        }
        for (int i = 0; i < props.count(); ++i)
            if (props.at(i).key == key) {
                props[i].value = value;
                return;
            }
        props.append(Property(key, value));
    }

    inline void clearProperty(qint32 key)
    {
        for (int i = 0; i < props.count(); ++i)
            if (props.at(i).key == key) {
                hashDirty = true;
                if ((key >= QTextFormat::FirstFontProperty && key <= QTextFormat::LastFontProperty)
                        || key == QTextFormat::FontLetterSpacingType) {
                    fontDirty = true;
                }
                props.remove(i);
                return;
            }
    }

    inline int propertyIndex(qint32 key) const
    {
        for (int i = 0; i < props.count(); ++i)
            if (props.at(i).key == key)
                return i;
        return -1;
    }

    inline QVariant property(qint32 key) const
    {
        const int idx = propertyIndex(key);
        if (idx < 0)
            return QVariant();
        return props.at(idx).value;
    }

    inline bool hasProperty(qint32 key) const
    { return propertyIndex(key) != -1; }

    void resolveFont(const QFont &defaultFont);

    inline const QFont &font() const {
        if (fontDirty)
            recalcFont();
        return fnt;
    }

    QVector<Property> props;
private:

    uint recalcHash() const;
    void recalcFont() const;

    mutable bool hashDirty;
    mutable bool fontDirty;
    mutable uint hashValue;
    mutable QFont fnt;

    friend QDataStream &operator<<(QDataStream &, const QTextFormat &);
    friend QDataStream &operator>>(QDataStream &, QTextFormat &);
};

static inline uint hash(const QColor &color)
{
    return (color.isValid()) ? color.rgba() : 0x234109;
}

static inline uint hash(const QPen &pen)
{
    return hash(pen.color()) + qHash(pen.widthF());
}

static inline uint hash(const QBrush &brush)
{
    return hash(brush.color()) + (brush.style() << 3);
}

static inline uint variantHash(const QVariant &variant)
{
    // simple and fast hash functions to differentiate between type and value
    switch (variant.userType()) { // sorted by occurrence frequency
    case QMetaType::QString: return qHash(variant.toString());
    case QMetaType::Double: return qHash(variant.toDouble());
    case QMetaType::Int: return 0x811890U + variant.toInt();
    case QMetaType::QBrush:
        return 0x01010101 + hash(qvariant_cast<QBrush>(variant));
    case QMetaType::Bool: return 0x371818 + variant.toBool();
    case QMetaType::QPen: return 0x02020202 + hash(qvariant_cast<QPen>(variant));
    case QMetaType::QVariantList:
        return 0x8377U + qvariant_cast<QVariantList>(variant).count();
    case QMetaType::QColor: return hash(qvariant_cast<QColor>(variant));
      case QMetaType::QTextLength:
        return 0x377 + hash(qvariant_cast<QTextLength>(variant).rawValue());
    case QMetaType::Float: return qHash(variant.toFloat());
    case QMetaType::UnknownType: return 0;
    default: break;
    }
    return qHash(variant.typeName());
}

static inline int getHash(const QTextFormatPrivate *d, int format)
{
    return (d ? d->hash() : 0) + format;
}

uint QTextFormatPrivate::recalcHash() const
{
    hashValue = 0;
    for (QVector<Property>::ConstIterator it = props.constBegin(); it != props.constEnd(); ++it)
        hashValue += (static_cast<quint32>(it->key) << 16) + variantHash(it->value);

    hashDirty = false;

    return hashValue;
}

void QTextFormatPrivate::resolveFont(const QFont &defaultFont)
{
    recalcFont();
    const uint oldMask = fnt.resolve();
    fnt = fnt.resolve(defaultFont);

    if (hasProperty(QTextFormat::FontSizeAdjustment)) {
        const qreal scaleFactors[7] = {qreal(0.7), qreal(0.8), qreal(1.0), qreal(1.2), qreal(1.5), qreal(2), qreal(2.4)};

        const int htmlFontSize = qBound(0, property(QTextFormat::FontSizeAdjustment).toInt() + 3 - 1, 6);


        if (defaultFont.pointSize() <= 0) {
            qreal pixelSize = scaleFactors[htmlFontSize] * defaultFont.pixelSize();
            fnt.setPixelSize(qRound(pixelSize));
        } else {
            qreal pointSize = scaleFactors[htmlFontSize] * defaultFont.pointSizeF();
            fnt.setPointSizeF(pointSize);
        }
    }

    fnt.resolve(oldMask);
}

void QTextFormatPrivate::recalcFont() const
{
    // update cached font as well
    QFont f;

    bool hasSpacingInformation = false;
    QFont::SpacingType spacingType = QFont::PercentageSpacing;
    qreal letterSpacing = 0.0;

    for (int i = 0; i < props.count(); ++i) {
        switch (props.at(i).key) {
            case QTextFormat::FontFamily:
                f.setFamily(props.at(i).value.toString());
                break;
            case QTextFormat::FontFamilies:
                f.setFamilies(props.at(i).value.toStringList());
                break;
            case QTextFormat::FontStyleName:
                f.setStyleName(props.at(i).value.toString());
                break;
            case QTextFormat::FontPointSize:
                f.setPointSizeF(props.at(i).value.toReal());
                break;
            case  QTextFormat::FontPixelSize:
                f.setPixelSize(props.at(i).value.toInt());
                break;
            case QTextFormat::FontWeight: {
                const QVariant weightValue = props.at(i).value;
                int weight = weightValue.toInt();
                if (weight >= 0 && weightValue.isValid())
                    f.setWeight(weight);
                break; }
            case QTextFormat::FontItalic:
                f.setItalic(props.at(i).value.toBool());
                break;
            case QTextFormat::FontUnderline:
                if (! hasProperty(QTextFormat::TextUnderlineStyle)) // don't use the old one if the new one is there.
                    f.setUnderline(props.at(i).value.toBool());
                break;
            case QTextFormat::TextUnderlineStyle:
                f.setUnderline(static_cast<QTextCharFormat::UnderlineStyle>(props.at(i).value.toInt()) == QTextCharFormat::SingleUnderline);
                break;
            case QTextFormat::FontOverline:
                f.setOverline(props.at(i).value.toBool());
                break;
            case QTextFormat::FontStrikeOut:
                f.setStrikeOut(props.at(i).value.toBool());
                break;
            case QTextFormat::FontLetterSpacingType:
                spacingType = static_cast<QFont::SpacingType>(props.at(i).value.toInt());
                hasSpacingInformation = true;
                break;
            case QTextFormat::FontLetterSpacing:
                letterSpacing = props.at(i).value.toReal();
                hasSpacingInformation = true;
                break;
            case QTextFormat::FontWordSpacing:
                f.setWordSpacing(props.at(i).value.toReal());
                break;
            case QTextFormat::FontCapitalization:
                f.setCapitalization(static_cast<QFont::Capitalization> (props.at(i).value.toInt()));
                break;
            case QTextFormat::FontFixedPitch: {
                const bool value = props.at(i).value.toBool();
                if (f.fixedPitch() != value)
                    f.setFixedPitch(value);
                break; }
            case QTextFormat::FontStretch:
                f.setStretch(props.at(i).value.toInt());
                break;
            case QTextFormat::FontStyleHint:
                f.setStyleHint(static_cast<QFont::StyleHint>(props.at(i).value.toInt()), f.styleStrategy());
                break;
            case QTextFormat::FontHintingPreference:
                f.setHintingPreference(static_cast<QFont::HintingPreference>(props.at(i).value.toInt()));
                break;
            case QTextFormat::FontStyleStrategy:
                f.setStyleStrategy(static_cast<QFont::StyleStrategy>(props.at(i).value.toInt()));
                break;
            case QTextFormat::FontKerning:
                f.setKerning(props.at(i).value.toBool());
                break;
            default:
                break;
            }
    }

    if (hasSpacingInformation)
        f.setLetterSpacing(spacingType, letterSpacing);

    fnt = f;
    fontDirty = false;
}

#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &stream, const QTextFormat &fmt)
{
    stream << fmt.format_type << fmt.properties();
    return stream;
}

Q_GUI_EXPORT QDataStream &operator>>(QDataStream &stream, QTextFormat &fmt)
{
    QMap<qint32, QVariant> properties;
    stream >> fmt.format_type >> properties;

    // QTextFormat's default constructor doesn't allocate the private structure, so
    // we have to do this, in case fmt is a default constructed value.
    if(!fmt.d)
        fmt.d = new QTextFormatPrivate();

    for (QMap<qint32, QVariant>::ConstIterator it = properties.constBegin();
         it != properties.constEnd(); ++it)
        fmt.d->insertProperty(it.key(), it.value());

    return stream;
}
#endif // QT_NO_DATASTREAM

/*!
    \class QTextFormat
    \reentrant

    \brief The QTextFormat class provides formatting information for a
    QTextDocument.
    \inmodule QtGui

    \ingroup richtext-processing
    \ingroup shared

    A QTextFormat is a generic class used for describing the format of
    parts of a QTextDocument. The derived classes QTextCharFormat,
    QTextBlockFormat, QTextListFormat, and QTextTableFormat are usually
    more useful, and describe the formatting that is applied to
    specific parts of the document.

    A format has a \c FormatType which specifies the kinds of text item it
    can format; e.g. a block of text, a list, a table, etc. A format
    also has various properties (some specific to particular format
    types), as described by the Property enum. Every property has a
    corresponding Property.

    The format type is given by type(), and the format can be tested
    with isCharFormat(), isBlockFormat(), isListFormat(),
    isTableFormat(), isFrameFormat(), and isImageFormat(). If the
    type is determined, it can be retrieved with toCharFormat(),
    toBlockFormat(), toListFormat(), toTableFormat(), toFrameFormat(),
    and toImageFormat().

    A format's properties can be set with the setProperty() functions,
    and retrieved with boolProperty(), intProperty(), doubleProperty(),
    and stringProperty() as appropriate. All the property IDs used in
    the format can be retrieved with allPropertyIds(). One format can
    be merged into another using merge().

    A format's object index can be set with setObjectIndex(), and
    retrieved with objectIndex(). These methods can be used to
    associate the format with a QTextObject. It is used to represent
    lists, frames, and tables inside the document.

    \sa {Rich Text Processing}
*/

/*!
    \enum QTextFormat::FormatType

    This enum describes the text item a QTextFormat object is formatting.

    \value InvalidFormat An invalid format as created by the default
                         constructor
    \value BlockFormat The object formats a text block
    \value CharFormat The object formats a single character
    \value ListFormat The object formats a list
    \omitvalue TableFormat \omit Unused Value, a table's FormatType is FrameFormat. \endomit
    \value FrameFormat The object formats a frame

    \value UserFormat

    \sa QTextCharFormat, QTextBlockFormat, QTextListFormat,
    QTextTableFormat, type()
*/

/*!
    \enum QTextFormat::Property

    This enum describes the different properties a format can have.

    \value ObjectIndex The index of the formatted object. See objectIndex().

    Paragraph and character properties

    \value CssFloat How a frame is located relative to the surrounding text
    \value LayoutDirection  The layout direction of the text in the document
                            (Qt::LayoutDirection).

    \value OutlinePen
    \value ForegroundBrush
    \value BackgroundBrush
    \value BackgroundImageUrl

    Paragraph properties

    \value BlockAlignment
    \value BlockTopMargin
    \value BlockBottomMargin
    \value BlockLeftMargin
    \value BlockRightMargin
    \value TextIndent
    \value TabPositions     Specifies the tab positions.  The tab positions are structs of QTextOption::Tab which are stored in
                            a QList (internally, in a QList<QVariant>).
    \value BlockIndent
    \value LineHeight
    \value LineHeightType
    \value BlockNonBreakableLines
    \value BlockTrailingHorizontalRulerWidth The width of a horizontal ruler element.
    \value HeadingLevel     The level of a heading, for example 1 corresponds to an HTML H1 tag; otherwise 0.
                            This enum value has been added in Qt 5.12.
    \value BlockCodeFence   The character that was used in the "fences" around a Markdown code block.
                            If the code block was indented rather than fenced, the block should not have this property.
                            This enum value has been added in Qt 5.14.

    \value BlockQuoteLevel  The depth of nested quoting on this block: 1 means the block is a top-level block quote.
                            Blocks that are not block quotes should not have this property.
                            This enum value has been added in Qt 5.14.
    \value BlockCodeLanguage The programming language in a preformatted or code block.
                            Blocks that do not contain code should not have this property.
                            This enum value has been added in Qt 5.14.
    \value BlockMarker      The \l{QTextBlockFormat::MarkerType}{type of adornment} to be shown alongside the block.
                            This enum value has been added in Qt 5.14.

    Character properties

    \value FontFamily
    \value FontFamilies
    \value FontStyleName
    \value FontPointSize
    \value FontPixelSize
    \value FontSizeAdjustment       Specifies the change in size given to the fontsize already set using
                                    FontPointSize or FontPixelSize.
    \value FontFixedPitch
    \omitvalue FontSizeIncrement
    \value FontWeight
    \value FontItalic
    \value FontUnderline \e{This property has been deprecated.} Use QTextFormat::TextUnderlineStyle instead.
    \value FontOverline
    \value FontStrikeOut
    \value FontCapitalization Specifies the capitalization type that is to be applied to the text.
    \value FontLetterSpacingType Specifies the meaning of the FontLetterSpacing property. The default
                                 is QFont::PercentageSpacing.
    \value FontLetterSpacing Changes the default spacing between individual letters in the font. The value is
                             specified as a percentage or absolute value, depending on FontLetterSpacingType.
                             The default value is 100%.
    \value FontWordSpacing  Changes the default spacing between individual words. A positive value increases the word spacing
                                                 by the corresponding pixels; a negative value decreases the spacing.
    \value FontStretch          Corresponds to the QFont::Stretch property
    \value FontStyleHint        Corresponds to the QFont::StyleHint property
    \value FontStyleStrategy    Corresponds to the QFont::StyleStrategy property
    \value FontKerning          Specifies whether the font has kerning turned on.
    \value FontHintingPreference Controls the use of hinting according to values
                                 of the QFont::HintingPreference enum.

    \omitvalue FirstFontProperty
    \omitvalue LastFontProperty

    \value TextUnderlineColor
    \value TextVerticalAlignment
    \value TextOutline
    \value TextUnderlineStyle
    \value TextToolTip Specifies the (optional) tool tip to be displayed for a fragment of text.

    \value IsAnchor
    \value AnchorHref
    \value AnchorName
    \value ObjectType

    List properties

    \value ListStyle        Specifies the style used for the items in a list,
                            described by values of the QTextListFormat::Style enum.
    \value ListIndent       Specifies the amount of indentation used for a list.
    \value ListNumberPrefix Defines the text which is prepended to item numbers in
                            numeric lists.
    \value ListNumberSuffix Defines the text which is appended to item numbers in
                            numeric lists.

    Table and frame properties

    \value FrameBorder
    \value FrameBorderBrush
    \value FrameBorderStyle See the \l{QTextFrameFormat::BorderStyle}{BorderStyle} enum.
    \value FrameBottomMargin
    \value FrameHeight
    \value FrameLeftMargin
    \value FrameMargin
    \value FramePadding
    \value FrameRightMargin
    \value FrameTopMargin
    \value FrameWidth
    \value TableCellSpacing
    \value TableCellPadding
    \value TableColumns
    \value TableColumnWidthConstraints
    \value TableHeaderRowCount
    \value TableBorderCollapse Specifies the \l QTextTableFormat::borderCollapse property.

    Table cell properties

    \value TableCellRowSpan
    \value TableCellColumnSpan
    \value TableCellLeftPadding
    \value TableCellRightPadding
    \value TableCellTopPadding
    \value TableCellBottomPadding

    Table cell properties intended for use with \l QTextTableFormat::borderCollapse enabled

    \value TableCellTopBorder
    \value TableCellBottomBorder
    \value TableCellLeftBorder
    \value TableCellRightBorder

    \value TableCellTopBorderStyle
    \value TableCellBottomBorderStyle
    \value TableCellLeftBorderStyle
    \value TableCellRightBorderStyle

    \value TableCellTopBorderBrush
    \value TableCellBottomBorderBrush
    \value TableCellLeftBorderBrush
    \value TableCellRightBorderBrush

    Image properties

    \value ImageName        The filename or source of the image.
    \value ImageTitle       The title attribute of an HTML image tag, or
                            the quoted string that comes after the URL in a Markdown image link.
                            This enum value has been added in Qt 5.14.
    \value ImageAltText     The alt attribute of an HTML image tag, or
                            the image description in a Markdown image link.
                            This enum value has been added in Qt 5.14.
    \value ImageWidth
    \value ImageHeight
    \value ImageQuality

    Selection properties

    \value FullWidthSelection When set on the characterFormat of a selection,
                              the whole width of the text will be shown selected.

    Page break properties

    \value PageBreakPolicy Specifies how pages are broken. See the PageBreakFlag enum.

    \value UserProperty

    \sa property(), setProperty()
*/

/*!
    \enum QTextFormat::ObjectTypes

    This enum describes what kind of QTextObject this format is associated with.

    \value NoObject
    \value ImageObject
    \value TableObject
    \value TableCellObject
    \value UserObject The first object that can be used for application-specific purposes.

    \sa QTextObject, QTextTable, QTextObject::format()
*/

/*!
    \enum QTextFormat::PageBreakFlag
    \since 4.2

    This enum describes how page breaking is performed when printing. It maps to the
    corresponding css properties.

    \value PageBreak_Auto The page break is determined automatically depending on the
                          available space on the current page
    \value PageBreak_AlwaysBefore The page is always broken before the paragraph/table
    \value PageBreak_AlwaysAfter  A new page is always started after the paragraph/table

    \sa QTextBlockFormat::pageBreakPolicy(), QTextFrameFormat::pageBreakPolicy(),
    PageBreakPolicy
*/

/*!
    \fn bool QTextFormat::isValid() const

    Returns \c true if the format is valid (i.e. is not
    InvalidFormat); otherwise returns \c false.
*/

/*!
    \fn bool QTextFormat::isEmpty() const
    \since 5.3

    Returns true if the format does not store any properties; false otherwise.

    \sa propertyCount(), properties()
*/

/*!
    \fn bool QTextFormat::isCharFormat() const

    Returns \c true if this text format is a \c CharFormat; otherwise
    returns \c false.
*/


/*!
    \fn bool QTextFormat::isBlockFormat() const

    Returns \c true if this text format is a \c BlockFormat; otherwise
    returns \c false.
*/


/*!
    \fn bool QTextFormat::isListFormat() const

    Returns \c true if this text format is a \c ListFormat; otherwise
    returns \c false.
*/


/*!
    \fn bool QTextFormat::isTableFormat() const

    Returns \c true if this text format is a \c TableFormat; otherwise
    returns \c false.
*/


/*!
    \fn bool QTextFormat::isFrameFormat() const

    Returns \c true if this text format is a \c FrameFormat; otherwise
    returns \c false.
*/


/*!
    \fn bool QTextFormat::isImageFormat() const

    Returns \c true if this text format is an image format; otherwise
    returns \c false.
*/


/*!
    \fn bool QTextFormat::isTableCellFormat() const
    \since 4.4

    Returns \c true if this text format is a \c TableCellFormat; otherwise
    returns \c false.
*/


/*!
    Creates a new text format with an \c InvalidFormat.

    \sa FormatType
*/
QTextFormat::QTextFormat()
    : format_type(InvalidFormat)
{
}

/*!
    Creates a new text format of the given \a type.

    \sa FormatType
*/
QTextFormat::QTextFormat(int type)
    : format_type(type)
{
}


/*!
    \fn QTextFormat::QTextFormat(const QTextFormat &other)

    Creates a new text format with the same attributes as the \a other
    text format.
*/
QTextFormat::QTextFormat(const QTextFormat &rhs)
    : d(rhs.d), format_type(rhs.format_type)
{
}

/*!
    \fn QTextFormat &QTextFormat::operator=(const QTextFormat &other)

    Assigns the \a other text format to this text format, and returns a
    reference to this text format.
*/
QTextFormat &QTextFormat::operator=(const QTextFormat &rhs)
{
    d = rhs.d;
    format_type = rhs.format_type;
    return *this;
}

/*!
    \fn void QTextFormat::swap(QTextFormat &other)
    \since 5.0

    Swaps this text format with \a other. This function is very fast
    and never fails.
*/

/*!
    Destroys this text format.
*/
QTextFormat::~QTextFormat()
{
}


/*!
   Returns the text format as a QVariant
*/
QTextFormat::operator QVariant() const
{
    return QVariant(QMetaType::QTextFormat, this);
}

/*!
    Merges the \a other format with this format; where there are
    conflicts the \a other format takes precedence.
*/
void QTextFormat::merge(const QTextFormat &other)
{
    if (format_type != other.format_type)
        return;

    if (!d) {
        d = other.d;
        return;
    }

    if (!other.d)
        return;

    QTextFormatPrivate *d = this->d;

    const QVector<QT_PREPEND_NAMESPACE(Property)> &otherProps = other.d->props;
    d->props.reserve(d->props.size() + otherProps.size());
    for (int i = 0; i < otherProps.count(); ++i) {
        const QT_PREPEND_NAMESPACE(Property) &p = otherProps.at(i);
        d->insertProperty(p.key, p.value);
    }
}

/*!
    Returns the type of this format.

    \sa FormatType
*/
int QTextFormat::type() const
{
    return format_type;
}

/*!
    Returns this format as a block format.
*/
QTextBlockFormat QTextFormat::toBlockFormat() const
{
    return QTextBlockFormat(*this);
}

/*!
    Returns this format as a character format.
*/
QTextCharFormat QTextFormat::toCharFormat() const
{
    return QTextCharFormat(*this);
}

/*!
    Returns this format as a list format.
*/
QTextListFormat QTextFormat::toListFormat() const
{
    return QTextListFormat(*this);
}

/*!
    Returns this format as a table format.
*/
QTextTableFormat QTextFormat::toTableFormat() const
{
    return QTextTableFormat(*this);
}

/*!
    Returns this format as a frame format.
*/
QTextFrameFormat QTextFormat::toFrameFormat() const
{
    return QTextFrameFormat(*this);
}

/*!
    Returns this format as an image format.
*/
QTextImageFormat QTextFormat::toImageFormat() const
{
    return QTextImageFormat(*this);
}

/*!
    \since 4.4

    Returns this format as a table cell format.
*/
QTextTableCellFormat QTextFormat::toTableCellFormat() const
{
    return QTextTableCellFormat(*this);
}

/*!
    Returns the value of the property specified by \a propertyId. If the
    property isn't of QTextFormat::Bool type, false is returned instead.

    \sa setProperty(), intProperty(), doubleProperty(), stringProperty(), colorProperty(),
        lengthProperty(), lengthVectorProperty(), Property
*/
bool QTextFormat::boolProperty(int propertyId) const
{
    if (!d)
        return false;
    const QVariant prop = d->property(propertyId);
    if (prop.userType() != QMetaType::Bool)
        return false;
    return prop.toBool();
}

/*!
    Returns the value of the property specified by \a propertyId. If the
    property is not of QTextFormat::Integer type, 0 is returned instead.

    \sa setProperty(), boolProperty(), doubleProperty(), stringProperty(), colorProperty(),
        lengthProperty(), lengthVectorProperty(), Property
*/
int QTextFormat::intProperty(int propertyId) const
{
    // required, since the default layout direction has to be LayoutDirectionAuto, which is not integer 0
    int def = (propertyId == QTextFormat::LayoutDirection) ? int(Qt::LayoutDirectionAuto) : 0;

    if (!d)
        return def;
    const QVariant prop = d->property(propertyId);
    if (prop.userType() != QMetaType::Int)
        return def;
    return prop.toInt();
}

/*!
    Returns the value of the property specified by \a propertyId. If the
    property isn't of QVariant::Double or QMetaType::Float type, 0 is
    returned instead.

    \sa setProperty(), boolProperty(), intProperty(), stringProperty(), colorProperty(),
        lengthProperty(), lengthVectorProperty(), Property
*/
qreal QTextFormat::doubleProperty(int propertyId) const
{
    if (!d)
        return 0.;
    const QVariant prop = d->property(propertyId);
    if (prop.userType() != QMetaType::Double && prop.userType() != QMetaType::Float)
        return 0.;
    return qvariant_cast<qreal>(prop);
}

/*!
    Returns the value of the property given by \a propertyId; if the
    property isn't of QVariant::String type, an empty string is
    returned instead.

    \sa setProperty(), boolProperty(), intProperty(), doubleProperty(), colorProperty(),
        lengthProperty(), lengthVectorProperty(), Property
*/
QString QTextFormat::stringProperty(int propertyId) const
{
    if (!d)
        return QString();
    const QVariant prop = d->property(propertyId);
    if (prop.userType() != QMetaType::QString)
        return QString();
    return prop.toString();
}

/*!
    Returns the value of the property given by \a propertyId; if the
    property isn't of QVariant::Color type, an invalid color is
    returned instead.

    \sa setProperty(), boolProperty(), intProperty(), doubleProperty(),
        stringProperty(), lengthProperty(), lengthVectorProperty(), Property
*/
QColor QTextFormat::colorProperty(int propertyId) const
{
    if (!d)
        return QColor();
    const QVariant prop = d->property(propertyId);
    if (prop.userType() != QMetaType::QColor)
        return QColor();
    return qvariant_cast<QColor>(prop);
}

/*!
    Returns the value of the property given by \a propertyId; if the
    property isn't of QVariant::Pen type, Qt::NoPen is
    returned instead.

    \sa setProperty(), boolProperty(), intProperty(), doubleProperty(), stringProperty(),
        lengthProperty(), lengthVectorProperty(), Property
*/
QPen QTextFormat::penProperty(int propertyId) const
{
    if (!d)
        return QPen(Qt::NoPen);
    const QVariant prop = d->property(propertyId);
    if (prop.userType() != QMetaType::QPen)
        return QPen(Qt::NoPen);
    return qvariant_cast<QPen>(prop);
}

/*!
    Returns the value of the property given by \a propertyId; if the
    property isn't of QVariant::Brush type, Qt::NoBrush is
    returned instead.

    \sa setProperty(), boolProperty(), intProperty(), doubleProperty(), stringProperty(),
        lengthProperty(), lengthVectorProperty(), Property
*/
QBrush QTextFormat::brushProperty(int propertyId) const
{
    if (!d)
        return QBrush(Qt::NoBrush);
    const QVariant prop = d->property(propertyId);
    if (prop.userType() != QMetaType::QBrush)
        return QBrush(Qt::NoBrush);
    return qvariant_cast<QBrush>(prop);
}

/*!
    Returns the value of the property given by \a propertyId.

    \sa setProperty(), boolProperty(), intProperty(), doubleProperty(), stringProperty(),
        colorProperty(), lengthVectorProperty(), Property
*/
QTextLength QTextFormat::lengthProperty(int propertyId) const
{
    if (!d)
        return QTextLength();
    return qvariant_cast<QTextLength>(d->property(propertyId));
}

/*!
    Returns the value of the property given by \a propertyId. If the
    property isn't of QTextFormat::LengthVector type, an empty length
    vector is returned instead.

    \sa setProperty(), boolProperty(), intProperty(), doubleProperty(), stringProperty(),
        colorProperty(), lengthProperty(), Property
*/
QVector<QTextLength> QTextFormat::lengthVectorProperty(int propertyId) const
{
    QVector<QTextLength> vector;
    if (!d)
        return vector;
    const QVariant prop = d->property(propertyId);
    if (prop.userType() != QMetaType::QVariantList)
        return vector;

    QList<QVariant> propertyList = prop.toList();
    for (int i=0; i<propertyList.size(); ++i) {
        QVariant var = propertyList.at(i);
        if (var.userType() == QMetaType::QTextLength)
            vector.append(qvariant_cast<QTextLength>(var));
    }

    return vector;
}

/*!
    Returns the property specified by the given \a propertyId.

    \sa Property
*/
QVariant QTextFormat::property(int propertyId) const
{
    return d ? d->property(propertyId) : QVariant();
}

/*!
    Sets the property specified by the \a propertyId to the given \a value.

    \sa Property
*/
void QTextFormat::setProperty(int propertyId, const QVariant &value)
{
    if (!d)
        d = new QTextFormatPrivate;
    if (!value.isValid())
        clearProperty(propertyId);
    else
        d->insertProperty(propertyId, value);
}

/*!
    Sets the value of the property given by \a propertyId to \a value.

    \sa lengthVectorProperty(), Property
*/
void QTextFormat::setProperty(int propertyId, const QVector<QTextLength> &value)
{
    if (!d)
        d = new QTextFormatPrivate;
    QVariantList list;
    const int numValues = value.size();
    list.reserve(numValues);
    for (int i = 0; i < numValues; ++i)
        list << value.at(i);
    d->insertProperty(propertyId, list);
}

/*!
    Clears the value of the property given by \a propertyId

    \sa Property
*/
void QTextFormat::clearProperty(int propertyId)
{
    if (!d)
        return;
    d->clearProperty(propertyId);
}


/*!
    \fn void QTextFormat::setObjectType(int type)

    Sets the text format's object type to \a type.

    \sa ObjectTypes, objectType()
*/


/*!
    \fn int QTextFormat::objectType() const

    Returns the text format's object type.

    \sa ObjectTypes, setObjectType()
*/


/*!
    Returns the index of the format object, or -1 if the format object is invalid.

    \sa setObjectIndex()
*/
int QTextFormat::objectIndex() const
{
    if (!d)
        return -1;
    const QVariant prop = d->property(ObjectIndex);
    if (prop.userType() != QMetaType::Int) // ####
        return -1;
    return prop.toInt();
}

/*!
    \fn void QTextFormat::setObjectIndex(int index)

    Sets the format object's object \a index.

    \sa objectIndex()
*/
void QTextFormat::setObjectIndex(int o)
{
    if (o == -1) {
        if (d)
            d->clearProperty(ObjectIndex);
    } else {
        if (!d)
            d = new QTextFormatPrivate;
        // ### type
        d->insertProperty(ObjectIndex, o);
    }
}

/*!
    Returns \c true if the text format has a property with the given \a
    propertyId; otherwise returns \c false.

    \sa properties(), Property
*/
bool QTextFormat::hasProperty(int propertyId) const
{
    return d ? d->hasProperty(propertyId) : false;
}

/*
    Returns the property type for the given \a propertyId.

    \sa hasProperty(), allPropertyIds(), Property
*/

/*!
    Returns a map with all properties of this text format.
*/
QMap<int, QVariant> QTextFormat::properties() const
{
    QMap<int, QVariant> map;
    if (d) {
        for (int i = 0; i < d->props.count(); ++i)
            map.insert(d->props.at(i).key, d->props.at(i).value);
    }
    return map;
}

/*!
    \since 4.3
    Returns the number of properties stored in the format.
*/
int QTextFormat::propertyCount() const
{
    return d ? d->props.count() : 0;
}

/*!
    \fn bool QTextFormat::operator!=(const QTextFormat &other) const

    Returns \c true if this text format is different from the \a other text
    format.
*/


/*!
    \fn bool QTextFormat::operator==(const QTextFormat &other) const

    Returns \c true if this text format is the same as the \a other text
    format.
*/
bool QTextFormat::operator==(const QTextFormat &rhs) const
{
    if (format_type != rhs.format_type)
        return false;

    if (d == rhs.d)
        return true;

    if (d && d->props.isEmpty() && !rhs.d)
        return true;

    if (!d && rhs.d && rhs.d->props.isEmpty())
        return true;

    if (!d || !rhs.d)
        return false;

    return *d == *rhs.d;
}

/*!
    \class QTextCharFormat
    \reentrant

    \brief The QTextCharFormat class provides formatting information for
    characters in a QTextDocument.
    \inmodule QtGui

    \ingroup richtext-processing
    \ingroup shared

    The character format of text in a document specifies the visual properties
    of the text, as well as information about its role in a hypertext document.

    The font used can be set by supplying a font to the setFont() function, and
    each aspect of its appearance can be adjusted to give the desired effect.
    setFontFamily() and setFontPointSize() define the font's family (e.g. Times)
    and printed size; setFontWeight() and setFontItalic() provide control over
    the style of the font. setFontUnderline(), setFontOverline(),
    setFontStrikeOut(), and setFontFixedPitch() provide additional effects for
    text.

    The color is set with setForeground(). If the text is intended to be used
    as an anchor (for hyperlinks), this can be enabled with setAnchor(). The
    setAnchorHref() and setAnchorNames() functions are used to specify the
    information about the hyperlink's destination and the anchor's name.

    \sa QTextFormat, QTextBlockFormat, QTextTableFormat, QTextListFormat
*/

/*!
    \enum QTextCharFormat::VerticalAlignment

    This enum describes the ways that adjacent characters can be vertically
    aligned.

    \value AlignNormal  Adjacent characters are positioned in the standard
                        way for text in the writing system in use.
    \value AlignSuperScript Characters are placed above the base line for
                            normal text.
    \value AlignSubScript   Characters are placed below the base line for
                            normal text.
    \value AlignMiddle The center of the object is vertically aligned with the
                       base line. Currently, this is only implemented for
                       inline objects.
    \value AlignBottom The bottom edge of the object is vertically aligned with
                       the base line.
    \value AlignTop    The top edge of the object is vertically aligned with
                       the base line.
    \value AlignBaseline The base lines of the characters are aligned.
*/

/*!
    \enum QTextCharFormat::UnderlineStyle

    This enum describes the different ways drawing underlined text.

    \value NoUnderline          Text is draw without any underlining decoration.
    \value SingleUnderline      A line is drawn using Qt::SolidLine.
    \value DashUnderline        Dashes are drawn using Qt::DashLine.
    \value DotLine              Dots are drawn using Qt::DotLine;
    \value DashDotLine          Dashs and dots are drawn using Qt::DashDotLine.
    \value DashDotDotLine       Underlines draw drawn using Qt::DashDotDotLine.
    \value WaveUnderline        The text is underlined using a wave shaped line.
    \value SpellCheckUnderline  The underline is drawn depending on the SpellCheckUnderlineStyle
                                theme hint of QPlatformTheme. By default this is mapped to
                                WaveUnderline, on \macos it is mapped to DotLine.

    \sa Qt::PenStyle
*/

/*!
    \fn QTextCharFormat::QTextCharFormat()

    Constructs a new character format object.
*/
QTextCharFormat::QTextCharFormat() : QTextFormat(CharFormat) {}

/*!
    \internal
    \fn QTextCharFormat::QTextCharFormat(const QTextFormat &other)

    Creates a new character format with the same attributes as the \a given
    text format.
*/
QTextCharFormat::QTextCharFormat(const QTextFormat &fmt)
 : QTextFormat(fmt)
{
}

/*!
    \fn bool QTextCharFormat::isValid() const

    Returns \c true if this character format is valid; otherwise returns
    false.
*/


/*!
    \fn void QTextCharFormat::setFontFamily(const QString &family)

    Sets the text format's font \a family.

    \sa setFont()
*/


/*!
    \fn QString QTextCharFormat::fontFamily() const

    Returns the text format's font family.

    \sa font()
*/

/*!
    \fn void QTextCharFormat::setFontFamilies(const QStringList &families)
    \since 5.13

    Sets the text format's font \a families.

    \sa setFont()
*/

/*!
    \fn QStringList QTextCharFormat::fontFamilies() const
    \since 5.13

    Returns the text format's font families.

    \sa font()
*/

/*!
    \fn void QTextCharFormat::setFontStyleName(const QString &styleName)
    \since 5.13

    Sets the text format's font \a styleName.

    \sa setFont(), QFont::setStyleName()
*/

/*!
    \fn QStringList QTextCharFormat::fontStyleName() const
    \since 5.13

    Returns the text format's font style name.

    \sa font(), QFont::styleName()
*/

/*!
    \fn void QTextCharFormat::setFontPointSize(qreal size)

    Sets the text format's font \a size.

    \sa setFont()
*/


/*!
    \fn qreal QTextCharFormat::fontPointSize() const

    Returns the font size used to display text in this format.

    \sa font()
*/


/*!
    \fn void QTextCharFormat::setFontWeight(int weight)

    Sets the text format's font weight to \a weight.

    \sa setFont(), QFont::Weight
*/


/*!
    \fn int QTextCharFormat::fontWeight() const

    Returns the text format's font weight.

    \sa font(), QFont::Weight
*/


/*!
    \fn void QTextCharFormat::setFontItalic(bool italic)

    If \a italic is true, sets the text format's font to be italic; otherwise
    the font will be non-italic.

    \sa setFont()
*/


/*!
    \fn bool QTextCharFormat::fontItalic() const

    Returns \c true if the text format's font is italic; otherwise
    returns \c false.

    \sa font()
*/


/*!
    \fn void QTextCharFormat::setFontUnderline(bool underline)

    If \a underline is true, sets the text format's font to be underlined;
    otherwise it is displayed non-underlined.

    \sa setFont()
*/


/*!
    \fn bool QTextCharFormat::fontUnderline() const

    Returns \c true if the text format's font is underlined; otherwise
    returns \c false.

    \sa font()
*/
bool QTextCharFormat::fontUnderline() const
{
    if (hasProperty(TextUnderlineStyle))
        return underlineStyle() == SingleUnderline;
    return boolProperty(FontUnderline);
}

/*!
    \fn UnderlineStyle QTextCharFormat::underlineStyle() const
    \since 4.2

    Returns the style of underlining the text.
*/

/*!
    \fn void QTextCharFormat::setUnderlineStyle(UnderlineStyle style)
    \since 4.2

    Sets the style of underlining the text to \a style.
*/
void QTextCharFormat::setUnderlineStyle(UnderlineStyle style)
{
    setProperty(TextUnderlineStyle, style);
    // for compatibility
    setProperty(FontUnderline, style == SingleUnderline);
}

/*!
    \fn void QTextCharFormat::setFontOverline(bool overline)

    If \a overline is true, sets the text format's font to be overlined;
    otherwise the font is displayed non-overlined.

    \sa setFont()
*/


/*!
    \fn bool QTextCharFormat::fontOverline() const

    Returns \c true if the text format's font is overlined; otherwise
    returns \c false.

    \sa font()
*/


/*!
    \fn void QTextCharFormat::setFontStrikeOut(bool strikeOut)

    If \a strikeOut is true, sets the text format's font with strike-out
    enabled (with a horizontal line through it); otherwise it is displayed
    without strikeout.

    \sa setFont()
*/


/*!
    \fn bool QTextCharFormat::fontStrikeOut() const

    Returns \c true if the text format's font is struck out (has a horizontal line
    drawn through it); otherwise returns \c false.

    \sa font()
*/


/*!
    \since 4.5
    \fn void QTextCharFormat::setFontStyleHint(QFont::StyleHint hint, QFont::StyleStrategy strategy)

    Sets the font style \a hint and \a strategy.

    Qt does not support style hints on X11 since this information is not provided by the window system.

    \sa setFont()
    \sa QFont::setStyleHint()
*/


/*!
    \since 4.5
    \fn void QTextCharFormat::setFontStyleStrategy(QFont::StyleStrategy strategy)

    Sets the font style \a strategy.

    \sa setFont()
    \sa QFont::setStyleStrategy()
*/


/*!
    \since 4.5
    \fn void QTextCharFormat::setFontKerning(bool enable)
    Enables kerning for this font if \a enable is true; otherwise disables it.

    When kerning is enabled, glyph metrics do not add up anymore, even for
    Latin text. In other words, the assumption that width('a') + width('b')
    is equal to width("ab") is not neccesairly true.

    \sa setFont()
*/


/*!
    \fn QTextCharFormat::StyleHint QTextCharFormat::fontStyleHint() const
    \since 4.5

    Returns the font style hint.

    \sa setFontStyleHint(), font()
*/


/*!
    \since 4.5
    \fn QTextCharFormat::StyleStrategy QTextCharFormat::fontStyleStrategy() const

    Returns the current font style strategy.

    \sa setFontStyleStrategy()
    \sa font()
*/


/*!
    \since 4.5
    \fn  bool QTextCharFormat::fontKerning() const
    Returns \c true if the font kerning is enabled.

    \sa setFontKerning()
    \sa font()
*/


/*!
    \fn void QTextCharFormat::setFontFixedPitch(bool fixedPitch)

    If \a fixedPitch is true, sets the text format's font to be fixed pitch;
    otherwise a non-fixed pitch font is used.

    \sa setFont()
*/


/*!
    \fn bool QTextCharFormat::fontFixedPitch() const

    Returns \c true if the text format's font is fixed pitch; otherwise
    returns \c false.

    \sa font()
*/

/*!
    \since 4.8

    \fn void QTextCharFormat::setFontHintingPreference(QFont::HintingPreference hintingPreference)

    Sets the hinting preference of the text format's font to be \a hintingPreference.

    \sa setFont(), QFont::setHintingPreference()
*/

/*!
    \since 4.8

    \fn QFont::HintingPreference QTextCharFormat::fontHintingPreference() const

    Returns the hinting preference set for this text format.

    \sa font(), QFont::hintingPreference()
*/

/*!
    \fn QPen QTextCharFormat::textOutline() const

    Returns the pen used to draw the outlines of characters in this format.
*/


/*!
    \fn void QTextCharFormat::setTextOutline(const QPen &pen)

    Sets the pen used to draw the outlines of characters to the given \a pen.
*/

/*!
    \fn void QTextCharFormat::setToolTip(const QString &text)
    \since 4.3

    Sets the tool tip for a fragment of text to the given \a text.
*/

/*!
    \fn QString QTextCharFormat::toolTip() const
    \since 4.3

    Returns the tool tip that is displayed for a fragment of text.
*/

/*!
    \fn void QTextFormat::setForeground(const QBrush &brush)

    Sets the foreground brush to the specified \a brush. The foreground
    brush is mostly used to render text.

    \sa foreground(), clearForeground(), setBackground()
*/


/*!
    \fn QBrush QTextFormat::foreground() const

    Returns the brush used to render foreground details, such as text,
    frame outlines, and table borders.

    \sa setForeground(), clearForeground(), background()
*/

/*!
    \fn void QTextFormat::clearForeground()

    Clears the brush used to paint the document's foreground. The default
    brush will be used.

    \sa foreground(), setForeground(), clearBackground()
*/


/*!
    \fn void QTextCharFormat::setAnchor(bool anchor)

    If \a anchor is true, text with this format represents an anchor, and is
    formatted in the appropriate way; otherwise the text is formatted normally.
    (Anchors are hyperlinks which are often shown underlined and in a different
    color from plain text.)

    The way the text is rendered is independent of whether or not the format
    has a valid anchor defined. Use setAnchorHref(), and optionally
    setAnchorNames() to create a hypertext link.

    \sa isAnchor()
*/


/*!
    \fn bool QTextCharFormat::isAnchor() const

    Returns \c true if the text is formatted as an anchor; otherwise
    returns \c false.

    \sa setAnchor(), setAnchorHref(), setAnchorNames()
*/


/*!
    \fn void QTextCharFormat::setAnchorHref(const QString &value)

    Sets the hypertext link for the text format to the given \a value.
    This is typically a URL like "http://example.com/index.html".

    The anchor will be displayed with the \a value as its display text;
    if you want to display different text call setAnchorNames().

    To format the text as a hypertext link use setAnchor().
*/


/*!
    \fn QString QTextCharFormat::anchorHref() const

    Returns the text format's hypertext link, or an empty string if
    none has been set.
*/


#if QT_DEPRECATED_SINCE(5, 13)
/*!
    \fn void QTextCharFormat::setAnchorName(const QString &name)
    \obsolete

    This function is deprecated. Use setAnchorNames() instead.

    Sets the text format's anchor \a name. For the anchor to work as a
    hyperlink, the destination must be set with setAnchorHref() and
    the anchor must be enabled with setAnchor().
*/
#endif

/*!
    \fn void QTextCharFormat::setAnchorNames(const QStringList &names)
    \since 4.3

    Sets the text format's anchor \a names. For the anchor to work as a
    hyperlink, the destination must be set with setAnchorHref() and
    the anchor must be enabled with setAnchor().
*/

#if QT_DEPRECATED_SINCE(5, 13)
/*!
    \fn QString QTextCharFormat::anchorName() const
    \obsolete

    This function is deprecated. Use anchorNames() instead.

    Returns the anchor name associated with this text format, or an empty
    string if none has been set. If the anchor name is set, text with this
    format can be the destination of a hypertext link.
*/
QString QTextCharFormat::anchorName() const
{
    QVariant prop = property(AnchorName);
    if (prop.userType() == QMetaType::QStringList)
        return prop.toStringList().value(0);
    else if (prop.userType() != QMetaType::QString)
        return QString();
    return prop.toString();
}
#endif

/*!
    \fn QStringList QTextCharFormat::anchorNames() const
    \since 4.3

    Returns the anchor names associated with this text format, or an empty
    string list if none has been set. If the anchor names are set, text with this
    format can be the destination of a hypertext link.
*/
QStringList QTextCharFormat::anchorNames() const
{
    QVariant prop = property(AnchorName);
    if (prop.userType() == QMetaType::QStringList)
        return prop.toStringList();
    else if (prop.userType() != QMetaType::QString)
        return QStringList();
    return QStringList(prop.toString());
}


/*!
    \fn void QTextCharFormat::setTableCellRowSpan(int tableCellRowSpan)
    \internal

    If this character format is applied to characters in a table cell,
    the cell will span \a tableCellRowSpan rows.
*/


/*!
    \fn int QTextCharFormat::tableCellRowSpan() const
    \internal

    If this character format is applied to characters in a table cell,
    this function returns the number of rows spanned by the text (this may
    be 1); otherwise it returns 1.
*/

/*!
    \fn void QTextCharFormat::setTableCellColumnSpan(int tableCellColumnSpan)
    \internal

    If this character format is applied to characters in a table cell,
    the cell will span \a tableCellColumnSpan columns.
*/


/*!
    \fn int QTextCharFormat::tableCellColumnSpan() const
    \internal

    If this character format is applied to characters in a table cell,
    this function returns the number of columns spanned by the text (this
    may be 1); otherwise it returns 1.
*/

/*!
    \fn void QTextCharFormat::setUnderlineColor(const QColor &color)

    Sets the underline color used for the characters with this format to
    the \a color specified.

    \sa underlineColor()
*/

/*!
    \fn QColor QTextCharFormat::underlineColor() const

    Returns the color used to underline the characters with this format.

    \sa setUnderlineColor()
*/

/*!
    \fn void QTextCharFormat::setVerticalAlignment(VerticalAlignment alignment)

    Sets the vertical alignment used for the characters with this format to
    the \a alignment specified.

    \sa verticalAlignment()
*/

/*!
    \fn VerticalAlignment QTextCharFormat::verticalAlignment() const

    Returns the vertical alignment used for characters with this format.

    \sa setVerticalAlignment()
*/

/*!
    \enum QTextCharFormat::FontPropertiesInheritanceBehavior
    \since 5.3

    This enum specifies how the setFont() function should behave with
    respect to unset font properties.

    \value FontPropertiesSpecifiedOnly  If a property is not explicitly set, do not
                                        change the text format's property value.
    \value FontPropertiesAll  If a property is not explicitly set, override the
                              text format's property with a default value.

    \sa setFont()
*/

/*!
    \overload

    Sets the text format's \a font.

    \sa font()
*/
void QTextCharFormat::setFont(const QFont &font)
{
    setFont(font, FontPropertiesAll);
}

/*!
    \since 5.3

    Sets the text format's \a font.

    If \a behavior is QTextCharFormat::FontPropertiesAll, the font property that
    has not been explicitly set is treated like as it were set with default value;
    If \a behavior is QTextCharFormat::FontPropertiesSpecifiedOnly, the font property that
    has not been explicitly set is ignored and the respective property value
    remains unchanged.

    \sa font()
*/
void QTextCharFormat::setFont(const QFont &font, FontPropertiesInheritanceBehavior behavior)
{
    const uint mask = behavior == FontPropertiesAll ? uint(QFont::AllPropertiesResolved)
                                                    : font.resolve();

    if (mask & QFont::FamilyResolved)
        setFontFamily(font.family());
    if (mask & QFont::FamiliesResolved)
        setFontFamilies(font.families());
    if (mask & QFont::StyleNameResolved)
        setFontStyleName(font.styleName());

    if (mask & QFont::SizeResolved) {
        const qreal pointSize = font.pointSizeF();
        if (pointSize > 0) {
            setFontPointSize(pointSize);
        } else {
            const int pixelSize = font.pixelSize();
            if (pixelSize > 0)
                setProperty(QTextFormat::FontPixelSize, pixelSize);
        }
    }

    if (mask & QFont::WeightResolved)
        setFontWeight(font.weight());
    if (mask & QFont::StyleResolved)
        setFontItalic(font.style() != QFont::StyleNormal);
    if (mask & QFont::UnderlineResolved)
        setUnderlineStyle(font.underline() ? SingleUnderline : NoUnderline);
    if (mask & QFont::OverlineResolved)
        setFontOverline(font.overline());
    if (mask & QFont::StrikeOutResolved)
        setFontStrikeOut(font.strikeOut());
    if (mask & QFont::FixedPitchResolved)
        setFontFixedPitch(font.fixedPitch());
    if (mask & QFont::CapitalizationResolved)
        setFontCapitalization(font.capitalization());
    if (mask & QFont::WordSpacingResolved)
        setFontWordSpacing(font.wordSpacing());
    if (mask & QFont::LetterSpacingResolved) {
        setFontLetterSpacingType(font.letterSpacingType());
        setFontLetterSpacing(font.letterSpacing());
    }
    if (mask & QFont::StretchResolved)
        setFontStretch(font.stretch());
    if (mask & QFont::StyleHintResolved)
        setFontStyleHint(font.styleHint());
    if (mask & QFont::StyleStrategyResolved)
        setFontStyleStrategy(font.styleStrategy());
    if (mask & QFont::HintingPreferenceResolved)
        setFontHintingPreference(font.hintingPreference());
    if (mask & QFont::KerningResolved)
        setFontKerning(font.kerning());
}

/*!
    Returns the font for this character format.
*/
QFont QTextCharFormat::font() const
{
    return d ? d->font() : QFont();
}

/*!
    \class QTextBlockFormat
    \reentrant

    \brief The QTextBlockFormat class provides formatting information for
    blocks of text in a QTextDocument.
    \inmodule QtGui

    \ingroup richtext-processing
    \ingroup shared

    A document is composed of a list of blocks, represented by QTextBlock
    objects. Each block can contain an item of some kind, such as a
    paragraph of text, a table, a list, or an image. Every block has an
    associated QTextBlockFormat that specifies its characteristics.

    To cater for left-to-right and right-to-left languages you can set
    a block's direction with setLayoutDirection(). Paragraph alignment is
    set with setAlignment(). Margins are controlled by setTopMargin(),
    setBottomMargin(), setLeftMargin(), setRightMargin(). Overall
    indentation is set with setIndent(), the indentation of the first
    line with setTextIndent().

    Line spacing is set with setLineHeight() and retrieved via lineHeight()
    and lineHeightType(). The types of line spacing available are in the
    LineHeightTypes enum.

    Line breaking can be enabled and disabled with setNonBreakableLines().

    The brush used to paint the paragraph's background
    is set with \l{QTextFormat::setBackground()}{setBackground()}, and other
    aspects of the text's appearance can be customized by using the
    \l{QTextFormat::setProperty()}{setProperty()} function with the
    \c OutlinePen, \c ForegroundBrush, and \c BackgroundBrush
    \l{QTextFormat::Property} values.

    If a text block is part of a list, it can also have a list format that
    is accessible with the listFormat() function.

    \sa QTextBlock, QTextCharFormat
*/

/*!
    \since 4.8
    \enum QTextBlockFormat::LineHeightTypes

    This enum describes the various types of line spacing support paragraphs can have.

    \value SingleHeight This is the default line height: single spacing.
    \value ProportionalHeight This sets the spacing proportional to the line (in percentage).
                              For example, set to 200 for double spacing.
    \value FixedHeight This sets the line height to a fixed line height (in pixels).
    \value MinimumHeight This sets the minimum line height (in pixels).
    \value LineDistanceHeight This adds the specified height between lines (in pixels).

    \sa lineHeight(), lineHeightType(), setLineHeight()
*/

/*!
    \fn QTextBlockFormat::QTextBlockFormat()

    Constructs a new QTextBlockFormat.
*/
QTextBlockFormat::QTextBlockFormat() : QTextFormat(BlockFormat) {}

/*!
    \internal
    \fn QTextBlockFormat::QTextBlockFormat(const QTextFormat &other)

    Creates a new block format with the same attributes as the \a given
    text format.
*/
QTextBlockFormat::QTextBlockFormat(const QTextFormat &fmt)
 : QTextFormat(fmt)
{
}

/*!
    \since 4.4
    Sets the tab positions for the text block to those specified by
    \a tabs.

    \sa tabPositions()
*/
void QTextBlockFormat::setTabPositions(const QList<QTextOption::Tab> &tabs)
{
    QList<QVariant> list;
    list.reserve(tabs.count());
    QList<QTextOption::Tab>::ConstIterator iter = tabs.constBegin();
    while (iter != tabs.constEnd()) {
        QVariant v;
        v.setValue<QTextOption::Tab>(*iter);
        list.append(v);
        ++iter;
    }
    setProperty(TabPositions, list);
}

/*!
    \since 4.4
    Returns a list of tab positions defined for the text block.

    \sa setTabPositions()
*/
QList<QTextOption::Tab> QTextBlockFormat::tabPositions() const
{
    QVariant variant = property(TabPositions);
    if(variant.isNull())
        return QList<QTextOption::Tab>();
    QList<QTextOption::Tab> answer;
    QList<QVariant> variantsList = qvariant_cast<QList<QVariant> >(variant);
    QList<QVariant>::Iterator iter = variantsList.begin();
    answer.reserve(variantsList.count());
    while(iter != variantsList.end()) {
        answer.append( qvariant_cast<QTextOption::Tab>(*iter));
        ++iter;
    }
    return answer;
}

/*!
    \fn QTextBlockFormat::isValid() const

    Returns \c true if this block format is valid; otherwise returns
    false.
*/

/*!
    \fn void QTextFormat::setLayoutDirection(Qt::LayoutDirection direction)

    Sets the document's layout direction to the specified \a direction.

    \sa layoutDirection()
*/


/*!
    \fn Qt::LayoutDirection QTextFormat::layoutDirection() const

    Returns the document's layout direction.

    \sa setLayoutDirection()
*/


/*!
    \fn void QTextBlockFormat::setAlignment(Qt::Alignment alignment)

    Sets the paragraph's \a alignment.

    \sa alignment()
*/


/*!
    \fn Qt::Alignment QTextBlockFormat::alignment() const

    Returns the paragraph's alignment.

    \sa setAlignment()
*/


/*!
    \fn void QTextBlockFormat::setTopMargin(qreal margin)

    Sets the paragraph's top \a margin.

    \sa topMargin(), setBottomMargin(), setLeftMargin(), setRightMargin()
*/


/*!
    \fn qreal QTextBlockFormat::topMargin() const

    Returns the paragraph's top margin.

    \sa setTopMargin(), bottomMargin()
*/


/*!
    \fn void QTextBlockFormat::setBottomMargin(qreal margin)

    Sets the paragraph's bottom \a margin.

    \sa bottomMargin(), setTopMargin(), setLeftMargin(), setRightMargin()
*/


/*!
    \fn qreal QTextBlockFormat::bottomMargin() const

    Returns the paragraph's bottom margin.

    \sa setBottomMargin(), topMargin()
*/


/*!
    \fn void QTextBlockFormat::setLeftMargin(qreal margin)

    Sets the paragraph's left \a margin. Indentation can be applied separately
    with setIndent().

    \sa leftMargin(), setRightMargin(), setTopMargin(), setBottomMargin()
*/


/*!
    \fn qreal QTextBlockFormat::leftMargin() const

    Returns the paragraph's left margin.

    \sa setLeftMargin(), rightMargin(), indent()
*/


/*!
    \fn void QTextBlockFormat::setRightMargin(qreal margin)

    Sets the paragraph's right \a margin.

    \sa rightMargin(), setLeftMargin(), setTopMargin(), setBottomMargin()
*/


/*!
    \fn qreal QTextBlockFormat::rightMargin() const

    Returns the paragraph's right margin.

    \sa setRightMargin(), leftMargin()
*/


/*!
    \fn void QTextBlockFormat::setTextIndent(qreal indent)

    Sets the \a indent for the first line in the block. This allows the first
    line of a paragraph to be indented differently to the other lines,
    enhancing the readability of the text.

    \sa textIndent(), setLeftMargin(), setRightMargin(), setTopMargin(), setBottomMargin()
*/


/*!
    \fn qreal QTextBlockFormat::textIndent() const

    Returns the paragraph's text indent.

    \sa setTextIndent()
*/


/*!
    \fn void QTextBlockFormat::setIndent(int indentation)

    Sets the paragraph's \a indentation. Margins are set independently of
    indentation with setLeftMargin() and setTextIndent().
    The \a indentation is an integer that is multiplied with the document-wide
    standard indent, resulting in the actual indent of the paragraph.

    \sa indent(), QTextDocument::indentWidth()
*/


/*!
    \fn int QTextBlockFormat::indent() const

    Returns the paragraph's indent.

    \sa setIndent()
*/


/*!
    \fn void QTextBlockFormat::setHeadingLevel(int level)
    \since 5.12

    Sets the paragraph's heading \a level, where 1 is the highest-level heading
    type (usually with the largest possible heading font size), and increasing
    values are progressively deeper into the document (and usually with smaller
    font sizes). For example when reading an HTML H1 tag, the heading level is
    set to 1. Setting the heading level does not automatically change the font
    size; however QTextDocumentFragment::fromHtml() sets both the heading level
    and the font size simultaneously.

    If the paragraph is not a heading, the level should be set to 0 (the default).

    \sa headingLevel()
*/


/*!
    \fn int QTextBlockFormat::headingLevel() const
    \since 5.12

    Returns the paragraph's heading level if it is a heading, or 0 if not.

    \sa setHeadingLevel()
*/


/*!
    \fn void QTextBlockFormat::setMarker(MarkerType marker)
    \since 5.14

    Sets the type of adornment that should be rendered alongside the paragraph to \a marker.
    For example, a list item can be adorned with a checkbox, either checked
    or unchecked, as a replacement for its bullet. The default is \c NoMarker.

    \sa marker()
*/


/*!
    \fn MarkerType QTextBlockFormat::marker() const
    \since 5.14

    Returns the paragraph's marker if one has been set, or \c NoMarker if not.

    \sa setMarker()
*/


/*!
    \since 5.14
    \enum QTextBlockFormat::MarkerType

    This enum describes the types of markers a list item can have.
    If a list item (a paragraph for which \l QTextBlock::textList() returns the list)
    has a marker, it is rendered instead of the normal bullet.
    In this way, checkable list items can be mixed with plain list items in the
    same list, overriding the type of bullet specified by the
    \l QTextListFormat::style() for the entire list.

    \value NoMarker This is the default: the list item's bullet will be shown.
    \value Unchecked Instead of the list item's bullet, an unchecked checkbox will be shown.
    \value Checked Instead of the list item's bullet, a checked checkbox will be shown.

    In the future, this may be extended to specify other types of paragraph
    decorations.

    \sa QTextListFormat::style()
*/


/*!
    \fn void QTextBlockFormat::setLineHeight(qreal height, int heightType)
    \since 4.8

    Sets the line height for the paragraph to the value given by \a height
    which is dependent on \a heightType in the way described by the
    LineHeightTypes enum.

    \sa LineHeightTypes, lineHeight(), lineHeightType()
*/


/*!
    \fn qreal QTextBlockFormat::lineHeight(qreal scriptLineHeight, qreal scaling) const
    \since 4.8

    Returns the height of the lines in the paragraph based on the height of the
    script line given by \a scriptLineHeight and the specified \a scaling
    factor.

    The value that is returned is also dependent on the given LineHeightType of
    the paragraph as well as the LineHeight setting that has been set for the
    paragraph.

    The scaling is needed for heights that include a fixed number of pixels, to
    scale them appropriately for printing.

    \sa LineHeightTypes, setLineHeight(), lineHeightType()
*/


/*!
    \fn qreal QTextBlockFormat::lineHeight() const
    \since 4.8

    This returns the LineHeight property for the paragraph.

    \sa LineHeightTypes, setLineHeight(), lineHeightType()
*/


/*!
    \fn qreal QTextBlockFormat::lineHeightType() const
    \since 4.8

    This returns the LineHeightType property of the paragraph.

    \sa LineHeightTypes, setLineHeight(), lineHeight()
*/


/*!
    \fn void QTextBlockFormat::setNonBreakableLines(bool b)

    If \a b is true, the lines in the paragraph are treated as
    non-breakable; otherwise they are breakable.

    \sa nonBreakableLines()
*/


/*!
    \fn bool QTextBlockFormat::nonBreakableLines() const

    Returns \c true if the lines in the paragraph are non-breakable;
    otherwise returns \c false.

    \sa setNonBreakableLines()
*/

/*!
    \fn QTextFormat::PageBreakFlags QTextBlockFormat::pageBreakPolicy() const
    \since 4.2

    Returns the currently set page break policy for the paragraph. The default is
    QTextFormat::PageBreak_Auto.

    \sa setPageBreakPolicy()
*/

/*!
    \fn void QTextBlockFormat::setPageBreakPolicy(PageBreakFlags policy)
    \since 4.2

    Sets the page break policy for the paragraph to \a policy.

    \sa pageBreakPolicy()
*/

/*!
    \class QTextListFormat
    \reentrant

    \brief The QTextListFormat class provides formatting information for
    lists in a QTextDocument.
    \inmodule QtGui

    \ingroup richtext-processing
    \ingroup shared

    A list is composed of one or more items, represented as text blocks.
    The list's format specifies the appearance of items in the list.
    In particular, it determines the indentation and the style of each item.

    The indentation of the items is an integer value that causes each item to
    be offset from the left margin by a certain amount. This value is read with
    indent() and set with setIndent().

    The style used to decorate each item is set with setStyle() and can be read
    with the style() function. The style controls the type of bullet points and
    numbering scheme used for items in the list. Note that lists that use the
    decimal numbering scheme begin counting at 1 rather than 0.

    Style properties can be set to further configure the appearance of list
    items; for example, the ListNumberPrefix and ListNumberSuffix properties
    can be used to customize the numbers used in an ordered list so that they
    appear as (1), (2), (3), etc.:

    \snippet textdocument-listitemstyles/mainwindow.cpp add a styled, ordered list

    \sa QTextList
*/

/*!
    \enum QTextListFormat::Style

    This enum describes the symbols used to decorate list items:

    \value ListDisc        a filled circle
    \value ListCircle      an empty circle
    \value ListSquare      a filled square
    \value ListDecimal     decimal values in ascending order
    \value ListLowerAlpha  lower case Latin characters in alphabetical order
    \value ListUpperAlpha  upper case Latin characters in alphabetical order
    \value ListLowerRoman  lower case roman numerals (supports up to 4999 items only)
    \value ListUpperRoman  upper case roman numerals (supports up to 4999 items only)
    \omitvalue ListStyleUndefined
*/

/*!
    \fn QTextListFormat::QTextListFormat()

    Constructs a new list format object.
*/
QTextListFormat::QTextListFormat()
    : QTextFormat(ListFormat)
{
    setIndent(1);
}

/*!
    \internal
    \fn QTextListFormat::QTextListFormat(const QTextFormat &other)

    Creates a new list format with the same attributes as the \a given
    text format.
*/
QTextListFormat::QTextListFormat(const QTextFormat &fmt)
 : QTextFormat(fmt)
{
}

/*!
    \fn bool QTextListFormat::isValid() const

    Returns \c true if this list format is valid; otherwise
    returns \c false.
*/

/*!
    \fn void QTextListFormat::setStyle(Style style)

    Sets the list format's \a style.

    \sa style(), Style
*/

/*!
    \fn Style QTextListFormat::style() const

    Returns the list format's style.

    \sa setStyle(), Style
*/


/*!
    \fn void QTextListFormat::setIndent(int indentation)

    Sets the list format's \a indentation.
    The indentation is multiplied by the QTextDocument::indentWidth
    property to get the effective indent in pixels.

    \sa indent()
*/


/*!
    \fn int QTextListFormat::indent() const

    Returns the list format's indentation.
    The indentation is multiplied by the QTextDocument::indentWidth
    property to get the effective indent in pixels.

    \sa setIndent()
*/

/*!
    \fn void QTextListFormat::setNumberPrefix(const QString &numberPrefix)
    \since 4.8

    Sets the list format's number prefix to the string specified by
    \a numberPrefix. This can be used with all sorted list types. It does not
    have any effect on unsorted list types.

    The default prefix is an empty string.

    \sa numberPrefix()
*/

/*!
    \fn int QTextListFormat::numberPrefix() const
    \since 4.8

    Returns the list format's number prefix.

    \sa setNumberPrefix()
*/

/*!
    \fn void QTextListFormat::setNumberSuffix(const QString &numberSuffix)
    \since 4.8

    Sets the list format's number suffix to the string specified by
    \a numberSuffix. This can be used with all sorted list types. It does not
    have any effect on unsorted list types.

    The default suffix is ".".

    \sa numberSuffix()
*/

/*!
    \fn int QTextListFormat::numberSuffix() const
    \since 4.8

    Returns the list format's number suffix.

    \sa setNumberSuffix()
*/

/*!
    \class QTextFrameFormat
    \reentrant

    \brief The QTextFrameFormat class provides formatting information for
    frames in a QTextDocument.
    \inmodule QtGui

    \ingroup richtext-processing
    \ingroup shared

    A text frame groups together one or more blocks of text, providing a layer
    of structure larger than the paragraph. The format of a frame specifies
    how it is rendered and positioned on the screen. It does not directly
    specify the behavior of the text formatting within, but provides
    constraints on the layout of its children.

    The frame format defines the width() and height() of the frame on the
    screen. Each frame can have a border() that surrounds its contents with
    a rectangular box. The border is surrounded by a margin() around the frame,
    and the contents of the frame are kept separate from the border by the
    frame's padding(). This scheme is similar to the box model used by Cascading
    Style Sheets for HTML pages.

    \image qtextframe-style.png

    The position() of a frame is set using setPosition() and determines how it
    is located relative to the surrounding text.

    The validity of a QTextFrameFormat object can be determined with the
    isValid() function.

    \sa QTextFrame, QTextBlockFormat
*/

/*!
    \enum QTextFrameFormat::Position

    This enum describes how a frame is located relative to the surrounding text.

    \value InFlow
    \value FloatLeft
    \value FloatRight

    \sa position(), CssFloat
*/

/*!
    \enum QTextFrameFormat::BorderStyle
    \since 4.3

    This enum describes different border styles for the text frame.

    \value BorderStyle_None
    \value BorderStyle_Dotted
    \value BorderStyle_Dashed
    \value BorderStyle_Solid
    \value BorderStyle_Double
    \value BorderStyle_DotDash
    \value BorderStyle_DotDotDash
    \value BorderStyle_Groove
    \value BorderStyle_Ridge
    \value BorderStyle_Inset
    \value BorderStyle_Outset

    \sa borderStyle(), FrameBorderStyle
*/

/*!
    \fn QTextFrameFormat::QTextFrameFormat()

    Constructs a text frame format object with the default properties.
*/
QTextFrameFormat::QTextFrameFormat() : QTextFormat(FrameFormat)
{
    setBorderStyle(BorderStyle_Outset);
    setBorderBrush(Qt::darkGray);
}

/*!
    \internal
    \fn QTextFrameFormat::QTextFrameFormat(const QTextFormat &other)

    Creates a new frame format with the same attributes as the \a given
    text format.
*/
QTextFrameFormat::QTextFrameFormat(const QTextFormat &fmt)
 : QTextFormat(fmt)
{
}

/*!
    \fn bool QTextFrameFormat::isValid() const

    Returns \c true if the format description is valid; otherwise returns \c false.
*/

/*!
    \fn void QTextFrameFormat::setPosition(Position policy)

    Sets the \a policy for positioning frames with this frame format.

*/

/*!
    \fn Position QTextFrameFormat::position() const

    Returns the positioning policy for frames with this frame format.
*/

/*!
    \fn void QTextFrameFormat::setBorder(qreal width)

    Sets the \a width (in pixels) of the frame's border.
*/

/*!
    \fn qreal QTextFrameFormat::border() const

    Returns the width of the border in pixels.
*/

/*!
    \fn void QTextFrameFormat::setBorderBrush(const QBrush &brush)
    \since 4.3

    Sets the \a brush used for the frame's border.
*/

/*!
    \fn QBrush QTextFrameFormat::borderBrush() const
    \since 4.3

    Returns the brush used for the frame's border.
*/

/*!
    \fn void QTextFrameFormat::setBorderStyle(BorderStyle style)
    \since 4.3

    Sets the \a style of the frame's border.
*/

/*!
    \fn BorderStyle QTextFrameFormat::borderStyle() const
    \since 4.3

    Returns the style of the frame's border.
*/

/*!
    \fn void QTextFrameFormat::setMargin(qreal margin)

    Sets the frame's \a margin in pixels.
    This method also sets the left, right, top and bottom margins
    of the frame to the same value. The individual margins override
    the general margin.
*/
void QTextFrameFormat::setMargin(qreal amargin)
{
    setProperty(FrameMargin, amargin);
    setProperty(FrameTopMargin, amargin);
    setProperty(FrameBottomMargin, amargin);
    setProperty(FrameLeftMargin, amargin);
    setProperty(FrameRightMargin, amargin);
}


/*!
    \fn qreal QTextFrameFormat::margin() const

    Returns the width of the frame's external margin in pixels.
*/

/*!
    \fn void QTextFrameFormat::setTopMargin(qreal margin)
    \since 4.3

    Sets the frame's top \a margin in pixels.
*/

/*!
    \fn qreal QTextFrameFormat::topMargin() const
    \since 4.3

    Returns the width of the frame's top margin in pixels.
*/
qreal QTextFrameFormat::topMargin() const
{
    if (!hasProperty(FrameTopMargin))
        return margin();
    return doubleProperty(FrameTopMargin);
}

/*!
    \fn void QTextFrameFormat::setBottomMargin(qreal margin)
    \since 4.3

    Sets the frame's bottom \a margin in pixels.
*/

/*!
    \fn qreal QTextFrameFormat::bottomMargin() const
    \since 4.3

    Returns the width of the frame's bottom margin in pixels.
*/
qreal QTextFrameFormat::bottomMargin() const
{
    if (!hasProperty(FrameBottomMargin))
        return margin();
    return doubleProperty(FrameBottomMargin);
}

/*!
    \fn void QTextFrameFormat::setLeftMargin(qreal margin)
    \since 4.3

    Sets the frame's left \a margin in pixels.
*/

/*!
    \fn qreal QTextFrameFormat::leftMargin() const
    \since 4.3

    Returns the width of the frame's left margin in pixels.
*/
qreal QTextFrameFormat::leftMargin() const
{
    if (!hasProperty(FrameLeftMargin))
        return margin();
    return doubleProperty(FrameLeftMargin);
}

/*!
    \fn void QTextFrameFormat::setRightMargin(qreal margin)
    \since 4.3

    Sets the frame's right \a margin in pixels.
*/

/*!
    \fn qreal QTextFrameFormat::rightMargin() const
    \since 4.3

    Returns the width of the frame's right margin in pixels.
*/
qreal QTextFrameFormat::rightMargin() const
{
    if (!hasProperty(FrameRightMargin))
        return margin();
    return doubleProperty(FrameRightMargin);
}

/*!
    \fn void QTextFrameFormat::setPadding(qreal width)

    Sets the \a width of the frame's internal padding in pixels.
*/

/*!
    \fn qreal QTextFrameFormat::padding() const

    Returns the width of the frame's internal padding in pixels.
*/

/*!
    \fn void QTextFrameFormat::setWidth(const QTextLength &width)

    Sets the frame's border rectangle's \a width.

    \sa QTextLength
*/

/*!
    \fn void QTextFrameFormat::setWidth(qreal width)
    \overload

    Convenience method that sets the width of the frame's border
    rectangle's width to the specified fixed \a width.
*/

/*!
    \fn QTextFormat::PageBreakFlags QTextFrameFormat::pageBreakPolicy() const
    \since 4.2

    Returns the currently set page break policy for the frame/table. The default is
    QTextFormat::PageBreak_Auto.

    \sa setPageBreakPolicy()
*/

/*!
    \fn void QTextFrameFormat::setPageBreakPolicy(PageBreakFlags policy)
    \since 4.2

    Sets the page break policy for the frame/table to \a policy.

    \sa pageBreakPolicy()
*/

/*!
    \fn QTextLength QTextFrameFormat::width() const

    Returns the width of the frame's border rectangle.

    \sa QTextLength
*/

/*!
    \fn void QTextFrameFormat::setHeight(const QTextLength &height)

    Sets the frame's \a height.
*/

/*!
    \fn void QTextFrameFormat::setHeight(qreal height)
    \overload

    Sets the frame's \a height.
*/

/*!
    \fn qreal QTextFrameFormat::height() const

    Returns the height of the frame's border rectangle.
*/

/*!
    \class QTextTableFormat
    \reentrant

    \brief The QTextTableFormat class provides formatting information for
    tables in a QTextDocument.
    \inmodule QtGui

    \ingroup richtext-processing
    \ingroup shared

    A table is a group of cells ordered into rows and columns. Each table
    contains at least one row and one column. Each cell contains a block.
    Tables in rich text documents are formatted using the properties
    defined in this class.

    Tables are horizontally justified within their parent frame according to the
    table's alignment. This can be read with the alignment() function and set
    with setAlignment().

    Cells within the table are separated by cell spacing. The number of pixels
    between cells is set with setCellSpacing() and read with cellSpacing().
    The contents of each cell is surrounded by cell padding. The number of pixels
    between each cell edge and its contents is set with setCellPadding() and read
    with cellPadding().

    \image qtexttableformat-cell.png

    The table's background color can be read with the background() function,
    and can be specified with setBackground(). The background color of each
    cell can be set independently, and will control the color of the cell within
    the padded area.

    The table format also provides a way to constrain the widths of the columns
    in the table. Columns can be assigned a fixed width, a variable width, or
    a percentage of the available width (see QTextLength). The columns() function
    returns the number of columns with constraints, and the
    columnWidthConstraints() function returns the constraints defined for the
    table. These quantities can also be set by calling setColumnWidthConstraints()
    with a vector containing new constraints. If no constraints are
    required, clearColumnWidthConstraints() can be used to remove them.

    \sa QTextTable, QTextTableCell, QTextLength
*/

/*!
    \fn QTextTableFormat::QTextTableFormat()

    Constructs a new table format object.
*/
QTextTableFormat::QTextTableFormat()
 : QTextFrameFormat()
{
    setObjectType(TableObject);
    setCellSpacing(2);
    setBorder(1);
}

/*!
    \internal
    \fn QTextTableFormat::QTextTableFormat(const QTextFormat &other)

    Creates a new table format with the same attributes as the \a given
    text format.
*/
QTextTableFormat::QTextTableFormat(const QTextFormat &fmt)
 : QTextFrameFormat(fmt)
{
}

/*!
    \fn bool QTextTableFormat::isValid() const

    Returns \c true if this table format is valid; otherwise
    returns \c false.
*/


/*!
    \fn int QTextTableFormat::columns() const

    Returns the number of columns specified by the table format.
*/


/*!
    \internal
    \fn void QTextTableFormat::setColumns(int columns)

    Sets the number of \a columns required by the table format.

    \sa columns()
*/

/*!
    \fn void QTextTableFormat::clearColumnWidthConstraints()

    Clears the column width constraints for the table.

    \sa columnWidthConstraints(), setColumnWidthConstraints()
*/

/*!
    \fn void QTextTableFormat::setColumnWidthConstraints(const QVector<QTextLength> &constraints)

    Sets the column width \a constraints for the table.

    \sa columnWidthConstraints(), clearColumnWidthConstraints()
*/

/*!
    \fn QVector<QTextLength> QTextTableFormat::columnWidthConstraints() const

    Returns a list of constraints used by this table format to control the
    appearance of columns in a table.

    \sa setColumnWidthConstraints()
*/

/*!
    \fn qreal QTextTableFormat::cellSpacing() const

    Returns the table's cell spacing. This describes the distance between
    adjacent cells.
*/

/*!
    \fn void QTextTableFormat::setCellSpacing(qreal spacing)

    Sets the cell \a spacing for the table. This determines the distance
    between adjacent cells.

    This property will be ignored if \l borderCollapse is enabled.
*/

/*!
    \fn qreal QTextTableFormat::cellPadding() const

    Returns the table's cell padding. This describes the distance between
    the border of a cell and its contents.
*/

/*!
    \fn void QTextTableFormat::setCellPadding(qreal padding)

    Sets the cell \a padding for the table. This determines the distance
    between the border of a cell and its contents.
*/

/*!
    \fn void QTextTableFormat::setAlignment(Qt::Alignment alignment)

    Sets the table's \a alignment.

    \sa alignment()
*/

/*!
    \fn Qt::Alignment QTextTableFormat::alignment() const

    Returns the table's alignment.

    \sa setAlignment()
*/

/*!
    \fn void QTextTableFormat::setHeaderRowCount(int count)
    \since 4.2

    Declares the first \a count rows of the table as table header.
    The table header rows get repeated when a table is broken
    across a page boundary.
*/

/*!
    \fn int QTextTableFormat::headerRowCount() const
    \since 4.2

    Returns the number of rows in the table that define the header.

    \sa setHeaderRowCount()
*/

/*!
    \fn void QTextTableFormat::setBorderCollapse(bool borderCollapse)
    \since 5.14

    Enabling \a borderCollapse will have the following implications:
    \list
    \li The borders and grid of the table will be rendered following the
        CSS table \c border-collapse: \c collapse rules
    \li Setting the \c border property to a minimum value of \c 1 will render a
        one pixel solid inner table grid using the \l borderBrush property and an
        outer border as specified
    \li The various border style properties of \l QTextTableCellFormat can be used to
        customize the grid and have precedence over the border and grid of the table
    \li The \l cellSpacing property will be ignored
    \li For print pagination:
        \list
        \li Columns continued on a page will not have their top cell border rendered
        \li Repeated header rows will always have their bottom cell border rendered
        \endlist
    \endlist

    With borderCollapse disabled, cell borders can still be styled
    using QTextTableCellFormat but styling will be applied only within
    the cell's frame, which is probably not very useful in practice.

    \sa setBorder(), setBorderBrush(), setBorderStyle()
    \sa QTextTableCellFormat
*/

/*!
    \fn bool QTextTableFormat::borderCollapse() const
    \since 5.14

    Returns true if borderCollapse is enabled.

    \sa setBorderCollapse()
*/

/*!
    \fn void QTextFormat::setBackground(const QBrush &brush)

    Sets the brush use to paint the document's background to the
    \a brush specified.

    \sa background(), clearBackground(), setForeground()
*/

/*!
    \fn QColor QTextFormat::background() const

    Returns the brush used to paint the document's background.

    \sa setBackground(), clearBackground(), foreground()
*/

/*!
    \fn void QTextFormat::clearBackground()

    Clears the brush used to paint the document's background. The default
    brush will be used.

    \sa background(), setBackground(), clearForeground()
*/


/*!
    \class QTextImageFormat
    \reentrant

    \brief The QTextImageFormat class provides formatting information for
    images in a QTextDocument.
    \inmodule QtGui

    \ingroup richtext-processing
    \ingroup shared

    Inline images are represented by a Unicode value U+FFFC (OBJECT
    REPLACEMENT CHARACTER) which has an associated QTextImageFormat. The
    image format specifies a name with setName() that is used to
    locate the image. The size of the rectangle that the image will
    occupy is specified in pixels using setWidth() and setHeight().
    The desired image quality may be set with setQuality().

    Images can be supplied in any format for which Qt has an image
    reader, so SVG drawings can be included alongside PNG, TIFF and
    other bitmap formats.

    \sa QImage, QImageReader
*/

/*!
    \fn QTextImageFormat::QTextImageFormat()

    Creates a new image format object.
*/
QTextImageFormat::QTextImageFormat() : QTextCharFormat() { setObjectType(ImageObject); }

/*!
    \internal
    \fn QTextImageFormat::QTextImageFormat(const QTextFormat &other)

    Creates a new image format with the same attributes as the \a given
    text format.
*/
QTextImageFormat::QTextImageFormat(const QTextFormat &fmt)
 : QTextCharFormat(fmt)
{
}

/*!
    \fn bool QTextImageFormat::isValid() const

    Returns \c true if this image format is valid; otherwise returns \c false.
*/


/*!
    \fn void QTextImageFormat::setName(const QString &name)

    Sets the \a name of the image. The \a name is used to locate the image
    in the application's resources.

    \sa name()
*/


/*!
    \fn QString QTextImageFormat::name() const

    Returns the name of the image. The name refers to an entry in the
    application's resources file.

    \sa setName()
*/

/*!
    \fn void QTextImageFormat::setWidth(qreal width)

    Sets the \a width of the rectangle occupied by the image.

    \sa width(), setHeight()
*/


/*!
    \fn qreal QTextImageFormat::width() const

    Returns the width of the rectangle occupied by the image.

    \sa height(), setWidth()
*/


/*!
    \fn void QTextImageFormat::setHeight(qreal height)

    Sets the \a height of the rectangle occupied by the image.

    \sa height(), setWidth()
*/


/*!
    \fn qreal QTextImageFormat::height() const

    Returns the height of the rectangle occupied by the image.

    \sa width(), setHeight()
*/

/*!
    \fn void QTextImageFormat::setQuality(int quality = 100)
    \since 5.12

    Sets the quality that should be used by exporters when exporting the image. QTextDocumentWriter
    will export jpg images with the \a quality set here when exporting to ODF files if \a quality is
    set to a value between 0 and 100. Or it will export png images if \a quality is set to 100
    (default) or greater.

    \sa quality()
*/


/*!
    \fn qreal QTextImageFormat::quality() const
    \since 5.12

    Returns the value set by setQuality().

    \sa setQuality()
*/

/*!
    \fn void QTextCharFormat::setFontCapitalization(QFont::Capitalization capitalization)
    \since 4.4

    Sets the capitalization of the text that apppears in this font to \a capitalization.

    A font's capitalization makes the text appear in the selected capitalization mode.

    \sa fontCapitalization()
*/

/*!
    \fn Capitalization QTextCharFormat::fontCapitalization() const
    \since 4.4

    Returns the current capitalization type of the font.
*/

/*!
    \fn void QTextCharFormat::setFontLetterSpacingType(QFont::SpacingType letterSpacingType)
    \since 5.0

    Sets the letter spacing type of this format to \a letterSpacingType.

    \sa fontLetterSpacingType()
    \sa setFontLetterSpacing()
    \sa fontLetterSpacing()
*/

/*!
    \fn QFont::SpacingType QTextCharFormat::fontLetterSpacingType() const
    \since 5.0

    Returns the letter spacing type of this format..

    \sa setFontLetterSpacingType()
    \sa setFontLetterSpacing()
    \sa fontLetterSpacing()
*/

/*!
    \fn void QTextCharFormat::setFontLetterSpacing(qreal spacing)
    \since 4.4

    Sets the letter spacing of this format to the given \a spacing. The meaning of the value
    depends on the font letter spacing type.

    For percentage spacing a value of 100 indicates default spacing; a value of 200 doubles the
    amount of space a letter takes.

    \sa fontLetterSpacing()
    \sa setFontLetterSpacingType()
    \sa fontLetterSpacingType()
*/

/*!
    \fn qreal QTextCharFormat::fontLetterSpacing() const
    \since 4.4

    Returns the current letter spacing.

    \sa setFontLetterSpacing()
    \sa setFontLetterSpacingType()
    \sa fontLetterSpacingType()
*/

/*!
    \fn void QTextCharFormat::setFontWordSpacing(qreal spacing)
    \since 4.4

    Sets the word spacing of this format to the given \a spacing, in pixels.

    \sa fontWordSpacing()
*/

/*!
    \fn qreal QTextCharFormat::fontWordSpacing() const
    \since 4.4

    Returns the current word spacing value.
*/

/*!
    \fn void QTextCharFormat::setFontStretch(int factor)
    \since 5.0

    Sets the stretch factor for the font to \a factor.

    The stretch factor changes the width of all characters in the font by factor percent. For example, setting \a factor to 150 results in all characters in the font being 1.5 times (ie. 150%) wider. The default stretch factor is 100. The minimum stretch factor is 1, and the maximum stretch factor is 4000.

    The stretch factor is only applied to outline fonts. The stretch factor is ignored for bitmap fonts.

    \sa fontStretch()
*/

/*!
    \fn int QTextCharFormat::fontStretch() const
    \since 5.0

    Returns the current font stretching.
    \sa setFontStretch()
*/

/*!
   \fn qreal QTextTableCellFormat::topPadding() const
    \since 4.4

   Gets the top padding of the table cell.

   \sa setTopPadding(), leftPadding(), rightPadding(), bottomPadding()
*/

/*!
   \fn qreal QTextTableCellFormat::bottomPadding() const
    \since 4.4

   Gets the bottom padding of the table cell.

   \sa setBottomPadding(), leftPadding(), rightPadding(), topPadding()
*/

/*!
   \fn qreal QTextTableCellFormat::leftPadding() const
    \since 4.4

   Gets the left padding of the table cell.

   \sa setLeftPadding(), rightPadding(), topPadding(), bottomPadding()
*/

/*!
   \fn qreal QTextTableCellFormat::rightPadding() const
    \since 4.4

   Gets the right padding of the table cell.

   \sa setRightPadding(), leftPadding(), topPadding(), bottomPadding()
*/

/*!
   \fn void QTextTableCellFormat::setTopPadding(qreal padding)
    \since 4.4

   Sets the top \a padding of the table cell.

   \sa topPadding(), setLeftPadding(), setRightPadding(), setBottomPadding()
*/

/*!
   \fn void QTextTableCellFormat::setBottomPadding(qreal padding)
    \since 4.4

   Sets the bottom \a padding of the table cell.

   \sa bottomPadding(), setLeftPadding(), setRightPadding(), setTopPadding()
*/

/*!
   \fn void QTextTableCellFormat::setLeftPadding(qreal padding)
    \since 4.4

   Sets the left \a padding of the table cell.

   \sa leftPadding(), setRightPadding(), setTopPadding(), setBottomPadding()
*/

/*!
   \fn void QTextTableCellFormat::setRightPadding(qreal padding)
    \since 4.4

   Sets the right \a padding of the table cell.

   \sa rightPadding(), setLeftPadding(), setTopPadding(), setBottomPadding()
*/

/*!
   \fn void QTextTableCellFormat::setPadding(qreal padding)
    \since 4.4

   Sets the left, right, top, and bottom \a padding of the table cell.

   \sa setLeftPadding(), setRightPadding(), setTopPadding(), setBottomPadding()
*/

/*!
   \fn void QTextTableCellFormat::setTopBorder(qreal width)
   \since 5.14

   Sets the top border \a width of the table cell.

   \sa QTextTableFormat::setBorderCollapse
*/

/*!
   \fn qreal QTextTableCellFormat::topBorder() const
   \since 5.14

   Returns the top border width of the table cell.
*/

/*!
   \fn void QTextTableCellFormat::setBottomBorder(qreal width)
   \since 5.14

   Sets the bottom border \a width of the table cell.

   \sa QTextTableFormat::setBorderCollapse
*/

/*!
   \fn qreal QTextTableCellFormat::bottomBorder() const
   \since 5.14

   Returns the bottom border width of the table cell.
*/

/*!
   \fn void QTextTableCellFormat::setLeftBorder(qreal width)
   \since 5.14

   Sets the left border \a width of the table cell.

   \sa QTextTableFormat::setBorderCollapse
*/

/*!
   \fn qreal QTextTableCellFormat::leftBorder() const
   \since 5.14

   Returns the left border width of the table cell.
*/

/*!
   \fn void QTextTableCellFormat::setRightBorder(qreal width)
   \since 5.14

   Sets the right border \a width of the table cell.

   \sa QTextTableFormat::setBorderCollapse
*/

/*!
   \fn qreal QTextTableCellFormat::rightBorder() const
   \since 5.14

   Returns the right border width of the table cell.
*/

/*!
   \fn void QTextTableCellFormat::setBorder(qreal width)
   \since 5.14

   Sets the left, right, top, and bottom border \a width of the table cell.

   \sa setLeftBorder(), setRightBorder(), setTopBorder(), setBottomBorder()
   \sa QTextTableFormat::setBorderCollapse
*/

/*!
   \fn void QTextTableCellFormat::setTopBorderStyle(QTextFrameFormat::BorderStyle style)
   \since 5.14

   Sets the top border \a style of the table cell.

   \sa QTextTableFormat::setBorderCollapse
*/

/*!
   \fn QTextFrameFormat::BorderStyle QTextTableCellFormat::topBorderStyle() const
   \since 5.14

   Returns the top border style of the table cell.
*/

/*!
   \fn void QTextTableCellFormat::setBottomBorderStyle(QTextFrameFormat::BorderStyle style)
   \since 5.14

   Sets the bottom border \a style of the table cell.

   \sa QTextTableFormat::setBorderCollapse
*/

/*!
   \fn QTextFrameFormat::BorderStyle QTextTableCellFormat::bottomBorderStyle() const
   \since 5.14

   Returns the bottom border style of the table cell.
*/

/*!
   \fn void QTextTableCellFormat::setLeftBorderStyle(QTextFrameFormat::BorderStyle style)
   \since 5.14

   Sets the left border \a style of the table cell.

   \sa QTextTableFormat::setBorderCollapse
*/

/*!
   \fn QTextFrameFormat::BorderStyle QTextTableCellFormat::leftBorderStyle() const
   \since 5.14

   Returns the left border style of the table cell.
*/

/*!
   \fn void QTextTableCellFormat::setRightBorderStyle(QTextFrameFormat::BorderStyle style)
   \since 5.14

   Sets the right border \a style of the table cell.

   \sa QTextTableFormat::setBorderCollapse
*/

/*!
   \fn QTextFrameFormat::BorderStyle QTextTableCellFormat::rightBorderStyle() const
   \since 5.14

   Returns the right border style of the table cell.
*/

/*!
   \fn void QTextTableCellFormat::setBorderStyle(QTextFrameFormat::BorderStyle style)
   \since 5.14

   Sets the left, right, top, and bottom border \a style of the table cell.

   \sa setLeftBorderStyle(), setRightBorderStyle(), setTopBorderStyle(), setBottomBorderStyle()
   \sa QTextTableFormat::setBorderCollapse
*/

/*!
   \fn void QTextTableCellFormat::setTopBorderBrush(const QBrush &brush)
   \since 5.14

   Sets the top border \a brush of the table cell.

   \sa QTextTableFormat::setBorderCollapse
*/

/*!
   \fn QBrush QTextTableCellFormat::topBorderBrush() const
   \since 5.14

   Returns the top border brush of the table cell.
*/

/*!
   \fn void QTextTableCellFormat::setBottomBorderBrush(const QBrush &brush)
   \since 5.14

   Sets the bottom border \a brush of the table cell.

   \sa QTextTableFormat::setBorderCollapse
*/

/*!
   \fn QBrush QTextTableCellFormat::bottomBorderBrush() const
   \since 5.14

   Returns the bottom border brush of the table cell.
*/

/*!
   \fn void QTextTableCellFormat::setLeftBorderBrush(const QBrush &brush)
   \since 5.14

   Sets the left border \a brush of the table cell.

   \sa QTextTableFormat::setBorderCollapse
*/

/*!
   \fn QBrush QTextTableCellFormat::leftBorderBrush() const
   \since 5.14

   Returns the left border brush of the table cell.
*/

/*!
   \fn void QTextTableCellFormat::setRightBorderBrush(const QBrush &brush)
   \since 5.14

   Sets the right border \a brush of the table cell.

   \sa QTextTableFormat::setBorderCollapse
*/

/*!
   \fn QBrush QTextTableCellFormat::rightBorderBrush() const
   \since 5.14

   Returns the right border brush of the table cell.
*/

/*!
   \fn void QTextTableCellFormat::setBorderBrush(const QBrush &brush)
   \since 5.14

   Sets the left, right, top, and bottom border \a brush of the table cell.

   \sa setLeftBorderBrush(), setRightBorderBrush(), setTopBorderBrush(), setBottomBorderBrush()
   \sa QTextTableFormat::setBorderCollapse
*/

/*!
    \fn bool QTextTableCellFormat::isValid() const
    \since 4.4

    Returns \c true if this table cell format is valid; otherwise returns \c false.
*/

/*!
    \fn QTextTableCellFormat::QTextTableCellFormat()
    \since 4.4

    Constructs a new table cell format object.
*/
QTextTableCellFormat::QTextTableCellFormat()
    : QTextCharFormat()
{
    setObjectType(TableCellObject);
}

/*!
    \internal
    \fn QTextTableCellFormat::QTextTableCellFormat(const QTextFormat &other)

    Creates a new table cell format with the same attributes as the \a given
    text format.
*/
QTextTableCellFormat::QTextTableCellFormat(const QTextFormat &fmt)
    : QTextCharFormat(fmt)
{
}

/*!
    \class QTextTableCellFormat
    \reentrant
    \since 4.4

    \brief The QTextTableCellFormat class provides formatting information for
    table cells in a QTextDocument.
    \inmodule QtGui

    \ingroup richtext-processing
    \ingroup shared

    The table cell format of a table cell in a document specifies the visual
    properties of the table cell.

    The padding properties of a table cell are controlled by setLeftPadding(),
    setRightPadding(), setTopPadding(), and setBottomPadding(). All the paddings
    can be set at once using setPadding().

    \sa QTextFormat, QTextBlockFormat, QTextTableFormat, QTextCharFormat
*/

// ------------------------------------------------------

QTextFormatCollection::~QTextFormatCollection()
{
}

void QTextFormatCollection::clear()
{
    formats.clear();
    objFormats.clear();
    hashes.clear();
}

int QTextFormatCollection::indexForFormat(const QTextFormat &format)
{
    uint hash = getHash(format.d, format.format_type);
    QMultiHash<uint, int>::const_iterator i = hashes.constFind(hash);
    while (i != hashes.constEnd() && i.key() == hash) {
        if (formats.value(i.value()) == format) {
            return i.value();
        }
        ++i;
    }

    int idx = formats.size();
    formats.append(format);

    QT_TRY{
        QTextFormat &f = formats.last();
        if (!f.d)
            f.d = new QTextFormatPrivate;
        f.d->resolveFont(defaultFnt);

        hashes.insert(hash, idx);

    } QT_CATCH(...) {
        formats.pop_back();
        QT_RETHROW;
    }
    return idx;
}

bool QTextFormatCollection::hasFormatCached(const QTextFormat &format) const
{
    uint hash = getHash(format.d, format.format_type);
    QMultiHash<uint, int>::const_iterator i = hashes.constFind(hash);
    while (i != hashes.constEnd() && i.key() == hash) {
        if (formats.value(i.value()) == format) {
            return true;
        }
        ++i;
    }
    return false;
}

int QTextFormatCollection::objectFormatIndex(int objectIndex) const
{
    if (objectIndex == -1)
        return -1;
    return objFormats.at(objectIndex);
}

void QTextFormatCollection::setObjectFormatIndex(int objectIndex, int formatIndex)
{
    objFormats[objectIndex] = formatIndex;
}

int QTextFormatCollection::createObjectIndex(const QTextFormat &f)
{
    const int objectIndex = objFormats.size();
    objFormats.append(indexForFormat(f));
    return objectIndex;
}

QTextFormat QTextFormatCollection::format(int idx) const
{
    if (idx < 0 || idx >= formats.count())
        return QTextFormat();

    return formats.at(idx);
}

void QTextFormatCollection::setDefaultFont(const QFont &f)
{
    defaultFnt = f;
    for (int i = 0; i < formats.count(); ++i)
        if (formats.at(i).d)
            formats[i].d->resolveFont(defaultFnt);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QTextLength &l)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QTextLength(QTextLength::Type(" << l.type() << "))";
    return dbg;
}

QDebug operator<<(QDebug dbg, const QTextFormat &f)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QTextFormat(QTextFormat::FormatType(" << f.type() << "))";
    return dbg;
}

#endif

QT_END_NAMESPACE

#include "moc_qtextformat.cpp"
