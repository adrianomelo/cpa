// Testes.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "Setup_CPA.h"
#include <stdio.h>
#include <conio.h>
#include <process.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <io.h>
#include <wchar.h>
#include <sys/stat.h>
#include "Dir.h"

#define MAX_LOADSTRING 100

char UninstallBatch[MAX_PATH];
// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	MainDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	UninstallDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	InstOKDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL FileExists (const char *filePathPtr);
INT_PTR CALLBACK	BemVindoDlgProc(HWND, UINT, WPARAM, LPARAM);
void InitHelpFileName (void);
char szHelpFile[TC_MAX_PATH];
char szHelpFile2[TC_MAX_PATH];
HCURSOR hCurs1 = NULL;
int xPos;
int yPos;
BOOL StatRemoveDirectory (char *lpszDir);
BOOL StatDeleteFile (char *lpszFile);
HFONT hTitleFont = NULL;
BYTE *MapResource (char *resourceType, int resourceId, PDWORD size);
char path[MAX_PATH+20];
ITEMIDLIST *itemList;
char InstallationPath[260];
char SetupFilesDir[260];
char szDestFile[260];
char szOrgFile[260];
char szLinkDir[260];
char szDir[260];
char szTmp[260];
char szTmp2[260];
char * GetLegalNotices ();
HKEY hkey = 0;
char *key;
DWORD dw;
BOOL bOK = FALSE;
BOOL bInstalado;

void GetProgramPath (HWND hwndDlg, char *path);
HRESULT CreateLink (char *lpszPathObj, char *lpszArguments,
	    char *lpszPathLink);

