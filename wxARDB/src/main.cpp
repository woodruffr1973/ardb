/*  Anarch Revolt Deck Builder - a VTES inventory manager / deck builder
*
*  Copyright (C) 2002 Francois Gombault
*  gombault.francois@wanadoo.fr
*
*  Copyright (C) 2007 Graham Smith
*  graham.r.smith@gmail.com
*  
*  contributors:
*    meshee.knight@gmail.com
*	 afri@afri.cz
*  
*  Official project page: http://code.google.com/p/ardb/
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software Foundation,
* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
*/

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif


#define BUILD_DATE wxT (__DATE__" "__TIME__)
#define BAD_HEIGHT_OR_WIDTH_X_OR_Y -32000

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/image.h>
#include <wx/filename.h>
#include <wx/splash.h>
#include <wx/mimetype.h>
#include <wx/tooltip.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>

class BrowserFrame ;

#include "main.h"
//#include "configfile.h"
#include "editionsdialog.h"
#include "drawsimulator.h"
#include "inventorymodel.h"
#include "ardb_db_edition_filter.h"

#ifndef __WXMSW__
#  include "icon.xpm"
#endif
#  include "ardbsplash.xpm"

BEGIN_EVENT_TABLE (BrowserFrame, wxFrame)
EVT_MENU (ID_BROWSER_CLOSE_TAB, BrowserFrame::OnBrowserCloseTab)
EVT_MENU (ID_BROWSER_NEW_CRYPT, BrowserFrame::OnBrowserNewCrypt)
EVT_MENU (ID_BROWSER_NEW_LIBRARY, BrowserFrame::OnBrowserNewLibrary)
EVT_MENU (ID_FILE_DECKBUILDER, BrowserFrame::OnFileDeckBuilder)
//EVT_MENU (ID_FILE_EDITIONS, BrowserFrame::OnFileEditions)
EVT_MENU (ID_FILE_PREFERENCES, BrowserFrame::OnFilePreferences)
EVT_MENU (ID_FILE_UPDATEDB, BrowserFrame::OnFileUpdateDatabase)
EVT_MENU (ID_FILE_EXIT, BrowserFrame::OnFileExit)
EVT_MENU (ID_INV_OPEN, BrowserFrame::OnInventoryOpen)
EVT_MENU (ID_INV_SAVE, BrowserFrame::OnInventorySave)
EVT_MENU (ID_INV_IMPORT, BrowserFrame::OnInventoryImport)
EVT_MENU (ID_INV_EXPORT_CSV, BrowserFrame::OnInventoryExportCSV)
EVT_MENU (ID_INV_EXPORT_HTML, BrowserFrame::OnInventoryExportHTML)
EVT_MENU (ID_HELP_ABOUT, BrowserFrame::OnHelpAbout)
EVT_MENU (ID_HELP_MANUAL, BrowserFrame::OnHelpManual)
EVT_CLOSE (BrowserFrame::OnClose)

EVT_NOTEBOOK_PAGE_CHANGED(ID_BROWSER_NOTEBOOK,BrowserFrame::TabChanged)

END_EVENT_TABLE ()

IMPLEMENT_APP (MyApp)


int
MyApp::OnExit ()
{
	return 0;
}


