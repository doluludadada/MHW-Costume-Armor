#include "MainWindow.hpp"
#include "Config.h"
#include <QThread>
#include <QMovie>
#include <QInputDialog>

// TODO :
// - Design InputDialog for saved set features.
// - "Unsafe Mode"

/// Begin Main Window Member definitions

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle(PROJECT_NAME);
    ui->FetchButton->setEnabled(false);
    ui->WriteButton->setEnabled(false);

    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(_aboutInfo()));
    connect(ui->actionTutorial, SIGNAL(triggered()), this, SLOT(_Instructions()));
    connect(ui->actionSafe_Mode, SIGNAL(triggered()), this, SLOT(_ToggleSafe() )  );
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()) );

    connect(ui->SearchButton, SIGNAL(released()), this, SLOT(_FindAddr()));
    connect(ui->FetchButton, SIGNAL(released()), this, SLOT(_FetchData()));
    connect(ui->WriteButton, SIGNAL(released()), this, SLOT(_WriteData()));
    connect(ui->ClearButton, SIGNAL(released()), this, SLOT(_ClearArmor()));

    connect(ui->actionSave_Current_Armor, SIGNAL(triggered()), this, SLOT(_SaveCurrentSet()));
    connect(ui->actionLoad_Armor, SIGNAL(triggered()), this, SLOT(_LoadSavedSet()) );
    connect(ui->actionManaged_Saved_Sets, SIGNAL(triggered()), this, SLOT(_NotImplemented()));

    _InputBoxes[0] = ui->headEdit ;
    _InputBoxes[1] = ui->bodyEdit ;
    _InputBoxes[2] = ui->armsEdit ;
    _InputBoxes[3] = ui->waistEdit;
    _InputBoxes[4] = ui->legsEdit ;

    this->_ArmorDataFound = _LoadConfigFiles();
    if (!_ArmorDataFound || !this->_MHManager.ProcessOpen())
        return;
    _PopulateComboBoxes();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::show()
{
    if (!this->_ArmorDataFound)
    {
        DialogWindow *Dia = new DialogWindow(nullptr, "ERROR FATAL", "Couldn't find " +
            ArmorDataFile.filename().string() +
            "\nIt may have been deleted, if that's the case, re-download the program.", Status::ERROR0);
        Dia->getOkButton()->setStyleSheet("");
        Dia->show();
        this->close();
    }
    else if (!this->_MHManager.ProcessOpen())
    {
        DialogWindow *Dia = new DialogWindow(nullptr, "ERROR", "MHW is Closed\nPlease open it before starting.", Status::ERROR0);
        Dia->getOkButton()->setStyleSheet("");
        Dia->show();
        this->close();
    }
    else
        QMainWindow::show();
}

bool MainWindow::_LoadConfigFiles()
{
    if (fs::exists(SettingsFile))
    {
        std::ifstream i(SettingsFile);
        i >> _Settings;
    }
    if (fs::exists(SavedSetsFile))
    {
        std::ifstream i(SavedSetsFile);
        i >> _SavedSets;
    }
    if(!fs::exists(ArmorDataFile))
        return false;
    
    std::ifstream i(ArmorDataFile);
    i >> _ArmorData;
    return true;
}

void MainWindow::_PopulateComboBoxes()
{
    int i;
    int ID;

    for (i = 0; i < 5; ++i)
        this->_InputBoxes[i]->addItem("Nothing", 255);

    for (auto &el : _ArmorData.items())
    {
        ID = el.value()["ID"];
        
        if (el.value()["Danger"])
        {
            _UnSafeArmors.insert(el.key());
            continue;
        }

        for(i=0; i<5; ++i)
        {
            if (el.value()[Armor::Names[i]])
                this->_InputBoxes[i]->addItem(el.key().c_str(), ID);
        }
    }
    for (i = 0; i < 5; ++i)
        _SafeCount[i] = this->_InputBoxes[i]->count();
}

void MainWindow::_ToggleSafe()
{
    _SafeMode = !_SafeMode;
    if (!_SafeMode)
    {
        this->setWindowTitle( QString(PROJECT_NAME) + "-Unsafe" );
        DEBUG_LOG(WARNING,"Safe Mode was turned off");
        DialogWindow *Dia = new DialogWindow(this, "Warning", "Safe Mode was turned OFF\n(!) Marked Armors May Cause Game Crashes\nUse with caution.", Status::WARNING);
        Dia->show();
        this->_AddUnsafe();
    }
    else
    {
        this->_DeleteUnsafe();
        this->setWindowTitle(PROJECT_NAME);
    }
}