bool CheckVendor(void);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{

  	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	HKEY hkey;
	

	if (! IsUserAnAdmin ())	
    {
	   MessageBoxA(NULL, "Execute o setup como administrador", "Acesso não permitido", MB_ICONHAND);
	   return 0;
    }

	if (!CheckVendor())
	{
	   MessageBoxA(NULL, "Sistema exclusivo para uso em máquinas Itautec.", "Acesso não permitido", MB_ICONHAND);
	   return 0;
	}
    SHGetFolderPath (NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szTmp);
    strcat (szTmp, "\\");
	strcat (szTmp, "ArquivosSeguros.it");				
    if (FileExists(szTmp))
	{
		MessageBox(NULL, "Desinstalação do CPA não pode ser executada!!!\nATENÇÃO: Será necessário que você remova a unidade segura, antes de efetuar a desinstalação do CPA.", "Central de proteção de arquivos", MB_OK | MB_ICONERROR);
	    return 0;
	}

	if (FindWindow(NULL, "Central de proteção de arquivos") != NULL)
	{
		MessageBox(NULL, "Desinstalação do CPA não pode ser executada!!!\nATENÇÃO: Será necessário fechar a central de proteção de arquivos antes de efetuar a desinstalação do CPA.", "Central de proteção de arquivos", MB_OK | MB_ICONERROR);
	    return 0;
	}
	
	/* Setup directory */
		char *s;
		GetModuleFileNameA (NULL, SetupFilesDir, sizeof (SetupFilesDir));
		s = strrchr (SetupFilesDir, '\\');
		if (s)
			s[1] = 0;
	    

    // Default "Program Files" path. 
		SHGetSpecialFolderLocation (NULL, CSIDL_PROGRAM_FILES, &itemList);
		SHGetPathFromIDListA (itemList, path);
		strncat (path, "\\CPA\\", min (strlen("\\CPA\\"), sizeof(path)-strlen(path)-1));
		strncpy (InstallationPath, path, sizeof(InstallationPath)-1);
    // Make sure the path ends with a backslash
	if (InstallationPath [strlen (InstallationPath) - 1] != '\\')
	{
		strcat (InstallationPath, "\\");
	}
    
	//MyRegisterClass(hInstance);
    InitInstance(hInstance, 0);

		// Required for RichEdit text fields to work
	if (LoadLibrary("Riched20.dll") == NULL)
	{
		// This error is fatal e.g. because legal notices could not be displayed
		MessageBox(NULL, "Erro ao carregar contrato de licença. Não encontrada Riched20.dll.", "Central de proteção de arquivos", MB_OK | MB_ICONERROR);
	    return 0;
	}
		
	strcpy(szOrgFile, &InstallationPath[0]);
	strcat(szOrgFile, "CPA.exe");
    bInstalado = FileExists(szOrgFile);
	if (!bInstalado)	      
	    DialogBox (hInstance, MAKEINTRESOURCE (IDD_BEMVINDO), NULL, BemVindoDlgProc);
        //DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), NULL, MainDlgProc); 
	else
	  DialogBox(hInstance, MAKEINTRESOURCE(IDD_DLG_UNINSTALL), NULL, UninstallDlgProc);

	if (UninstallBatch[0])
			{
				STARTUPINFO si;
				PROCESS_INFORMATION pi;

				ZeroMemory (&si, sizeof (si));
				si.cb = sizeof (si);
				si.dwFlags = STARTF_USESHOWWINDOW;
				si.wShowWindow = SW_HIDE;

				if (!CreateProcess (UninstallBatch, NULL, NULL, NULL, FALSE, IDLE_PRIORITY_CLASS, NULL, NULL, &si, &pi))
					DeleteFile (UninstallBatch);
				else
				{
					CloseHandle (pi.hProcess);
					CloseHandle (pi.hThread);
				}
			}

     return 0;

	// TODO: Place code here.
	
	//MSG msg;
	//HACCEL hAccelTable;

	// Initialize global strings
	//LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	//LoadString(hInstance, IDC_TESTES, szWindowClass, MAX_LOADSTRING);
	

	// Perform application initialization:
	//if (!InitInstance (hInstance, nCmdShow))
	//{
	//	return FALSE;
	//}

	//hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTES));

	//// Main message loop:
	//while (GetMessage(&msg, NULL, 0, 0))
	//{
	//	if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
	//	{
	//		TranslateMessage(&msg);
	//		DispatchMessage(&msg);
	//	}
	//}

	//return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	//wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTES));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	//wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_TESTES);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   //hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
   //   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   //if (!hWnd)
   //{
   //   return FALSE;
   //}

   //ShowWindow(hWnd, nCmdShow);
   //UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for MainDlgProc
INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	char *licenseText = NULL;
	DWORD dwError;

	switch (message)
	{
	case WM_INITDIALOG:
    	licenseText = GetLegalNotices ();
		if (licenseText != NULL)
		{
			SetWindowText (GetDlgItem (hDlg, IDC_LICENSE_TEXT), licenseText);
			free (licenseText);
		}
		else
		{
			MessageBox (hDlg, "Não foi possível encontrar o arquivo de licença. Clique no botão CANCELAR para interromper a instalação", "Central de proteção de arquivos", MB_OK|MB_ICONWARNING);
			exit (1);
		}
		
		int nHeight;
	            LOGFONTW lf;	       
			    nHeight = -12;
	            lf.lfHeight = nHeight;
	            lf.lfWidth = 0;
	            lf.lfEscapement = 0;
	            lf.lfOrientation = 0;
	            lf.lfWeight = FW_ULTRABOLD; //FW_NORMAL; //
	            lf.lfItalic = TRUE;
	            lf.lfUnderline = FALSE;
	            lf.lfStrikeOut = FALSE;
	            lf.lfCharSet = DEFAULT_CHARSET;
	            lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	            lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	            lf.lfQuality = PROOF_QUALITY;
	            lf.lfPitchAndFamily = FF_DONTCARE;
	            wcsncpy (lf.lfFaceName, L"Arial", sizeof (lf.lfFaceName)/2);
	            hTitleFont = CreateFontIndirectW (&lf);
			    SendMessage (GetDlgItem (hDlg, IDC_ATENCAO), WM_SETFONT, (WPARAM) hTitleFont, (LPARAM) TRUE);
			    
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK )
		{
			//if (! bInstalado)
			//{
                               
				if (MessageBox (hDlg, "O CPA será instalado no seu computador. Clique em 'OK' para continuar ou em 'CANCELAR' para interromper a operação.", "Central de proteção de arquivos", MB_OKCANCEL|MB_ICONWARNING|MB_DEFBUTTON2) == IDOK)
				{
				    ShowWindow(GetDlgItem (hDlg,IDC_AGUARDE), SW_SHOW);                				    
                    //ShowWindow(GetDlgItem (hDlg,IDC_ACEITO), SW_HIDE); 
                    EnableWindow(GetDlgItem(hDlg, IDOK), false);
                    EnableWindow(GetDlgItem(hDlg, IDOK2), false);
                    EnableWindow(GetDlgItem(hDlg, IDVOLTAR), false);
                    InvalidateRect(hDlg, NULL, true);
                    SendMessage(hDlg, WM_PAINT, NULL, NULL);
                    RedrawWindow(hDlg, NULL, NULL, RDW_UPDATENOW);
					bOK = false;
					//Passo 1: criar pasta
					CreateDirectory(InstallationPath, NULL);		
					dwError = GetLastError ();

					//Passo 2: copiar CPA.EXE
					strcpy(szOrgFile, &SetupFilesDir[0]);
					strcat(szOrgFile, "CPA.exe");
					strcpy(szDestFile,  InstallationPath);
					strcat(&szDestFile[0], "CPA.exe");        
					CopyFileA(szOrgFile, szDestFile, false); 
					dwError = GetLastError (); 

  				   //Passo 3: Copiar TrueCrypt.SYS
					strcpy(szOrgFile, &SetupFilesDir[0]);
					strcat(szOrgFile, "TrueCrypt.SYS");
					strcpy(szDestFile,  InstallationPath);
					strcat(&szDestFile[0], "TrueCrypt.SYS");        
					CopyFileA(szOrgFile, szDestFile, false); 
					dwError = GetLastError (); 

					//Passo 4: Copiar TrueCrypt_x64.SYS
					strcpy(szOrgFile, &SetupFilesDir[0]);
					strcat(szOrgFile, "TrueCrypt-x64.SYS");
					strcpy(szDestFile,  InstallationPath);
					strcat(&szDestFile[0], "TrueCrypt-x64.SYS");        
					CopyFileA(szOrgFile, szDestFile, false); 
					dwError = GetLastError (); 

					//Passo 5: Copiar Guia do usuário
					struct _finddata_t c_file;
					intptr_t hFile;

					strcpy(szTmp, InstallationPath);
                    strcat(szTmp, "Ajuda\\");        
					CreateDirectory(szTmp, NULL);

                    strcpy(szTmp, &SetupFilesDir[0]);
					strcat(szTmp, "Ajuda\\");        
                    strcat(szTmp, "*.*");        
					
					if( (hFile = _findfirst(szTmp, &c_file )) != -1)
					{
						//strcpy(szTmp, &SetupFilesDir[0]);
						//strcat(szTmp, "Ajuda\\");        
						strcpy(szTmp2,  InstallationPath);
						strcat(szTmp2, "Ajuda\\"); 
						do
						{
							if (strlen(c_file.name) > 3) //para pula '.' e '..'
							{
								strcpy(szOrgFile, &SetupFilesDir[0]);
						        strcat(szOrgFile, "Ajuda\\");
								strcat(szOrgFile, c_file.name);
                                strcpy(szDestFile,  szTmp2);
								strcat(szDestFile, c_file.name);        
								CopyFileA(szOrgFile, szDestFile, false); 
							}
						
						} while (_findnext( hFile, &c_file )==0);
						
						_findclose(hFile);
					}
					dwError = GetLastError (); 

					//strcpy(szOrgFile, &SetupFilesDir[0]);
					//strcat(szOrgFile, "CPA - Guia do usuário.pdf");
					//strcpy(szDestFile,  InstallationPath);
					//strcat(&szDestFile[0], "CPA - Guia do usuário.pdf");        
					//CopyFileA(szOrgFile, szDestFile, false); 
					//dwError = GetLastError (); 

					//Passo 6: Copiar Setup_CPA.exe
					strcpy(szOrgFile, &SetupFilesDir[0]);
					strcat(szOrgFile, "Setup_CPA.exe");
					strcpy(szDestFile,  InstallationPath);
					strcat(&szDestFile[0], "Setup_CPA.exe");        
					CopyFileA(szOrgFile, szDestFile, false); 
					dwError = GetLastError (); 

					//Passo 7: Criar link na área de trabalho
					strcpy (szDir, InstallationPath);
					SHGetSpecialFolderPathA (NULL, szLinkDir, CSIDL_DESKTOPDIRECTORY, 0);
					sprintf (szTmp, "%s%s", szDir, "CPA.exe");
					sprintf (szTmp2, "%s%s", szLinkDir, "\\CPA.lnk");
					CoInitialize (NULL);
					CreateLink (szTmp, "", szTmp2);
					//CoUninitialize();
	                
					//Passo 8 - Criar pasta e links no menu Program files
					bool bSlash;
					GetProgramPath (NULL, szLinkDir);
					int x = strlen (szLinkDir);
					if (szLinkDir[x - 1] == '\\')
						bSlash = TRUE;
					else
						bSlash = FALSE;

					if (bSlash == FALSE)
						strcat (szLinkDir, "\\");
					strcat (szLinkDir, "Central de proteção de arquivos");
					FILE *f;

					if (mkfulldir (szLinkDir, TRUE) != 0)
					{
						mkfulldir (szLinkDir, FALSE);
					}
					sprintf (szTmp, "%s%s", szDir, "CPA.exe");
					sprintf (szTmp2, "%s%s", szLinkDir, "\\CPA.lnk");
					CreateLink (szTmp, "", szTmp2);

					sprintf (szTmp, "%s%s", szDir, "Setup_CPA.exe");
					sprintf (szTmp2, "%s%s", szLinkDir, "\\Desinstalar.lnk");
					CreateLink (szTmp, "Desintalar", szTmp2);

					sprintf (szTmp, "%s%s", szDir, "\\Ajuda\\Index.html");
					sprintf (szTmp2, "%s%s", szLinkDir, "\\Ajuda.lnk");
					CreateLink (szTmp, "", szTmp2);

					CoUninitialize();


					//Passo 9 - registrar desinstalador
					key = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\CPA";
					if (RegCreateKeyEx (HKEY_LOCAL_MACHINE,
						key,
						0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, &dw) != ERROR_SUCCESS)
						goto error;

					sprintf (szTmp, "%s%s", szDir, "Setup_CPA.exe");
					if (RegSetValueEx (hkey, "UninstallString", 0, REG_SZ, (BYTE *) szTmp, strlen (szTmp) + 1) != ERROR_SUCCESS)
						goto error;

					strcpy (szTmp, "Central de Proteção de Arquivos");
					if (RegSetValueEx (hkey, "DisplayName", 0, REG_SZ, (BYTE *) szTmp, strlen (szTmp) + 1) != ERROR_SUCCESS)
						goto error;

					strcpy (szTmp, "Itautec");
					if (RegSetValueEx (hkey, "Publisher", 0, REG_SZ, (BYTE *) szTmp, strlen (szTmp) + 1) != ERROR_SUCCESS)
						goto error;

                    ShowWindow(GetDlgItem (hDlg,IDC_AGUARDE), SW_HIDE);
                    InvalidateRect(hDlg, NULL, true);
                    SendMessage(hDlg, WM_PAINT, NULL, NULL);
                    RedrawWindow(hDlg, NULL, NULL, RDW_UPDATENOW);
                    
					DialogBoxW (hInst, MAKEINTRESOURCEW (IDD_DLG_INSTOK), hDlg,(DLGPROC) InstOKDlgProc);
					//MessageBox (hDlg, "PARABÉNS. O programa foi instalado com sucesso.", "Central de proteção de arquivos", MB_OK|MB_ICONINFORMATION);
					bOK = TRUE;
					
				}
				else
				{
					EndDialog(hDlg, LOWORD(wParam));
        		    PostQuitMessage(0);
		        	return (INT_PTR)TRUE;
				}
			//}
			//else //bInstalado = True --> Desinstalar
			//{
			//	if (MessageBox (hDlg, "O programa será desinstalado agora. Clique no botão OK para continuar, e CANCELAR para interromper a desinstalação", "Central de proteção de arquivos", MB_OKCANCEL|MB_ICONWARNING|MB_DEFBUTTON2) == IDOK)
			//	{
			//		bOK = false;
			//	}
			//}
error:
			if (hkey != 0)
				RegCloseKey (hkey);

			if (bOK == FALSE)
				MessageBox (hDlg, "Erro na execução do instalador/desinstalador. Feche o programa e tente novamente.", "Central de proteção de arquivos", MB_OK|MB_ICONINFORMATION);

			EndDialog(hDlg, LOWORD(wParam));
		    PostQuitMessage(0);
			return (INT_PTR)TRUE;
		}

		if (LOWORD(wParam) == IDCANCEL )
		{
			EndDialog(hDlg, LOWORD(wParam));
			PostQuitMessage(0);
			return (INT_PTR)TRUE;
		}
	    if (LOWORD(wParam) == IDC_ACEITO )
		{
			long lresult;
			lresult = SendMessage(GetDlgItem(hDlg, IDC_ACEITO), BM_GETCHECK, 0, 0);
			if(lresult == BST_CHECKED)
			  EnableWindow(GetDlgItem(hDlg, IDOK), true);
			else
			  EnableWindow(GetDlgItem(hDlg, IDOK), false);
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDVOLTAR)
		{		
    		EndDialog (hDlg, IDOK);
    		DialogBox(hInst, MAKEINTRESOURCE(IDD_BEMVINDO), NULL, BemVindoDlgProc);	
			return (INT_PTR)TRUE;
		}

		break;
	}
	return (INT_PTR)FALSE;
}