bool
MyApp::OnInit ()
{
	wxSplashScreen* pSplash = NULL;

	::wxInitAllImageHandlers();
	g_pIcon = new wxIcon (wxICON(icon));
	// new stuff which doesn't quite work  g_pSplashBitmap = new wxBitmap (wxBITMAP (ardbsplash));
	g_pSplashBitmap = new wxBitmap (ardbsplash_xpm);

	// Init database
	Database::Instance();

	// config file
	wxFileConfig *pConfig = new wxFileConfig (wxT ("Anarch Revolt Deck Builder"), wxT (""), wxT ("ardb.ini"), wxEmptyString, wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_RELATIVE_PATH);
	wxFileConfig::Set (pConfig);

	// Init interface data
	InterfaceData::Instance();

	wxString sSplashEntry = wxT ("DisplaySplashScreen");
	bool bDisplaySplash;
	if (!pConfig->Read (sSplashEntry, &bDisplaySplash, TRUE))
	{
		pConfig->Write (sSplashEntry, bDisplaySplash);
		pConfig->Flush (TRUE);
	}

	if (bDisplaySplash)
	{
		pSplash = new wxSplashScreen(*g_pSplashBitmap,
			wxSPLASH_CENTRE_ON_SCREEN,
			6000, NULL, -1, wxDefaultPosition, wxDefaultSize,
			wxSIMPLE_BORDER|wxSTAY_ON_TOP);
		wxYield();
	}

	int iHeight;
	int iWidth;
	int iX;
	int iY;

	wxString sBrowserHeightEntry = wxT ("BrowserWindowHeight");
	wxString sBrowserWidthEntry = wxT ("BrowserWindowWidth");

	wxString sBrowserXEntry = wxT("BrowserWindowX");
	wxString sBrowserYEntry = wxT("BrowserWindowY");


	if (!pConfig->Read (sBrowserHeightEntry, &iHeight, 600))
	{
		pConfig->Write (sBrowserHeightEntry, iHeight);
		pConfig->Flush (TRUE);
	}

	if (!pConfig->Read (sBrowserWidthEntry, &iWidth, 800))
	{
		pConfig->Write (sBrowserWidthEntry, iWidth);
		pConfig->Flush (TRUE);
	}

	pConfig->Read (sBrowserXEntry, &iX, -1);
	pConfig->Read (sBrowserYEntry, &iY, -1);

	wxToolTip::SetDelay (2000);

	g_pMainWindow = new BrowserFrame (wxT ("Anarch Revolt - Card Browser"),
		wxPoint(iX,iY), 
		wxSize (iWidth, iHeight));
	SetTopWindow (g_pMainWindow);

	//reset edition filter
	//ardb_db_ef_reset();

	if (pSplash != NULL) delete pSplash;

	return TRUE;
}