void MainWindow::_FindAddr()
{
    ui->SearchButton->setText("Searching for Save Data...");
    ui->SearchButton->setEnabled(false);

    DialogWindow *Dia = new DialogWindow(this, "Wait a Sec...", "Searching MHW for Character Data", Status::SUCCESS);
    Dia->setWindowFlags(Qt::SplashScreen);
    QLabel *IconLabel = Dia->getIconLabel();
    std::string PrevText = IconLabel->text().toUtf8().constData();
    QMovie *movie = new QMovie(":/ajax-loader.gif");
    movie->setScaledSize(IconLabel->frameSize());
    IconLabel->setMovie(movie);
    movie->start();

    Dia->getOkButton()->setText("Cancel");
    Dia->setAttribute(Qt::WA_DeleteOnClose, false);

    Dia->show();

    QThread *thread = QThread::create( [this] {this->_MHManager.FindAddress(); });
    thread->start();

    while (thread->isRunning() )
    {
        if(!Dia->isVisible())
        {
            thread->quit();
            delete Dia;
            ui->SearchButton->setEnabled(true);
            ui->SearchButton->setText("Search For MHW Save Data");
            return;
        }
        QCoreApplication::processEvents();
    }

    if (!this->_MHManager.DataAddressFound())
    {
        Dia->setAttribute(Qt::WA_DeleteOnClose, true);
        Dia->close();
        ui->SearchButton->setEnabled(true);
        DialogWindow *Dia = new DialogWindow(this, "ERROR", "Couldn't Find Save Data Address", Status::ERROR0);
        Dia->show();
        ui->SearchButton->setText("Search For MHW Save Data");
        return;
    }
    Dia->getOkButton()->setText("Ok");
    Dia->getIconLabel()->clear();
    Dia->getIconLabel()->setText(PrevText.c_str());
    Dia->getMsgLabel()->setText("MHW Data Found Successfully!");
    Dia->setWindowTitle("Succes!!");
    Dia->setAttribute(Qt::WA_DeleteOnClose, true);

    ui->SearchButton->setText("Ready To Go.");
    ui->SearchButton->setEnabled(false);

    ui->FetchButton->setEnabled(true);
    ui->WriteButton->setEnabled(true);
}

void MainWindow::_UpdateArmorValues()
{
    auto Data = _MHManager.getPlayerData().getData();
    int index;
    for (int i = 0; i < 5; ++i)
    {
        index = _InputBoxes[i]->findData((int)Data[i]);
        if (index < 0)
        {
            index = 0;
            DEBUG_LOG(WARNING, "Encountered unknown value (" << Data[i] << ") changing it to 255 for safety");
        }
        _InputBoxes[i]->setCurrentIndex(index);
    }
}

void MainWindow::_FetchData(bool noMessage)
{
    int slot = std::stoi(ui->comboBox->currentText().toUtf8().constData());
    if (!_MHManager.FetchPlayerData(slot-1))
    {
        DialogWindow *Dia = new DialogWindow(this, "ERROR", "Couldn't Fetch Character Data...", Status::ERROR0);
        Dia->show();
        return;
    }
    this->_UpdateArmorValues();
    std::string msg = "Successfully fetched Data for Character Slot " + std::to_string(slot);
    if(!noMessage)
    {
        DialogWindow *Dia = new DialogWindow(this, "Success!!", msg, Status::SUCCESS);
        Dia->show();
    }
}