/*
 * Método: 		BemVindoDlgProc
 * Descrição:	Exibe janela de boas vindas.
 * Data: 		05/02/2010
 * Autor: 		Gustavo R. de Castro
 * Entrada:		hDlg - manipulador da janela
 * 				message - mensagem a ser impressa
 * 				wParam - propriedades da janela
 * Saída:
 */
INT_PTR CALLBACK BemVindoDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	WORD lw = LOWORD (wParam);
	WORD hw = HIWORD (wParam);
   
	switch (message)
	{
	
	    case WM_INITDIALOG:
		    {
			    int nHeight;
	            LOGFONTW lf;	       
			    nHeight = -26;
	            lf.lfHeight = nHeight;
	            lf.lfWidth = 0;
	            lf.lfEscapement = 0;
	            lf.lfOrientation = 0;
	            lf.lfWeight = FW_ULTRABOLD; //FW_NORMAL; //
	            lf.lfItalic = TRUE;
	            lf.lfUnderline = FALSE;
	            lf.lfStrikeOut = FALSE;
	            lf.lfCharSet = DEFAULT_CHARSET;
	            lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	            lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	            lf.lfQuality = PROOF_QUALITY;
	            lf.lfPitchAndFamily = FF_DONTCARE;
	            wcsncpy (lf.lfFaceName, L"Arial", sizeof (lf.lfFaceName)/2);
	            hTitleFont = CreateFontIndirectW (&lf);
			    SendMessage (GetDlgItem (hDlg, IDC_TITULO_INST), WM_SETFONT, (WPARAM) hTitleFont, (LPARAM) TRUE);			
    			
			    nHeight = -12;
	            lf.lfHeight = nHeight;
	            lf.lfWidth = 0;
	            lf.lfEscapement = 0;
	            lf.lfOrientation = 0;
	            lf.lfWeight = FW_BOLD; //FW_NORMAL; //
	            lf.lfItalic = TRUE;
	            lf.lfUnderline = TRUE;
	            lf.lfStrikeOut = FALSE;
	            lf.lfCharSet = DEFAULT_CHARSET;
	            lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	            lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	            lf.lfQuality = PROOF_QUALITY;
	            lf.lfPitchAndFamily = FF_DONTCARE;
	            wcsncpy (lf.lfFaceName, L"Arial", sizeof (lf.lfFaceName)/2);
	            hTitleFont = CreateFontIndirectW (&lf);
			    SendMessage (GetDlgItem (hDlg, IDC_LINKMANUAL), WM_SETFONT, (WPARAM) hTitleFont, (LPARAM) TRUE);											
			    SetFocus (GetDlgItem (hDlg, IDOK));						
		        return (INT_PTR)TRUE;
		    }       	
	    case WM_COMMAND:
	    {
		    if (lw == IDCANCEL)
		    {
			    EndDialog (hDlg, IDCANCEL);
			    return 1;
		    }		
		    if (lw == IDOK)
		    {	
		        EndDialog (hDlg, IDOK);
		        DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), NULL, MainDlgProc);	
		        return 1;
		    }
		    if (lw == IDC_LINKMANUAL)
		    {
    	        InitHelpFileName();
					    //OpenPageHelp (hwndDlg, nCurPageNo);
					    int r = (int)ShellExecute (NULL, "open", szHelpFile, NULL, NULL, SW_SHOWNORMAL);
 					    if (r == ERROR_FILE_NOT_FOUND)
					    {
						    r = (int)ShellExecute (NULL, "open", szHelpFile2, NULL, NULL, SW_SHOWNORMAL);
						    if (r == ERROR_FILE_NOT_FOUND)
						      MessageBox(NULL, "Arquivo de ajuda não encontrado", "Central de proteção de arquivos", MB_OK | MB_ICONSTOP);
					    }
					    return 1;
		    }
		    return (INT_PTR)TRUE;
	    }
	    break;
	    default:
	    	break;
	}
	return (INT_PTR)FALSE;
}