BrowserFrame::BrowserFrame (const wxString& title, const wxPoint& pos, const wxSize& size) :
wxFrame (0, -1, title, pos, size),
m_pBrowserCryptModel (NULL),
m_pBrowserLibraryModel (NULL),
m_pPapaSizer (NULL),
m_uiCryptBrowserCount (0),
m_uiLibraryBrowserCount (0)
{
	SetSizeHints (640, 480);

	// create sizer
	m_pPapaSizer = new wxBoxSizer (wxVERTICAL);

	// create Notebook
	m_pNotebook = new wxNotebook(this, ID_BROWSER_NOTEBOOK);
	m_pPapaSizer->Add (m_pNotebook, 3, wxEXPAND);

	SetSizer (m_pPapaSizer);
	//  m_pPapaSizer->Layout ();

	//  menu setup goes here
	wxMenu *pBrowserMenu = new wxMenu ();
	pBrowserMenu->Append (ID_BROWSER_NEW_CRYPT, wxT ("New Crypt browser\tCtrl+Shift+C"), wxT (""));
	pBrowserMenu->Append (ID_BROWSER_NEW_LIBRARY, wxT ("New Library browser\tCtrl+Shift+L"), wxT (""));
	pBrowserMenu->AppendSeparator ();
	pBrowserMenu->Append (ID_BROWSER_CLOSE_TAB, wxT ("Kill current browser\tCtrl+Shift+K"), wxT (""));

	wxMenu *pFileMenu = new wxMenu (); 
	//   pFileMenu->Append (ID_FILE_PREFERENCES, wxT ("Preferences"), wxT (""));
	//   pFileMenu->AppendSeparator () ;
	pFileMenu->Append (ID_FILE_DECKBUILDER, wxT ("Deck Builder\tCtrl+D"), wxT (""));
	pFileMenu->AppendSeparator () ;
	//pFileMenu->Append (ID_FILE_EDITIONS, wxT ("VTES Sets..."), wxT (""));
	pFileMenu->Append (ID_FILE_UPDATEDB, wxT ("Update Database"), wxT (""));
	pFileMenu->AppendSeparator () ;
	pFileMenu->Append (ID_FILE_EXIT, wxT ("Quit\tCtrl+Q"), wxT (""));

	wxMenu *pInventoryMenu = new wxMenu (); 
	pInventoryMenu->Append (ID_INV_OPEN, wxT ("Open"), wxT (""));
	pInventoryMenu->Append (ID_INV_IMPORT, wxT ("Import from (F)ELDB"), wxT (""));
	pInventoryMenu->AppendSeparator () ;
	pInventoryMenu->Append (ID_INV_SAVE, wxT ("Save"), wxT (""));
	pInventoryMenu->Append (ID_INV_EXPORT_HTML, wxT ("Export to HTML"), wxT (""));
	pInventoryMenu->Append (ID_INV_EXPORT_CSV, wxT ("Export for (F)ELDB"), wxT (""));

	wxMenu *pHelpMenu = new wxMenu (); 
	//  pHelpMenu->Append (ID_HELP_MANUAL, wxT ("Manual"), wxT (""));
	pHelpMenu -> Append (ID_HELP_ABOUT, wxT ("About"), wxT (""));

	wxMenuBar *pMenuBar = new wxMenuBar;

	pMenuBar->Append (pFileMenu, wxT ("File"));

#if 0	
	//Opening extra browser windows does not work
	//under windows.
	pMenuBar->Append (pBrowserMenu, wxT ("Browser"));
#endif

	pMenuBar->Append (pInventoryMenu, wxT("Inventory"));
	pMenuBar->Append (pHelpMenu, wxT ("Help"));

	SetMenuBar (pMenuBar); 

	// Add Crypt Browser
	m_pBrowserCryptModel = new BrowserCryptModel (m_pNotebook, m_uiCryptBrowserCount++);
	// Add Library Browser
	m_pBrowserLibraryModel = new BrowserLibraryModel (m_pNotebook, m_uiLibraryBrowserCount++);
	m_pNotebook->SetSelection (0);

	SetIcon (*g_pIcon);

	Show ();
}


BrowserFrame::~BrowserFrame ()
{
	wxFileConfig *pConfig = (wxFileConfig *) wxFileConfig::Get ();
	if (pConfig)
	{
		int h = GetSize().GetHeight();
		int w = GetSize().GetWidth();

		if (h != BAD_HEIGHT_OR_WIDTH_X_OR_Y)
		{
			pConfig->Write (wxT ("BrowserWindowHeight"), h);
		}

		if (w != BAD_HEIGHT_OR_WIDTH_X_OR_Y)
		{
			pConfig->Write (wxT ("BrowserWindowWidth"), w);
		}

		pConfig->Flush (TRUE);
	}
}

void BrowserFrame::TabChanged(wxNotebookEvent &event)
{
	int selectedPage;
	selectedPage = event.GetSelection();

	if (m_pNotebook != NULL)
	{
		wxNotebookPage* pPage = m_pNotebook->GetPage(selectedPage);

		if (pPage != NULL)
		{
			pPage->SetFocus();
		}
	}
}

void
BrowserFrame::OnBrowserCloseTab (wxCommandEvent& WXUNUSED (event))
{
	int iPage;

	iPage = m_pNotebook->GetSelection ();
	if (iPage >= 2)
	{
		m_pNotebook->DeletePage (iPage);
	}

}


void
BrowserFrame::OnBrowserNewCrypt (wxCommandEvent& WXUNUSED (event))
{
	new BrowserCryptModel(m_pNotebook, m_uiCryptBrowserCount++);

}


void
BrowserFrame::OnBrowserNewLibrary (wxCommandEvent& WXUNUSED (event))
{
	new BrowserLibraryModel (m_pNotebook, m_uiLibraryBrowserCount++);
}


