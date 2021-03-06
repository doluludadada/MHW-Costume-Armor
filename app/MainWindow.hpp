#pragma once
#include <QMainWindow>
#include <set>
#include <iomanip>
#include <map>

#include "ui_MainWindow.h"
#include "AboutWindow.hpp"
#include "DialogWindow.hpp"

#include "MHMemory.hpp"
#include "json.hpp"

// for convenience
using json = nlohmann::json;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void debugPrints() const ;
    void show();
    void _showTestGui();

    fs::Path settingsFile = CurrentExecutableDir().append("Settings.json");
    fs::Path savedSetsFile = CurrentExecutableDir().append("SavedSets.json");
    fs::Path armorDataFile = CurrentExecutableDir().append("ArmorData.json");

  private:
    Ui::MainWindow *ui;
    QActionGroup *_versionGroup = nullptr;
    QActionGroup *_langGroup = nullptr;
    MH_Memory _MHManager;
    json _Settings;
    json _SavedSets;
    json _ArmorData;
    bool _armorDataFound;
    std::array<QComboBox*,5> _inputBoxes = {nullptr,nullptr,nullptr,nullptr,nullptr};
    std::vector<QAction*> _versionActions;
    std::vector<QAction*> _langActions;
    std::set<std::string> _unsafeArmors;
    std::array<int,5> _safeCount = {0,0,0,0,0};
    json _transArmorData;

  private slots:
    // Inits
    bool _loadConfigFiles();
    void _populateComboBoxes();
    void _populateVersionSelector();
    void _populateLanguages();
    // Dialogs
    void _instructions();
    void _aboutInfo();
    void _unsafeWarning();
    void _notImplemented();
    // Memory
    void _findAddr();
    void _writeData();
    void _fetchData(bool noMessage = false);
    //Settings
    void _updateSelectedVersion();
    bool _flushSettings();
    void _getCustomSteamPath();
    void _toggleSafe();
    //Saved Sets
    bool _flushSavedSets();
    void _saveCurrentSet();
    void _deleteCurrentSet();
    void _loadSavedSet();
    void _loadSavedSetPopup();
    void _populateSavedSets();
    //Armor
    void _translateArmorData();
    bool _parseInputBoxes();
    void _updateArmorValues();
    void _clearArmor();
    void _changeAll();
    void _manualInputValue();
    void _addUnsafe();
    void _deleteUnsafe();
};