HRESULT CreateLink (char *lpszPathObj, char *lpszArguments,
	    char *lpszPathLink)
{
	HRESULT hres;
	IShellLink *psl;

	/* Get a pointer to the IShellLink interface.  */
	hres = CoCreateInstance (CLSID_ShellLink, NULL,
			       CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *) &psl);
	if (SUCCEEDED (hres))
	{
		IPersistFile *ppf;

		/* Set the path to the shortcut target, and add the
		   description.  */
		psl->SetPath (lpszPathObj);
		psl->SetArguments (lpszArguments);

		/* Query IShellLink for the IPersistFile interface for saving
		   the shortcut in persistent storage.  */
		hres = psl->QueryInterface (IID_IPersistFile,
						    (void **) &ppf);

		if (SUCCEEDED (hres))
		{
			wchar_t wsz[260];

			/* Ensure that the string is ANSI.  */
			MultiByteToWideChar (CP_ACP, 0, lpszPathLink, -1,
					     wsz, sizeof(wsz) / sizeof(wsz[0]));

			/* Save the link by calling IPersistFile::Save.  */
			hres = ppf->Save (wsz, TRUE);
			ppf->Release ();
		}
		psl->Release ();
	}
	DWORD dwError = GetLastError();
	return hres;
}

char * GetLegalNotices ()
{
	static char *resource1;
	static char *resource2;
	static DWORD size1;
	static DWORD size2;

	char *buf = NULL;

	if (resource1 == NULL)
	{
		resource1 = (char *) MapResource ("Text", IDR_LICENSE_CPA, &size1);
		resource2 = (char *) MapResource ("Text", IDR_LICENSE, &size2);
	}

	if (resource1 != NULL)
	{
		buf = (char *) malloc (size1 + size2 + 1);
		if (buf != NULL)
		{
			memcpy (buf, resource1, size1);
			memcpy (&buf[size1], resource2, size2);
			buf[size1 + size2] = 0;
		}
	}

	return buf;
}