void
BrowserFrame::OnClose (wxCloseEvent& WXUNUSED(event))
{

	int iX = 0;
	int iY = 0;
	wxString sBrowserXEntry = wxT("BrowserWindowX");
	wxString sBrowserYEntry = wxT("BrowserWindowY");

	GetPosition(&iX, &iY);

	wxFileConfig *pConfig = (wxFileConfig *) wxFileConfig::Get ();

	if (pConfig)
	{
		if (iX != BAD_HEIGHT_OR_WIDTH_X_OR_Y)
		{
			pConfig->Write(sBrowserXEntry, iX);
		}

		if (iY != BAD_HEIGHT_OR_WIDTH_X_OR_Y)
		{
			pConfig->Write(sBrowserYEntry, iY);
		}

		pConfig->Flush(TRUE);
	}


	Database::DeleteInstance ();
	DeckModel::DeleteInstance ();
	DrawSimulator::DeleteInstance ();
	Updater::DeleteInstance ();
	//EditionsDialog::DeleteInstance ();

	delete g_pIcon;
	delete g_pSplashBitmap;

	Destroy ();
}



void
BrowserFrame::OnFileExit (wxCommandEvent& WXUNUSED (event)) 
{
	Close ();
}


void
BrowserFrame::OnFileDeckBuilder (wxCommandEvent& WXUNUSED (event)) 
{
	DeckModel::Instance ();
}

/*
void
BrowserFrame::OnFileEditions (wxCommandEvent& WXUNUSED (event)) 
{
EditionsDialog *pDialog = EditionsDialog::Instance ();
if (pDialog) pDialog->Show ();
m_pBrowserCryptModel->Reset ();
m_pBrowserLibraryModel->Reset ();
}
*/

void
BrowserFrame::OnFilePreferences (wxCommandEvent& WXUNUSED (event)) 
{
	//Respond to menu here 
}


void
BrowserFrame::OnFileUpdateDatabase (wxCommandEvent& WXUNUSED (event)) 
{
	Updater *pUpdater = Updater::Instance ();
	pUpdater->DoUpdate ();

	m_pBrowserCryptModel->Reset ();
	m_pBrowserLibraryModel->Reset ();
}


void
BrowserFrame::OnHelpManual (wxCommandEvent& WXUNUSED (event)) 
{
	//Respond to menu here 
}



void
BrowserFrame::OnHelpAbout (wxCommandEvent& WXUNUSED (event)) 
{
	wxString about;
	about.Printf(wxT ("The Anarch Revolt Deck Builder\nVersion 2.8\r\nby Francois Gombault & Graham Smith\r\nEmail: graham.r.smith@gmail.com\r\nBuilt: %s"),BUILD_DATE);
	wxMessageBox(about, wxT ("About"));
}


void
BrowserFrame::OnInventoryExportCSV (wxCommandEvent& WXUNUSED (event))
{
	InventoryModel *pInv = InventoryModel::Instance ();

	if (pInv) pInv->ExportToCSV ();
}


void
BrowserFrame::OnInventoryExportHTML (wxCommandEvent& WXUNUSED (event))
{
	InventoryModel *pInv = InventoryModel::Instance ();

	if (pInv) pInv->ExportToHTML ();
}


void
BrowserFrame::OnInventoryImport (wxCommandEvent& WXUNUSED (event))
{
	InventoryModel *pInv = InventoryModel::Instance ();

	if (pInv) 
	{
		if (pInv->ImportFromCSV ())
		{
			m_pBrowserCryptModel->Reset ();
			m_pBrowserLibraryModel->Reset ();
		}
	}
}


void
BrowserFrame::OnInventoryOpen (wxCommandEvent& WXUNUSED (event))
{
	InventoryModel *pInv = InventoryModel::Instance ();

	if (pInv) 
	{
		if (pInv->ImportFromXML ())
		{
			m_pBrowserCryptModel->Reset ();
			m_pBrowserLibraryModel->Reset ();
		}
	}
}


void
BrowserFrame::OnInventorySave (wxCommandEvent& WXUNUSED (event))
{
	InventoryModel *pInv = InventoryModel::Instance ();

	if (pInv) pInv->ExportToXML ();
}


