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

#include "utilsicons.h"

namespace Utils {
namespace Icons {

const Icon HOME({
        {":/utils/images/home.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon HOME_TOOLBAR({
        {":/utils/images/home.png", Theme::IconsBaseColor}});
const Icon EDIT_CLEAR({
        {":/utils/images/editclear.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon EDIT_CLEAR_TOOLBAR({
        {":/utils/images/editclear.png", Theme::IconsBaseColor}});
const Icon LOCKED_TOOLBAR({
        {":/utils/images/locked.png", Theme::IconsBaseColor}});
const Icon LOCKED({
        {":/utils/images/locked.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon UNLOCKED_TOOLBAR({
        {":/utils/images/unlocked.png", Theme::IconsBaseColor}});
const Icon UNLOCKED({
        {":/utils/images/unlocked.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon PINNED({
        {":/utils/images/pinned.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon NEXT({
        {":/utils/images/next.png", Theme::IconsWarningColor}}, Icon::MenuTintedStyle);
const Icon NEXT_TOOLBAR({
        {":/utils/images/next.png", Theme::IconsNavigationArrowsColor}});
const Icon PREV({
        {":/utils/images/prev.png", Theme::IconsWarningColor}}, Icon::MenuTintedStyle);
const Icon PREV_TOOLBAR({
        {":/utils/images/prev.png", Theme::IconsNavigationArrowsColor}});
const Icon PROJECT({
        {":/utils/images/project.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon ZOOM({
        {":/utils/images/zoom.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon ZOOM_TOOLBAR({
        {":/utils/images/zoom.png", Theme::IconsBaseColor}});
const Icon ZOOMIN_TOOLBAR({
        {":/utils/images/zoom.png", Theme::IconsBaseColor},
        {":/utils/images/zoomin_overlay.png", Theme::IconsBaseColor}});
const Icon ZOOMOUT_TOOLBAR({
        {":/utils/images/zoom.png", Theme::IconsBaseColor},
        {":/utils/images/zoomout_overlay.png", Theme::IconsBaseColor}});
const Icon FITTOVIEW_TOOLBAR({
        {":/utils/images/fittoview.png", Theme::IconsBaseColor}});
const Icon OK({
        {":/utils/images/ok.png", Theme::IconsRunColor}}, Icon::Tint);
const Icon NOTLOADED({
        {":/utils/images/notloaded.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon BROKEN({
        {":/utils/images/broken.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon CRITICAL({
        {":/utils/images/warningfill.png", Theme::BackgroundColorNormal},
        {":/utils/images/error.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon BOOKMARK({
        {":/utils/images/bookmark.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon BOOKMARK_TOOLBAR({
        {":/utils/images/bookmark.png", Theme::IconsBaseColor}});
const Icon BOOKMARK_TEXTEDITOR({
        {":/utils/images/bookmark.png", Theme::Bookmarks_TextMarkColor}}, Icon::Tint);
const Icon SNAPSHOT_TOOLBAR({
        {":/utils/images/snapshot.png", Theme::IconsBaseColor}});
const Icon NEWSEARCH_TOOLBAR({
        {":/utils/images/zoom.png", Theme::IconsBaseColor},
        {":/utils/images/iconoverlay_add_small.png", Theme::IconsRunColor}});
const Icon SETTINGS_TOOLBAR({
        {":/utils/images/settings.png", Theme::IconsBaseColor}});

const Icon NEWFILE({
        {":/utils/images/filenew.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon OPENFILE({
        {":/utils/images/fileopen.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon OPENFILE_TOOLBAR({
        {":/utils/images/fileopen.png", Theme::IconsBaseColor}});
const Icon SAVEFILE({
        {":/utils/images/filesave.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon SAVEFILE_TOOLBAR({
        {":/utils/images/filesave.png", Theme::IconsBaseColor}});

const Icon EXPORTFILE_TOOLBAR({
        {":/utils/images/fileexport.png", Theme::IconsBaseColor}});
const Icon MULTIEXPORTFILE_TOOLBAR({
        {":/utils/images/filemultiexport.png", Theme::IconsBaseColor}});

const Icon UNKNOWN_FILE({
        {":/utils/images/unknownfile.png", Theme::IconsBaseColor}});
const Icon DIR({
        {":/utils/images/dir.png", Theme::IconsBaseColor}});

const Icon UNDO({
        {":/utils/images/undo.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon UNDO_TOOLBAR({
        {":/utils/images/undo.png", Theme::IconsBaseColor}});
const Icon REDO({
        {":/utils/images/redo.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon REDO_TOOLBAR({
        {":/utils/images/redo.png", Theme::IconsBaseColor}});
const Icon COPY({
        {":/utils/images/editcopy.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon COPY_TOOLBAR({
        {":/utils/images/editcopy.png", Theme::IconsBaseColor}});
const Icon PASTE({
        {":/utils/images/editpaste.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon PASTE_TOOLBAR({
        {":/utils/images/editpaste.png", Theme::IconsBaseColor}});
const Icon CUT({
        {":/utils/images/editcut.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon CUT_TOOLBAR({
        {":/utils/images/editcut.png", Theme::IconsBaseColor}});
const Icon RESET({
        {":/utils/images/reset.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon RESET_TOOLBAR({
        {":/utils/images/reset.png", Theme::IconsBaseColor}});

const Icon ARROW_UP({
        {":/utils/images/arrowup.png", Theme::IconsBaseColor}});
const Icon ARROW_DOWN({
        {":/utils/images/arrowdown.png", Theme::IconsBaseColor}});
const Icon MINUS({
        {":/utils/images/minus.png", Theme::IconsBaseColor}});
const Icon PLUS_TOOLBAR({
        {":/utils/images/plus.png", Theme::IconsBaseColor}});
const Icon PLUS({
        {":/utils/images/plus.png", Theme::PaletteText}}, Icon::Tint);
const Icon MAGNIFIER({
        {":/utils/images/magnifier.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon CLEAN({
        {":/utils/images/clean_pane_small.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon CLEAN_TOOLBAR({
        {":/utils/images/clean_pane_small.png", Theme::IconsBaseColor}});
const Icon RELOAD({
        {":/utils/images/reload_gray.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon RELOAD_TOOLBAR({
        {":/utils/images/reload_gray.png", Theme::IconsBaseColor}});
const Icon TOGGLE_LEFT_SIDEBAR({
        {":/utils/images/leftsidebaricon.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon TOGGLE_LEFT_SIDEBAR_TOOLBAR({
        {":/utils/images/leftsidebaricon.png", Theme::IconsBaseColor}});
const Icon TOGGLE_RIGHT_SIDEBAR({
        {":/utils/images/rightsidebaricon.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon TOGGLE_RIGHT_SIDEBAR_TOOLBAR({
        {":/utils/images/rightsidebaricon.png", Theme::IconsBaseColor}});
const Icon CLOSE_TOOLBAR({
        {":/utils/images/close.png", Theme::IconsBaseColor}});
const Icon CLOSE_FOREGROUND({
        {":/utils/images/close.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon CLOSE_BACKGROUND({
        {":/utils/images/close.png", Theme::PanelTextColorLight}}, Icon::Tint);
const Icon SPLIT_HORIZONTAL({
        {":/utils/images/splitbutton_horizontal.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon SPLIT_HORIZONTAL_TOOLBAR({
        {":/utils/images/splitbutton_horizontal.png", Theme::IconsBaseColor}});
const Icon SPLIT_VERTICAL({
        {":/utils/images/splitbutton_vertical.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon SPLIT_VERTICAL_TOOLBAR({
        {":/utils/images/splitbutton_vertical.png", Theme::IconsBaseColor}});
const Icon CLOSE_SPLIT_TOP({
        {":/utils/images/splitbutton_closetop.png", Theme::IconsBaseColor}});
const Icon CLOSE_SPLIT_BOTTOM({
        {":/utils/images/splitbutton_closebottom.png", Theme::IconsBaseColor}});
const Icon CLOSE_SPLIT_LEFT({
        {":/utils/images/splitbutton_closeleft.png", Theme::IconsBaseColor}});
const Icon CLOSE_SPLIT_RIGHT({
        {":/utils/images/splitbutton_closeright.png", Theme::IconsBaseColor}});
const Icon FILTER({
        {":/utils/images/filtericon.png", Theme::IconsBaseColor},
        {":/utils/images/toolbuttonexpandarrow.png", Theme::IconsBaseColor}});
const Icon LINK({
        {":/utils/images/linkicon.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon LINK_TOOLBAR({
        {":/utils/images/linkicon.png", Theme::IconsBaseColor}});
const Icon SORT_ALPHABETICALLY_TOOLBAR({
        {":/utils/images/sort_alphabetically.png", Theme::IconsBaseColor}});
const Icon TOGGLE_PROGRESSDETAILS_TOOLBAR({
        {":/utils/images/toggleprogressdetails.png", Theme::IconsBaseColor}});
const Icon ONLINE({
        {":/utils/images/online.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon ONLINE_TOOLBAR({
        {":/utils/images/online.png", Theme::IconsBaseColor}});
const Icon DOWNLOAD({
        {":/utils/images/download.png", Theme::PanelTextColorMid}}, Icon::Tint);

const Icon WARNING({
        {":/utils/images/warningfill.png", Theme::BackgroundColorNormal},
        {":/utils/images/warning.png", Theme::IconsWarningColor}}, Icon::Tint);
const Icon WARNING_TOOLBAR({
        {":/utils/images/warning.png", Theme::IconsWarningToolBarColor}});
const Icon CRITICAL_TOOLBAR({
        {":/utils/images/error.png", Theme::IconsErrorToolBarColor}});
const Icon ERROR_TASKBAR({
        {":/utils/images/compile_error_taskbar.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon INFO({
        {":/utils/images/warningfill.png", Theme::BackgroundColorNormal},
        {":/utils/images/info.png", Theme::IconsInfoColor}}, Icon::Tint);
const Icon INFO_TOOLBAR({
        {":/utils/images/info.png", Theme::IconsInfoToolBarColor}});
const Icon EXPAND_ALL_TOOLBAR({
        {":/find/images/expand.png", Theme::IconsBaseColor}});
const Icon TOOLBAR_EXTENSION({
        {":/utils/images/extension.png", Theme::IconsBaseColor}});
const Icon RUN_SMALL({
        {":/utils/images/run_small.png", Theme::IconsRunColor}}, Icon::MenuTintedStyle);
const Icon RUN_SMALL_TOOLBAR({
        {":/utils/images/run_small.png", Theme::IconsRunToolBarColor}});
const Icon STOP_SMALL({
        {":/utils/images/stop_small.png", Theme::IconsStopColor}}, Icon::MenuTintedStyle);
const Icon STOP_SMALL_TOOLBAR({
        {":/utils/images/stop_small.png", Theme::IconsStopToolBarColor}});
const Icon INTERRUPT_SMALL({
        {":/utils/images/interrupt_small.png", Theme::IconsInterruptColor}}, Icon::MenuTintedStyle);
const Icon INTERRUPT_SMALL_TOOLBAR({
        {":/utils/images/interrupt_small.png", Theme::IconsInterruptToolBarColor}});
const Icon BOUNDING_RECT({
        {":/utils/images/boundingrect.png", Theme::IconsBaseColor}});
const Icon EYE_OPEN_TOOLBAR({
        {":/utils/images/eye_open.png", Theme::IconsBaseColor}});
const Icon EYE_CLOSED_TOOLBAR({
        {":/utils/images/eye_closed.png", Theme::IconsBaseColor}});
const Icon REPLACE({
        {":/utils/images/replace_a.png", Theme::PanelTextColorMid},
        {":/utils/images/replace_b.png", Theme::IconsInfoColor}}, Icon::Tint);
const Icon EXPAND({
        {":/utils/images/expand.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon EXPAND_TOOLBAR({
        {":/utils/images/expand.png", Theme::IconsBaseColor}});
const Icon COLLAPSE({
        {":/utils/images/collapse.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon COLLAPSE_TOOLBAR({
        {":/utils/images/collapse.png", Theme::IconsBaseColor}});
const Icon PAN_TOOLBAR({
        {":/utils/images/pan.png", Theme::IconsBaseColor}});
const Icon EMPTY14(":/utils/images/empty14.png");
const Icon EMPTY16(":/utils/images/empty16.png");
const Icon OVERLAY_ADD({
        {":/utils/images/iconoverlay_add_background.png", Theme::BackgroundColorNormal},
        {":/utils/images/iconoverlay_add.png", Theme::IconsRunColor}}, Icon::Tint);
const Icon OVERLAY_WARNING({
        {":/utils/images/iconoverlay_warning_background.png", Theme::BackgroundColorNormal},
        {":/utils/images/iconoverlay_warning.png", Theme::IconsWarningColor}}, Icon::Tint);
const Icon OVERLAY_ERROR({
        {":/utils/images/iconoverlay_error_background.png", Theme::BackgroundColorNormal},
        {":/utils/images/iconoverlay_error.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon RUN_FILE({
        {":/utils/images/run_small.png", Theme::IconsRunColor},
        {":/utils/images/run_file.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon RUN_FILE_TOOLBAR({
        {":/utils/images/run_small.png", Theme::IconsRunToolBarColor},
        {":/utils/images/run_file.png", Theme::IconsBaseColor}});
const Icon RUN_SELECTED({
        {":/utils/images/run_small.png", Theme::IconsRunColor},
        {":/utils/images/runselected_boxes.png", Theme::PanelTextColorMid},
        {":/utils/images/runselected_tickmarks.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon RUN_SELECTED_TOOLBAR({
        {":/utils/images/run_small.png", Theme::IconsRunToolBarColor},
        {":/utils/images/runselected_boxes.png", Theme::IconsBaseColor},
        {":/utils/images/runselected_tickmarks.png", Theme::IconsBaseColor}});

const Icon CODEMODEL_ERROR({
        {":/utils/images/codemodelerror.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon CODEMODEL_WARNING({
        {":/utils/images/codemodelwarning.png", Theme::IconsWarningColor}}, Icon::Tint);
const Icon CODEMODEL_DISABLED_ERROR({
        {":/utils/images/codemodelerror.png", Theme::IconsDisabledColor}}, Icon::Tint);
const Icon CODEMODEL_DISABLED_WARNING({
        {":/utils/images/codemodelwarning.png", Theme::IconsDisabledColor}}, Icon::Tint);
const Icon CODEMODEL_FIXIT({
        {":/utils/images/lightbulbcap.png", Theme::PanelTextColorMid},
        {":/utils/images/lightbulb.png", Theme::IconsWarningColor}}, Icon::Tint);

const Icon MACOS_TOUCHBAR_BOOKMARK(
        ":/utils/images/macos_touchbar_bookmark.png");
const Icon MACOS_TOUCHBAR_CLEAR(
        ":/utils/images/macos_touchbar_clear.png");
} // namespace Icons

QIcon CodeModelIcon::iconForType(CodeModelIcon::Type type)
{
    static const IconMaskAndColor classRelationIcon {
        ":/codemodel/images/classrelation.png", Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor classRelationBackgroundIcon {
        ":/codemodel/images/classrelationbackground.png", Theme::IconsCodeModelOverlayBackgroundColor};
    static const IconMaskAndColor classMemberFunctionIcon {
        ":/codemodel/images/classmemberfunction.png", Theme::IconsCodeModelFunctionColor};
    static const IconMaskAndColor classMemberVariableIcon {
        ":/codemodel/images/classmembervariable.png", Theme::IconsCodeModelVariableColor};
    static const IconMaskAndColor functionIcon {
        ":/codemodel/images/member.png", Theme::IconsCodeModelFunctionColor};
    static const IconMaskAndColor variableIcon {
        ":/codemodel/images/member.png", Theme::IconsCodeModelVariableColor};
    static const IconMaskAndColor signalIcon {
        ":/codemodel/images/signal.png", Theme::IconsCodeModelFunctionColor};
    static const IconMaskAndColor slotIcon {
        ":/codemodel/images/slot.png", Theme::IconsCodeModelFunctionColor};
    static const IconMaskAndColor propertyIcon {
        ":/codemodel/images/property.png", Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor propertyBackgroundIcon {
        ":/codemodel/images/propertybackground.png", Theme::IconsCodeModelOverlayBackgroundColor};
    static const IconMaskAndColor protectedIcon {
        ":/codemodel/images/protected.png", Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor protectedBackgroundIcon {
        ":/codemodel/images/protectedbackground.png", Theme::IconsCodeModelOverlayBackgroundColor};
    static const IconMaskAndColor privateIcon {
        ":/codemodel/images/private.png", Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor privateBackgroundIcon {
        ":/codemodel/images/privatebackground.png", Theme::IconsCodeModelOverlayBackgroundColor};
    static const IconMaskAndColor staticIcon {
        ":/codemodel/images/static.png", Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor staticBackgroundIcon {
        ":/codemodel/images/staticbackground.png", Theme::IconsCodeModelOverlayBackgroundColor};

    switch (type) {
    case Class: {
        const static QIcon icon(Icon({
            classRelationBackgroundIcon, classRelationIcon,
            {":/codemodel/images/classparent.png", Theme::IconsCodeModelClassColor},
            classMemberFunctionIcon, classMemberVariableIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case Struct: {
        const static QIcon icon(Icon({
            classRelationBackgroundIcon, classRelationIcon,
            {":/codemodel/images/classparent.png", Theme::IconsCodeModelStructColor},
            classMemberFunctionIcon, classMemberVariableIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case Enum: {
        const static QIcon icon(Icon({
            {":/codemodel/images/enum.png", Theme::IconsCodeModelEnumColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case Enumerator: {
        const static QIcon icon(Icon({
            {":/codemodel/images/enumerator.png", Theme::IconsCodeModelEnumColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncPublic: {
        const static QIcon icon(Icon({
                functionIcon}, Icon::Tint).icon());
        return icon;
    }
    case FuncProtected: {
        const static QIcon icon(Icon({
                functionIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncPrivate: {
        const static QIcon icon(Icon({
            functionIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncPublicStatic: {
        const static QIcon icon(Icon({
            functionIcon, staticBackgroundIcon, staticIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncProtectedStatic: {
        const static QIcon icon(Icon({
            functionIcon, staticBackgroundIcon, staticIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncPrivateStatic: {
        const static QIcon icon(Icon({
            functionIcon, staticBackgroundIcon, staticIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case Namespace: {
        const static QIcon icon(Icon({
            {":/utils/images/namespace.png", Theme::IconsCodeModelKeywordColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case VarPublic: {
        const static QIcon icon(Icon({
            variableIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarProtected: {
        const static QIcon icon(Icon({
            variableIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarPrivate: {
        const static QIcon icon(Icon({
            variableIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarPublicStatic: {
        const static QIcon icon(Icon({
            variableIcon, staticBackgroundIcon, staticIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarProtectedStatic: {
        const static QIcon icon(Icon({
            variableIcon, staticBackgroundIcon, staticIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarPrivateStatic: {
        const static QIcon icon(Icon({
            variableIcon, staticBackgroundIcon, staticIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case Signal: {
        const static QIcon icon(Icon({
            signalIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case SlotPublic: {
        const static QIcon icon(Icon({
            slotIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case SlotProtected: {
        const static QIcon icon(Icon({
            slotIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case SlotPrivate: {
        const static QIcon icon(Icon({
            slotIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case Keyword: {
        const static QIcon icon(Icon({
            {":/codemodel/images/keyword.png", Theme::IconsCodeModelKeywordColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case Macro: {
        const static QIcon icon(Icon({
            {":/codemodel/images/macro.png", Theme::IconsCodeModelMacroColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case Property: {
        const static QIcon icon(Icon({
            variableIcon, propertyBackgroundIcon, propertyIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case Unknown: {
        const static QIcon icon(Icons::EMPTY16.icon());
        return icon;
    }
    default:
        break;
    }
    return QIcon();
}

} // namespace Utils
