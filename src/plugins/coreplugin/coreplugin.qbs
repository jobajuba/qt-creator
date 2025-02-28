import qbs 1.0
import qbs.FileInfo
import qbs.Utilities

Project {
    name: "Core"

    QtcPlugin {
        Depends {
            name: "Qt"
            submodules: ["widgets", "xml", "network", "qml", "sql", "printsupport"]
        }

        Depends {
            name: "Qt.gui-private"
            condition: qbs.targetOS.contains("windows")
        }

        Depends { name: "Utils" }
        Depends { name: "Aggregation" }

        Depends { name: "app_version_header" }

        cpp.dynamicLibraries: {
            if (qbs.targetOS.contains("windows"))
                return ["ole32", "user32"]
        }

        cpp.frameworks: qbs.targetOS.contains("macos") ? ["AppKit"] : undefined

        Group {
            name: "General"
            files: [
                "basefilewizard.cpp",
                "basefilewizard.h",
                "basefilewizardfactory.cpp",
                "basefilewizardfactory.h",
                "core.qrc",
                "core_global.h",
                "coreconstants.h",
                "coreicons.cpp",
                "coreicons.h",
                "corejsextensions.cpp",
                "corejsextensions.h",
                "coreplugin.cpp",
                "coreplugin.h",
                "designmode.cpp",
                "designmode.h",
                "diffservice.cpp",
                "diffservice.h",
                "documentmanager.cpp",
                "documentmanager.h",
                "editmode.cpp",
                "editmode.h",
                "editortoolbar.cpp",
                "editortoolbar.h",
                "externaltool.cpp",
                "externaltool.h",
                "externaltoolmanager.cpp",
                "externaltoolmanager.h",
                "fancyactionbar.cpp",
                "fancyactionbar.h",
                "fancyactionbar.qrc",
                "fancytabwidget.cpp",
                "fancytabwidget.h",
                "featureprovider.cpp",
                "featureprovider.h",
                "fileiconprovider.cpp",
                "fileiconprovider.h",
                "fileutils.cpp",
                "fileutils.h",
                "findplaceholder.cpp",
                "findplaceholder.h",
                "generalsettings.cpp",
                "generalsettings.h",
                "generalsettings.ui",
                "generatedfile.cpp",
                "generatedfile.h",
                "helpitem.cpp",
                "helpitem.h",
                "helpmanager.cpp",
                "helpmanager.h",
                "helpmanager_implementation.h",
                "icontext.cpp",
                "icontext.h",
                "icore.cpp",
                "icore.h",
                "idocument.cpp",
                "idocument.h",
                "idocumentfactory.cpp",
                "idocumentfactory.h",
                "ifilewizardextension.h",
                "imode.cpp",
                "imode.h",
                "inavigationwidgetfactory.cpp",
                "inavigationwidgetfactory.h",
                "ioutputpane.cpp",
                "ioutputpane.h",
                "iversioncontrol.cpp",
                "iversioncontrol.h",
                "iwelcomepage.cpp",
                "iwelcomepage.h",
                "iwizardfactory.cpp",
                "iwizardfactory.h",
                "jsexpander.cpp",
                "jsexpander.h",
                "mainwindow.cpp",
                "mainwindow.h",
                "manhattanstyle.cpp",
                "manhattanstyle.h",
                "menubarfilter.cpp",
                "menubarfilter.h",
                "messagebox.cpp",
                "messagebox.h",
                "messagemanager.cpp",
                "messagemanager.h",
                "messageoutputwindow.cpp",
                "messageoutputwindow.h",
                "mimetypemagicdialog.cpp",
                "mimetypemagicdialog.h",
                "mimetypemagicdialog.ui",
                "mimetypesettings.cpp",
                "mimetypesettings.h",
                "mimetypesettingspage.ui",
                "minisplitter.cpp",
                "minisplitter.h",
                "modemanager.cpp",
                "modemanager.h",
                "navigationsubwidget.cpp",
                "navigationsubwidget.h",
                "navigationwidget.cpp",
                "navigationwidget.h",
                "opendocumentstreeview.cpp",
                "opendocumentstreeview.h",
                "outputpane.cpp",
                "outputpane.h",
                "outputpanemanager.cpp",
                "outputpanemanager.h",
                "outputwindow.cpp",
                "outputwindow.h",
                "patchtool.cpp",
                "patchtool.h",
                "plugindialog.cpp",
                "plugindialog.h",
                "plugininstallwizard.cpp",
                "plugininstallwizard.h",
                "rightpane.cpp",
                "rightpane.h",
                "settingsdatabase.cpp",
                "settingsdatabase.h",
                "shellcommand.cpp",
                "shellcommand.h",
                "sidebar.cpp",
                "sidebar.h",
                "sidebarwidget.cpp",
                "sidebarwidget.h",
                "statusbarmanager.cpp",
                "statusbarmanager.h",
                "styleanimator.cpp",
                "styleanimator.h",
                "systemsettings.cpp",
                "systemsettings.h",
                "systemsettings.ui",
                "textdocument.cpp",
                "textdocument.h",
                "themechooser.cpp",
                "themechooser.h",
                "vcsmanager.cpp",
                "vcsmanager.h",
                "versiondialog.cpp",
                "versiondialog.h",
                "welcomepagehelper.cpp",
                "welcomepagehelper.h",
                "windowsupport.cpp",
                "windowsupport.h",
            ]
        }

        Group {
            name: "Action Manager"
            prefix: "actionmanager/"
            files: [
                "actioncontainer.cpp", "actioncontainer.h", "actioncontainer_p.h",
                "actionmanager.cpp", "actionmanager.h", "actionmanager_p.h",
                "command.cpp", "command.h", "command_p.h",
                "commandbutton.cpp", "commandbutton.h",
                "commandmappings.cpp", "commandmappings.h",
                "commandsfile.cpp", "commandsfile.h",
            ]
        }

        Group {
            name: "Dialogs"
            prefix: "dialogs/"
            files: [
                "addtovcsdialog.cpp", "addtovcsdialog.h", "addtovcsdialog.ui",
                "codecselector.cpp", "codecselector.h",
                "externaltoolconfig.cpp", "externaltoolconfig.h", "externaltoolconfig.ui",
                "filepropertiesdialog.cpp", "filepropertiesdialog.h", "filepropertiesdialog.ui",
                "ioptionspage.cpp", "ioptionspage.h",
                "newdialog.cpp", "newdialog.h", "newdialog.ui",
                "newdialogwidget.cpp", "newdialogwidget.h",
                "openwithdialog.cpp", "openwithdialog.h", "openwithdialog.ui",
                "promptoverwritedialog.cpp", "promptoverwritedialog.h",
                "readonlyfilesdialog.cpp", "readonlyfilesdialog.h", "readonlyfilesdialog.ui",
                "restartdialog.cpp", "restartdialog.h",
                "saveitemsdialog.cpp", "saveitemsdialog.h", "saveitemsdialog.ui",
                "settingsdialog.cpp", "settingsdialog.h",
                "shortcutsettings.cpp", "shortcutsettings.h",
            ]
        }

        Group {
            name: "Editor Manager"
            prefix: "editormanager/"
            files: [
                "documentmodel.cpp", "documentmodel.h", "documentmodel_p.h",
                "editorarea.cpp", "editorarea.h",
                "editormanager.cpp", "editormanager.h", "editormanager_p.h",
                "editorview.cpp", "editorview.h",
                "editorwindow.cpp", "editorwindow.h",
                "ieditor.cpp", "ieditor.h",
                "ieditorfactory.cpp", "ieditorfactory.h", "ieditorfactory_p.h",
                "iexternaleditor.cpp", "iexternaleditor.h",
                "openeditorsview.cpp", "openeditorsview.h",
                "openeditorswindow.cpp", "openeditorswindow.h",
                "systemeditor.cpp", "systemeditor.h",
            ]
        }

        Group {
            name: "Progress Manager"
            prefix: "progressmanager/"
            files: [
                "futureprogress.cpp", "futureprogress.h",
                "progressbar.cpp", "progressbar.h",
                "progressmanager.cpp", "progressmanager.h", "progressmanager_p.h",
                "progressview.cpp", "progressview.h",
            ]
        }

        Group {
            name: "ProgressManager_win"
            condition: qbs.targetOS.contains("windows")
            files: [
                "progressmanager/progressmanager_win.cpp",
            ]
        }

        Group {
            name: "ProgressManager_mac"
            condition: qbs.targetOS.contains("macos")
            files: [
                "progressmanager/progressmanager_mac.mm",
            ]
        }

        Group {
            name: "ProgressManager_x11"
            condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("macos")
            files: [
                "progressmanager/progressmanager_x11.cpp",
            ]
        }

        Group {
            name: "Tests"
            condition: qtc.testsEnabled
            files: [
                "testdatadir.cpp",
                "testdatadir.h",
                "locator/locatorfiltertest.cpp",
                "locator/locatorfiltertest.h",
                "locator/locator_test.cpp"
            ]

            cpp.defines: outer.concat(['SRCDIR="' + path + '"'])
        }

        Group {
            name: "Find"
            prefix: "find/"
            files: [
                "basetextfind.cpp",
                "basetextfind.h",
                "currentdocumentfind.cpp",
                "currentdocumentfind.h",
                "find.qrc",
                "finddialog.ui",
                "findplugin.cpp",
                "findplugin.h",
                "findtoolbar.cpp",
                "findtoolbar.h",
                "findtoolwindow.cpp",
                "findtoolwindow.h",
                "findwidget.ui",
                "highlightscrollbarcontroller.cpp",
                "highlightscrollbarcontroller.h",
                "ifindfilter.cpp",
                "ifindfilter.h",
                "ifindsupport.cpp",
                "ifindsupport.h",
                "itemviewfind.cpp",
                "itemviewfind.h",
                "optionspopup.cpp",
                "optionspopup.h",
                "searchresultcolor.h",
                "searchresultitem.h",
                "searchresulttreeitemdelegate.cpp",
                "searchresulttreeitemdelegate.h",
                "searchresulttreeitemroles.h",
                "searchresulttreeitems.cpp",
                "searchresulttreeitems.h",
                "searchresulttreemodel.cpp",
                "searchresulttreemodel.h",
                "searchresulttreeview.cpp",
                "searchresulttreeview.h",
                "searchresultwidget.cpp",
                "searchresultwidget.h",
                "searchresultwindow.cpp",
                "searchresultwindow.h",
                "textfindconstants.h",
            ]
        }

        Group {
            name: "Locator"
            prefix: "locator/"
            files: [
                "basefilefilter.cpp",
                "basefilefilter.h",
                "commandlocator.cpp",
                "commandlocator.h",
                "directoryfilter.cpp",
                "directoryfilter.h",
                "directoryfilter.ui",
                "executefilter.cpp",
                "executefilter.h",
                "externaltoolsfilter.cpp",
                "externaltoolsfilter.h",
                "filesystemfilter.cpp",
                "filesystemfilter.h",
                "filesystemfilter.ui",
                "ilocatorfilter.cpp",
                "ilocatorfilter.h",
                "javascriptfilter.cpp",
                "javascriptfilter.h",
                "locatorconstants.h",
                "locatorfiltersfilter.cpp",
                "locatorfiltersfilter.h",
                "locatormanager.cpp",
                "locatormanager.h",
                "locator.cpp",
                "locator.h",
                "locatorsearchutils.cpp",
                "locatorsearchutils.h",
                "locatorsettingspage.cpp",
                "locatorsettingspage.h",
                "locatorsettingspage.ui",
                "locatorwidget.cpp",
                "locatorwidget.h",
                "opendocumentsfilter.cpp",
                "opendocumentsfilter.h",
                "spotlightlocatorfilter.h",
                "spotlightlocatorfilter.cpp",
                "urllocatorfilter.cpp",
                "urllocatorfilter.h",
                "urllocatorfilter.ui"
            ]
        }

        Export {
            Depends { name: "Aggregation" }
            Depends { name: "Utils" }
        }
    }
}