BYTE *MapResource (char *resourceType, int resourceId, PDWORD size)
{
	HGLOBAL hResL; 
    HRSRC hRes;

	hRes = FindResource (NULL, MAKEINTRESOURCE(resourceId), resourceType);
	hResL = LoadResource (NULL, hRes);

	if (size != NULL)
		*size = SizeofResource (NULL, hRes);
  
	return (BYTE *) LockResource (hResL);
}

// Message handler for MainDlgProc
INT_PTR CALLBACK UninstallDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	char *licenseText = NULL;
	DWORD dwError;
	char temp[260];
	FILE *f;

	switch (message)
	{
	case WM_INITDIALOG:
		UninstallBatch[0] = '\0';//para não executar batch de exclusão
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK )
		{
			bOK = false;
		    if (MessageBox (hDlg, "O CPA será desinstalado do seu computador. Clique em 'OK' para continuar ou em 'CANCELAR' para interromper a operação.", "Central de proteção de arquivos", MB_OKCANCEL|MB_ICONWARNING|MB_DEFBUTTON2) == IDOK)
			{
								                				
				GetTempPath (sizeof (temp), temp);
				_snprintf (UninstallBatch, sizeof (UninstallBatch), "%s\\CPA-Uninstall.bat", temp);

				UninstallBatch [sizeof(UninstallBatch)-1] = 0;

				// Create uninstall batch
				f = fopen (UninstallBatch, "w");
				if (!f)
					bOK = FALSE;
				else
				{
					fprintf (f, ":loop\n"
						"attrib -R \"%s%s\"\n"
						"del \"%s%s\"\n"
						"if exist \"%s%s\" goto loop\n"
						"rmdir \"%s\"\n"
						"del \"%s\"",
						InstallationPath, "Setup_CPA.exe",
						InstallationPath, "Setup_CPA.exe",
						InstallationPath, "Setup_CPA.exe",
						InstallationPath,
						UninstallBatch
						);
					fclose (f);
				}

				//Passo 8 - Remover Program folder
				bool bSlash;
				GetProgramPath (NULL, szLinkDir);
				int x = strlen (szLinkDir);
				if (szLinkDir[x - 1] == '\\')
					bSlash = TRUE;
				else
					bSlash = FALSE;
				if (bSlash == FALSE)
						strcat (szLinkDir, "\\");
				strcat (szLinkDir, "Central de proteção de arquivos");
                
				sprintf (szTmp2, "%s%s", szLinkDir, "\\CPA.lnk");                
				SetFileAttributes( szTmp2, FILE_ATTRIBUTE_NORMAL); 
				DeleteFile (szTmp2);
				sprintf (szTmp2, "%s%s", szLinkDir, "\\Desinstalar.lnk");                
				SetFileAttributes( szTmp2, FILE_ATTRIBUTE_NORMAL); 
				DeleteFile (szTmp2);
				sprintf (szTmp2, "%s%s", szLinkDir, "\\Ajuda.lnk");                
				SetFileAttributes( szTmp2, FILE_ATTRIBUTE_NORMAL); 
				DeleteFile (szTmp2);				
				RemoveDirectory (szLinkDir);
				//{
				//	handleWin32Error (hwndDlg);
				//	bOK = FALSE;
				//}

				//Passo 7: REMOVER link na área de trabalho
				SHGetSpecialFolderPathA (NULL, szLinkDir, CSIDL_DESKTOPDIRECTORY, 0);
				sprintf (szTmp2, "%s%s", szLinkDir, "\\CPA.lnk");
				SetFileAttributes( szTmp2, FILE_ATTRIBUTE_NORMAL); 
				StatDeleteFile (szTmp2); 

				//Passo 9: REMOVER desinstaldor da lista de programas do Windows
				RegDeleteKey (HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\CPA");

				//Passo 6: REMOVER Setup_CPA.exe
				//strcpy(szDestFile,  InstallationPath);
				//strcat(&szDestFile[0], "Setup_CPA.exe");        
				//StatDeleteFile (szDestFile);	

				//Passo 5: REMOVER Guia do usuário
  				struct _finddata_t c_file;
				intptr_t hFile;
				strcpy(szDestFile,  InstallationPath);
				strcat(szDestFile, "Ajuda\\");        
					
                strcpy(szTmp, szDestFile);
                strcat(szTmp, "*.*");        
					
					if( (hFile = _findfirst(szTmp, &c_file )) != -1)
					{
						do
						{
							if (strlen(c_file.name) > 3) //para pula '.' e '..'
 							{
				                strcpy(szDestFile,  InstallationPath);
				                strcat(szDestFile, "Ajuda\\");        
								strcat(szDestFile, c_file.name);   
								SetFileAttributes( szDestFile, FILE_ATTRIBUTE_NORMAL); 
								StatDeleteFile (szDestFile);						
							}
						
						} while (_findnext( hFile, &c_file )==0);
						
						_findclose(hFile);
					}
				strcpy(szDestFile,  InstallationPath);
				strcat(szDestFile, "Ajuda"); 
                StatRemoveDirectory (szDestFile); //remover pasta ajuda
                
				//Passo 4: REMOVER TrueCrypt_x64.SYS
				strcpy(szDestFile,  InstallationPath);
				strcat(&szDestFile[0], "TrueCrypt-x64.SYS"); 
				SetFileAttributes( szDestFile, FILE_ATTRIBUTE_NORMAL); 
				StatDeleteFile (szDestFile);						

				//Passo 3: REMOVER TrueCrypt.SYS
				strcpy(szDestFile,  InstallationPath);
				strcat(&szDestFile[0], "TrueCrypt.SYS");        
				SetFileAttributes( szDestFile, FILE_ATTRIBUTE_NORMAL); 
				StatDeleteFile (szDestFile);
				
				//Passo 2: REMOVER CPA.EXE
				strcpy(szDestFile,  InstallationPath);
				strcat(&szDestFile[0], "CPA.exe");        
				SetFileAttributes( szDestFile, FILE_ATTRIBUTE_NORMAL); 
				StatDeleteFile (szDestFile);

				//Passo 2: REMOVER Configuration.XML
				strcpy(szDestFile,  InstallationPath);
				strcat(&szDestFile[0], "Configuration.XML");        
				SetFileAttributes( szDestFile, FILE_ATTRIBUTE_NORMAL); 
				StatDeleteFile (szDestFile);

				//Passo 1: REMOVER pasta
				//InstallationPath[strlen(InstallationPath) - 1] = 0x00; //remover "/" do final
				//StatRemoveDirectory (InstallationPath);
				
				bOK = true;
			    MessageBox (hDlg, "Central de proteção de arquivos REMOVIDA com sucesso.", "Central de proteção de arquivos", MB_OK|MB_ICONINFORMATION);
			}
			else
				UninstallBatch[0] = '\0';//para não executar batch de exclusão

			EndDialog(hDlg, LOWORD(wParam));
		    PostQuitMessage(0);
			return (INT_PTR)TRUE;
		}

		if (LOWORD(wParam) == IDCANCEL )
		{
			EndDialog(hDlg, LOWORD(wParam));
			PostQuitMessage(0);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// Returns TRUE if the file exists (otherwise FALSE).
BOOL FileExists (const char *filePathPtr)
{
	char filePath [260];

	// Strip quotation marks (if any)
	if (filePathPtr [0] == '"')
	{
		strcpy (filePath, filePathPtr + 1);
	}
	else
	{
		strcpy (filePath, filePathPtr);
	}

	// Strip quotation marks (if any)
	if (filePath [strlen (filePath) - 1] == '"')
		filePath [strlen (filePath) - 1] = 0;

    return (_access (filePath, 0) != -1);
}

BOOL StatDeleteFile (char *lpszFile)
{
	struct __stat64 st;

	if (_stat64 (lpszFile, &st) == 0)
		return DeleteFile (lpszFile);
	else
		return TRUE;
}

BOOL StatRemoveDirectory (char *lpszDir)
{
	struct __stat64 st;

	if (_stat64 (lpszDir, &st) == 0)
		return RemoveDirectory (lpszDir);
	else
		return TRUE;
}

/*
BOOL DoShortcutsInstall (HWND hwndDlg, char *szDestDir, BOOL bProgGroup, BOOL bDesktopIcon)
{
	char szLinkDir[TC_MAX_PATH], szDir[TC_MAX_PATH];
	char szTmp[TC_MAX_PATH], szTmp2[TC_MAX_PATH], szTmp3[TC_MAX_PATH];
	BOOL bSlash, bOK = FALSE;
	HRESULT hOle;
	int x;

	if (bProgGroup == FALSE && bDesktopIcon == FALSE)
 		return TRUE;

	hOle = OleInitialize (NULL);

	GetProgramPath (hwndDlg, szLinkDir);

	x = strlen (szLinkDir);
	if (szLinkDir[x - 1] == '\\')
		bSlash = TRUE;
	else
		bSlash = FALSE;

	if (bSlash == FALSE)
		strcat (szLinkDir, "\\");

	strcat (szLinkDir, "TrueCrypt");

	strcpy (szDir, szDestDir);
	x = strlen (szDestDir);
	if (szDestDir[x - 1] == '\\')
		bSlash = TRUE;
	else
		bSlash = FALSE;

	if (bSlash == FALSE)
		strcat (szDir, "\\");

	if (bProgGroup)
	{
		FILE *f;

		if (mkfulldir (szLinkDir, TRUE) != 0)
		{
			if (mkfulldir (szLinkDir, FALSE) != 0)
			{
				wchar_t szTmp[TC_MAX_PATH];

				handleWin32Error (hwndDlg);
				wsprintfW (szTmp, GetString ("CANT_CREATE_FOLDER"), szLinkDir);
				MessageBoxW (hwndDlg, szTmp, lpszTitle, MB_ICONHAND);
				goto error;
			}
		}

		sprintf (szTmp, "%s%s", szDir, "TrueCrypt.exe");
		sprintf (szTmp2, "%s%s", szLinkDir, "\\TrueCrypt.lnk");

		IconMessage (hwndDlg, szTmp2);
		if (CreateLink (szTmp, "", szTmp2) != S_OK)
			goto error;

		sprintf (szTmp2, "%s%s", szLinkDir, "\\TrueCrypt Website.url");
		IconMessage (hwndDlg, szTmp2);
		f = fopen (szTmp2, "w");
		if (f)
		{
			fprintf (f, "[InternetShortcut]\nURL=%s&dest=index\n", TC_APPLINK);
			fclose (f);
		}
		else
			goto error;

		sprintf (szTmp, "%s%s", szDir, "TrueCrypt Setup.exe");
		sprintf (szTmp2, "%s%s", szLinkDir, "\\Uninstall TrueCrypt.lnk");
		strcpy (szTmp3, "/u");

		IconMessage (hwndDlg, szTmp2);
		if (CreateLink (szTmp, szTmp3, szTmp2) != S_OK)
			goto error;

		sprintf (szTmp2, "%s%s", szLinkDir, "\\TrueCrypt User's Guide.lnk");
		DeleteFile (szTmp2);
	}

	if (bDesktopIcon)
	{
		strcpy (szDir, szDestDir);
		x = strlen (szDestDir);
		if (szDestDir[x - 1] == '\\')
			bSlash = TRUE;
		else
			bSlash = FALSE;

		if (bSlash == FALSE)
			strcat (szDir, "\\");

		if (bForAllUsers)
			SHGetSpecialFolderPath (hwndDlg, szLinkDir, CSIDL_COMMON_DESKTOPDIRECTORY, 0);
		else
			SHGetSpecialFolderPath (hwndDlg, szLinkDir, CSIDL_DESKTOPDIRECTORY, 0);

		sprintf (szTmp, "%s%s", szDir, "TrueCrypt.exe");
		sprintf (szTmp2, "%s%s", szLinkDir, "\\TrueCrypt.lnk");

		IconMessage (hwndDlg, szTmp2);

		if (CreateLink (szTmp, "", szTmp2) != S_OK)
			goto error;
	}

	bOK = TRUE;

error:
	OleUninitialize ();

	return bOK;
}
*/
void GetProgramPath (HWND hwndDlg, char *path)
{
	ITEMIDLIST *i;
	HRESULT res;

	//if (bForAllUsers)
    //    res = SHGetSpecialFolderLocation (hwndDlg, CSIDL_COMMON_PROGRAMS, &i);
	//else
        res = SHGetSpecialFolderLocation (hwndDlg, CSIDL_PROGRAMS, &i);

	SHGetPathFromIDList (i, path);
}

/*
 * Método: 		InstOKDlgProc
 * Descrição:	Except in response to the WM_INITDIALOG message, the dialog box procedure
 * 				should return nonzero if it processes the message, and zero if it does
 * 				not. - see DialogProc
 * Data: 		05/02/2010
 * Autor: 		Gustavo R. de Castro
 * Entrada:		hwndDlg - ponteiro da janela
 * 				msg - mensagem a ser impressa
 * 				wParam - propriedades da janela
 *
 */
static LRESULT CALLBACK InstOKDlgProc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WORD lw = LOWORD (wParam);
	WORD hw = HIWORD (wParam);
   
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			SetFocus (GetDlgItem (hwndDlg, IDOK));
		    break;
		}       
	case WM_COMMAND:
		if (lw == IDCANCEL)
		{
			EndDialog (hwndDlg, IDCANCEL);
			return 1;
		}		
		if (lw == IDOK)
		{		
		    strcpy (szDir, InstallationPath);
		    sprintf (szTmp, "%s%s", szDir, "CPA.exe");
		    //LONG r = ShellExecute(NULL, "open", "http://www.microsoft.com", NULL, NULL, SW_SHOWNORMAL);
			WinExec(szTmp, SW_SHOWNORMAL);		
    		EndDialog (hwndDlg, IDOK);
			return 1;
		}		
		break;
	default:
		break;
	}
	return 0;
}