void MainWindow::_LoadSavedSet()
{
    if (_SavedSets.empty())
    {
        DialogWindow *Dia = new DialogWindow(this, "Warning", "There are no saved sets", Status::WARNING);
        Dia->show();
        return;
    }

    QStringList items;
    for (const auto &it : _SavedSets.items() )
        items << it.key().c_str();
    
    bool ok;
    QString text = QInputDialog::getItem(this, "Select Armor Set", "Select set: ", items, 0, false, &ok,
                                         Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    if (!ok)
        return;
    
    try
    {
        for(int i=0; i<5 ;++i)
            _MHManager.getPlayerData().setArmorPiece(i, _SavedSets[text.toStdString()][i]);
    }
    catch (std::exception &e)
    {
        DialogWindow *Dia = new DialogWindow(this, "ERROR", "Invalid Value for armor", Status::ERROR0);
        Dia->show();
        DEBUG_LOG(ERROR,"Invalid value at json file. Error "<<e.what() );
        return;
    }
    this->_UpdateArmorValues();
}

void MainWindow::_ClearArmor()
{
    for(int i=0; i<5 ;++i)
        _MHManager.getPlayerData().setArmorPiece(i, 255);
    this->_UpdateArmorValues();
}

bool MainWindow::_ParseInputBoxes()
{
    int Val;
    for (int i = 0; i < 5; ++i)
    {
        try
        {
            Val = _InputBoxes[i]->currentData().toInt();
            this->_MHManager.getPlayerData().setArmorPiece(i, Val);
        }
        catch (std::exception &e)
        {

            DialogWindow *Dia = new DialogWindow(this, "ERROR", "Invalid Value for armor", Status::ERROR0);
            Dia->show();
            this->_FetchData(true);
            return false;
        }
    }
    return true;
}

void MainWindow::_SaveCurrentSet()
{
    if (!this->_ParseInputBoxes())
        return;

    bool ok;
    QString text;
    while (true)
    {
        text = QInputDialog::getText(this, "Saving Armor Set...", "Name for the set: ", QLineEdit::Normal, "", &ok,
                                     Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint );

        if (_SavedSets.find(text.toUtf8().constData()) != _SavedSets.end())
        {
            DialogWindow *Dia = new DialogWindow(this, "WARNING", "The name already exists.", Status::WARNING);
            Dia->show();
            while (Dia->isVisible())
                QCoreApplication::processEvents();
            continue;
        }

        if (!ok)
            return;
        if(!text.isEmpty())
            break;
    }

    auto Data = _MHManager.getPlayerData().getData();
    _SavedSets[text.toUtf8().constData()] = Data;

    if(!this->_FlushSavedSets())
    {
        DialogWindow *Dia = new DialogWindow(this, "ERROR", "Couldn't save set.", Status::ERROR0);
        Dia->show();
        return;
    }
    DialogWindow *Dia = new DialogWindow(this, "Success", "Successfully saved set!", Status::SUCCESS);
    Dia->show();
}

void MainWindow::_WriteData()
{
    if (!this->_ParseInputBoxes())
        return;

    int slot = std::stoi(ui->comboBox->currentText().toUtf8().constData());
    if (!_MHManager.WriteArmor(slot-1,_SafeMode))
    {
        DialogWindow *Dia = new DialogWindow(this, "ERROR", "Couldn't Write Save Data!", Status::ERROR0);
        Dia->show();
    }
    else
    {
        DialogWindow *Dia = new DialogWindow(this, "Success!!", "Success writing to Game!!\nEnter your room to reload (Do not save before reloading!)", Status::SUCCESS);
        Dia->show();
    }
    this->_FetchData(true);
}

bool MainWindow::_FlushSavedSets()
{
    try
    {
        std::ofstream o(SavedSetsFile.c_str());
        o << std::setw(2) << _SavedSets << std::endl;
    }
    catch (std::exception &e)
    {
        DEBUG_LOG(ERROR,"Couldn't write to file because " << e.what() );
        return false;
    }
    return true;
}

void MainWindow::_AddUnsafe()
{
    int i;
    int ID;
    for(const auto &_Name : this->_UnSafeArmors)
    {
        ID = _ArmorData[_Name]["ID"];
        for (i = 0; i < 5; ++i)
        {
            if (_ArmorData[_Name][Armor::Names[i]])
                this->_InputBoxes[i]->addItem(std::string("(!) ").append(_Name).c_str(), ID);
        }
    }
}

void MainWindow::_DeleteUnsafe()
{
    for (int i = 0; i < 5; ++i)
    {
        for (int index = _InputBoxes[i]->count(); index >= _SafeCount[i]; --index)
            _InputBoxes[i]->removeItem(index);
    }
}

void MainWindow::debugPrints() const
{
    if (_MHManager.SteamFound())
    {
        DEBUG_LOG(DEBUG, "Steam UserData ID: " << _MHManager.getSteamID() );
        DEBUG_LOG(DEBUG, "Steam Game Directory: " << _MHManager.getSteamPath() );
    }
    else
    {
        DEBUG_LOG(ERROR, "Couldn't Find Steam Data" );
    }
}

void MainWindow::_aboutInfo()
{
    AboutWindow *Dia = new AboutWindow(this);
    Dia->show();
}

void MainWindow::_Instructions()
{
    Instructions *Dia = new Instructions(this);
    Dia->show();
}

void MainWindow::_NotImplemented() 
{
    DialogWindow *Dia = new DialogWindow(this, "Warning", "Functionality Not Implemented... Yet", Status::WARNING);
    Dia->show();
}
