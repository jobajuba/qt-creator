/****************************************************************************
**
** Copyright (C) 2016 Thorben Kroeger <thorbenkroeger@gmail.com>.
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

#pragma once

#include "../utils_global.h"

#include <QBrush> // QGradientStops
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QSettings)
QT_FORWARD_DECLARE_CLASS(QPalette)

namespace Utils {

class ThemePrivate;

class QTCREATOR_UTILS_EXPORT Theme : public QObject
{
    Q_OBJECT
public:
    Theme(const QString &id, QObject *parent = nullptr);
    ~Theme() override;

    enum Color {
        BackgroundColorAlternate,
        BackgroundColorDark,
        BackgroundColorHover,
        BackgroundColorNormal,
        BackgroundColorSelected,
        BackgroundColorDisabled,
        BadgeLabelBackgroundColorChecked,
        BadgeLabelBackgroundColorUnchecked,
        BadgeLabelTextColorChecked,
        BadgeLabelTextColorUnchecked,
        CanceledSearchTextColor,
        ComboBoxArrowColor,
        ComboBoxArrowColorDisabled,
        ComboBoxTextColor,
        DetailsButtonBackgroundColorHover,
        DetailsWidgetBackgroundColor,
        DockWidgetResizeHandleColor,
        DoubleTabWidget1stSeparatorColor,
        DoubleTabWidget1stTabActiveTextColor,
        DoubleTabWidget1stTabBackgroundColor,
        DoubleTabWidget1stTabInactiveTextColor,
        DoubleTabWidget2ndSeparatorColor,
        DoubleTabWidget2ndTabActiveTextColor,
        DoubleTabWidget2ndTabBackgroundColor,
        DoubleTabWidget2ndTabInactiveTextColor,
        EditorPlaceholderColor,
        FancyToolBarSeparatorColor,
        FancyTabBarBackgroundColor,
        FancyTabBarSelectedBackgroundColor,
        FancyTabWidgetDisabledSelectedTextColor,
        FancyTabWidgetDisabledUnselectedTextColor,
        FancyTabWidgetEnabledSelectedTextColor,
        FancyTabWidgetEnabledUnselectedTextColor,
        FancyToolButtonHoverColor,
        FancyToolButtonSelectedColor,
        FutureProgressBackgroundColor,
        InfoBarBackground,
        InfoBarText, // TODO: Deprecate. Unused.
        MenuBarEmptyAreaBackgroundColor,
        MenuBarItemBackgroundColor,
        MenuBarItemTextColorDisabled,
        MenuBarItemTextColorNormal,
        MenuItemTextColorDisabled,
        MenuItemTextColorNormal,
        MiniProjectTargetSelectorBackgroundColor, // TODO: Deprecate. -> Utils::StyleHelper().baseColor()
        MiniProjectTargetSelectorBorderColor,
        MiniProjectTargetSelectorSummaryBackgroundColor, // TODO: Deprecate. -> Utils::StyleHelper().baseColor()
        MiniProjectTargetSelectorTextColor,
        OutputPaneButtonFlashColor,
        OutputPaneToggleButtonTextColorChecked,
        OutputPaneToggleButtonTextColorUnchecked,
        PanelStatusBarBackgroundColor,
        PanelsWidgetSeparatorLineColor, // TODO: Deprecate. Unused.
        PanelTextColorDark,
        PanelTextColorMid,
        PanelTextColorLight,
        ProgressBarColorError,
        ProgressBarColorFinished,
        ProgressBarColorNormal,
        ProgressBarTitleColor,
        ProgressBarBackgroundColor,
        SplitterColor,
        TextColorDisabled,
        TextColorError,
        TextColorHighlight,
        TextColorHighlightBackground,
        TextColorLink,
        TextColorLinkVisited,
        TextColorNormal,
        ToggleButtonBackgroundColor,
        ToolBarBackgroundColor,
        TreeViewArrowColorNormal,
        TreeViewArrowColorSelected,

        /* Palette for QPalette */

        PaletteWindow,
        PaletteWindowText,
        PaletteBase,
        PaletteAlternateBase,
        PaletteToolTipBase,
        PaletteToolTipText,
        PaletteText,
        PaletteButton,
        PaletteButtonText,
        PaletteBrightText,
        PaletteHighlight,
        PaletteHighlightedText,
        PaletteLink,
        PaletteLinkVisited,

        PaletteLight,
        PaletteMidlight,
        PaletteDark,
        PaletteMid,
        PaletteShadow,

        PaletteWindowDisabled,
        PaletteWindowTextDisabled,
        PaletteBaseDisabled,
        PaletteAlternateBaseDisabled,
        PaletteToolTipBaseDisabled,
        PaletteToolTipTextDisabled,
        PaletteTextDisabled,
        PaletteButtonDisabled,
        PaletteButtonTextDisabled,
        PaletteBrightTextDisabled,
        PaletteHighlightDisabled,
        PaletteHighlightedTextDisabled,
        PaletteLinkDisabled,
        PaletteLinkVisitedDisabled,

        PaletteLightDisabled,
        PaletteMidlightDisabled,
        PaletteDarkDisabled,
        PaletteMidDisabled,
        PaletteShadowDisabled,

        PalettePlaceholderText,
        PalettePlaceholderTextDisabled,

        /* Icons */

        IconsBaseColor,
        IconsDisabledColor,
        IconsInfoColor,
        IconsInfoToolBarColor,
        IconsWarningColor,
        IconsWarningToolBarColor,
        IconsErrorColor,
        IconsErrorToolBarColor,
        IconsRunColor,
        IconsRunToolBarColor,
        IconsStopColor,
        IconsStopToolBarColor,
        IconsInterruptColor,
        IconsInterruptToolBarColor,
        IconsDebugColor,
        IconsNavigationArrowsColor,
        IconsBuildHammerHandleColor,
        IconsBuildHammerHeadColor,
        IconsModeWelcomeActiveColor,
        IconsModeEditActiveColor,
        IconsModeDesignActiveColor,
        IconsModeDebugActiveColor,
        IconsModeProjectActiveColor,
        IconsModeAnalyzeActiveColor,
        IconsModeHelpActiveColor,

        /* Code model Icons */

        IconsCodeModelKeywordColor,
        IconsCodeModelClassColor,
        IconsCodeModelStructColor,
        IconsCodeModelFunctionColor,
        IconsCodeModelVariableColor,
        IconsCodeModelEnumColor,
        IconsCodeModelMacroColor,
        IconsCodeModelAttributeColor,
        IconsCodeModelUniformColor,
        IconsCodeModelVaryingColor,
        IconsCodeModelOverlayBackgroundColor,
        IconsCodeModelOverlayForegroundColor,

        /* Code model text marks */

        CodeModel_Error_TextMarkColor,
        CodeModel_Warning_TextMarkColor,

        /* Output panes */

        OutputPanes_DebugTextColor,
        OutputPanes_ErrorMessageTextColor,
        OutputPanes_MessageOutput,
        OutputPanes_NormalMessageTextColor,
        OutputPanes_StdErrTextColor,
        OutputPanes_StdOutTextColor,
        OutputPanes_WarningMessageTextColor,
        OutputPanes_TestPassTextColor,
        OutputPanes_TestFailTextColor,
        OutputPanes_TestXFailTextColor,
        OutputPanes_TestXPassTextColor,
        OutputPanes_TestSkipTextColor,
        OutputPanes_TestWarnTextColor,
        OutputPanes_TestFatalTextColor,
        OutputPanes_TestDebugTextColor,

        /* Debugger Log Window */

        Debugger_LogWindow_LogInput,
        Debugger_LogWindow_LogStatus,
        Debugger_LogWindow_LogTime,

        /* Debugger Watch Item */

        Debugger_WatchItem_ValueNormal,
        Debugger_WatchItem_ValueInvalid,
        Debugger_WatchItem_ValueChanged,

        /* Welcome Plugin */

        Welcome_TextColor,
        Welcome_ForegroundPrimaryColor,
        Welcome_ForegroundSecondaryColor,
        Welcome_BackgroundColor,
        Welcome_ButtonBackgroundColor,
        Welcome_DividerColor,
        Welcome_LinkColor,
        Welcome_HoverColor,
        Welcome_DisabledLinkColor,

        /* Timeline Library */
        Timeline_TextColor,
        Timeline_BackgroundColor1,
        Timeline_BackgroundColor2,
        Timeline_DividerColor,
        Timeline_HighlightColor,
        Timeline_PanelBackgroundColor,
        Timeline_PanelHeaderColor,
        Timeline_HandleColor,
        Timeline_RangeColor,

        /* VcsBase Plugin */
        VcsBase_FileStatusUnknown_TextColor,
        VcsBase_FileAdded_TextColor,
        VcsBase_FileModified_TextColor,
        VcsBase_FileDeleted_TextColor,
        VcsBase_FileRenamed_TextColor,
        VcsBase_FileUnmerged_TextColor,

        /* Bookmarks Plugin */
        Bookmarks_TextMarkColor,

        /* TextEditor Plugin */
        TextEditor_SearchResult_ScrollBarColor,
        TextEditor_CurrentLine_ScrollBarColor,

        /* Debugger Plugin */
        Debugger_Breakpoint_TextMarkColor,

        /* ProjectExplorer Plugin */
        ProjectExplorer_TaskError_TextMarkColor,
        ProjectExplorer_TaskWarn_TextMarkColor,

        /* QmlDesigner Plugin */
        QmlDesigner_BackgroundColor,
        QmlDesigner_HighlightColor,
        QmlDesigner_FormEditorSelectionColor,
        QmlDesigner_FormEditorForegroundColor,
        QmlDesigner_BackgroundColorDarker,
        QmlDesigner_BackgroundColorDarkAlternate,
        QmlDesigner_TabLight,
        QmlDesigner_TabDark,
        QmlDesigner_ButtonColor,
        QmlDesigner_BorderColor,
        QmlDesigner_FormeditorBackgroundColor,
        QmlDesigner_AlternateBackgroundColor,
        QmlDesigner_ScrollBarHandleColor,

        /* Palette for DS Controls */

        DSpanelBackground,
        DSinteraction,
        DSerrorColor,
        DSdisabledColor,
        DScontrolBackground,
        DScontrolBackgroundInteraction,
        DScontrolBackgroundDisabled,
        DScontrolBackgroundGlobalHover,
        DScontrolBackgroundHover,
        DScontrolOutline,
        DScontrolOutlineInteraction,
        DScontrolOutlineDisabled,
        DStextColor,
        DStextColorDisabled,
        DStextSelectionColor,
        DStextSelectedTextColor,

        DSplaceholderTextColor,
        DSplaceholderTextColorInteraction,

        DSiconColor,
        DSiconColorHover,
        DSiconColorInteraction,
        DSiconColorDisabled,
        DSiconColorSelected,
        DSlinkIndicatorColor,
        DSlinkIndicatorColorHover,
        DSlinkIndicatorColorInteraction,
        DSlinkIndicatorColorDisabled,
        DSpopupBackground,
        DSpopupOverlayColor,
        DSsliderActiveTrack,
        DSsliderActiveTrackHover,
        DSsliderActiveTrackFocus,
        DSsliderInactiveTrack,
        DSsliderInactiveTrackHover,
        DSsliderInactiveTrackFocus,
        DSsliderHandle,
        DSsliderHandleHover,
        DSsliderHandleFocus,
        DSsliderHandleInteraction,
        DSscrollBarTrack,
        DSscrollBarHandle,
        DSsectionHeadBackground,
        DSstateDefaultHighlight,
        DSstateSeparatorColor,
        DSstateBackgroundColor,
        DSstatePreviewOutline,
        DSchangedStateText,
        DS3DAxisXColor,
        DS3DAxisYColor,
        DS3DAxisZColor,
        DSactionBinding,
        DSactionAlias,
        DSactionKeyframe,
        DSactionJIT,

        DStableHeaderBackground,
        DStableHeaderText,

        DSdockContainerBackground,
        DSdockContainerSplitter,
        DSdockAreaBackground,

        DSdockWidgetBackground,
        DSdockWidgetSplitter,
        DSdockWidgetTitleBar,

        DStitleBarText,
        DStitleBarIcon,
        DStitleBarButtonHover,
        DStitleBarButtonPress,

        DStabContainerBackground,
        DStabSplitter,

        DStabInactiveBackground,
        DStabInactiveText,
        DStabInactiveIcon,
        DStabInactiveButtonHover,
        DStabInactiveButtonPress,

        DStabActiveBackground,
        DStabActiveText,
        DStabActiveIcon,
        DStabActiveButtonHover,
        DStabActiveButtonPress,

        DStabFocusBackground,
        DStabFocusText,
        DStabFocusIcon,
        DStabFocusButtonHover,
        DStabFocusButtonPress,

        DSnavigatorBranch,
        DSnavigatorBranchIndicator,
        DSnavigatorItemBackground,
        DSnavigatorItemBackgroundHover,
        DSnavigatorItemBackgroundSelected,
        DSnavigatorText,
        DSnavigatorTextHover,
        DSnavigatorTextSelected,
        DSnavigatorIcon,
        DSnavigatorIconHover,
        DSnavigatorIconSelected,
        DSnavigatorAliasIconChecked,
        DSnavigatorDropIndicatorBackground,
        DSnavigatorDropIndicatorOutline,

        DSheaderViewBackground,
        DStableViewAlternateBackground,

        DStoolTipBackground,
        DStoolTipOutline,
        DStoolTipText,

        DSUnimportedModuleColor
    };

    enum Gradient {
        DetailsWidgetHeaderGradient,
    };

    enum ImageFile {
        IconOverlayCSource,
        IconOverlayCppHeader,
        IconOverlayCppSource,
        IconOverlayPri,
        IconOverlayPrf,
        IconOverlayPro,
        StandardPixmapFileIcon,
        StandardPixmapDirIcon
    };

    enum Flag {
        DrawTargetSelectorBottom,
        DrawSearchResultWidgetFrame,
        DrawIndicatorBranch,
        DrawToolBarHighlights,
        DrawToolBarBorders,
        ComboBoxDrawTextShadow,
        DerivePaletteFromTheme,
        ApplyThemePaletteGlobally,
        FlatToolBars,
        FlatSideBarIcons,
        FlatProjectsMode,
        FlatMenuBar,
        ToolBarIconShadow,
        WindowColorAsBase,
        DarkUserInterface
    };

    Q_ENUM(Color)
    Q_ENUM(ImageFile)
    Q_ENUM(Gradient)
    Q_ENUM(Flag)

    Q_INVOKABLE bool flag(Flag f) const;
    Q_INVOKABLE QColor color(Color role) const;
    QString imageFile(ImageFile imageFile, const QString &fallBack) const;
    QGradientStops gradient(Gradient role) const;
    QPalette palette() const;
    QStringList preferredStyles() const;
    QString defaultTextEditorColorScheme() const;

    QString id() const;
    QString filePath() const;
    QString displayName() const;
    void setDisplayName(const QString &displayName);

    void readSettings(QSettings &settings);

    static bool systemUsesDarkMode();
    static QPalette initialPalette();

protected:
    Theme(Theme *originTheme, QObject *parent = nullptr);
    ThemePrivate *d;

private:
    friend QTCREATOR_UTILS_EXPORT Theme *creatorTheme();
    friend QTCREATOR_UTILS_EXPORT Theme *proxyTheme();
    QPair<QColor, QString> readNamedColor(const QString &color) const;
};

QTCREATOR_UTILS_EXPORT Theme *creatorTheme();
QTCREATOR_UTILS_EXPORT Theme *proxyTheme();

} // namespace Utils