/*
 * Método: 		InitHelpFileName
 * Descrição:	Cria arquivo de ajuda.
 * Autor:		Gustavo R. de Castro
 * Data:		05/02/2010
 */
void InitHelpFileName (void)
{
	char *lpszTmp;

	GetModuleFileName (NULL, szHelpFile, sizeof (szHelpFile));
	lpszTmp = strrchr (szHelpFile, '\\');
	if (lpszTmp)
	{
		char szTemp[TC_MAX_PATH];

		// Primary file name
		//if (strcmp (GetPreferredLangId(), "en") == 0
		//	|| GetPreferredLangId() == NULL)
		//{
			//strcpy (++lpszTmp, "CPA - Guia do usuário.pdf");
			strcpy (++lpszTmp, "Ajuda\\Index.html");
		//}
		//else
		//{
		//	sprintf (szTemp, "TrueCrypt User Guide.%s.pdf", GetPreferredLangId());
		//	strcpy (++lpszTmp, szTemp);
		//}

		// Secondary file name (used when localized documentation is not found).
		GetModuleFileName (NULL, szHelpFile2, sizeof (szHelpFile2));
		lpszTmp = strrchr (szHelpFile2, '\\');
		if (lpszTmp)
		{
			strcpy (++lpszTmp, "CPA - Guia do usuário.pdf");
			//strcpy (++lpszTmp, "CPA - Guia do usuário.hta");
		}
	}
}
