///////////////////////////////////////////////////////////////////////////////////////////////////////////// \file TcQformat.c
/// \par Contents:
/// Ponto de entrada do aplicativo
/// \class 
/// \brief junta todas as funcionalidades no mesmo arquivo -seguindo modelo original do Truecrypt
/// \author Gustavo Castro
/// 
/// \version 1.0
/// \date 12-2008
/// 
/// \warning Deve ser executado como administrador, caso contrário não será possível
///          alterar o label do volume criado para UNID_SEGURA.
/// \note  Foi copiado o wizard para se montar um novo volume (projeto FORMAT).
///        Deste Wizard foram deixados somente os passos: INTRO_PAGE, PASSWORD_PAGE e FORAMT_PAGE 
///        Todos os demais passos (caixas de diálogo) foram bypassadas, assumindo-se valores padrão.
///        Algumas funções foram copiadas do projto MOUNT, pois o projeto Format apenas cria um novo
///        volume e não possuia as funções necessárias para se montar e liberar este volume.
///        Também foram desativadas as funções de chamadas de strings do arquivo XML (assim solicitado)
///        colocou-se as strings de mensagens fixas no código.
///        
//////////////////////////////////////////////////////////////////////////////////////////////////////////



/*
 Legal Notice: Some portions of the source code contained in this file were
 derived from the source code of Encryption for the Masses 2.02a, which is
 Copyright (c) 1998-2000 Paul Le Roux and which is governed by the 'License
 Agreement for Encryption for the Masses'. Modifications and additions to
 the original source code (contained in this file) and all other portions of
 this file are Copyright (c) 2003-2008 TrueCrypt Foundation and are governed
 by the TrueCrypt License 2.6 the full text of which is contained in the
 file License.txt included in TrueCrypt binary and source code distribution
 packages. */

#include "Tcdefs.h"

#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <errno.h>
#include <io.h>
#include <sys/stat.h>
#include <shlobj.h>
#include <string.h>
#include <Windows.h>
#include <WinBase.h>

#include "Crypto.h"
#include "Apidrvr.h"
#include "Dlgcode.h"
#include "Language.h"
#include "Combo.h"
#include "Registry.h"
#include "Boot/Windows/BootDefs.h"
#include "Common/Common.h"
#include "Common/BootEncryption.h"
#include "Common/Dictionary.h"
#include "Common/Endian.h"
#include "Common/resource.h"
#include "Platform/Finally.h"
#include "Platform/ForEach.h"
#include "Random.h"
#include "Fat.h"
#include "InPlace.h"
#include "resource.h"
#include "TcFormat.h"
#include "Format.h"
#include "FormatCom.h"
#include "Password.h"
#include "Progress.h"
#include "Tests.h"
#include "Cmdline.h"
#include "Volumes.h"
#include "Wipe.h"
#include "Xml.h"
#include "QMount.h"
#include "Dbt.h"
#include "Pkcs5.h"

using namespace TrueCrypt;

#define TC_ALERTA_SENHA	"ATENÇÃO: Cuidado para não perder a senha, pois não existe nenhuma maneira de recuperá-la. O acesso aos arquivos, que fazem parte da unidade de arquivos protegidos, depende exclusivamente da senha de acesso.", "Central de proteção de arquivos"

enum wizard_pages
{
	/* IMPORTANT: IF YOU ADD/REMOVE/MOVE ANY PAGES THAT ARE RELATED TO SYSTEM ENCRYPTION,
	REVISE THE 'DECOY_OS_INSTRUCTIONS' STRING! */

	INTRO_PAGE,
			SYSENC_TYPE_PAGE,
					SYSENC_HIDDEN_OS_REQ_CHECK_PAGE,
			SYSENC_SPAN_PAGE,
			SYSENC_PRE_DRIVE_ANALYSIS_PAGE,
			SYSENC_DRIVE_ANALYSIS_PAGE,
			SYSENC_MULTI_BOOT_MODE_PAGE,
			SYSENC_MULTI_BOOT_SYS_EQ_BOOT_PAGE,
			SYSENC_MULTI_BOOT_NBR_SYS_DRIVES_PAGE,
			SYSENC_MULTI_BOOT_ADJACENT_SYS_PAGE,
			SYSENC_MULTI_BOOT_NONWIN_BOOT_LOADER_PAGE,
			SYSENC_MULTI_BOOT_OUTCOME_PAGE,
	VOLUME_TYPE_PAGE,
				HIDDEN_VOL_WIZARD_MODE_PAGE,
	VOLUME_LOCATION_PAGE,
		DEVICE_TRANSFORM_MODE_PAGE,
				HIDDEN_VOL_HOST_PRE_CIPHER_PAGE,
				HIDDEN_VOL_PRE_CIPHER_PAGE,
	CIPHER_PAGE,
	SIZE_PAGE,
				HIDDEN_VOL_HOST_PASSWORD_PAGE,
	PASSWORD_PAGE,
		FILESYS_PAGE,
			SYSENC_COLLECTING_RANDOM_DATA_PAGE,
			SYSENC_KEYS_GEN_PAGE,
			SYSENC_RESCUE_DISK_CREATION_PAGE,
			SYSENC_RESCUE_DISK_BURN_PAGE,
			SYSENC_RESCUE_DISK_VERIFIED_PAGE,
			SYSENC_WIPE_MODE_PAGE,
			SYSENC_PRETEST_INFO_PAGE,
			SYSENC_PRETEST_RESULT_PAGE,
			SYSENC_ENCRYPTION_PAGE,
		NONSYS_INPLACE_ENC_RESUME_PASSWORD_PAGE,
		NONSYS_INPLACE_ENC_RESUME_PARTITION_SEL_PAGE,
		NONSYS_INPLACE_ENC_RAND_DATA_PAGE,
		NONSYS_INPLACE_ENC_WIPE_MODE_PAGE,
		NONSYS_INPLACE_ENC_ENCRYPTION_PAGE,
		NONSYS_INPLACE_ENC_ENCRYPTION_FINISHED_PAGE,
	FORMAT_PAGE,
	FORMAT_FINISHED_PAGE,
					SYSENC_HIDDEN_OS_INITIAL_INFO_PAGE,
					SYSENC_HIDDEN_OS_WIPE_INFO_PAGE,
						DEVICE_WIPE_MODE_PAGE,
						DEVICE_WIPE_PAGE
};

#define TIMER_INTERVAL_RANDVIEW							30
#define TIMER_INTERVAL_SYSENC_PROGRESS					30
#define TIMER_INTERVAL_NONSYS_INPLACE_ENC_PROGRESS		30
#define TIMER_INTERVAL_SYSENC_DRIVE_ANALYSIS_PROGRESS	100
#define TIMER_INTERVAL_WIPE_PROGRESS					30
#define TIMER_INTERVAL_KEYB_LAYOUT_GUARD				10

enum sys_encryption_cmd_line_switches
{
	SYSENC_COMMAND_NONE = 0,
	SYSENC_COMMAND_RESUME,
	SYSENC_COMMAND_STARTUP_SEQ_RESUME,
	SYSENC_COMMAND_ENCRYPT,
	SYSENC_COMMAND_DECRYPT,
	SYSENC_COMMAND_CREATE_HIDDEN_OS,
	SYSENC_COMMAND_CREATE_HIDDEN_OS_ELEV
};

typedef struct 
{
	int NumberOfSysDrives;			// Number of drives that contain an operating system. -1: unknown, 1: one, 2: two or more
	int MultipleSystemsOnDrive;		// Multiple systems are installed on the drive where the currently running system resides.  -1: unknown, 0: no, 1: yes
	int BootLoaderLocation;			// Boot loader (boot manager) installed in: 1: MBR/1st cylinder, 0: partition/bootsector: -1: unknown
	int BootLoaderBrand;			// -1: unknown, 0: Microsoft Windows, 1: any non-Windows boot manager/loader
	int SystemOnBootDrive;			// If the currently running operating system is installed on the boot drive. -1: unknown, 0: no, 1: yes
} SYSENC_MULTIBOOT_CFG;

#define SYSENC_PAUSE_RETRY_INTERVAL		100
#define SYSENC_PAUSE_RETRIES			200

// Expected duration of system drive analysis, in ms 
#define SYSENC_DRIVE_ANALYSIS_ETA		(4*60000)

BootEncryption			*BootEncObj = NULL;
BootEncryptionStatus	BootEncStatus;

HWND hCurPage = NULL;		/* Handle to current wizard page */
int nCurPageNo = -1;		/* The current wizard page */
int nLastPageNo = -1;
volatile int WizardMode = DEFAULT_VOL_CREATION_WIZARD_MODE; /* IMPORTANT: Never change this value directly -- always use ChangeWizardMode() instead. */
volatile BOOL bHiddenOS = FALSE;		/* If TRUE, we are performing or (or supposed to perform) actions relating to an operating system installed in a hidden volume (i.e., encrypting a decoy OS partition or creating the outer/hidden volume for the hidden OS). To determine or set the phase of the process, call ChangeHiddenOSCreationPhase() and DetermineHiddenOSCreationPhase()) */
BOOL bDirectSysEncMode = FALSE;
BOOL bDirectSysEncModeCommand = SYSENC_COMMAND_NONE;
BOOL DirectDeviceEncMode = FALSE;
BOOL DirectNonSysInplaceEncResumeMode = FALSE;
BOOL DirectPromptNonSysInplaceEncResumeMode = FALSE;
volatile BOOL bInPlaceEncNonSys = FALSE;		/* If TRUE, existing data on a non-system partition/volume are to be encrypted (for system encryption, this flag is ignored) */
volatile BOOL bInPlaceEncNonSysResumed = FALSE;	/* If TRUE, the wizard is supposed to resume (or has resumed) process of non-system in-place encryption. */
volatile BOOL bFirstNonSysInPlaceEncResumeDone = FALSE;
__int64 NonSysInplaceEncBytesDone = 0;
__int64 NonSysInplaceEncTotalSectors = 0;
BOOL bDeviceTransformModeChoiceMade = FALSE;		/* TRUE if the user has at least once manually selected the 'in-place' or 'format' option (on the 'device transform mode' page). */
int nNeedToStoreFilesOver4GB = 0;		/* Whether the user wants to be able to store files larger than 4GB on the volume: -1 = Undecided or error, 0 = No, 1 = Yes */
int nVolumeEA = 1;			/* Default encryption algorithm */
BOOL bSystemEncryptionInProgress = FALSE;		/* TRUE when encrypting/decrypting the system partition/drive (FALSE when paused). */
BOOL bWholeSysDrive = FALSE;	/* Whether to encrypt the entire system drive or just the system partition. */
static BOOL bSystemEncryptionStatusChanged = FALSE;   /* TRUE if this instance changed the value of SystemEncryptionStatus (it's set to FALSE each time the system encryption settings are saved to the config file). This value is to be treated as protected -- only the wizard can change this value (others may only read it). */
volatile BOOL bSysEncDriveAnalysisInProgress = FALSE;
volatile BOOL bSysEncDriveAnalysisTimeOutOccurred = FALSE;
int SysEncDetectHiddenSectors = -1;		/* Whether the user wants us to detect and encrypt the Host Protect Area (if any): -1 = Undecided or error, 0 = No, 1 = Yes */
int SysEncDriveAnalysisStart;
BOOL bDontVerifyRescueDisk = FALSE;
BOOL bFirstSysEncResumeDone = FALSE;
int nMultiBoot = 0;			/* The number of operating systems installed on the computer, according to the user. 0: undetermined, 1: one, 2: two or more */
volatile BOOL bHiddenVol = FALSE;	/* If true, we are (or will be) creating a hidden volume. */
volatile BOOL bHiddenVolHost = FALSE;	/* If true, we are (or will be) creating the host volume (called "outer") for a hidden volume. */
volatile BOOL bHiddenVolDirect = FALSE;	/* If true, the wizard omits creating a host volume in the course of the process of hidden volume creation. */
volatile BOOL bHiddenVolFinished = FALSE;
int hiddenVolHostDriveNo = -1;	/* Drive letter for the volume intended to host a hidden volume. */
BOOL bRemovableHostDevice = FALSE;	/* TRUE when creating a device/partition-hosted volume on a removable device. State undefined when creating file-hosted volumes. */
int realClusterSize;		/* Parameter used when determining the maximum possible size of a hidden volume. */
int hash_algo = DEFAULT_HASH_ALGORITHM;	/* Which PRF to use in header key derivation (PKCS #5) and in the RNG. */
unsigned __int64 nUIVolumeSize = 0;		/* The volume size. Important: This value is not in bytes. It has to be multiplied by nMultiplier. Do not use this value when actually creating the volume (it may chop off 512 bytes, if it is not a multiple of 1024 bytes). */
unsigned __int64 nVolumeSize = 0;		/* The volume size, in bytes. */
unsigned __int64 nHiddenVolHostSize = 0;	/* Size of the hidden volume host, in bytes */
__int64 nMaximumHiddenVolSize = 0;		/* Maximum possible size of the hidden volume, in bytes */
__int64 nbrFreeClusters = 0;
int nMultiplier = BYTES_PER_MB;		/* Size selection multiplier. */
char szFileName[TC_MAX_PATH+1];	/* The file selected by the user */
char szDiskFile[TC_MAX_PATH+1];	/* Fully qualified name derived from szFileName */
char szRescueDiskISO[TC_MAX_PATH+1];	/* The filename and path to the Rescue Disk ISO file to be burned (for boot encryption) */
BOOL bDeviceWipeInProgress = FALSE;
volatile BOOL bTryToCorrectReadErrors = FALSE;

volatile BOOL bVolTransformThreadCancel = FALSE;	/* TRUE if the user cancels/pauses volume encryption/format */
volatile BOOL bVolTransformThreadRunning = FALSE;	/* Is the volume encryption/format thread running */
volatile BOOL bVolTransformThreadToRun = FALSE;		/* TRUE if the Format/Encrypt button has been clicked and we are proceeding towards launching the thread. */

volatile BOOL bConfirmQuit = FALSE;		/* If TRUE, the user is asked to confirm exit when he clicks the X icon, Exit, etc. */
volatile BOOL bConfirmQuitSysEncPretest = FALSE;

BOOL bDevice = FALSE;		/* Is this a partition volume ? */

BOOL showKeys = TRUE;
volatile HWND hMasterKey = NULL;		/* Text box showing hex dump of the master key */
volatile HWND hHeaderKey = NULL;		/* Text box showing hex dump of the header key */
volatile HWND hRandPool = NULL;		/* Text box showing hex dump of the random pool */
volatile HWND hRandPoolSys = NULL;	/* Text box showing hex dump of the random pool for system encryption */
volatile HWND hPasswordInputField = NULL;	/* Password input field */
volatile HWND hVerifyPasswordInputField = NULL;		/* Verify-password input field */

HBITMAP hbmWizardBitmapRescaled = NULL;

char OrigKeyboardLayout [8+1] = "00000409";
BOOL bKeyboardLayoutChanged = FALSE;		/* TRUE if the keyboard layout was changed to the standard US keyboard layout (from any other layout). */ 
BOOL bKeybLayoutAltKeyWarningShown = FALSE;	/* TRUE if the user has been informed that it is not possible to type characters by pressing keys while the right Alt key is held down. */ 

#ifndef _DEBUG
	BOOL bWarnDeviceFormatAdvanced = TRUE;
#else
	BOOL bWarnDeviceFormatAdvanced = FALSE;
#endif

BOOL bWarnOuterVolSuitableFileSys = TRUE;

Password volumePassword;			/* User password */
char szVerify[MAX_PASSWORD + 1];	/* Tmp password buffer */
char szRawPassword[MAX_PASSWORD + 1];	/* Password before keyfile was applied to it */

BOOL bHistoryCmdLine = FALSE; /* History control is always disabled */
BOOL ComServerMode = FALSE;

int nPbar = 0;			/* Control ID of progress bar:- for format code */

char HeaderKeyGUIView [KEY_GUI_VIEW_SIZE];
char MasterKeyGUIView [KEY_GUI_VIEW_SIZE];

#define RANDPOOL_DISPLAY_COLUMNS	15
#define RANDPOOL_DISPLAY_ROWS		8
#define RANDPOOL_DISPLAY_BYTE_PORTION	(RANDPOOL_DISPLAY_COLUMNS * RANDPOOL_DISPLAY_ROWS)
#define RANDPOOL_DISPLAY_SIZE	(RANDPOOL_DISPLAY_BYTE_PORTION * 3 + RANDPOOL_DISPLAY_ROWS + 2)
unsigned char randPool [RANDPOOL_DISPLAY_BYTE_PORTION];
unsigned char lastRandPool [RANDPOOL_DISPLAY_BYTE_PORTION];
unsigned char outRandPoolDispBuffer [RANDPOOL_DISPLAY_SIZE];
BOOL bDisplayPoolContents = TRUE;

volatile BOOL bSparseFileSwitch = FALSE;
volatile BOOL quickFormat = FALSE;	/* WARNING: Meaning of this variable depends on bSparseFileSwitch. If bSparseFileSwitch is TRUE, this variable represents the sparse file flag. */
volatile int fileSystem = FILESYS_NONE;	
volatile int clusterSize = 0;

SYSENC_MULTIBOOT_CFG	SysEncMultiBootCfg;
wchar_t SysEncMultiBootCfgOutcome [4096] = {'N','/','A',0};
volatile int NonSysInplaceEncStatus = NONSYS_INPLACE_ENC_STATUS_NONE;

vector <HostDevice> DeferredNonSysInPlaceEncDevices;

/// Alteração executada pelo projeto CPA.EXE
/// Trecho de variáveis copiadas do projeto MOUNT - 
/// seguindo o padrão anterior do Truecrypt, foram mantidas variáveis globais :(
/// utilizadas para montar e demontar volumes
static char PasswordDlgVolume[MAX_PATH];
static BOOL PasswordDialogDisableMountOptions;
static char *PasswordDialogTitleStringId;
char PasswordDialogTitleString[80]="Acessar Unidade";
BOOL bCloseDismountedWindows=TRUE;	/* Close all open explorer windows of dismounted volume */
VOLUME_NOTIFICATIONS_LIST	VolumeNotificationsList;	
static int pwdChangeDlgMode	= PCDM_CHANGE_PASSWORD;
static int bSysEncPwdChangeDlgMode = FALSE;
static int bPrebootPasswordDlgMode = FALSE;
char szConfigFileName[TC_MAX_PATH+1];	/* The file selected by the user */
char szConfigVazio[255];
char szConfigLiberado[255];
char szConfigMontado[255];
char szConfigUnidade[255];

bool g_bExibirUnidade = false; /* exibir unidade no Windows explorer após criar/montar */

HBRUSH HB_BACK = NULL;
/// Alteração executada pelo projeto CPA.EXE
/// Bitmaps utilizados pela nova interface gráfica
///         são alocados em HabilitarBotes e liberados em localclenup()
HBITMAP Hbmp_Montar = NULL;
HBITMAP Hbmp_Montar_c = NULL;
HBITMAP Hbmp_Liberar = NULL;
HBITMAP Hbmp_Liberar_c = NULL;
HBITMAP Hbmp_Trocar = NULL;
HBITMAP Hbmp_Trocar_c = NULL;
HBITMAP Hbmp_Concluir = NULL;
HBITMAP Hbmp_Concluir_c = NULL;
HBITMAP Hbmp_Avancar = NULL;
HBITMAP Hbmp_Avancar_c = NULL;
HBITMAP Hbmp_Desistir = NULL;
HBITMAP Hbmp_Desistir_c = NULL;
HBITMAP Hbmp_OK = NULL;
HBITMAP Hbmp_OK_c = NULL;
HBITMAP Hbmp_Cancelar = NULL;
HBITMAP Hbmp_Cancelar_c = NULL;
HBITMAP Hbmp_Excluir = NULL;
HBITMAP Hbmp_Excluir_c = NULL;

//********* Rotina LerConfiguracoes() *********************************************
///
/// Cria e lê nome do arquivo segura em CPA.CONF (na pasta MeusDocumentos)
///
/// \param void
/// \note foi desativada. O nome do arquivo retornado será sempre "ArquivosSeguros.it"
///       A função foi mantida prevendo alterações futuras     
///
/// \return void
/// 
//*******************************************************************
BOOL LerConfiguracoes()
{
//  BOOL tmpbDevice; 
//  HANDLE hCnfFile;
//  __int8 *buffer;
  //DWORD bytesRead, bytesWritten;
	
    szDiskFile[0]=0;
    szFileName[0]=0;

	SHGetFolderPath (NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szDiskFile);	
    strcpy(szFileName, "ArquivosSeguros.it");
	strcat (szDiskFile, "\\");
	strcat (szDiskFile, szFileName);				

    //SHGetFolderPath (NULL, CSIDL_MYDOCUMENTS, NULL, 0, szConfigFileName);
	SHGetFolderPath (NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szConfigFileName);
	strcat (szConfigFileName, "\\cpa.conf");			
    
	GetPrivateProfileString("CPA", "VAZIO", "FALSE", szConfigVazio, 255, szConfigFileName);
	GetPrivateProfileString("CPA", "LIBERADO", "FALSE", szConfigLiberado, 255, szConfigFileName);
	GetPrivateProfileString("CPA", "MONTADO", "FALSE", szConfigMontado, 255, szConfigFileName);
	GetPrivateProfileString("CPA", "UNIDADE", "", szConfigUnidade, 255, szConfigFileName);
	return true;
}

//********* Rotina  GravarConfiguracoes *********************************************
///
/// Chamada pelo LerConfiguracoes. cria  arquivo CPA.CONF quando não existir
/// e grava eventual alteração nonome do arquivo (funcionalidade desabilitada)
///
///
/// \return BOOL
//*******************************************************************
BOOL GravarConfiguracoes(char *szchave, char *szvalor)
{
    //szchave in "VAZIO", "LIBERADO", "MONTADO" --> criar enum.
	SHGetFolderPath (NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szConfigFileName);
	strcat (szConfigFileName, "\\cpa.conf");			  
    WritePrivateProfileString("CPA", szchave, szvalor, szConfigFileName);	
	return true;
}

void ChecaUnidadeVazia()
{
	char sztmp[255];
    struct _finddata_t c_file;
    intptr_t hFile;
	bool bVazio = false;
    char szFileUpper[255];

	LerConfiguracoes();
	//MessageBox(NULL, szConfigMontado, "montado", MB_OK);
	if ( strcmp(szConfigMontado, "TRUE")) //se não estiver n=montado, pula fora;
		return;

   strcpy (sztmp, szConfigUnidade);
   strcat(sztmp, "*.*");
   //MessageBox(NULL, sztmp, "sztmp", MB_OK);
   if( (hFile = _findfirst( sztmp, &c_file )) == -1L )
   {
	   bVazio = true;
	   //GravarConfiguracoes("VAZIO", "TRUE");
	   //MessageBox(NULL, "VAZIO", "TRUE", MB_OK);
   }
   else //sempre deve existir ao menos um arquivo, o $RECYCLE.BIN
   {
	   int i;
       for (i=0; c_file.name[i] != '\0'; i++)
		   szFileUpper[i] = toupper(c_file.name[i]);

	   szFileUpper[i] = '\0';
	   while ( ! bVazio &&
		     ( !strcmp(szFileUpper,"RECYCLE.BIN") ||
			   !strcmp(szFileUpper,"$RECYCLE.BIN") ||
		       !strcmp(szFileUpper,"SYSTEM VOLUME INFORMATION") ||
			   !strcmp(szFileUpper,"$SYSTEM VOLUME INFORMATION") ||
			   !strcmp(szFileUpper,"RECYCLED") ||
			   !strcmp(szFileUpper,"$RECYCLED") )
			 )
	 {
        bVazio = (_findnext(hFile, &c_file) == -1L);
		
       for (i=0; c_file.name[i] != '\0'; i++)
		   szFileUpper[i] = toupper(c_file.name[i]);

	   szFileUpper[i] = '\0';
	 }
   }
 
   if (bVazio)
	   GravarConfiguracoes("VAZIO", "TRUE");   
   else
   {
	   GravarConfiguracoes("VAZIO", "FALSE");
	   GravarConfiguracoes("ARQ", c_file.name);  
   }


   _findclose( hFile );
}

//********* Rotina HabilitarBotoes() *********************************************
///
/// Habilita e desabilita funções na INTRO_PAGE. Também carrega os respectivos
/// Bitmaps para cada botão da tela principal
///
/// \param HWND hwndDlg - Handle do dialogo com os botões
/// 
///
/// \return void
/// 
//*******************************************************************
void HabilitarBotoes(HWND hwndDlg)
{
	bool bArquivoExiste = false;
	bool bVolumeMontado = false;

	/////////////////////////////////////////////////////////////////////////////
	/// Não esquecer de liberar os bitmaps na funcao localclenup (risco de grande MemoryLeak)!!!
	/////////////////////////////////////////////////////////////////////////////
	if (Hbmp_Montar == NULL)
	{
		 Hbmp_Montar = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ACC_PASTA));
		 Hbmp_Montar_c = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ACC_PASTA_C));
		 Hbmp_Liberar = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LIBERAR_PASTA));
		 Hbmp_Liberar_c = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LIBERAR_PASTA_C));
		 Hbmp_Trocar = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ALT_SENHA));
		 Hbmp_Trocar_c = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ALT_SENHA_C));
		 Hbmp_Concluir = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CONCLUIR));
		 Hbmp_Concluir_c = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CONCLUIR_C));
		 Hbmp_Avancar = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AVANCAR));
		 Hbmp_Avancar_c = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AVANCAR_C));
		 Hbmp_Desistir = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_DESISTIR));
		 Hbmp_Desistir_c = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_DESISTIR_C));
		 Hbmp_OK = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_OK));
		 Hbmp_OK_c = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_OK_C));
 		 Hbmp_Cancelar = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CANCELAR));
		 Hbmp_Cancelar_c = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CANCELAR_C));
  		 Hbmp_Excluir = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_REMOVER_UNIDADE));
		 Hbmp_Excluir_c = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_REMOVER_UNIDADE_C));

	}
    if (szDiskFile[0]==0)
		LerConfiguracoes();

	bVolumeMontado = IsMountedVolume (szDiskFile);
    bArquivoExiste = FileExists(szDiskFile);
   
    HWND hBtMontar = GetDlgItem (hwndDlg, IDC_MONTAR_VOLUME);
    HWND hBtLiberar = GetDlgItem (hwndDlg, IDC_DESMONTAR_VOLUME);
    HWND hBtTrocar = GetDlgItem (hwndDlg, IDC_ALTERAR_SENHA);  
    HWND hBtExcluir = GetDlgItem (hwndDlg, IDC_EXCLUIR_UNIDADE);
	HWND hBtNext = GetDlgItem (GetParent(hwndDlg), IDC_NEXT);
	HWND hBtCancel = GetDlgItem (GetParent(hwndDlg), IDCANCEL);


	if (nCurPageNo == INTRO_PAGE)
	{
		SetWindowText (GetDlgItem (GetParent (hwndDlg), IDC_BOX_TITLE), "");
		EnableWindow (hBtCancel, true);
        SendMessage  (hBtCancel, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Cancelar);				
		ShowWindow(hBtNext, SW_HIDE);
		if (bVolumeMontado) //habilitar somente Desmontar
		{
			EnableWindow (hBtMontar,  false);
			SendMessage(hBtMontar, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Montar_c);
			EnableWindow (hBtLiberar, true);
			SendMessage(hBtLiberar, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Liberar);
			EnableWindow (hBtTrocar,  false);		
			SendMessage(hBtTrocar, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Trocar_c);
            SendMessage(hBtExcluir, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Excluir_c);
			EnableWindow (hBtExcluir,  false);		
		}
		else
		{
			if (bArquivoExiste)
			{
				//nao montado, existe
				EnableWindow (hBtMontar,  true);
				SendMessage(hBtMontar, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Montar);
				EnableWindow (hBtLiberar, false);
				SendMessage(hBtLiberar, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Liberar_c);
				EnableWindow (hBtTrocar,  true);
				SendMessage(hBtTrocar, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Trocar);
                SendMessage(hBtExcluir, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Excluir);
				EnableWindow (hBtExcluir,  true);		
			}
			else 
			{
				//nao montado, não existe
				EnableWindow (hBtMontar,  true);
				SendMessage(hBtMontar, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Montar);
				EnableWindow (hBtLiberar, false);
				SendMessage(hBtLiberar, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Liberar_c);
				EnableWindow (hBtTrocar,  false);		
				SendMessage(hBtTrocar, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Trocar_c);
				SendMessage(hBtExcluir, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Excluir_c);				
				EnableWindow (hBtExcluir,  false);		

			}
		}
	}
	else if (nCurPageNo == PASSWORD_PAGE)
	{
		ShowWindow(hBtNext, SW_SHOW);
		EnableWindow (hBtNext,  true);
		SendMessage(hBtNext, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Avancar);
		EnableWindow (hBtCancel, true);
        SendMessage  (hBtCancel, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Cancelar);				
	}
	else if (nCurPageNo == FORMAT_PAGE)
	{
		
	}
		
	InvalidateRect (MainDlg, NULL, TRUE);
}

//********* Rotina PasswordChangeEnable() *********************************************
///
/// Habilita e desabilita botão Avançar quando solicitado senha para criação de um novo volume
/// Verifica se senha atual está correta e se nova senha bate com a verificação da nova senha
///
/// \param WND hwndDlg - Handle da janela c
/// \param int button  - ID do botão (avançar) a ser manipulado 
/// \param int passwordId - ID do campo com a senha atual
/// \param BOOL keyFilesEnabled - não utilizado
/// \param int newPasswordId - ID do campo com a nova senha
/// \param int newVerifyId - ID do campo com a verificação da nova senha
/// 
/// \note Mantida versão original, alterada apenas a troca gráfica dos botões
//        de acordo com a nova interface gráfica
/// \return void
/// 
//*******************************************************************
static void PasswordChangeEnable (HWND hwndDlg, int button, int passwordId, BOOL keyFilesEnabled,
								  int newPasswordId, int newVerifyId, BOOL newKeyFilesEnabled)
{
	char password[MAX_PASSWORD + 1];
	char newPassword[MAX_PASSWORD + 1];
	char newVerify[MAX_PASSWORD + 1];
	BOOL bEnable = TRUE;
	HBITMAP hImagem;
	HBITMAP hImagem_c;

	GetWindowText (GetDlgItem (hwndDlg, passwordId), password, sizeof (password));

	if (pwdChangeDlgMode == PCDM_CHANGE_PKCS5_PRF)
		newKeyFilesEnabled = keyFilesEnabled;

	switch (pwdChangeDlgMode)
	{
	case PCDM_REMOVE_ALL_KEYFILES_FROM_VOL:
	case PCDM_ADD_REMOVE_VOL_KEYFILES:
	case PCDM_CHANGE_PKCS5_PRF:
		memcpy (newPassword, password, sizeof (newPassword));
		memcpy (newVerify, password, sizeof (newVerify));
		break;

	default:
		GetWindowText (GetDlgItem (hwndDlg, newPasswordId), newPassword, sizeof (newPassword));
		GetWindowText (GetDlgItem (hwndDlg, newVerifyId), newVerify, sizeof (newVerify));
	}

	if (!keyFilesEnabled && strlen (password) < MIN_PASSWORD)
		bEnable = FALSE;
	else if (strcmp (newPassword, newVerify) != 0)
		bEnable = FALSE;
	else if (!newKeyFilesEnabled && strlen (newPassword) < MIN_PASSWORD)
		bEnable = FALSE;

	burn (password, sizeof (password));
	burn (newPassword, sizeof (newPassword));
	burn (newVerify, sizeof (newVerify));
 
//	if (button==BTOK)
//	{
		hImagem = Hbmp_OK;
		hImagem_c = Hbmp_OK_c;
//	}
//	else
//	{
//	    hImagem = Hbmp_Avancar;
//		hImagem_c = Hbmp_Avancar_c;		
//	}

	HWND hBtAvancar = (HWND) GetDlgItem (hwndDlg, button); 
	if (bEnable)
	{	
		EnableWindow (hBtAvancar,  true);
		SendMessage(hBtAvancar, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) hImagem);
	}
	else
	{
		EnableWindow (hBtAvancar,  false);
		SendMessage(hBtAvancar, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) hImagem_c);
	}

	//EnableWindow (GetDlgItem (hwndDlg, button), bEnable);

}



/* Except in response to the WM_INITDIALOG message, the dialog box procedure
   should return nonzero if it processes the message, and zero if it does
   not. - see DialogProc */
//********* Rotina PasswordChangeDlgProc() *********************************************
///
/// DlgProc para o dialogo de troca de senha. Mantida versão original
///
/// 
/// \note Mantida versão original, alterada apenas a troca gráfica dos botões
//        de acordo com a nova interface gráfica
/// \return static LRESULT
/// 
//*******************************************************************
static LRESULT CALLBACK PasswordChangeDlgProc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static KeyFilesDlgParam newKeyFilesParam;

	WORD lw = LOWORD (wParam);
	WORD hw = HIWORD (wParam);
    if (HB_BACK == NULL)
	  HB_BACK = CreateSolidBrush(RGB(241,241,241));

	switch (msg)
	{

	/*case WM_CTLCOLORDLG:
  	  return (LONG) HB_BACK;
    case WM_CTLCOLORSTATIC:
  	  return (LONG) HB_BACK;
    */


	case WM_INITDIALOG:
		{
			LPARAM nIndex;
			HWND hComboBox = GetDlgItem (hwndDlg, IDC_PKCS5_PRF_ID);
			int i;
            SendMessage(GetDlgItem (hwndDlg, IDC_BOX_HELP), WM_SETFONT, (WPARAM) hTitleFont, (LPARAM) TRUE);
			ShowWindow(GetDlgItem (hwndDlg, IDC_BOX_HELP), false);

			ZeroMemory (&newKeyFilesParam, sizeof (newKeyFilesParam));

			SetWindowTextW (hwndDlg, GetString ("IDD_PASSWORDCHANGE_DLG"));
			LocalizeDialog (hwndDlg, "IDD_PASSWORDCHANGE_DLG");
			
			SendMessage (GetDlgItem (hwndDlg, IDC_HEADER), WM_SETFONT, (WPARAM) hTitleFont, (LPARAM) TRUE);
			SendMessage (GetDlgItem (hwndDlg, IDC_OLD_PASSWORD), EM_LIMITTEXT, MAX_PASSWORD, 0);
			SendMessage (GetDlgItem (hwndDlg, IDC_NEWPASSWORD), EM_LIMITTEXT, MAX_PASSWORD, 0);
			SendMessage (GetDlgItem (hwndDlg, IDC_NEWVERIFY), EM_LIMITTEXT, MAX_PASSWORD, 0);
			EnableWindow (GetDlgItem (hwndDlg, IDOK), FALSE);

			SetCheckBox (hwndDlg, IDC_ENABLE_KEYFILES, KeyFilesEnable);
			EnableWindow (GetDlgItem (hwndDlg, IDC_KEYFILES), TRUE);
			EnableWindow (GetDlgItem (hwndDlg, IDC_NEW_KEYFILES), TRUE);

			SendMessage (hComboBox, CB_RESETCONTENT, 0, 0);

			nIndex = SendMessageW (hComboBox, CB_ADDSTRING, 0, (LPARAM) GetString ("UNCHANGED"));
			SendMessage (hComboBox, CB_SETITEMDATA, nIndex, (LPARAM) 0);

			for (i = FIRST_PRF_ID; i <= LAST_PRF_ID; i++)
			{
				if (!HashIsDeprecated (i))
				{
					nIndex = SendMessage (hComboBox, CB_ADDSTRING, 0, (LPARAM) get_pkcs5_prf_name(i));
					SendMessage (hComboBox, CB_SETITEMDATA, nIndex, (LPARAM) i);
				}
			}

			SendMessage (hComboBox, CB_SETCURSEL, 0, 0);

			switch (pwdChangeDlgMode)
			{
			case PCDM_CHANGE_PKCS5_PRF:
				SetWindowTextW (hwndDlg, GetString ("IDD_PCDM_CHANGE_PKCS5_PRF"));
				LocalizeDialog (hwndDlg, "IDD_PCDM_CHANGE_PKCS5_PRF");
				EnableWindow (GetDlgItem (hwndDlg, IDC_NEWPASSWORD), FALSE);
				EnableWindow (GetDlgItem (hwndDlg, IDC_NEWVERIFY), FALSE);
				EnableWindow (GetDlgItem (hwndDlg, IDC_ENABLE_NEW_KEYFILES), FALSE);
				EnableWindow (GetDlgItem (hwndDlg, IDC_SHOW_PASSWORD_CHPWD_NEW), FALSE);
				EnableWindow (GetDlgItem (hwndDlg, IDC_NEW_KEYFILES), FALSE);
				EnableWindow (GetDlgItem (hwndDlg, IDT_NEW_PASSWORD), FALSE);
				EnableWindow (GetDlgItem (hwndDlg, IDT_CONFIRM_PASSWORD), FALSE);
				break;



			case PCDM_CHANGE_PASSWORD:
			default:
				// NOP
				break;
			};

			if (bSysEncPwdChangeDlgMode)
			{
				ToBootPwdField (hwndDlg, IDC_NEWPASSWORD);
				ToBootPwdField (hwndDlg, IDC_NEWVERIFY);
				ToBootPwdField (hwndDlg, IDC_OLD_PASSWORD);

				if ((DWORD) GetKeyboardLayout (NULL) != 0x00000409 && (DWORD) GetKeyboardLayout (NULL) != 0x04090409)
				{
					DWORD keybLayout = (DWORD) LoadKeyboardLayout ("00000409", KLF_ACTIVATE);

					if (keybLayout != 0x00000409 && keybLayout != 0x04090409)
					{
						Error ("CANT_CHANGE_KEYB_LAYOUT_FOR_SYS_ENCRYPTION");
						EndDialog (hwndDlg, IDCANCEL);
						return 0;
					}

					bKeyboardLayoutChanged = TRUE;
				}

				ShowWindow(GetDlgItem(hwndDlg, IDC_SHOW_PASSWORD_CHPWD_NEW), SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg, IDC_SHOW_PASSWORD_CHPWD_ORI), SW_HIDE);

				if (SetTimer (hwndDlg, TIMER_ID_KEYB_LAYOUT_GUARD, TIMER_INTERVAL_KEYB_LAYOUT_GUARD, NULL) == 0)
				{
					Error ("CANNOT_SET_TIMER");
					EndDialog (hwndDlg, IDCANCEL);
					return 0;
				}

				newKeyFilesParam.EnableKeyFiles = FALSE;
				KeyFilesEnable = FALSE;
				SetCheckBox (hwndDlg, IDC_ENABLE_KEYFILES, FALSE);
				EnableWindow (GetDlgItem (hwndDlg, IDC_ENABLE_KEYFILES), FALSE);
				EnableWindow (GetDlgItem (hwndDlg, IDC_ENABLE_NEW_KEYFILES), FALSE);
			}

			CheckCapsLock (hwndDlg, FALSE);

			return 0;
		}

	case WM_TIMER:
		switch (wParam)
		{
		case TIMER_ID_KEYB_LAYOUT_GUARD:
			if (bSysEncPwdChangeDlgMode)
			{
				DWORD keybLayout = (DWORD) GetKeyboardLayout (NULL);

				/* Watch the keyboard layout */

				if (keybLayout != 0x00000409 && keybLayout != 0x04090409)
				{
					// Keyboard layout is not standard US

					// Attempt to wipe passwords stored in the input field buffers
					char tmp[MAX_PASSWORD+1];
					memset (tmp, 'X', MAX_PASSWORD);
					tmp [MAX_PASSWORD] = 0;
					SetWindowText (GetDlgItem (hwndDlg, IDC_OLD_PASSWORD), tmp);
					SetWindowText (GetDlgItem (hwndDlg, IDC_NEWPASSWORD), tmp);
					SetWindowText (GetDlgItem (hwndDlg, IDC_NEWVERIFY), tmp);

					SetWindowText (GetDlgItem (hwndDlg, IDC_OLD_PASSWORD), "");
					SetWindowText (GetDlgItem (hwndDlg, IDC_NEWPASSWORD), "");
					SetWindowText (GetDlgItem (hwndDlg, IDC_NEWVERIFY), "");

					keybLayout = (DWORD) LoadKeyboardLayout ("00000409", KLF_ACTIVATE);

					if (keybLayout != 0x00000409 && keybLayout != 0x04090409)
					{
						KillTimer (hwndDlg, TIMER_ID_KEYB_LAYOUT_GUARD);
						Error ("CANT_CHANGE_KEYB_LAYOUT_FOR_SYS_ENCRYPTION");
						EndDialog (hwndDlg, IDCANCEL);
						return 1;
					}

					bKeyboardLayoutChanged = TRUE;

					wchar_t szTmp [4096];
					wcscpy (szTmp, GetString ("KEYB_LAYOUT_CHANGE_PREVENTED"));
					wcscat (szTmp, L"\n\n");
					wcscat (szTmp, GetString ("KEYB_LAYOUT_SYS_ENC_EXPLANATION"));
					MessageBoxW (MainDlg, szTmp, lpszTitle, MB_ICONWARNING | MB_SETFOREGROUND | MB_TOPMOST);
				}


				/* Watch the right Alt key (which is used to enter various characters on non-US keyboards) */

				if (bKeyboardLayoutChanged && !bKeybLayoutAltKeyWarningShown)
				{
					if (GetAsyncKeyState (VK_RMENU) < 0)
					{
						bKeybLayoutAltKeyWarningShown = TRUE;

						wchar_t szTmp [4096];
						wcscpy (szTmp, GetString ("ALT_KEY_CHARS_NOT_FOR_SYS_ENCRYPTION"));
						wcscat (szTmp, L"\n\n");
						wcscat (szTmp, GetString ("KEYB_LAYOUT_SYS_ENC_EXPLANATION"));
						MessageBoxW (MainDlg, szTmp, lpszTitle, MB_ICONINFORMATION  | MB_SETFOREGROUND | MB_TOPMOST);
					}
				}
			}
			return 1;
		}
		return 0;

	case WM_COMMAND:
		if (lw == IDCANCEL)
		{
			// Attempt to wipe passwords stored in the input field buffers
			char tmp[MAX_PASSWORD+1];
			memset (tmp, 'X', MAX_PASSWORD);
			tmp[MAX_PASSWORD] = 0;
			SetWindowText (GetDlgItem (hwndDlg, IDC_NEWPASSWORD), tmp);	
			SetWindowText (GetDlgItem (hwndDlg, IDC_OLD_PASSWORD), tmp);	
			SetWindowText (GetDlgItem (hwndDlg, IDC_NEWVERIFY), tmp);	
			RestoreDefaultKeyFilesParam ();

			EndDialog (hwndDlg, IDCANCEL);
			return 1;
		}

		if ((lw != IDCANCEL) && (hw == EN_CHANGE))
		{
			PasswordChangeEnable (hwndDlg, IDOK,
				IDC_OLD_PASSWORD,
				KeyFilesEnable && FirstKeyFile != NULL,
				IDC_NEWPASSWORD, IDC_NEWVERIFY, 
				newKeyFilesParam.EnableKeyFiles && newKeyFilesParam.FirstKeyFile != NULL);		

			return 1;
		}

		if (lw == IDOK && hw == 0)
		{
			//HWND hParent = GetParent (hwndDlg);
			Password oldPassword;
			Password newPassword;
			int nStatus;
			int pkcs5 = SendMessage (GetDlgItem (hwndDlg, IDC_PKCS5_PRF_ID), CB_GETITEMDATA, 
					SendMessage (GetDlgItem (hwndDlg, IDC_PKCS5_PRF_ID), CB_GETCURSEL, 0, 0), 0);

			if (!CheckPasswordCharEncoding (GetDlgItem (hwndDlg, IDC_NEWPASSWORD), NULL))
			{
				//Error ("UNSUPPORTED_CHARS_IN_PWD");
				MessageBox (hwndDlg, (LPCSTR) "Caracteres não permitidos na senha. Evite utilizar acentos ou caracteres não imnprimíveis.", (LPCSTR) "Central de proteção de arquivos", MB_ICONHAND);
				return 1;
			}

			if (pwdChangeDlgMode == PCDM_CHANGE_PKCS5_PRF)
			{
				newKeyFilesParam.EnableKeyFiles = KeyFilesEnable;
			}
			else if (!(newKeyFilesParam.EnableKeyFiles && newKeyFilesParam.FirstKeyFile != NULL)
				&& pwdChangeDlgMode == PCDM_CHANGE_PASSWORD)
			{
				if (!CheckPasswordLength (hwndDlg, GetDlgItem (hwndDlg, IDC_NEWPASSWORD)))
					return 1;

                MessageBox(hwndDlg, TC_ALERTA_SENHA, MB_OK | MB_ICONINFORMATION);				
			}

			
			GetWindowText (GetDlgItem (hwndDlg, IDC_OLD_PASSWORD), (LPSTR) oldPassword.Text, sizeof (oldPassword.Text));
			oldPassword.Length = strlen ((char *) oldPassword.Text);

			switch (pwdChangeDlgMode)
			{
			case PCDM_REMOVE_ALL_KEYFILES_FROM_VOL:
			case PCDM_ADD_REMOVE_VOL_KEYFILES:
			case PCDM_CHANGE_PKCS5_PRF:
				memcpy (newPassword.Text, oldPassword.Text, sizeof (newPassword.Text));
				newPassword.Length = strlen ((char *) oldPassword.Text);
				break;

			default:
				GetWindowText (GetDlgItem (hwndDlg, IDC_NEWPASSWORD), (LPSTR) newPassword.Text, sizeof (newPassword.Text));
				newPassword.Length = strlen ((char *) newPassword.Text);
			}

			WaitCursor ();
			//strcpy(szTmp,"AGUARDE enquanto o processo de troca de senha é executado. Poderá demorar alguns minutos, não interrompa");
 		    //SendMessage (GetDlgItem (hwndDlg, IDC_BOX_HELP), WM_SETTEXT, NULL, (LPARAM)szTmp);
 		    ShowWindow(GetDlgItem (hwndDlg, IDOK), SW_HIDE);				
            ShowWindow(GetDlgItem (hwndDlg, IDCANCEL), SW_HIDE);
            
			ShowWindow(GetDlgItem (hwndDlg, IDC_BOX_HELP), true);
			SendMessage(GetDlgItem (hwndDlg, IDC_BOX_HELP), WM_SETFONT, (WPARAM) hBoldFont, (LPARAM) TRUE);
			SendMessage (GetDlgItem (hwndDlg, IDC_BOX_HELP), WM_PAINT, NULL, NULL);
            InvalidateRect(hwndDlg, NULL, true);
            SendMessage(hwndDlg, WM_PAINT, NULL, NULL);
            RedrawWindow(hwndDlg, NULL, NULL, RDW_UPDATENOW);	
			if (KeyFilesEnable)
				KeyFilesApply (&oldPassword, FirstKeyFile);

			if (newKeyFilesParam.EnableKeyFiles)
				KeyFilesApply (&newPassword,
				pwdChangeDlgMode==PCDM_CHANGE_PKCS5_PRF ? FirstKeyFile : newKeyFilesParam.FirstKeyFile);

			if (bSysEncPwdChangeDlgMode)
			{
				// System

				pkcs5 = 0;	// PKCS-5 PRF unchanged (currently system encryption supports only RIPEMD-160)

				try
				{
					nStatus = BootEncObj->ChangePassword (&oldPassword, &newPassword, pkcs5);
				}
				catch (Exception &e)
				{
					e.Show (MainDlg);
					nStatus = ERR_OS_ERROR;
				}
			}
			else
			{
				// Non-system

				nStatus = ChangePwd (szDiskFile, &oldPassword, &newPassword, pkcs5, hwndDlg);

				if (nStatus == ERR_OS_ERROR
					&& GetLastError () == ERROR_ACCESS_DENIED
					&& IsUacSupported ()
					&& IsVolumeDeviceHosted (szDiskFile))
				{
					//nStatus = UacChangePwd (szFileName, &oldPassword, &newPassword, pkcs5, hwndDlg);
					nStatus = ChangePwd (szDiskFile, &oldPassword, &newPassword, pkcs5, (HWND) hwndDlg);
				}
			}

			burn (&oldPassword, sizeof (oldPassword));
			burn (&newPassword, sizeof (newPassword));
			ShowWindow(GetDlgItem (hwndDlg, IDC_BOX_HELP), false);
		    ShowWindow(GetDlgItem (hwndDlg, IDOK), SW_SHOW);				
            ShowWindow(GetDlgItem (hwndDlg, IDCANCEL), SW_SHOW);


			NormalCursor ();

			if (nStatus == 0)
			{
				// Attempt to wipe passwords stored in the input field buffers
				char tmp[MAX_PASSWORD+1];
				memset (tmp, 'X', MAX_PASSWORD);
				tmp[MAX_PASSWORD] = 0;
				SetWindowText (GetDlgItem (hwndDlg, IDC_NEWPASSWORD), tmp);	
				SetWindowText (GetDlgItem (hwndDlg, IDC_OLD_PASSWORD), tmp);	
				SetWindowText (GetDlgItem (hwndDlg, IDC_NEWVERIFY), tmp);	

				KeyFileRemoveAll (&newKeyFilesParam.FirstKeyFile);
				RestoreDefaultKeyFilesParam ();

				if (bSysEncPwdChangeDlgMode)
				{
					KillTimer (hwndDlg, TIMER_ID_KEYB_LAYOUT_GUARD);
				}

				EndDialog (hwndDlg, IDOK);
			}
			return 1;
		}
		return 0;
	}
	return 0;
}

//********* Rotina IsVolumeNameValid *********************************************
///
/// Verifica se o nome do volume (arquivoSeguro.it) é válido.
///  Fazia sentido somente na primeira versão do projeto, quando era possível alterar
///  o nome do arquivo. Como este nome agora é fixo, não seria mais necessário verificar.
///  Para possível expansão no futuro, foi mantida.
/// \param pchar *szName - Nome do volume a ser testado
///
/// \return BOOL
//*******************************************************************
BOOL IsVolumeNameValid(char *szName)
{
	
	HANDLE hFile;
		
	hFile = CreateFile (szName,
		GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	else
	{
        CloseHandle(hFile);
        DeleteFile(szName);
		return TRUE;
	}
	return FALSE;
}

/* Except in response to the WM_INITDIALOG message, the dialog box procedure
   should return nonzero if it processes the message, and zero if it does
   not. - see DialogProc */
//********* Rotina RenomearVolumeDlgProc *********************************************
///
/// Desabilitada (recurso foi desativado (botão renomear volume excluído)
///
//*******************************************************************
static LRESULT CALLBACK RenomearVolumeDlgProc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static KeyFilesDlgParam newKeyFilesParam;

	WORD lw = LOWORD (wParam);
//	WORD hw = HIWORD (wParam);
    
	switch (msg)
	{

	case WM_INITDIALOG:
		{
			//LocalizeDialog (hwndDlg, "IDD_RENOMEAR_VOLUME_DLG");
			LerConfiguracoes();
			SetWindowText (GetDlgItem (hwndDlg, IDC_NOME_ATUAL), szFileName);
			SetFocus (GetDlgItem (hwndDlg, IDC_NOVO_NOME));
			//EnableWindow (GetDlgItem (hwndDlg, IDOK), TRUE);
		    return 0;
		}       
	case WM_COMMAND:
		if (lw == IDCANCEL)
		{
			EndDialog (hwndDlg, IDCANCEL);
			return 1;
		}
		
		if (lw == IDOK)
		{		
            char szNovoNome[TC_MAX_PATH+1];
			char szNewDiskFile[TC_MAX_PATH+1]; 
			GetWindowText (GetDlgItem (hwndDlg, IDC_NOVO_NOME), (LPSTR) szNovoNome, sizeof (szNovoNome));  
			SHGetFolderPath (NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szNewDiskFile);
	        strcat (szNewDiskFile, "\\");
	        strcat (szNewDiskFile, szNovoNome);					

			if (IsVolumeNameValid(szNewDiskFile) )
			{
               if (! FileExists(szNewDiskFile))
			   {
					if (MoveFile(szDiskFile, szNewDiskFile))
					{
						strcpy(szFileName,szNovoNome);
						strcpy(szDiskFile,szNewDiskFile);				
						//if (!GravarConfiguracoes())
						//{
							//Erro crítico, pois alterou nome do arquivo e não acertou o arquivo .conf
							//Desfazer operação de mudança de nome
							//não deveria ocorrer nunca.
						//	MoveFile(szNewDiskFile, szDiskFile);
						//	Error ("ERROR_CONFIG_FILE");

						//}
						EndDialog (hwndDlg, IDOK);
						return 1;
					}
					else
					{
						ERROR ("ERROR_RENOMEAR VOLUME");
					}
			   }
			   else
			   {
				   ERROR ("ERROR_RENOMEAR_FILE_EXISTS");		
				   SetFocus (GetDlgItem (hwndDlg, IDC_NOVO_NOME));
			   }
			}
			else
			{
				Info("INVALID_VOLUME_NAME");
				SetFocus (GetDlgItem (hwndDlg, IDC_NOVO_NOME));
			}
			return 1;
		}
		return 0;
	}
	return 0;
}

//********* Rotina RenomearVolume *********************************************
///
/// Desabilitada
///
//*******************************************************************
static void RenomearVolume (HWND hwndDlg)
{
	int result;
	
	//GetWindowText (GetDlgItem (hwndDlg, IDC_VOLUME), szFileName, sizeof (szFileName));
	if (IsMountedVolume (szDiskFile))
	{
		Warning ("MOUNTED_NOPWCHANGE");
		return;
	}

	result = DialogBoxW (hInst, MAKEINTRESOURCEW (IDD_RENOMEAR_VOLUME_DLG), hwndDlg,
		(DLGPROC) RenomearVolumeDlgProc);

	if (result == IDOK)
	{
        
		Info ("VOLUME_NAME_CHANGED");
	}
}

//********* Rotina ChangePassword *********************************************
///
/// Rotina chamada ao ser clicado o botão para troca de senha.
/// Ativa o diálogo de troca de senha.
///
/// \param HWND hwndDlg ? Handle da janela pai
///
/// \return void
//*******************************************************************
static void ChangePassword (HWND hwndDlg)
{
	int result;
	
	//GetWindowText (GetDlgItem (hwndDlg, IDC_VOLUME), szFileName, sizeof (szFileName));
	if (IsMountedVolume (szDiskFile))
	{
		Warning ("MOUNTED_NOPWCHANGE");
		return;
	}

	bSysEncPwdChangeDlgMode = FALSE;

	result = DialogBoxW (hInst, MAKEINTRESOURCEW (IDD_PASSWORDCHANGE_DLG), hwndDlg,
		(DLGPROC) PasswordChangeDlgProc);

	if (result == IDOK)
	{
	  MessageBox (hwndDlg, (LPCSTR) "Senha da unidade de arquivos protegidos, alterada com sucesso!!!", (LPCSTR) "Central de proteção de arquivos", MB_ICONEXCLAMATION);
	}
}

//********* Rotina DismountAll *********************************************
///
/// Desmonta o volume. Copiada do projeto MOUNT
///
/// \param HWND hwndDlg ? Handle da janela pai
/// \param BOOL forceUnmount ? forçar unmount, mesmo se volume estiver em uso - forçado para true
/// \param BOOL interact ? pedir confirmação do usuário -forçado para false
/// \param int dismountMaxRetries ? mantido como original
/// \param int dismountAutoRetryDelay ? mantido como original
///
/// \return BOOL
//*******************************************************************

static BOOL DismountAll (HWND hwndDlg, BOOL forceUnmount, BOOL interact, int dismountMaxRetries, int dismountAutoRetryDelay)
{
	BOOL status = TRUE;
	MOUNT_LIST_STRUCT mountList;
	DWORD dwResult;
	UNMOUNT_STRUCT unmount;
	BOOL bResult;
	unsigned __int32 prevMountedDrives = 0;
	int i;

retry:
	WaitCursor();

	DeviceIoControl (hDriver, TC_IOCTL_GET_MOUNTED_VOLUMES, &mountList, sizeof (mountList), &mountList, sizeof (mountList), &dwResult, NULL);

	if (mountList.ulMountedDrives == 0)
	{
		NormalCursor();
		return TRUE;
	}

	BroadcastDeviceChange (DBT_DEVICEREMOVEPENDING, 0, mountList.ulMountedDrives);

	prevMountedDrives = mountList.ulMountedDrives;

	for (i = 0; i < 26; i++)
	{
		if (mountList.ulMountedDrives & (1 << i))
		{
			if (bCloseDismountedWindows)
				CloseVolumeExplorerWindows (hwndDlg, i);
		}
	}

	unmount.nDosDriveNo = 0;
	unmount.ignoreOpenFiles = forceUnmount;

	do
	{
		bResult = DeviceIoControl (hDriver, TC_IOCTL_DISMOUNT_ALL_VOLUMES, &unmount,
			sizeof (unmount), &unmount, sizeof (unmount), &dwResult, NULL);

		if (bResult == FALSE)
		{
			NormalCursor();
			handleWin32Error (hwndDlg);
			return FALSE;
		}

		if (unmount.nReturnCode == ERR_SUCCESS
			&& unmount.HiddenVolumeProtectionTriggered
			&& !VolumeNotificationsList.bHidVolDamagePrevReported [unmount.nDosDriveNo])
		{
			wchar_t msg[4096];

			VolumeNotificationsList.bHidVolDamagePrevReported [unmount.nDosDriveNo] = TRUE;
			swprintf (msg, GetString ("DAMAGE_TO_HIDDEN_VOLUME_PREVENTED"), unmount.nDosDriveNo + 'A');
			SetForegroundWindow (hwndDlg);
			MessageBoxW (hwndDlg, msg, lpszTitle, MB_ICONWARNING | MB_SETFOREGROUND | MB_TOPMOST);

			unmount.HiddenVolumeProtectionTriggered = FALSE;
			continue;
		}

		if (unmount.nReturnCode == ERR_FILES_OPEN)
			Sleep (dismountAutoRetryDelay);
		else
			break;

	} while (--dismountMaxRetries > 0);

	memset (&mountList, 0, sizeof (mountList));
	DeviceIoControl (hDriver, TC_IOCTL_GET_MOUNTED_VOLUMES, &mountList, sizeof (mountList), &mountList, sizeof (mountList), &dwResult, NULL);
	BroadcastDeviceChange (DBT_DEVICEREMOVECOMPLETE, 0, prevMountedDrives & ~mountList.ulMountedDrives);

	//RefreshMainDlg (hwndDlg);

	//if (nCurrentOS == WIN_2000 && RemoteSession && !IsAdmin ())
	//	LoadDriveLetters (GetDlgItem (hwndDlg, IDC_DRIVELIST), 0);

	NormalCursor();

	if (unmount.nReturnCode != 0)
	{
		if (forceUnmount)
			status = FALSE;

		if (unmount.nReturnCode == ERR_FILES_OPEN)
		{
			if (interact && IDYES == AskWarnNoYes ("UNMOUNTALL_LOCK_FAILED"))
			{
				forceUnmount = TRUE;
				goto retry;
			}

			return FALSE;
		}
		
		if (interact)
			MessageBoxW (hwndDlg, GetString ("UNMOUNT_FAILED"), lpszTitle, MB_ICONERROR);
	}
	else
	{
		//if (bBeep)
			MessageBeep (-1);
	}

	return status;
}

//********* Rotina MontarVolume *********************************************
///
/// Copiado do projeto Mount. Ponto de chamada para se montar o volume com parâmetros escolhidos pelo usuario
/// Seguindo  projeto anterior, todos os parametros selecionados foram mantidos em variaveis globais
///
/// \param HWND hwndDlg ? Handle da Janela
/// \param wchar_t * szDrive ? Drive a ser montado o volume. Ignorado - será sempre selecionado o primerio drive disponivel
///
/// \return BOOL
//*******************************************************************
BOOL MontarVolume(HWND hwndDlg, wchar_t * szDrive)
{
	char szDriveLetter[4];				/* Drive Letter to mount */
	static const char * szVolumeName = "Unid_Segura";
	char szTmp[1000];
	
    MountOptions mountOptions;
    MountOptions defaultMountOptions;
    BOOL mounted = false;
    if (szDiskFile[0] == 0 || IsMountedVolume (szDiskFile) || volumePassword.Length == 0)
		return '\0';

	  	    szDriveLetter[0] = GetFirstAvailableDrive () + 'A';
			defaultMountOptions.PreserveTimestamp = ConfigReadInt ("PreserveTimestamps", TRUE);
			defaultMountOptions.Removable =	ConfigReadInt ("MountVolumesRemovable", FALSE);
			defaultMountOptions.ReadOnly =	ConfigReadInt ("MountVolumesReadOnly", FALSE);
			defaultMountOptions.ProtectHiddenVolume = FALSE;
			defaultMountOptions.PartitionInInactiveSysEncScope = FALSE;
			defaultMountOptions.RecoveryMode = FALSE;
			defaultMountOptions.UseBackupHeader =  FALSE;
            mountOptions = defaultMountOptions;          
			
			BOOL bCacheInDriver = false;
			BOOL bForceMount = false;
			Silent = true;							
		
			BOOL reportBadPasswd = volumePassword.Length > 0;
			mounted = MountVolume (hwndDlg, szDriveLetter[0] - 'A',
				szDiskFile, &volumePassword, bCacheInDriver, bForceMount,
				&mountOptions, Silent, reportBadPasswd);
			//wipe all
			burn (&volumePassword, sizeof (volumePassword));
			burn (&mountOptions, sizeof (mountOptions));
			burn (&defaultMountOptions, sizeof (defaultMountOptions));
			burn (&szFileName, sizeof(szFileName));
			volumePassword.Length = 0;
			Silent = false;
			szDriveLetter[1]=':';
			szDriveLetter[2]='\0';
			
			strcpy ((char *) &szDrive[0], (char *) &szDriveLetter[0]);
			ToUNICODE ((char *) &szDrive[0]);
            if (IsMountedVolume (szDiskFile))
			{
				if (IsUacSupported ())
				{
				  szDriveLetter[2]='\\';
				  szDriveLetter[3]='\0';
				  SetVolumeLabelA(szTmp, szVolumeName);
				}
				return true;
			}
			else
			  return false;
}

/* Except in response to the WM_INITDIALOG message, the dialog box procedure
   should return nonzero if it processes the message, and zero if it does
   not. - see DialogProc */
//********* Rotina PasswordDlgProc *********************************************
///
/// Copiado do projeto Mount. Inicializa o dialogo para se digitar a senha para se acessar uma unidade
/// (volume) já existente
///
/// \param HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam ? Parametros padrão de um DlgProc
///
/// \return static LRESULT
//*******************************************************************
static LRESULT CALLBACK PasswordDlgProc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WORD lw = LOWORD (wParam);
    WORD hw = HIWORD (wParam);

	static Password *szXPwd;	
    if (HB_BACK == NULL)
	  HB_BACK = CreateSolidBrush(RGB(241,241,241));
	switch (msg)
	{

	/*case WM_CTLCOLORDLG:
  	  return (LONG) HB_BACK;
    case WM_CTLCOLORSTATIC:
  	  return (LONG) HB_BACK;
    */
	
	case WM_INITDIALOG:
		{
			szXPwd = (Password *) lParam;
			LocalizeDialog (hwndDlg, "IDD_PASSWORD_DLG");
			DragAcceptFiles (hwndDlg, TRUE);

			SendMessage (GetDlgItem (hwndDlg, IDC_HEADER), WM_SETFONT, (WPARAM) hTitleFont, (LPARAM) TRUE);
			
			SetCheckBox (hwndDlg, IDC_CHK_EXIBIRUNIDADE, true);			
			
			if (PasswordDialogTitleString)
				SendMessage (GetDlgItem (hwndDlg, IDC_HEADER), WM_SETTEXT, (WPARAM) NULL, (LPARAM) PasswordDialogTitleString);

            if (!strcmp(PasswordDialogTitleString, "Acessar Unidade"))
                ShowWindow(GetDlgItem(hwndDlg, IDC_CHK_EXIBIRUNIDADE), SW_SHOW);                		
            else
                ShowWindow(GetDlgItem(hwndDlg, IDC_CHK_EXIBIRUNIDADE), SW_HIDE);                		
            
			/*
			if (PasswordDialogTitleStringId)
			{
				SetWindowTextW (hwndDlg, GetString (PasswordDialogTitleStringId));
			}
			else if (strlen (PasswordDlgVolume) > 0)
			{
				wchar_t s[1024];
				const int maxVisibleLen = 40;
				
				if (strlen (PasswordDlgVolume) > maxVisibleLen)
				{
					string volStr = PasswordDlgVolume;
					wsprintfW (s, GetString ("ENTER_PASSWORD_FOR"), ("..." + volStr.substr (volStr.size() - maxVisibleLen - 1)).c_str());
				}
				else
					wsprintfW (s, GetString ("ENTER_PASSWORD_FOR"), PasswordDlgVolume);

				SetWindowTextW (hwndDlg, s);
			}
            */
			SendMessage (GetDlgItem (hwndDlg, IDC_PASSWORD), EM_LIMITTEXT, MAX_PASSWORD, 0);
 			SetForegroundWindow (hwndDlg);
		}

		return 1;

	case WM_COMMAND:

		if ((hw==0) && (lw == IDCANCEL || lw == IDOK))
		{
			char tmp[MAX_PASSWORD+1];
			
			if (lw == IDOK)
			{
                /*ShowWindow(GetDlgItem (hwndDlg, IDOK), SW_HIDE);				
                ShowWindow(GetDlgItem (hwndDlg, IDCANCEL), SW_HIDE);
                InvalidateRect(hwndDlg, NULL, true);
                SendMessage(hwndDlg, WM_PAINT, NULL, NULL);
                RedrawWindow(hwndDlg, NULL, NULL, RDW_UPDATENOW);	*/			
				GetWindowText (GetDlgItem (hwndDlg, IDC_PASSWORD), (LPSTR) szXPwd->Text, MAX_PASSWORD + 1);
				szXPwd->Length = strlen ((char *) szXPwd->Text);
                g_bExibirUnidade = IsButtonChecked (GetDlgItem (hwndDlg, IDC_CHK_EXIBIRUNIDADE));
			}
			// Attempt to wipe password stored in the input field buffer
			memset (tmp, 'X', MAX_PASSWORD);
			tmp[MAX_PASSWORD] = 0;
			SetWindowText (GetDlgItem (hwndDlg, IDC_PASSWORD), tmp);	
			//SetWindowText (GetDlgItem (hwndDlg, IDC_PASSWORD_PROT_HIDVOL), tmp);	
			EndDialog (hwndDlg, lw);
			return 1;
		}
		return 0;
	}
	return 0;
}

//********* Rotina AskVolumePassword *********************************************
///
/// Copiado do projeto Mount. Chama o dialogo para pedir a senha da unidade.
///
/// \param HWND hwndDlg ? Handle da janela pai
/// \param Password *password ? Senha digitada
/// \param char *titleStringId ? Título da janela
/// \param BOOL enableMountOptions ? forçado para false (itens apagados do dialogo)
///
/// \return static LRESULT
//*******************************************************************

static int AskVolumePassword (HWND hwndDlg, Password *password, char *titleStringId, BOOL enableMountOptions)
{
	int result;

	//PasswordDialogTitleStringid = titleStringId;
	strcpy(PasswordDialogTitleString, titleStringId);
	
	result = DialogBoxParamW (hInst, 
		MAKEINTRESOURCEW (IDD_PASSWORD_DLG), hwndDlg,
		(DLGPROC) PasswordDlgProc, (LPARAM) password);

	if (result != IDOK)
	{
		password->Length = 0;
	}

	return result == IDOK;
}


//********* Rotina ElevateWholeWizardProcess *********************************************
///
/// Mantido original -- Eleva UAC para admin, exigido no setup e para alterar volume label
///
//*******************************************************************

static BOOL ElevateWholeWizardProcess (string arguments)
{
	char modPath[MAX_PATH];

	if (IsAdmin())
		return TRUE;

	if (!IsUacSupported())
		return IsAdmin();

	GetModuleFileName (NULL, modPath, sizeof (modPath));

	if ((int)ShellExecute (MainDlg, "runas", modPath, (string("/q UAC ") + arguments).c_str(), NULL, SW_SHOWNORMAL) > 32)
	{				
		exit (0);
	}
	else
	{
		Error ("UAC_INIT_ERROR");
		return FALSE;
	}
}

//********* Rotina WipePasswordsAndKeyfiles *********************************************
///
/// Mantido original -- Apaga valores armazenados em buffer (memória) para aumentar segurança do aplicativo
///
//*******************************************************************
static void WipePasswordsAndKeyfiles (void)
{
	char tmp[MAX_PASSWORD+1];

	// Attempt to wipe passwords stored in the input field buffers
	memset (tmp, 'X', MAX_PASSWORD);
	tmp [MAX_PASSWORD] = 0;
	SetWindowText (hPasswordInputField, tmp);
	SetWindowText (hVerifyPasswordInputField, tmp);

	burn (&szVerify[0], sizeof (szVerify));
	//burn (&volumePassword, sizeof (volumePassword));
	burn (&szRawPassword[0], sizeof (szRawPassword));

	SetWindowText (hPasswordInputField, "");
	SetWindowText (hVerifyPasswordInputField, "");

	KeyFileRemoveAll (&FirstKeyFile);
	KeyFileRemoveAll (&defaultKeyFilesParam.FirstKeyFile);
}

//********* Rotina localcleanup *********************************************
///
/// Mantido original - Acrescido da destruição dos Bitmaps alocados
///
//*******************************************************************

static void localcleanup (void)
{
	char tmp[RANDPOOL_DISPLAY_SIZE+1];

	// System encryption

	if (WizardMode == WIZARD_MODE_SYS_DEVICE
		&& InstanceHasSysEncMutex ())
	{
		try
		{
			BootEncStatus = BootEncObj->GetStatus();

			if (BootEncStatus.SetupInProgress)
			{
				BootEncObj->AbortSetup ();
			}
		}
		catch (...)
		{
			// NOP
		}
	}

	// Mon-system in-place encryption

	if (bInPlaceEncNonSys && (bVolTransformThreadRunning || bVolTransformThreadToRun))
	{
		NonSysInplaceEncPause ();
	}

	CloseNonSysInplaceEncMutex ();
	

	// Device wipe

	if (bDeviceWipeInProgress)
		WipeAbort();


	WipePasswordsAndKeyfiles ();

	Randfree ();

	burn (HeaderKeyGUIView, sizeof(HeaderKeyGUIView));
	burn (MasterKeyGUIView, sizeof(MasterKeyGUIView));
	burn (randPool, sizeof(randPool));
	burn (lastRandPool, sizeof(lastRandPool));
	burn (outRandPoolDispBuffer, sizeof(outRandPoolDispBuffer));
	burn (szFileName, sizeof(szFileName));
	burn (szDiskFile, sizeof(szDiskFile));

	// Attempt to wipe the GUI fields showing portions of randpool, of the master and header keys
	memset (tmp, 'X', sizeof(tmp));
	tmp [sizeof(tmp)-1] = 0;
	SetWindowText (hRandPool, tmp);
	SetWindowText (hRandPoolSys, tmp);
	SetWindowText (hMasterKey, tmp);
	SetWindowText (hHeaderKey, tmp);

	UnregisterRedTick (hInst);

	// Delete buffered bitmaps (if any)
	if (hbmWizardBitmapRescaled != NULL)
	{
		DeleteObject ((HGDIOBJ) hbmWizardBitmapRescaled);
		hbmWizardBitmapRescaled = NULL;
	}

	// Cleanup common code resources
	cleanup ();

	if (BootEncObj != NULL)
	{
		delete BootEncObj;
		BootEncObj = NULL;
	}

	if (HB_BACK != NULL)
		DeleteObject ((HGDIOBJ) HB_BACK);

    if (Hbmp_Montar != NULL)
	{
	     DeleteObject((HGDIOBJ) Hbmp_Montar);
		 DeleteObject((HGDIOBJ) Hbmp_Montar_c);
		 DeleteObject((HGDIOBJ) Hbmp_Liberar);
		 DeleteObject((HGDIOBJ) Hbmp_Liberar_c);
		 DeleteObject((HGDIOBJ) Hbmp_Trocar);
		 DeleteObject((HGDIOBJ) Hbmp_Trocar_c);
		 DeleteObject((HGDIOBJ) Hbmp_Concluir);
		 DeleteObject((HGDIOBJ) Hbmp_Concluir_c);
		 DeleteObject((HGDIOBJ) Hbmp_Avancar);
		 DeleteObject((HGDIOBJ) Hbmp_Avancar_c);
		 DeleteObject((HGDIOBJ) Hbmp_Desistir);
		 DeleteObject((HGDIOBJ) Hbmp_Desistir_c);
		 DeleteObject((HGDIOBJ) Hbmp_OK);
		 DeleteObject((HGDIOBJ) Hbmp_OK_c);
		 DeleteObject((HGDIOBJ) Hbmp_Cancelar);
		 DeleteObject((HGDIOBJ) Hbmp_Cancelar_c);
		 DeleteObject((HGDIOBJ) Hbmp_Excluir);
		 DeleteObject((HGDIOBJ) Hbmp_Excluir_c);


	}
}

//********* Rotina BroadcastSysEncCfgUpdateCallb *********************************************
///
/// Mantido original -Broadcast para avisar sobre alteração dos devices
///
//*******************************************************************
static BOOL CALLBACK BroadcastSysEncCfgUpdateCallb (HWND hwnd, LPARAM lParam)
{
	if (GetWindowLongPtr (hwnd, GWLP_USERDATA) == (LONG_PTR) 'TRUE')
	{
		char name[1024] = { 0 };
		GetWindowText (hwnd, name, sizeof (name) - 1);
		if (hwnd != MainDlg && strstr (name, "TrueCrypt"))
		{
			PostMessage (hwnd, TC_APPMSG_SYSENC_CONFIG_UPDATE, 0, 0);
		}
	}
	return TRUE;
}

//********* Rotina BroadcastSysEncCfgUpdate *********************************************
///
/// Mantido original - Utilizado pelo broadcast para avisar sobre alteração dos devices
///
//*******************************************************************

static BOOL BroadcastSysEncCfgUpdate (void)
{
	BOOL bSuccess = FALSE;
	EnumWindows (BroadcastSysEncCfgUpdateCallb, (LPARAM) &bSuccess);
	return bSuccess;
}

// IMPORTANT: This function may be called only by Format (other modules can only _read_ the system encryption config).
// Returns TRUE if successful (otherwise FALSE)
//********* Rotina SaveSysEncSettings *********************************************
///
/// Mantido original - Nao utilizado
///
//*******************************************************************
static BOOL SaveSysEncSettings (HWND hwndDlg)
{
	FILE *f;

	if (!bSystemEncryptionStatusChanged)
		return TRUE;

	if (hwndDlg == NULL && MainDlg != NULL)
		hwndDlg = MainDlg;

	if (!CreateSysEncMutex ())
		return FALSE;		// Only one instance that has the mutex can modify the system encryption settings

	if (SystemEncryptionStatus == SYSENC_STATUS_NONE)
	{
		if (remove (GetConfigPath (TC_APPD_FILENAME_SYSTEM_ENCRYPTION)) != 0)
		{
			Error ("CANNOT_SAVE_SYS_ENCRYPTION_SETTINGS");
			return FALSE;
		}

		bSystemEncryptionStatusChanged = FALSE;
		BroadcastSysEncCfgUpdate ();
		return TRUE;
	}

	f = fopen (GetConfigPath (TC_APPD_FILENAME_SYSTEM_ENCRYPTION), "w");
	if (f == NULL)
	{
		Error ("CANNOT_SAVE_SYS_ENCRYPTION_SETTINGS");
		handleWin32Error (hwndDlg);
		return FALSE;
	}

	if (XmlWriteHeader (f) < 0

	|| fputs ("\n\t<sysencryption>", f) < 0

	|| fprintf (f, "\n\t\t<config key=\"SystemEncryptionStatus\">%d</config>", SystemEncryptionStatus) < 0

	|| fprintf (f, "\n\t\t<config key=\"WipeMode\">%d</config>", (int) nWipeMode) < 0

	|| fputs ("\n\t</sysencryption>", f) < 0

	|| XmlWriteFooter (f) < 0)
	{
		handleWin32Error (hwndDlg);
		fclose (f);
		Error ("CANNOT_SAVE_SYS_ENCRYPTION_SETTINGS");
		return FALSE;
	}

	TCFlushFile (f);

	fclose (f);

	bSystemEncryptionStatusChanged = FALSE;
	BroadcastSysEncCfgUpdate ();

	return TRUE;
}

// WARNING: This function may take a long time to finish
//********* Rotina DetermineHiddenOSCreationPhase *********************************************
///
/// Mantido original - Nao utilizado
///
//*******************************************************************
static unsigned int DetermineHiddenOSCreationPhase (void)
{
	unsigned int phase = TC_HIDDEN_OS_CREATION_PHASE_NONE;

	try
	{
		phase = BootEncObj->GetHiddenOSCreationPhase();
	}
	catch (Exception &e)
	{
		e.Show (MainDlg);
		AbortProcess("ERR_GETTING_SYSTEM_ENCRYPTION_STATUS");
	}

	return phase;
}

// IMPORTANT: This function may be called only by Format (other modules can only _read_ the status).
// Returns TRUE if successful (otherwise FALSE)
//********* Rotina ChangeHiddenOSCreationPhase *********************************************
///
/// Mantido original - Nao utilizado
///
//*******************************************************************
static BOOL ChangeHiddenOSCreationPhase (int newPhase) 
{
	if (!CreateSysEncMutex ())
	{
		Error ("SYSTEM_ENCRYPTION_IN_PROGRESS_ELSEWHERE");
		return FALSE;
	}

	try
	{
		BootEncObj->SetHiddenOSCreationPhase (newPhase);
	}
	catch (Exception &e)
	{
		e.Show (MainDlg);
		return FALSE;
	}

	//// The contents of the following items might be inappropriate after a change of the phase
	//szFileName[0] = 0;
	//szDiskFile[0] = 0;
	//nUIVolumeSize = 0;
	//nVolumeSize = 0;

	return TRUE;
}

// IMPORTANT: This function may be called only by Format (other modules can only _read_ the system encryption status).
// Returns TRUE if successful (otherwise FALSE)
//********* Rotina ChangeSystemEncryptionStatus *********************************************
///
/// Mantido original - Nao utilizado
///
//*******************************************************************
static BOOL ChangeSystemEncryptionStatus (int newStatus)
{
	if (!CreateSysEncMutex ())
	{
		Error ("SYSTEM_ENCRYPTION_IN_PROGRESS_ELSEWHERE");
		return FALSE;		// Only one instance that has the mutex can modify the system encryption settings
	}

	SystemEncryptionStatus = newStatus;
	bSystemEncryptionStatusChanged = TRUE;

	if (newStatus == SYSENC_STATUS_ENCRYPTING)
	{
		// If the user has created a hidden OS and now is creating a decoy OS, we must wipe the hidden OS
		// config area in the MBR.
		WipeHiddenOSCreationConfig();
	}

	if (newStatus == SYSENC_STATUS_NONE && !IsHiddenOSRunning())
	{
		if (DetermineHiddenOSCreationPhase() != TC_HIDDEN_OS_CREATION_PHASE_NONE
			&& !ChangeHiddenOSCreationPhase (TC_HIDDEN_OS_CREATION_PHASE_NONE))
			return FALSE;

		WipeHiddenOSCreationConfig();
	}

	if (!SaveSysEncSettings (MainDlg))
	{
		return FALSE;
	}

	return TRUE;
}

// If the return code of this function is ignored and newWizardMode == WIZARD_MODE_SYS_DEVICE, then this function
// may be called only after CreateSysEncMutex() returns TRUE. It returns TRUE if successful (otherwise FALSE).
//********* Rotina ChangeWizardMode *********************************************
///
/// Mantido original - Nao utilizado
///
//*******************************************************************
static BOOL ChangeWizardMode (int newWizardMode)
{
	if (WizardMode != newWizardMode)	
	{
		if (WizardMode == WIZARD_MODE_SYS_DEVICE || newWizardMode == WIZARD_MODE_SYS_DEVICE)
		{
			if (newWizardMode == WIZARD_MODE_SYS_DEVICE)
			{
				if (!CreateSysEncMutex ())
				{
					Error ("SYSTEM_ENCRYPTION_IN_PROGRESS_ELSEWHERE");
					return FALSE;
				}
			}

			// If the previous mode was different, the password may have been typed using a different
			// keyboard layout (which might confuse the user and cause other problems if system encryption
			// was or will be involved).
			WipePasswordsAndKeyfiles();	
		}

		if (newWizardMode != WIZARD_MODE_NONSYS_DEVICE)
			bInPlaceEncNonSys = FALSE;

		if (newWizardMode == WIZARD_MODE_NONSYS_DEVICE && !IsAdmin() && IsUacSupported())
		{
			if (!ElevateWholeWizardProcess ("/e"))
				return FALSE;
		}

		// The contents of the following items may be inappropriate after a change of mode
		szFileName[0] = 0;
		szDiskFile[0] = 0;
		nUIVolumeSize = 0;
		nVolumeSize = 0;

		WizardMode = newWizardMode;
	}

	bDevice = (WizardMode != WIZARD_MODE_FILE_CONTAINER);

	if (newWizardMode != WIZARD_MODE_SYS_DEVICE 
		&& !bHiddenOS)
	{
		CloseSysEncMutex ();	
	}

	return TRUE;
}

// Determines whether the wizard directly affects system encryption in any way.
// Note, for example, that when the user enters a password for a hidden volume that is to host a hidden OS,
// WizardMode is NOT set to WIZARD_MODE_SYS_DEVICE. The keyboard layout, however, has to be US. That's why 
// this function has to be called instead of checking the value of WizardMode.
//********* Rotina SysEncInEffect *********************************************
///
/// Mantido original - Nao utilizado
///
//*******************************************************************
static BOOL SysEncInEffect (void)
{
	return (WizardMode == WIZARD_MODE_SYS_DEVICE
		|| CreatingHiddenSysVol());
}

//********* Rotina CreatingHiddenSysVol *********************************************
///
/// Mantido original - Nao utilizado
///
//*******************************************************************
static BOOL CreatingHiddenSysVol (void)
{
	return (bHiddenOS 
		&& bHiddenVol && !bHiddenVolHost);
}

//********* Rotina LoadSettings *********************************************
///
/// Mantido original - Nao utilizado
///
//*******************************************************************
static void LoadSettings (HWND hwndDlg)
{
	WipeAlgorithmId savedWipeAlgorithm = TC_WIPE_NONE;

	LoadSysEncSettings (hwndDlg);

	if (LoadNonSysInPlaceEncSettings (&savedWipeAlgorithm) != 0)
		bInPlaceEncNonSysPending = TRUE;

	defaultKeyFilesParam.EnableKeyFiles = FALSE;

	bStartOnLogon =	ConfigReadInt ("StartOnLogon", FALSE);

	HiddenSectorDetectionStatus = ConfigReadInt ("HiddenSectorDetectionStatus", 0);

	bHistory = ConfigReadInt ("SaveVolumeHistory", FALSE);

	ConfigReadString ("SecurityTokenLibrary", "", SecurityTokenLibraryPath, sizeof (SecurityTokenLibraryPath) - 1);
	if (SecurityTokenLibraryPath[0])
		InitSecurityTokenLibrary();

	if (hwndDlg != NULL)
	{
		LoadCombo (GetDlgItem (hwndDlg, IDC_COMBO_BOX));
		return;
	}

	if (bHistoryCmdLine)
		return;
}

//********* Rotina LoadSeSaveSettings *********************************************
///
/// Mantido original - Nao utilizado
///
//*******************************************************************
static void SaveSettings (HWND hwndDlg)
{
	WaitCursor ();

	if (hwndDlg != NULL)
		DumpCombo (GetDlgItem (hwndDlg, IDC_COMBO_BOX), !bHistory);

	ConfigWriteBegin ();

	ConfigWriteInt ("StartOnLogon",	bStartOnLogon);
	ConfigWriteInt ("HiddenSectorDetectionStatus", HiddenSectorDetectionStatus);
	ConfigWriteInt ("SaveVolumeHistory", bHistory);
	ConfigWriteString ("SecurityTokenLibrary", SecurityTokenLibraryPath[0] ? SecurityTokenLibraryPath : "");

	if (GetPreferredLangId () != NULL)
		ConfigWriteString ("Language", GetPreferredLangId ());

	ConfigWriteEnd ();

	NormalCursor ();
}

// WARNING: This function does NOT cause immediate application exit (use e.g. return 1 after calling it
// from a DialogProc function).
//********* Rotina EndMainDlg *********************************************
///
/// Mantido original - Encerra sessao
///
//*******************************************************************
static void EndMainDlg (HWND hwndDlg)
{
	if (nCurPageNo == VOLUME_LOCATION_PAGE)
	{
		if (IsWindow(GetDlgItem(hCurPage, IDC_NO_HISTORY)))
			bHistory = !IsButtonChecked (GetDlgItem (hCurPage, IDC_NO_HISTORY));

		MoveEditToCombo (GetDlgItem (hCurPage, IDC_COMBO_BOX), bHistory);
		SaveSettings (hCurPage);
	}
	else 
	{
		SaveSettings (NULL);
	}

	SaveSysEncSettings (hwndDlg);

	if (!bHistory)
		CleanLastVisitedMRU ();

	EndDialog (hwndDlg, 0);
}

// Returns TRUE if system encryption or decryption had been or is in progress and has not been completed
//********* Rotina SysEncryptionOrDecryptionRequired *********************************************
///
/// Mantido original - Nao utilizado
///
//*******************************************************************
static BOOL SysEncryptionOrDecryptionRequired (void)
{
	/* If you update this function, revise SysEncryptionOrDecryptionRequired() in Mount.c as well. */

	static BootEncryptionStatus locBootEncStatus;

	try
	{
		locBootEncStatus = BootEncObj->GetStatus();
	}
	catch (Exception &e)
	{
		e.Show (MainDlg);
	}

	return (SystemEncryptionStatus == SYSENC_STATUS_ENCRYPTING
		|| SystemEncryptionStatus == SYSENC_STATUS_DECRYPTING
		|| 
		(
			locBootEncStatus.DriveMounted 
			&& 
			(
				locBootEncStatus.ConfiguredEncryptedAreaStart != locBootEncStatus.EncryptedAreaStart
				|| locBootEncStatus.ConfiguredEncryptedAreaEnd != locBootEncStatus.EncryptedAreaEnd
			)
		)
	);
}

// Returns TRUE if the system partition/drive is completely encrypted
//********* Rotina SysDriveOrPartitionFullyEncrypted *********************************************
///
/// Mantido original - Nao utilizado
///
//*******************************************************************
static BOOL SysDriveOrPartitionFullyEncrypted (BOOL bSilent)
{
	/* If you update this function, revise SysDriveOrPartitionFullyEncrypted() in Mount.c as well. */

	static BootEncryptionStatus locBootEncStatus;

	try
	{
		locBootEncStatus = BootEncObj->GetStatus();
	}
	catch (Exception &e)
	{
		if (!bSilent)
			e.Show (MainDlg);
	}

	return (!locBootEncStatus.SetupInProgress
		&& locBootEncStatus.ConfiguredEncryptedAreaEnd != 0
		&& locBootEncStatus.ConfiguredEncryptedAreaEnd != -1
		&& locBootEncStatus.ConfiguredEncryptedAreaStart == locBootEncStatus.EncryptedAreaStart
		&& locBootEncStatus.ConfiguredEncryptedAreaEnd == locBootEncStatus.EncryptedAreaEnd);
}

// Adds or removes the wizard to/from the system startup sequence
//********* Rotina ManageStartupSeqWiz *********************************************
///
/// Mantido original - Nao utilizado
///
//*******************************************************************
void ManageStartupSeqWiz (BOOL bRemove, const char *arg)
{
	char regk [64];

	// Split the string in order to prevent some antivirus packages from falsely reporting  
	// TrueCrypt Format.exe to contain a possible Trojan horse because of this string (heuristic scan).
	sprintf (regk, "%s%s", "Software\\Microsoft\\Windows\\Curren", "tVersion\\Run");

	if (!bRemove)
	{
		char exe[MAX_PATH * 2] = { '"' };
		GetModuleFileName (NULL, exe + 1, sizeof (exe) - 1);

		if (strlen (arg) > 0)
		{
			strcat (exe, "\" ");
			strcat (exe, arg);
		}

		WriteRegistryString (regk, "TrueCrypt Format", exe);
	}
	else
		DeleteRegistryValue (regk, "TrueCrypt Format");
}

// This functions is to be used when the wizard mode needs to be changed to WIZARD_MODE_SYS_DEVICE.
// If the function fails to switch the mode, it returns FALSE (otherwise TRUE).
//********* Rotina SwitchWizardToSysEncMode *********************************************
///
/// Mantido original - Nao utilizado
///
//*******************************************************************
BOOL SwitchWizardToSysEncMode (void)
{
	WaitCursor ();

	try
	{
		BootEncStatus = BootEncObj->GetStatus();
		bWholeSysDrive = BootEncObj->SystemPartitionCoversWholeDrive();
	}
	catch (Exception &e)
	{
		e.Show (MainDlg);
		Error ("ERR_GETTING_SYSTEM_ENCRYPTION_STATUS");
		NormalCursor ();
		return FALSE;
	}

	// From now on, we should be the only instance of the TC wizard allowed to deal with system encryption
	if (!CreateSysEncMutex ())
	{
		Warning ("SYSTEM_ENCRYPTION_IN_PROGRESS_ELSEWHERE");
		NormalCursor ();
		return FALSE;
	}

	// User-mode app may have crashed and its mutex may have gotten lost, so we need to check the driver status too
	if (BootEncStatus.SetupInProgress)
	{
		if (AskWarnYesNo ("SYSTEM_ENCRYPTION_RESUME_PROMPT") == IDYES)
		{
			if (SystemEncryptionStatus != SYSENC_STATUS_ENCRYPTING
				&& SystemEncryptionStatus != SYSENC_STATUS_DECRYPTING)
			{
				// The config file with status was lost or not written correctly
				if (!ResolveUnknownSysEncDirection ())
				{
					CloseSysEncMutex ();	
					NormalCursor ();
					return FALSE;
				}
			}

			bDirectSysEncMode = TRUE;
			ChangeWizardMode (WIZARD_MODE_SYS_DEVICE);
			LoadPage (MainDlg, SYSENC_ENCRYPTION_PAGE);
			NormalCursor ();
			return TRUE;
		}
		else
		{
			CloseSysEncMutex ();	
			Error ("SYS_ENCRYPTION_OR_DECRYPTION_IN_PROGRESS");
			NormalCursor ();
			return FALSE;
		}
	}

	if (BootEncStatus.DriveMounted
		|| BootEncStatus.DriveEncrypted
		|| SysEncryptionOrDecryptionRequired ())
	{

		if (!SysDriveOrPartitionFullyEncrypted (FALSE)
			&& AskWarnYesNo ("SYSTEM_ENCRYPTION_RESUME_PROMPT") == IDYES)
		{
			if (SystemEncryptionStatus == SYSENC_STATUS_NONE)
			{
				// If the config file with status was lost or not written correctly, we
				// don't know whether to encrypt or decrypt (but we know that encryption or
				// decryption is required). Ask the user to select encryption, decryption, 
				// or cancel
				if (!ResolveUnknownSysEncDirection ())
				{
					CloseSysEncMutex ();	
					NormalCursor ();
					return FALSE;
				}
			}

			bDirectSysEncMode = TRUE;
			ChangeWizardMode (WIZARD_MODE_SYS_DEVICE);
			LoadPage (MainDlg, SYSENC_ENCRYPTION_PAGE);
			NormalCursor ();
			return TRUE;
		}
		else
		{
			CloseSysEncMutex ();	
			Error ("SETUP_FAILED_BOOT_DRIVE_ENCRYPTED");
			NormalCursor ();
			return FALSE;
		}
	}
	else
	{
		// Check compliance with requirements for boot encryption

		if (!IsAdmin())
		{
			if (!IsUacSupported())
			{
				Warning ("ADMIN_PRIVILEGES_WARN_DEVICES");
			}
		}

		try
		{
			BootEncObj->CheckRequirements ();
		}
		catch (Exception &e)
		{
			CloseSysEncMutex ();	
			e.Show (MainDlg);
			NormalCursor ();
			return FALSE;
		}

		if (!ChangeWizardMode (WIZARD_MODE_SYS_DEVICE))
		{
			NormalCursor ();
			return FALSE;
		}

		if (bSysDriveSelected || bSysPartitionSelected)
		{
			// The user selected the non-sys-device wizard mode but then selected a system device

			bWholeSysDrive = (bSysDriveSelected && !bSysPartitionSelected);

			bSysDriveSelected = FALSE;
			bSysPartitionSelected = FALSE;

			try
			{
				if (!bHiddenVol)
				{
					if (bWholeSysDrive && !BootEncObj->SystemPartitionCoversWholeDrive())
					{
						if (nCurrentOS != WIN_VISTA_OR_LATER)
						{
							if (BootEncObj->SystemDriveContainsExtendedPartition())
							{
								bWholeSysDrive = FALSE;

								Error ("WDE_UNSUPPORTED_FOR_EXTENDED_PARTITIONS");

								if (AskYesNo ("ASK_ENCRYPT_PARTITION_INSTEAD_OF_DRIVE") == IDNO)
								{
									ChangeWizardMode (WIZARD_MODE_NONSYS_DEVICE);
									return FALSE;
								}
							}
							else
								Warning ("WDE_EXTENDED_PARTITIONS_WARNING");
						}
					}
					else if (BootEncObj->SystemPartitionCoversWholeDrive() 
						&& !bWholeSysDrive)
						bWholeSysDrive = (AskYesNo ("WHOLE_SYC_DEVICE_RECOM") == IDYES);
				}

			}
			catch (Exception &e)
			{
				e.Show (MainDlg);
				return FALSE;
			}

			if (!bHiddenVol)
			{
				// Skip SYSENC_SPAN_PAGE and SYSENC_TYPE_PAGE as the user already made the choice
				LoadPage (MainDlg, bWholeSysDrive ? SYSENC_PRE_DRIVE_ANALYSIS_PAGE : SYSENC_MULTI_BOOT_MODE_PAGE);	
			}
			else
			{
				// The user selected the non-sys-device wizard mode but then selected a system device.
				// In addition, he selected the hidden volume mode.

				if (bWholeSysDrive)
					Warning ("HIDDEN_OS_PRECLUDES_SINGLE_KEY_WDE");

				bWholeSysDrive = FALSE;

				LoadPage (MainDlg, SYSENC_TYPE_PAGE);
			}
		}
		else
			LoadPage (MainDlg, SYSENC_TYPE_PAGE);

		NormalCursor ();
		return TRUE;
	}

	CloseSysEncMutex ();	
	NormalCursor ();
	return FALSE;
}

//********* Rotina SwitchWizardToFileContainerMode *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
void SwitchWizardToFileContainerMode (void)
{
	ChangeWizardMode (WIZARD_MODE_FILE_CONTAINER);

	LoadPage (MainDlg, VOLUME_LOCATION_PAGE);

	NormalCursor ();
}

//********* Rotina SwitchWizardToNonSysDeviceMode *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
void SwitchWizardToNonSysDeviceMode (void)
{
	ChangeWizardMode (WIZARD_MODE_NONSYS_DEVICE);

	LoadPage (MainDlg, VOLUME_TYPE_PAGE);

	NormalCursor ();
}

//********* Rotina SwitchWizardToHiddenOSMode *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
BOOL SwitchWizardToHiddenOSMode (void)
{
	if (SwitchWizardToSysEncMode())
	{
		if (nCurPageNo != SYSENC_ENCRYPTION_PAGE)	// If the user did not manually choose to resume encryption or decryption of the system partition/drive
		{
			bHiddenOS = TRUE;
			bHiddenVol = TRUE;
			bHiddenVolHost = TRUE;
			bHiddenVolDirect = FALSE;
			bWholeSysDrive = FALSE;
			bInPlaceEncNonSys = FALSE;

			if (bDirectSysEncModeCommand == SYSENC_COMMAND_CREATE_HIDDEN_OS_ELEV)
			{
				// Some of the requirements for hidden OS should have already been checked by the wizard process
				// that launched us (in order to elevate), but we must recheck them. Otherwise, an advanced user 
				// could bypass the checks by using the undocumented CLI switch. Moreover, some requirements
				// can be checked only at this point (when we are elevated).
				try
				{
					BootEncObj->CheckRequirementsHiddenOS ();

					BootEncObj->InitialSecurityChecksForHiddenOS ();
				}
				catch (Exception &e)
				{
					e.Show (MainDlg);
					return FALSE;
				}

				LoadPage (MainDlg, SYSENC_MULTI_BOOT_MODE_PAGE);
			}
			else
				LoadPage (MainDlg, SYSENC_HIDDEN_OS_REQ_CHECK_PAGE);

			NormalCursor ();
		}
		else
			return TRUE;
	}
	else
		return FALSE;

	return TRUE;
}

//********* Rotina SwitchWizardToNonSysInplaceEncResumeMode *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
void SwitchWizardToNonSysInplaceEncResumeMode (void)
{
	if (!IsAdmin() && IsUacSupported())
	{
		if (!ElevateWholeWizardProcess ("/zinplace"))
			AbortProcessSilent ();
	}

	if (!IsAdmin())
		AbortProcess("ADMIN_PRIVILEGES_WARN_DEVICES");

	CreateNonSysInplaceEncMutex ();

	bInPlaceEncNonSys = TRUE;
	bInPlaceEncNonSysResumed = TRUE;

	ChangeWizardMode (WIZARD_MODE_NONSYS_DEVICE);

	LoadPage (MainDlg, NONSYS_INPLACE_ENC_RESUME_PASSWORD_PAGE);
}

// Use this function e.g. if the config file with the system encryption settings was lost or not written
// correctly, and we don't know whether to encrypt or decrypt (but we know that encryption or decryption
// is required). Returns FALSE if failed or cancelled.
//********* Rotina ResolveUnknownSysEncDirection *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static BOOL ResolveUnknownSysEncDirection (void)
{
	if (CreateSysEncMutex ())
	{
		if (SystemEncryptionStatus != SYSENC_STATUS_ENCRYPTING
			&& SystemEncryptionStatus != SYSENC_STATUS_DECRYPTING)
		{
			try
			{
				BootEncStatus = BootEncObj->GetStatus();
			}
			catch (Exception &e)
			{
				e.Show (MainDlg);
				Error ("ERR_GETTING_SYSTEM_ENCRYPTION_STATUS");
				return FALSE;
			}

			if (BootEncStatus.SetupInProgress)
			{
				return ChangeSystemEncryptionStatus (
					(BootEncStatus.SetupMode != SetupDecryption) ? SYSENC_STATUS_ENCRYPTING : SYSENC_STATUS_DECRYPTING);
			}
			else
			{
				// Ask the user to select encryption, decryption, or cancel

				char *tmpStr[] = {0,
					!BootEncStatus.DriveEncrypted ? "CHOOSE_ENCRYPT_OR_DECRYPT_FINALIZE_DECRYPT_NOTE" : "CHOOSE_ENCRYPT_OR_DECRYPT",
					"ENCRYPT",
					"DECRYPT",
					"IDCANCEL",
					0};

				switch (AskMultiChoice ((void **) tmpStr, FALSE))
				{
				case 1:
					return ChangeSystemEncryptionStatus (SYSENC_STATUS_ENCRYPTING);
				case 2:
					return ChangeSystemEncryptionStatus (SYSENC_STATUS_DECRYPTING);
				default:
					return FALSE;
				}
			}
		}
		else
			return TRUE;
	}
	else
	{
		Error ("SYSTEM_ENCRYPTION_IN_PROGRESS_ELSEWHERE");
		return FALSE;
	}
}

// This function should be used to resolve inconsistencies that might lead to a deadlock (inability to encrypt or
// decrypt the system partition/drive and to uninstall TrueCrypt). The function removes the system encryption key 
// data ("volume header"), the TrueCrypt boot loader, restores the original system loader (if available),
// unregisters the boot driver, etc. Note that if the system partition/drive is encrypted, it will start decrypting
// it in the background (therefore, it should be used when the system partition/drive is not encrypted, ideally).
// Exceptions are handled and errors are reported within the function. Returns TRUE if successful.
//********* Rotina ForceRemoveSysEnc *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static BOOL ForceRemoveSysEnc (void)
{
	if (CreateSysEncMutex ())	// If no other instance is currently taking care of system encryption
	{
		BootEncryptionStatus locBootEncStatus;

		try
		{
			locBootEncStatus = BootEncObj->GetStatus();

			if (locBootEncStatus.SetupInProgress)
				BootEncObj->AbortSetupWait ();

			locBootEncStatus = BootEncObj->GetStatus();

			if (locBootEncStatus.DriveMounted)
			{
				// Remove the header
				BootEncObj->StartDecryption ();			
				locBootEncStatus = BootEncObj->GetStatus();

				while (locBootEncStatus.SetupInProgress)
				{
					Sleep (100);
					locBootEncStatus = BootEncObj->GetStatus();
				}

				BootEncObj->CheckEncryptionSetupResult ();
			}

			Sleep (50);
		}
		catch (Exception &e)
		{
			e.Show (MainDlg);
			return FALSE;
		}

		try
		{
			locBootEncStatus = BootEncObj->GetStatus();

			if (!locBootEncStatus.DriveMounted)
				BootEncObj->Deinstall ();
		}
		catch (Exception &e)
		{
			e.Show (MainDlg);
			return FALSE;
		}

		return TRUE;
	}
	else
		return FALSE;
}

// Returns 0 if there's an error.
//********* Rotina GetSystemPartitionSize *********************************************
///
/// Mantido original 
///
//*******************************************************************
__int64 GetSystemPartitionSize (void)
{
	try
	{
		return BootEncObj->GetSystemDriveConfiguration().SystemPartition.Info.PartitionLength.QuadPart;
	}
	catch (Exception &e)
	{
		e.Show (MainDlg);
		return 0;
	}
}

//********* Rotina ComboSelChangeEA *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
void ComboSelChangeEA (HWND hwndDlg)
{
	LPARAM nIndex = SendMessage (GetDlgItem (hwndDlg, IDC_COMBO_BOX), CB_GETCURSEL, 0, 0);

	if (nIndex == CB_ERR)
	{
		SetWindowText (GetDlgItem (hwndDlg, IDC_BOX_HELP), "");
	}
	else
	{
		char name[100];
		wchar_t auxLine[4096];
		wchar_t hyperLink[256] = { 0 };
		char cipherIDs[5];
		int i, cnt = 0;

		nIndex = SendMessage (GetDlgItem (hwndDlg, IDC_COMBO_BOX), CB_GETITEMDATA, nIndex, 0);
		EAGetName (name, nIndex);

		if (strcmp (name, "AES") == 0)
		{
			swprintf_s (hyperLink, sizeof(hyperLink) / 2, GetString ("MORE_INFO_ABOUT"), name);

			SetWindowTextW (GetDlgItem (hwndDlg, IDC_BOX_HELP), GetString ("AES_HELP"));
		}
		else if (strcmp (name, "Serpent") == 0)
		{
			swprintf_s (hyperLink, sizeof(hyperLink) / 2, GetString ("MORE_INFO_ABOUT"), name);
				
			SetWindowTextW (GetDlgItem (hwndDlg, IDC_BOX_HELP), GetString ("SERPENT_HELP"));
		}
		else if (strcmp (name, "Twofish") == 0)
		{
			swprintf_s (hyperLink, sizeof(hyperLink) / 2, GetString ("MORE_INFO_ABOUT"), name);

			SetWindowTextW (GetDlgItem (hwndDlg, IDC_BOX_HELP), GetString ("TWOFISH_HELP"));
		}
		else if (EAGetCipherCount (nIndex) > 1)
		{
			// Cascade
			cipherIDs[cnt++] = i = EAGetLastCipher(nIndex);
			while (i = EAGetPreviousCipher(nIndex, i))
			{
				cipherIDs[cnt] = i;
				cnt++; 
			}

			switch (cnt)	// Number of ciphers in the cascade
			{
			case 2:
				swprintf (auxLine, GetString ("TWO_LAYER_CASCADE_HELP"), 
					CipherGetName (cipherIDs[1]),
					CipherGetKeySize (cipherIDs[1])*8,
					CipherGetName (cipherIDs[0]),
					CipherGetKeySize (cipherIDs[0])*8);
				break;

			case 3:
				swprintf (auxLine, GetString ("THREE_LAYER_CASCADE_HELP"), 
					CipherGetName (cipherIDs[2]),
					CipherGetKeySize (cipherIDs[2])*8,
					CipherGetName (cipherIDs[1]),
					CipherGetKeySize (cipherIDs[1])*8,
					CipherGetName (cipherIDs[0]),
					CipherGetKeySize (cipherIDs[0])*8);
				break;
			}

			wcscpy_s (hyperLink, sizeof(hyperLink) / 2, GetString ("IDC_LINK_MORE_INFO_ABOUT_CIPHER"));

			SetWindowTextW (GetDlgItem (hwndDlg, IDC_BOX_HELP), auxLine);
		}
		else
		{
			// No info available for this encryption algorithm
			SetWindowTextW (GetDlgItem (hwndDlg, IDC_BOX_HELP), L"");
		}


		// Update hyperlink
		SetWindowTextW (GetDlgItem (hwndDlg, IDC_LINK_MORE_INFO_ABOUT_CIPHER), hyperLink);
		AccommodateTextField (hwndDlg, IDC_LINK_MORE_INFO_ABOUT_CIPHER, FALSE);
	}
}

//********* Rotina VerifySizeAndUpdate *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static void VerifySizeAndUpdate (HWND hwndDlg, BOOL bUpdate)
{
	BOOL bEnable = TRUE;
	char szTmp[50];
	__int64 lTmp;
	size_t i;
	static unsigned __int64 nLastVolumeSize = 0;

	GetWindowText (GetDlgItem (hwndDlg, IDC_SIZEBOX), szTmp, sizeof (szTmp));

	for (i = 0; i < strlen (szTmp); i++)
	{
		if (szTmp[i] >= '0' && szTmp[i] <= '9')
			continue;
		else
		{
			bEnable = FALSE;
			break;
		}
	}

	if (IsButtonChecked (GetDlgItem (hwndDlg, IDC_KB)))
		nMultiplier = BYTES_PER_KB;
	else if (IsButtonChecked (GetDlgItem (hwndDlg, IDC_MB)))
		nMultiplier = BYTES_PER_MB;
	else
		nMultiplier = BYTES_PER_GB;

	if (bDevice && !(bHiddenVol && !bHiddenVolHost))	// If raw device but not a hidden volume
	{
		lTmp = nVolumeSize;
		i = 1;
	}
	else
	{
		i = nMultiplier;
		lTmp = _atoi64 (szTmp);
	}

	if (bEnable)
	{
		if (lTmp * i < (bHiddenVolHost ? TC_MIN_HIDDEN_VOLUME_HOST_SIZE : (bHiddenVol ? TC_MIN_HIDDEN_VOLUME_SIZE : TC_MIN_VOLUME_SIZE)))
			bEnable = FALSE;

		if (!bHiddenVolHost && bHiddenVol)
		{
			if (lTmp * i > nMaximumHiddenVolSize)
				bEnable = FALSE;
		}
		else
		{
			if (lTmp * i > (bHiddenVolHost ? TC_MAX_HIDDEN_VOLUME_HOST_SIZE : TC_MAX_VOLUME_SIZE))
				bEnable = FALSE;
		}

		if (lTmp * i % SECTOR_SIZE != 0)
			bEnable = FALSE;
	}

	if (bUpdate)
	{
		nUIVolumeSize = lTmp;

		if (!bDevice || (bHiddenVol && !bHiddenVolHost))	// Update only if it's not a raw device or if it's a hidden volume
			nVolumeSize = i * lTmp;
	}

	EnableWindow (GetDlgItem (GetParent (hwndDlg), IDC_NEXT), bEnable);

	if (nVolumeSize != nLastVolumeSize)
	{
		// Change of volume size may make some file systems allowed or disallowed, so the default filesystem must
		// be reselected.
		fileSystem = FILESYS_NONE;	
		nLastVolumeSize = nVolumeSize;
	}
}

//********* Rotina UpdateWizardModeControls *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static void UpdateWizardModeControls (HWND hwndDlg, int setWizardMode)
{
	SendMessage (GetDlgItem (hwndDlg, IDC_FILE_CONTAINER),
		BM_SETCHECK,
		setWizardMode == WIZARD_MODE_FILE_CONTAINER ? BST_CHECKED : BST_UNCHECKED,
		0);

	SendMessage (GetDlgItem (hwndDlg, IDC_NONSYS_DEVICE),
		BM_SETCHECK,
		setWizardMode == WIZARD_MODE_NONSYS_DEVICE ? BST_CHECKED : BST_UNCHECKED,
		0);

	SendMessage (GetDlgItem (hwndDlg, IDC_SYS_DEVICE),
		BM_SETCHECK,
		setWizardMode == WIZARD_MODE_SYS_DEVICE ? BST_CHECKED : BST_UNCHECKED,
		0);
}

//********* Rotina GetSelectedWizardMode *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static int GetSelectedWizardMode (HWND hwndDlg)
{
	if (IsButtonChecked (GetDlgItem (hwndDlg, IDC_FILE_CONTAINER)))
		return WIZARD_MODE_FILE_CONTAINER;

	if (IsButtonChecked (GetDlgItem (hwndDlg, IDC_NONSYS_DEVICE)))
		return WIZARD_MODE_NONSYS_DEVICE;

	if (IsButtonChecked (GetDlgItem (hwndDlg, IDC_SYS_DEVICE)))
		return WIZARD_MODE_SYS_DEVICE;

	return DEFAULT_VOL_CREATION_WIZARD_MODE;
}

//********* Rotina RefreshMultiBootControls *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static void RefreshMultiBootControls (HWND hwndDlg)
{
	SendMessage (GetDlgItem (hwndDlg, IDC_SINGLE_BOOT),
		BM_SETCHECK,
		nMultiBoot == 1 ? BST_CHECKED : BST_UNCHECKED,
		0);

	SendMessage (GetDlgItem (hwndDlg, IDC_MULTI_BOOT),
		BM_SETCHECK,
		nMultiBoot > 1 ? BST_CHECKED : BST_UNCHECKED,
		0);
}

// -1 = Undecided or error, 0 = No, 1 = Yes
//********* Rotina Get2RadButtonPageAnswer *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static int Get2RadButtonPageAnswer (void)
{
	if (IsButtonChecked (GetDlgItem (hCurPage, IDC_CHOICE1)))
		return 1;

	if (IsButtonChecked (GetDlgItem (hCurPage, IDC_CHOICE2)))
		return 0;

	return -1;
}

// 0 = No, 1 = Yes
//********* Rotina Update2RadButtonPage *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static void Update2RadButtonPage (int answer)
{
	SendMessage (GetDlgItem (hCurPage, IDC_CHOICE1),
		BM_SETCHECK,
		answer == 1 ? BST_CHECKED : BST_UNCHECKED,
		0);

	SendMessage (GetDlgItem (hCurPage, IDC_CHOICE2),
		BM_SETCHECK,
		answer == 0 ? BST_CHECKED : BST_UNCHECKED,
		0);
}

// -1 = Undecided, 0 = No, 1 = Yes
//********* Rotina Init2RadButtonPageYesNo *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static void Init2RadButtonPageYesNo (int answer)
{
	SetWindowTextW (GetDlgItem (hCurPage, IDC_CHOICE1), GetString ("UISTR_YES"));
	SetWindowTextW (GetDlgItem (hCurPage, IDC_CHOICE2), GetString ("UISTR_NO"));

	SetWindowTextW (GetDlgItem (MainDlg, IDC_NEXT), GetString ("NEXT"));
	SetWindowTextW (GetDlgItem (MainDlg, IDC_PREV), GetString ("PREV"));
	SetWindowTextW (GetDlgItem (MainDlg, IDCANCEL), GetString ("CANCEL"));

	EnableWindow (GetDlgItem (MainDlg, IDC_NEXT), answer >= 0);
	EnableWindow (GetDlgItem (MainDlg, IDC_PREV), TRUE);

	Update2RadButtonPage (answer);
}

//********* Rotina UpdateSysEncProgressBar *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static void UpdateSysEncProgressBar (void)
{
	BootEncryptionStatus locBootEncStatus;
	static BOOL lastTransformWaitingForIdle = FALSE;

	try
	{
		locBootEncStatus = BootEncObj->GetStatus();
	}
	catch (...)
	{
		return;
	}

	if (locBootEncStatus.EncryptedAreaEnd == -1 
		|| locBootEncStatus.EncryptedAreaStart == -1)
	{
		UpdateProgressBarProc (0);
	}
	else
	{
		UpdateProgressBarProc ((locBootEncStatus.EncryptedAreaEnd - locBootEncStatus.EncryptedAreaStart + 1) / SECTOR_SIZE);

		if (locBootEncStatus.SetupInProgress)
		{
			wchar_t tmpStr[100];

			// Status

			if (locBootEncStatus.TransformWaitingForIdle)
				wcscpy (tmpStr, GetString ("PROGRESS_STATUS_WAITING"));
			else
				wcscpy (tmpStr, GetString (SystemEncryptionStatus == SYSENC_STATUS_DECRYPTING ? "PROGRESS_STATUS_DECRYPTING" : "PROGRESS_STATUS_ENCRYPTING"));

			wcscat (tmpStr, L" ");

			SetWindowTextW (GetDlgItem (hCurPage, IDC_WRITESPEED), tmpStr);

			// Remainining time 

			if (locBootEncStatus.TransformWaitingForIdle)
			{
				// The estimate cannot be computed correctly when speed is zero
				SetWindowTextW (GetDlgItem (hCurPage, IDC_TIMEREMAIN), GetString ("NOT_APPLICABLE_OR_NOT_AVAILABLE"));
			}

			if (locBootEncStatus.TransformWaitingForIdle != lastTransformWaitingForIdle)
			{
				if (lastTransformWaitingForIdle)
				{
					// Estimate of remaining time and other values may have been heavily distorted as the speed
					// was zero. Therefore, we're going to reinitialize the progress bar and all related variables.
					InitSysEncProgressBar ();
				}
				lastTransformWaitingForIdle = locBootEncStatus.TransformWaitingForIdle;
			}
		}
	}
}

//********* Rotina InitSysEncProgressBar *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static void InitSysEncProgressBar (void)
{
	BootEncryptionStatus locBootEncStatus;

	try
	{
		locBootEncStatus = BootEncObj->GetStatus();
	}
	catch (...)
	{
		return;
	}

	if (locBootEncStatus.ConfiguredEncryptedAreaEnd == -1 
		|| locBootEncStatus.ConfiguredEncryptedAreaStart == -1)
		return;

	InitProgressBar ((locBootEncStatus.ConfiguredEncryptedAreaEnd 
		- locBootEncStatus.ConfiguredEncryptedAreaStart + 1) / SECTOR_SIZE,
		(locBootEncStatus.EncryptedAreaEnd == locBootEncStatus.EncryptedAreaStart || locBootEncStatus.EncryptedAreaEnd == -1) ?
		0 :	locBootEncStatus.EncryptedAreaEnd - locBootEncStatus.EncryptedAreaStart + 1,
		SystemEncryptionStatus == SYSENC_STATUS_DECRYPTING,
		TRUE,
		TRUE,
		TRUE);
}
//********* Rotina UpdateSysEncControls *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static void UpdateSysEncControls (void)
{
	BootEncryptionStatus locBootEncStatus;

	try
	{
		locBootEncStatus = BootEncObj->GetStatus();
	}
	catch (...)
	{
		return;
	}

	EnableWindow (GetDlgItem (hCurPage, IDC_WIPE_MODE), 
		!locBootEncStatus.SetupInProgress 
		&& SystemEncryptionStatus == SYSENC_STATUS_ENCRYPTING);

	SetWindowTextW (GetDlgItem (hCurPage, IDC_PAUSE),
		GetString (locBootEncStatus.SetupInProgress ? "IDC_PAUSE" : "RESUME"));

	EnableWindow (GetDlgItem (MainDlg, IDC_NEXT), !locBootEncStatus.SetupInProgress && !bFirstSysEncResumeDone);

	if (!locBootEncStatus.SetupInProgress)
	{
		wchar_t tmpStr[100];

		wcscpy (tmpStr, GetString ((SysDriveOrPartitionFullyEncrypted (TRUE) || !locBootEncStatus.DriveMounted) ?
			"PROGRESS_STATUS_FINISHED" : "PROGRESS_STATUS_PAUSED"));
		wcscat (tmpStr, L" ");

		// Status
		SetWindowTextW (GetDlgItem (hCurPage, IDC_WRITESPEED), tmpStr);

		if (SysDriveOrPartitionFullyEncrypted (TRUE) || SystemEncryptionStatus == SYSENC_STATUS_NONE)
		{
			wcscpy (tmpStr, GetString ("PROCESSED_PORTION_100_PERCENT"));
			wcscat (tmpStr, L" ");

			SetWindowTextW (GetDlgItem (hCurPage, IDC_BYTESWRITTEN), tmpStr);
		}

		SetWindowText (GetDlgItem (hCurPage, IDC_TIMEREMAIN), " ");
	}
}

//********* Rotina SysEncPause *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static void SysEncPause (void)
{
	BootEncryptionStatus locBootEncStatus;

	if (CreateSysEncMutex ())
	{
		EnableWindow (GetDlgItem (hCurPage, IDC_PAUSE), FALSE);

		try
		{
			locBootEncStatus = BootEncObj->GetStatus();
		}
		catch (Exception &e)
		{
			e.Show (MainDlg);
			Error ("ERR_GETTING_SYSTEM_ENCRYPTION_STATUS");
			EnableWindow (GetDlgItem (hCurPage, IDC_PAUSE), TRUE);
			return;
		}

		if (!locBootEncStatus.SetupInProgress)
		{
			EnableWindow (GetDlgItem (hCurPage, IDC_PAUSE), TRUE);
			return;
		}

		WaitCursor ();

		try
		{
			int attempts = SYSENC_PAUSE_RETRIES;

			BootEncObj->AbortSetup ();

			locBootEncStatus = BootEncObj->GetStatus();

			while (locBootEncStatus.SetupInProgress && attempts > 0)
			{
				Sleep (SYSENC_PAUSE_RETRY_INTERVAL);
				attempts--;
				locBootEncStatus = BootEncObj->GetStatus();
			}

			if (!locBootEncStatus.SetupInProgress)
				BootEncObj->CheckEncryptionSetupResult ();

		}
		catch (Exception &e)
		{
			e.Show (MainDlg);
		}

		NormalCursor ();

		if (locBootEncStatus.SetupInProgress)
		{
			SetTimer (MainDlg, TIMER_ID_SYSENC_PROGRESS, TIMER_INTERVAL_SYSENC_PROGRESS, NULL);
			EnableWindow (GetDlgItem (hCurPage, IDC_PAUSE), TRUE);
			Error ("FAILED_TO_INTERRUPT_SYSTEM_ENCRYPTION");
			return;
		}
		
		UpdateSysEncControls ();
		EnableWindow (GetDlgItem (hCurPage, IDC_PAUSE), TRUE);
	}
	else
		Error ("SYSTEM_ENCRYPTION_IN_PROGRESS_ELSEWHERE");
}


//********* Rotina SysEncResume *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static void SysEncResume (void)
{
	BootEncryptionStatus locBootEncStatus;

	if (CreateSysEncMutex ())
	{
		EnableWindow (GetDlgItem (hCurPage, IDC_PAUSE), FALSE);

		try
		{
			locBootEncStatus = BootEncObj->GetStatus();
		}
		catch (Exception &e)
		{
			e.Show (MainDlg);
			Error ("ERR_GETTING_SYSTEM_ENCRYPTION_STATUS");
			EnableWindow (GetDlgItem (hCurPage, IDC_PAUSE), TRUE);
			return;
		}

		if (locBootEncStatus.SetupInProgress)
		{
			bSystemEncryptionInProgress = TRUE;
			UpdateSysEncControls ();
			SetTimer (MainDlg, TIMER_ID_SYSENC_PROGRESS, TIMER_INTERVAL_SYSENC_PROGRESS, NULL);
			EnableWindow (GetDlgItem (hCurPage, IDC_PAUSE), TRUE);
			return;
		}

		bSystemEncryptionInProgress = FALSE;
		WaitCursor ();

		try
		{
			switch (SystemEncryptionStatus)
			{
			case SYSENC_STATUS_ENCRYPTING:

				BootEncObj->StartEncryption (nWipeMode, bTryToCorrectReadErrors ? true : false);	
				break;

			case SYSENC_STATUS_DECRYPTING:

				if (locBootEncStatus.DriveMounted)	// If the drive is not encrypted we will just deinstall
					BootEncObj->StartDecryption ();	

				break;
			}

			bSystemEncryptionInProgress = TRUE;
		}
		catch (Exception &e)
		{
			e.Show (MainDlg);
		}

		NormalCursor ();

		if (!bSystemEncryptionInProgress)
		{
			EnableWindow (GetDlgItem (hCurPage, IDC_PAUSE), TRUE);
			Error ("FAILED_TO_RESUME_SYSTEM_ENCRYPTION");
			return;
		}

		bFirstSysEncResumeDone = TRUE;
		InitSysEncProgressBar ();
		UpdateSysEncProgressBar ();
		UpdateSysEncControls ();
		EnableWindow (GetDlgItem (hCurPage, IDC_PAUSE), TRUE);
		SetTimer (MainDlg, TIMER_ID_SYSENC_PROGRESS, TIMER_INTERVAL_SYSENC_PROGRESS, NULL);
	}
	else
		Error ("SYSTEM_ENCRYPTION_IN_PROGRESS_ELSEWHERE");
}


//********* Rotina GetDevicePathForHiddenOS *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static BOOL GetDevicePathForHiddenOS (void)
{
	BOOL tmpbDevice = FALSE;

	try
	{
		strncpy (szFileName, BootEncObj->GetPartitionForHiddenOS().DevicePath.c_str(), sizeof(szFileName));

		CreateFullVolumePath (szDiskFile, szFileName, &tmpbDevice);
	}
	catch (Exception &e)
	{
		e.Show (MainDlg);
		return FALSE;
	}

	return (szFileName[0] != 0 
		&& szDiskFile[0] != 0 
		&& tmpbDevice);
}


// Returns TRUE if there is unallocated space greater than 64 MB (max possible slack space size) between the 
// boot partition and the first partition behind it. If there's none or if an error occurs, returns FALSE.
//********* Rotina CheckGapBetweenSysAndHiddenOS *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static BOOL CheckGapBetweenSysAndHiddenOS (void)
{
	try
	{
		SystemDriveConfiguration sysDriveCfg = BootEncObj->GetSystemDriveConfiguration();

		return (sysDriveCfg.SystemPartition.Info.StartingOffset.QuadPart 
			+ sysDriveCfg.SystemPartition.Info.PartitionLength.QuadPart
			+ 64 * BYTES_PER_MB
			+ 128 * BYTES_PER_KB
			<= BootEncObj->GetPartitionForHiddenOS().Info.StartingOffset.QuadPart);
	}
	catch (Exception &e)
	{
		e.Show (MainDlg);
	}

	return FALSE;
}

//********* Rotina NonSysInplaceEncPause *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static void NonSysInplaceEncPause (void)
{
	bVolTransformThreadCancel = TRUE;

	WaitCursor ();

	int waitThreshold = 100;	// Do not block GUI events for more than 10 seconds. IMPORTANT: This prevents deadlocks when the thread calls us back e.g. to update GUI!
	
	while (bVolTransformThreadRunning || bVolTransformThreadToRun)
	{
		MSG guiMsg;

		bVolTransformThreadCancel = TRUE;

		if (waitThreshold <= 0)
		{
			while (PeekMessage (&guiMsg, NULL, 0, 0, PM_REMOVE) != 0)
			{
				DispatchMessage (&guiMsg);
			}
		}
		else
			waitThreshold--;

		Sleep (100);
	}
}

//********* Rotina NonSysInplaceEncResume *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
static void NonSysInplaceEncResume (void)
{
	if (bVolTransformThreadRunning || bVolTransformThreadToRun || bVolTransformThreadCancel)
		return;

	if (!bInPlaceEncNonSysResumed
		&& !FinalPreTransformPrompts ())
	{
		return;
	}

	CreateNonSysInplaceEncMutex ();

	bFirstNonSysInPlaceEncResumeDone = TRUE;

	SetTimer (MainDlg, TIMER_ID_NONSYS_INPLACE_ENC_PROGRESS, TIMER_INTERVAL_NONSYS_INPLACE_ENC_PROGRESS, NULL);

	bVolTransformThreadCancel = FALSE;
	bVolTransformThreadToRun = TRUE;

	UpdateNonSysInPlaceEncControls ();

	LastDialogId = "NONSYS_INPLACE_ENC_IN_PROGRESS";

	_beginthread (volTransformThreadFunction, 0, MainDlg);

	return;
}

//********* Rotina ShowNonSysInPlaceEncUIStatus *********************************************
///
/// Mantido original - não utilizado
///
//*******************************************************************
void ShowNonSysInPlaceEncUIStatus (void)
{
	wchar_t nonSysInplaceEncUIStatus [300] = {0};

	switch (NonSysInplaceEncStatus)
	{
	case NONSYS_INPLACE_ENC_STATUS_PAUSED:
		wcscpy (nonSysInplaceEncUIStatus, GetString ("PROGRESS_STATUS_PAUSED"));
		break;
	case NONSYS_INPLACE_ENC_STATUS_PREPARING:
		wcscpy (nonSysInplaceEncUIStatus, GetString ("PROGRESS_STATUS_PREPARING"));
		break;
	case NONSYS_INPLACE_ENC_STATUS_RESIZING:
		wcscpy (nonSysInplaceEncUIStatus, GetString ("PROGRESS_STATUS_RESIZING"));
		break;
	case NONSYS_INPLACE_ENC_STATUS_ENCRYPTING:
		wcscpy (nonSysInplaceEncUIStatus, GetString ("PROGRESS_STATUS_ENCRYPTING"));
		break;
	case NONSYS_INPLACE_ENC_STATUS_FINALIZING:
		wcscpy (nonSysInplaceEncUIStatus, GetString ("PROGRESS_STATUS_FINALIZING"));
		break;
	case NONSYS_INPLACE_ENC_STATUS_FINISHED:
		wcscpy (nonSysInplaceEncUIStatus, GetString ("PROGRESS_STATUS_FINISHED"));
		break;
	case NONSYS_INPLACE_ENC_STATUS_ERROR:
		wcscpy (nonSysInplaceEncUIStatus, GetString ("PROGRESS_STATUS_ERROR"));
		break;
	}

	wcscat (nonSysInplaceEncUIStatus, L" ");

	SetWindowTextW (GetDlgItem (hCurPage, IDC_WRITESPEED), nonSysInplaceEncUIStatus);
}


void UpdateNonSysInPlaceEncControls (void)
{
	EnableWindow (GetDlgItem (hCurPage, IDC_WIPE_MODE), !(bVolTransformThreadRunning || bVolTransformThreadToRun));

	SetWindowTextW (GetDlgItem (hCurPage, IDC_PAUSE),
		GetString ((bVolTransformThreadRunning || bVolTransformThreadToRun) ? "IDC_PAUSE" : "RESUME"));

	SetWindowTextW (GetDlgItem (MainDlg, IDCANCEL), GetString (bInPlaceEncNonSysResumed ? "DEFER" : "CANCEL"));

	EnableWindow (GetDlgItem (hCurPage, IDC_PAUSE), bFirstNonSysInPlaceEncResumeDone 
		&& NonSysInplaceEncStatus != NONSYS_INPLACE_ENC_STATUS_FINALIZING
		&& NonSysInplaceEncStatus != NONSYS_INPLACE_ENC_STATUS_FINISHED);

	EnableWindow (GetDlgItem (MainDlg, IDC_NEXT), !(bVolTransformThreadRunning || bVolTransformThreadToRun) && !bFirstNonSysInPlaceEncResumeDone);
	EnableWindow (GetDlgItem (MainDlg, IDC_PREV), !(bVolTransformThreadRunning || bVolTransformThreadToRun) && !bInPlaceEncNonSysResumed);
	EnableWindow (GetDlgItem (MainDlg, IDCANCEL), 
		!(bVolTransformThreadToRun 
		|| NonSysInplaceEncStatus == NONSYS_INPLACE_ENC_STATUS_PREPARING 
		|| NonSysInplaceEncStatus == NONSYS_INPLACE_ENC_STATUS_RESIZING
		|| NonSysInplaceEncStatus == NONSYS_INPLACE_ENC_STATUS_FINALIZING
		|| NonSysInplaceEncStatus == NONSYS_INPLACE_ENC_STATUS_FINISHED));

	if (bVolTransformThreadRunning || bVolTransformThreadToRun)
	{
		switch (NonSysInplaceEncStatus)
		{
		case NONSYS_INPLACE_ENC_STATUS_PREPARING:
		case NONSYS_INPLACE_ENC_STATUS_RESIZING:
		case NONSYS_INPLACE_ENC_STATUS_FINALIZING:
			ArrowWaitCursor ();
			break;

		case NONSYS_INPLACE_ENC_STATUS_ENCRYPTING:
			NormalCursor ();
			break;

		default:
			NormalCursor ();
			break;
		}

		if (bVolTransformThreadCancel)
			WaitCursor ();
	}
	else
	{
		NormalCursor ();

		if (bInPlaceEncNonSysResumed)
		{
			SetNonSysInplaceEncUIStatus (NONSYS_INPLACE_ENC_STATUS_PAUSED);
		}
		else
			SetWindowText (GetDlgItem (hCurPage, IDC_WRITESPEED), " ");

		SetWindowText (GetDlgItem (hCurPage, IDC_TIMEREMAIN), " ");
	}

	ShowNonSysInPlaceEncUIStatus ();

	UpdateNonSysInplaceEncProgressBar ();
}


static void UpdateNonSysInplaceEncProgressBar (void)
{
	static int lastNonSysInplaceEncStatus = NONSYS_INPLACE_ENC_STATUS_NONE;
	int nonSysInplaceEncStatus = NonSysInplaceEncStatus;
	__int64 totalSectors;

	totalSectors = NonSysInplaceEncTotalSectors;

	if (bVolTransformThreadRunning 
		&& (nonSysInplaceEncStatus == NONSYS_INPLACE_ENC_STATUS_ENCRYPTING
		|| nonSysInplaceEncStatus == NONSYS_INPLACE_ENC_STATUS_FINALIZING
		|| nonSysInplaceEncStatus == NONSYS_INPLACE_ENC_STATUS_FINISHED))
	{
		if (lastNonSysInplaceEncStatus != nonSysInplaceEncStatus
			&& nonSysInplaceEncStatus == NONSYS_INPLACE_ENC_STATUS_ENCRYPTING)
		{
			InitNonSysInplaceEncProgressBar ();
		}
		else
		{
			if (totalSectors <= 0 && nVolumeSize > 0)
				totalSectors = nVolumeSize / SECTOR_SIZE;

			if (totalSectors > 0)
				UpdateProgressBarProc (NonSysInplaceEncBytesDone / SECTOR_SIZE);
		}
	}

	ShowNonSysInPlaceEncUIStatus ();

	lastNonSysInplaceEncStatus = nonSysInplaceEncStatus;
}


static void InitNonSysInplaceEncProgressBar (void)
{
	__int64 totalSectors = NonSysInplaceEncTotalSectors;

	if (totalSectors <= 0)
	{
		if (nVolumeSize <= 0)
			return;

		totalSectors = nVolumeSize / SECTOR_SIZE;
	}

	InitProgressBar (totalSectors,
		NonSysInplaceEncBytesDone,
		FALSE,
		TRUE,
		TRUE,
		TRUE);
}

//********* Rotina DisplayRandPool *********************************************
///
/// Exibe chaves geradas randomicamente
///
/// \param HWND hPoolDisplay ? Handle do controle para exibir as chaves randomicas
/// \param BOOL bShow ? liga desliga exibição - forção para True
///
/// \return void
//*******************************************************************
void DisplayRandPool (HWND hPoolDisplay, BOOL bShow)
{		
	unsigned char tmp[4];
	unsigned char tmpByte;
	int col, row;
	static BOOL bRandPoolDispAscii = FALSE;

	if (!bShow)
	{
		SetWindowText (hPoolDisplay, "");
		return;
	}

	RandpeekBytes (randPool, sizeof (randPool));

	if (memcmp (lastRandPool, randPool, sizeof(lastRandPool)) != 0)
	{
		outRandPoolDispBuffer[0] = 0;

		for (row = 0; row < RANDPOOL_DISPLAY_ROWS; row++)
		{
			for (col = 0; col < RANDPOOL_DISPLAY_COLUMNS; col++)
			{
				tmpByte = randPool[row * RANDPOOL_DISPLAY_COLUMNS + col];

				sprintf ((char *) tmp, bRandPoolDispAscii ? ((tmpByte >= 32 && tmpByte < 255 && tmpByte != '&') ? " %c " : " . ") : "%02X ", tmpByte);
				strcat ((char *) outRandPoolDispBuffer, (char *) tmp);
			}
			strcat ((char *) outRandPoolDispBuffer, "\n");
		}
		SetWindowText (hPoolDisplay, (char *) outRandPoolDispBuffer);

		memcpy (lastRandPool, randPool, sizeof(lastRandPool));
	}
}

//mantido original
static void WipeAbort (void)
{
	EnableWindow (GetDlgItem (hCurPage, IDC_ABORT_BUTTON), FALSE);

	if (bHiddenOS && IsHiddenOSRunning())
	{
		/* Decoy system partition wipe */	
		
		DecoySystemWipeStatus decoySysPartitionWipeStatus;

		try
		{
			decoySysPartitionWipeStatus = BootEncObj->GetDecoyOSWipeStatus();
		}
		catch (Exception &e)
		{
			e.Show (MainDlg);
			EnableWindow (GetDlgItem (hCurPage, IDC_ABORT_BUTTON), TRUE);
			return;
		}

		if (!decoySysPartitionWipeStatus.WipeInProgress)
		{
			EnableWindow (GetDlgItem (hCurPage, IDC_ABORT_BUTTON), TRUE);
			return;
		}

		WaitCursor ();

		try
		{
			int attempts = SYSENC_PAUSE_RETRIES;

			BootEncObj->AbortDecoyOSWipe ();

			decoySysPartitionWipeStatus = BootEncObj->GetDecoyOSWipeStatus();

			while (decoySysPartitionWipeStatus.WipeInProgress && attempts > 0)
			{
				Sleep (SYSENC_PAUSE_RETRY_INTERVAL);
				attempts--;
				decoySysPartitionWipeStatus = BootEncObj->GetDecoyOSWipeStatus();
			}

			if (!decoySysPartitionWipeStatus.WipeInProgress)
				BootEncObj->CheckDecoyOSWipeResult ();

		}
		catch (Exception &e)
		{
			e.Show (MainDlg);
		}

		NormalCursor ();

		if (decoySysPartitionWipeStatus.WipeInProgress)
		{
			SetTimer (MainDlg, TIMER_ID_WIPE_PROGRESS, TIMER_INTERVAL_WIPE_PROGRESS, NULL);
			EnableWindow (GetDlgItem (hCurPage, IDC_ABORT_BUTTON), TRUE);
			Error ("FAILED_TO_INTERRUPT_WIPING");
			return;
		}
	}
	else
	{
		/* Regular device wipe (not decoy system partition wipe) */
	}

	UpdateWipeControls ();
	EnableWindow (GetDlgItem (hCurPage, IDC_ABORT_BUTTON), TRUE);
}

//mantido original
static void WipeStart (void)
{
	if (bHiddenOS && IsHiddenOSRunning())
	{
		/* Decoy system partition wipe */

		EnableWindow (GetDlgItem (hCurPage, IDC_ABORT_BUTTON), FALSE);

		bDeviceWipeInProgress = FALSE;
		WaitCursor ();

		try
		{
			BootEncObj->StartDecoyOSWipe (nWipeMode);	

			bDeviceWipeInProgress = TRUE;
		}
		catch (Exception &e)
		{
			e.Show (MainDlg);
		}

		NormalCursor ();

		if (!bDeviceWipeInProgress)
		{
			EnableWindow (GetDlgItem (hCurPage, IDC_ABORT_BUTTON), TRUE);
			Error ("FAILED_TO_START_WIPING");
			return;
		}
	}
	else
	{
		/* Regular device wipe (not decoy system partition wipe) */
	}

	InitWipeProgressBar ();
	UpdateWipeProgressBar ();
	UpdateWipeControls ();
	EnableWindow (GetDlgItem (hCurPage, IDC_ABORT_BUTTON), TRUE);
	SetTimer (MainDlg, TIMER_ID_WIPE_PROGRESS, TIMER_INTERVAL_WIPE_PROGRESS, NULL);
}

/// mantido original
static void UpdateWipeProgressBar (void)
{
	if (bHiddenOS && IsHiddenOSRunning())
	{
		/* Decoy system partition wipe */

		DecoySystemWipeStatus decoySysPartitionWipeStatus;

		try
		{
			decoySysPartitionWipeStatus = BootEncObj->GetDecoyOSWipeStatus();
			BootEncStatus = BootEncObj->GetStatus();
		}
		catch (...)
		{
			return;
		}

		if (decoySysPartitionWipeStatus.WipedAreaEnd == -1)
			UpdateProgressBarProc (0);
		else
			UpdateProgressBarProc ((decoySysPartitionWipeStatus.WipedAreaEnd - BootEncStatus.ConfiguredEncryptedAreaStart + 1) / SECTOR_SIZE);
	}
	else
	{
		/* Regular device wipe (not decoy system partition wipe) */
	}
}

/// mantido original
static void InitWipeProgressBar (void)
{
	if (bHiddenOS && IsHiddenOSRunning())
	{
		/* Decoy system partition wipe */

		DecoySystemWipeStatus decoySysPartitionWipeStatus;

		try
		{
			decoySysPartitionWipeStatus = BootEncObj->GetDecoyOSWipeStatus();
			BootEncStatus = BootEncObj->GetStatus();
		}
		catch (...)
		{
			return;
		}

		if (BootEncStatus.ConfiguredEncryptedAreaEnd == -1 
			|| BootEncStatus.ConfiguredEncryptedAreaStart == -1)
			return;

		InitProgressBar ((BootEncStatus.ConfiguredEncryptedAreaEnd - BootEncStatus.ConfiguredEncryptedAreaStart + 1) / SECTOR_SIZE,
			(decoySysPartitionWipeStatus.WipedAreaEnd == BootEncStatus.ConfiguredEncryptedAreaStart || decoySysPartitionWipeStatus.WipedAreaEnd == -1) ?
			0 :	decoySysPartitionWipeStatus.WipedAreaEnd - BootEncStatus.ConfiguredEncryptedAreaStart + 1,
			FALSE,
			TRUE,
			FALSE,
			TRUE);
	}
	else
	{
		/* Regular device wipe (not decoy system partition wipe) */
	}
}

/// mantido original
static void UpdateWipeControls (void)
{
	if (bHiddenOS && IsHiddenOSRunning())
	{
		/* Decoy system partition wipe */

		DecoySystemWipeStatus decoySysPartitionWipeStatus;

		try
		{
			decoySysPartitionWipeStatus = BootEncObj->GetDecoyOSWipeStatus();
			BootEncStatus = BootEncObj->GetStatus();
		}
		catch (...)
		{
			return;
		}

		EnableWindow (GetDlgItem (MainDlg, IDC_NEXT), !decoySysPartitionWipeStatus.WipeInProgress);
	}
	else
	{
		/* Regular device wipe (not decoy system partition wipe) */

		EnableWindow (GetDlgItem (MainDlg, IDC_NEXT), bDeviceWipeInProgress);

		if (!bDeviceWipeInProgress)
		{
			SetWindowText (GetDlgItem (hCurPage, IDC_TIMEREMAIN), " ");
		}
	}

	EnableWindow (GetDlgItem (hCurPage, IDC_ABORT_BUTTON), bDeviceWipeInProgress);
	EnableWindow (GetDlgItem (MainDlg, IDC_PREV), !bDeviceWipeInProgress);

	bConfirmQuit = bDeviceWipeInProgress;
}


/// mantido original
static void __cdecl sysEncDriveAnalysisThread (void *hwndDlgArg)
{
	// Mark the detection process as 'in progress'
	HiddenSectorDetectionStatus = 1;
	SaveSettings (NULL);
	BroadcastSysEncCfgUpdate ();

	try
	{
		BootEncObj->ProbeRealSystemDriveSize ();
		bSysEncDriveAnalysisTimeOutOccurred = FALSE;
	}
	catch (TimeOut &)
	{
		bSysEncDriveAnalysisTimeOutOccurred = TRUE;
	}
	catch (Exception &e)
	{
		e.Show (NULL);
		EndMainDlg (MainDlg);
		exit(0);
	}

	// Mark the detection process as successful
	HiddenSectorDetectionStatus = 0;
	SaveSettings (NULL);
	BroadcastSysEncCfgUpdate ();

	// This artificial delay prevents user confusion on systems where the analysis ends almost instantly
	Sleep (3000);

	bSysEncDriveAnalysisInProgress = FALSE;
}

//********* Rotina volTransformThreadFunction *********************************************
///
/// Cria o volume, num thread separado - mantido original
///
/// \return void
//*******************************************************************

static void __cdecl volTransformThreadFunction (void *hwndDlgArg)
{
	int nStatus;
	DWORD dwWin32FormatError;
	BOOL bHidden;
	HWND hwndDlg = (HWND) hwndDlgArg;
	volatile FORMAT_VOL_PARAMETERS *volParams = (FORMAT_VOL_PARAMETERS *) malloc (sizeof(FORMAT_VOL_PARAMETERS));

	if (volParams == NULL)
		AbortProcess ("ERR_MEM_ALLOC");

	VirtualLock ((LPVOID) volParams, sizeof(FORMAT_VOL_PARAMETERS));

	bVolTransformThreadRunning = TRUE;
	bVolTransformThreadToRun = FALSE;

	// Check administrator privileges
	if (!IsAdmin () && !IsUacSupported ())
	{
		if (fileSystem == FILESYS_NTFS)
		{
			if (MessageBoxW (hwndDlg, GetString ("ADMIN_PRIVILEGES_WARN_NTFS"), lpszTitle, MB_OKCANCEL|MB_ICONWARNING|MB_DEFBUTTON2) == IDCANCEL)
				goto cancel;
		}
		if (bDevice)
		{
			if (MessageBoxW (hwndDlg, GetString ("ADMIN_PRIVILEGES_WARN_DEVICES"), lpszTitle, MB_OKCANCEL|MB_ICONWARNING|MB_DEFBUTTON2) == IDCANCEL)
				goto cancel;
		}
	}

	if (!bInPlaceEncNonSys)
	{
		if (!bDevice)
		{
			int x = _access (szDiskFile, 06);
			if (x == 0 || errno != ENOENT)
			{
				wchar_t szTmp[512];

				if (! ((bHiddenVol && !bHiddenVolHost) && errno != EACCES))	// Only ask ask for permission to overwrite an existing volume if we're not creating a hidden volume
				{
					_snwprintf (szTmp, sizeof szTmp / 2,
						GetString (errno == EACCES ? "READONLYPROMPT" : "OVERWRITEPROMPT"),
						szDiskFile);

					x = MessageBoxW (hwndDlg, szTmp, lpszTitle, YES_NO|MB_ICONWARNING|MB_DEFBUTTON2);

					if (x != IDYES)
						goto cancel;
				}
			}

			if (_access (szDiskFile, 06) != 0)
			{
				if (errno == EACCES)
				{
					if (_chmod (szDiskFile, _S_IREAD | _S_IWRITE) != 0)
					{
						MessageBoxW (hwndDlg, GetString ("ACCESSMODEFAIL"), lpszTitle, ICON_HAND);
						goto cancel;
					}
				}
			}

		}
		else
		{
			// Partition / device / dynamic volume

			if (!FinalPreTransformPrompts ())
				goto cancel;
		}
	}

	bHidden = bHiddenVol && !bHiddenVolHost;

	volParams->bDevice = bDevice;
	volParams->hiddenVol = bHidden;
	volParams->volumePath = szDiskFile;
	volParams->size = nVolumeSize;
	volParams->hiddenVolHostSize = nHiddenVolHostSize;
	volParams->ea = nVolumeEA;
	volParams->pkcs5 = hash_algo;
	volParams->headerFlags = CreatingHiddenSysVol() ? TC_HEADER_FLAG_ENCRYPTED_SYSTEM : 0;
	volParams->fileSystem = fileSystem;
	volParams->clusterSize = clusterSize;
	volParams->sparseFileSwitch = bSparseFileSwitch;
	volParams->quickFormat = quickFormat;
	volParams->realClusterSize = &realClusterSize;
	volParams->password = &volumePassword;
	volParams->hwndDlg = hwndDlg;

	if (bInPlaceEncNonSys)
	{
		HANDLE hPartition = INVALID_HANDLE_VALUE;

		SetNonSysInplaceEncUIStatus (NONSYS_INPLACE_ENC_STATUS_PREPARING);

		if (!bInPlaceEncNonSysResumed)
		{
			bTryToCorrectReadErrors = FALSE;

			nStatus = EncryptPartitionInPlaceBegin (volParams, &hPartition, nWipeMode);

			if (nStatus == ERR_SUCCESS)
			{
				nStatus = EncryptPartitionInPlaceResume (hPartition, volParams, nWipeMode, &bTryToCorrectReadErrors);
			}
			else if (hPartition != INVALID_HANDLE_VALUE)
			{
				CloseHandle (hPartition);
				hPartition = INVALID_HANDLE_VALUE;
			}
		}
		else
		{
			nStatus = EncryptPartitionInPlaceResume (INVALID_HANDLE_VALUE, volParams, nWipeMode, &bTryToCorrectReadErrors);
		}
	}
	else
	{
		InitProgressBar (GetVolumeDataAreaSize (bHidden, nVolumeSize) / SECTOR_SIZE, 0, FALSE, FALSE, FALSE, TRUE);

		nStatus = TCFormatVolume (volParams);
	}

	if (nStatus == ERR_OUTOFMEMORY)
	{
		AbortProcess ("OUTOFMEMORY");
	}

	if (bInPlaceEncNonSys
		&& nStatus == ERR_USER_ABORT
		&& NonSysInplaceEncStatus == NONSYS_INPLACE_ENC_STATUS_FINISHED)
	{
		// Ignore user abort if non-system in-place encryption successfully finished
		nStatus = ERR_SUCCESS;
	}


	dwWin32FormatError = GetLastError ();

	if (bHiddenVolHost && !bVolTransformThreadCancel && nStatus == 0)
	{
		/* Auto mount the newly created hidden volume host */
		switch (MountHiddenVolHost (hwndDlg, szDiskFile, &hiddenVolHostDriveNo, &volumePassword, FALSE))
		{
		case ERR_NO_FREE_DRIVES:
			MessageBoxW (hwndDlg, GetString ("NO_FREE_DRIVE_FOR_OUTER_VOL"), lpszTitle, ICON_HAND);
			bVolTransformThreadCancel = TRUE;
			break;
		case ERR_VOL_MOUNT_FAILED:
		case ERR_PASSWORD_WRONG:
			MessageBoxW (hwndDlg, GetString ("CANT_MOUNT_OUTER_VOL"), lpszTitle, ICON_HAND);
			bVolTransformThreadCancel = TRUE;
			break;
		}
	}

	SetLastError (dwWin32FormatError);

	if ((bVolTransformThreadCancel || nStatus == ERR_USER_ABORT)
		&& !(bInPlaceEncNonSys && NonSysInplaceEncStatus == NONSYS_INPLACE_ENC_STATUS_FINISHED))	// Ignore user abort if non-system in-place encryption successfully finished.
	{
		if (!bDevice && !(bHiddenVol && !bHiddenVolHost))	// If we're not creating a hidden volume and if it's a file container
		{
			remove (szDiskFile);		// Delete the container
		}

		goto cancel;
	}

	if (nStatus != ERR_USER_ABORT)
	{
		if (nStatus != 0)
		{
			/* An error occurred */

			wchar_t szMsg[8192];

			handleError (hwndDlg, nStatus);

			if (bInPlaceEncNonSys)
			{
				if (bInPlaceEncNonSysResumed)
				{
					SetNonSysInplaceEncUIStatus (NONSYS_INPLACE_ENC_STATUS_PAUSED);
					Error ("INPLACE_ENC_GENERIC_ERR_RESUME");
				}
				else
				{
					SetNonSysInplaceEncUIStatus (NONSYS_INPLACE_ENC_STATUS_ERROR);
					ShowInPlaceEncErrMsgWAltSteps ("INPLACE_ENC_GENERIC_ERR_ALT_STEPS", TRUE);
				}
			}
			else if (!(bHiddenVolHost && hiddenVolHostDriveNo < 0))  // If the error was not that the hidden volume host could not be mounted (this error has already been reported to the user)
			{
				//swprintf (szMsg, GetString ("CREATE_FAILED"), szDiskFile);
				//MessageBoxW (hwndDlg, szMsg, lpszTitle, ICON_HAND);
				swprintf (szMsg, L"Falha ao criar a unidade de proteção de arquivos", szDiskFile);
				MessageBoxW (hwndDlg, szMsg, L"Central de proteção de arquivos", ICON_HAND);

			}

			if (!bDevice && !(bHiddenVol && !bHiddenVolHost))	// If we're not creating a hidden volume and if it's a file container
			{
				remove (szDiskFile);		// Delete the container
			}

			goto cancel;
		}
		else
		{
			/* Volume successfully created */

			RestoreDefaultKeyFilesParam ();

			if (bDevice && !bInPlaceEncNonSys)
			{
				// Handle assigned drive letter (if any)

				HandleOldAssignedDriveLetter ();
			}

			if (!bHiddenVolHost)
			{
				if (bHiddenVol)
				{
					bHiddenVolFinished = TRUE;

					if (!bHiddenOS)
						Warning ("HIDVOL_FORMAT_FINISHED_HELP");
				}
				else if (bInPlaceEncNonSys)
				{
					Warning ("NONSYS_INPLACE_ENC_FINISHED_INFO");

					HandleOldAssignedDriveLetter ();
				}
				else 
				{
					//Info("FORMAT_FINISHED_INFO");

					if (bSparseFileSwitch && quickFormat)
						Warning("SPARSE_FILE_SIZE_NOTE");
				}
			}
			else
			{
				/* We've just created an outer volume (to host a hidden volume within) */

				bHiddenVolHost = FALSE; 
				bHiddenVolFinished = FALSE;
				nHiddenVolHostSize = nVolumeSize;

				// Clear the outer volume password
				memset(&szVerify[0], 0, sizeof (szVerify));
				memset(&szRawPassword[0], 0, sizeof (szRawPassword));

				MessageBeep (MB_OK);
			}

			if (!bInPlaceEncNonSys)
				SetTimer (hwndDlg, TIMER_ID_RANDVIEW, TIMER_INTERVAL_RANDVIEW, NULL);

			if (volParams != NULL)
			{
				burn ((LPVOID) volParams, sizeof(FORMAT_VOL_PARAMETERS));
				VirtualUnlock ((LPVOID) volParams, sizeof(FORMAT_VOL_PARAMETERS));
				free ((LPVOID) volParams);
				volParams = NULL;
			}

			bVolTransformThreadRunning = FALSE;
			bVolTransformThreadCancel = FALSE;

			PostMessage (hwndDlg, bInPlaceEncNonSys ? TC_APPMSG_NONSYS_INPLACE_ENC_FINISHED : TC_APPMSG_FORMAT_FINISHED, 0, 0);

			LastDialogId = "FORMAT_FINISHED";
			_endthread ();
		}
	}

cancel:
	LastDialogId = (bInPlaceEncNonSys ? "NONSYS_INPLACE_ENC_CANCELED" : "FORMAT_CANCELED");

	if (!bInPlaceEncNonSys)
		SetTimer (hwndDlg, TIMER_ID_RANDVIEW, TIMER_INTERVAL_RANDVIEW, NULL);

	if (volParams != NULL)
	{
		burn ((LPVOID) volParams, sizeof(FORMAT_VOL_PARAMETERS));
		VirtualUnlock ((LPVOID) volParams, sizeof(FORMAT_VOL_PARAMETERS));
		free ((LPVOID) volParams);
		volParams = NULL;
	}

	bVolTransformThreadRunning = FALSE;
	bVolTransformThreadCancel = FALSE;

	PostMessage (hwndDlg, TC_APPMSG_VOL_TRANSFORM_THREAD_ENDED, 0, 0);

	if (bHiddenVolHost && hiddenVolHostDriveNo < -1 && !bVolTransformThreadCancel)	// If hidden volume host could not be mounted
		AbortProcessSilent ();

	_endthread ();
}


//********* Rotina LoadPage *********************************************
///
/// Carrega as páginas na sequncia do Wizard
/// \param HWND hwndDlg ? Handle da janela pai
/// \param int nPageNo ? Janela a ser carregada
///
/// \return void
//*******************************************************************
static void LoadPage (HWND hwndDlg, int nPageNo)
{
	RECT rD, rW;

	nLastPageNo = nCurPageNo;

	if (hCurPage != NULL)
	{
		// WARNING: nCurPageNo must be set to a non-existent ID here before wiping the password fields below in
		// this function, etc. Otherwise, such actions (SetWindowText) would invoke the EN_CHANGE handlers, which 
		// would, if keyfiles were applied, e.g. use strlen() on a buffer full of random data, in most cases 
		// not null-terminated.
		nCurPageNo = -1;


		// Place here any actions that need to be performed at the latest possible time when leaving a wizard page
		// (i.e. right before "destroying" the page). Also, code that needs to be executed both on IDC_NEXT and
		// on IDC_PREV can be placed here so as to avoid code doubling. 

		switch (nLastPageNo)
		{

		case PASSWORD_PAGE:
			{
				char tmp[MAX_PASSWORD+1];

				// Attempt to wipe passwords stored in the input field buffers. This is performed here (and 
				// not in the IDC_PREV or IDC_NEXT sections) in order to prevent certain race conditions
				// when keyfiles are used.
				memset (tmp, 'X', MAX_PASSWORD);
				tmp [MAX_PASSWORD] = 0;
				tmp[0]='\x0';
				SetWindowText (hPasswordInputField, tmp);
				SetWindowText (hVerifyPasswordInputField, tmp);
			}
			break;
		}

		DestroyWindow (hCurPage);
		hCurPage = NULL;
	}

	// This prevents the mouse pointer from remaining as the "hand" cursor when the user presses Enter
	// while hovering over a hyperlink.
	bHyperLinkBeingTracked = FALSE;
	NormalCursor();

	GetWindowRect (GetDlgItem (hwndDlg, IDC_POS_BOX), &rW);


	nCurPageNo = nPageNo;


	switch (nPageNo)
	{
    
	case INTRO_PAGE:
		hCurPage = CreateDialogW (hInst, MAKEINTRESOURCEW (IDD_INTRO_PAGE_DLG), hwndDlg,
					 (DLGPROC) PageDialogProc);
		break;
	case PASSWORD_PAGE:
		hCurPage = CreateDialogW (hInst, MAKEINTRESOURCEW (IDD_PASSWORD_PAGE_DLG), hwndDlg,
					 (DLGPROC) PageDialogProc);
		break;
	case FORMAT_PAGE:
		hCurPage = CreateDialogW (hInst, MAKEINTRESOURCEW (IDD_FORMAT_PAGE_DLG), hwndDlg,
					 (DLGPROC) PageDialogProc);
		break;
	case FORMAT_FINISHED_PAGE:
		hCurPage = CreateDialogW (hInst, MAKEINTRESOURCEW ((bHiddenVol && !bHiddenVolHost && !bHiddenVolFinished) ? IDD_HIDVOL_HOST_FILL_PAGE_DLG : IDD_INFO_PAGE_DLG), hwndDlg,
					 (DLGPROC) PageDialogProc);
		break;
	}

	//rD.left = 162;
	//Intro_Page teve seu tamanho alterado, ficando fora do padrão das demais páginas
	//ficará sobreposto ao IDC_BOX_TITLE --> esconder
	if(nPageNo == INTRO_PAGE)
	{
	  ShowWindow(GetDlgItem(hwndDlg, IDC_BOX_TITLE), SW_HIDE);
	  rD.top = 10;	
	  rW.top = rW.top - 20;	  
	}
	else
	{
	  rD.top = 25;   
	  ShowWindow(GetDlgItem(hwndDlg, IDC_BOX_TITLE), SW_SHOW);
	}
	
	rD.left = 160;
	rD.right = 0;
	rD.bottom = 0;
	MapDialogRect (hwndDlg, &rD);

	if (hCurPage != NULL)
	{
		MoveWindow (hCurPage, rD.left, rD.top, rW.right - rW.left, rW.bottom - rW.top, TRUE);
		ShowWindow (hCurPage, SW_SHOWNORMAL);

		// Place here any message boxes that need to be displayed as soon as a new page is displayed. This 
		// ensures that the page is fully rendered (otherwise it would remain blank, until the message box
		// is closed).
		switch (nPageNo)
		{
		case PASSWORD_PAGE:

			CheckCapsLock (hwndDlg, FALSE);

			if (CreatingHiddenSysVol())
				Warning ("PASSWORD_HIDDEN_OS_NOTE");

			break;
		}
	}
}


//mantido original
int PrintFreeSpace (HWND hwndTextBox, char *lpszDrive, PLARGE_INTEGER lDiskFree)
{
	char *nResourceString;
	int nMultiplier;
	wchar_t szTmp2[256];

	if (lDiskFree->QuadPart < BYTES_PER_KB)
		nMultiplier = 1;
	else if (lDiskFree->QuadPart < BYTES_PER_MB)
		nMultiplier = BYTES_PER_KB;
	else if (lDiskFree->QuadPart < BYTES_PER_GB)
		nMultiplier = BYTES_PER_MB;
	else
		nMultiplier = BYTES_PER_GB;

	if (nMultiplier == 1)
	{
		if (bHiddenVol && !bHiddenVolHost)	// If it's a hidden volume
			nResourceString = "MAX_HIDVOL_SIZE_BYTES";
		else if (bDevice)
			nResourceString = "DEVICE_FREE_BYTES";
		else
			nResourceString = "DISK_FREE_BYTES";
	}
	else if (nMultiplier == BYTES_PER_KB)
	{
		if (bHiddenVol && !bHiddenVolHost)	// If it's a hidden volume
			nResourceString = "MAX_HIDVOL_SIZE_KB";
		else if (bDevice)
			nResourceString = "DEVICE_FREE_KB";
		else
			nResourceString = "DISK_FREE_KB";
	}
	else if (nMultiplier == BYTES_PER_MB)
	{
		if (bHiddenVol && !bHiddenVolHost)	// If it's a hidden volume
			nResourceString = "MAX_HIDVOL_SIZE_MB";
		else if (bDevice)
			nResourceString = "DEVICE_FREE_MB";
		else
			nResourceString = "DISK_FREE_MB";
	}
 	else 
	{
		if (bHiddenVol && !bHiddenVolHost)	// If it's a hidden volume
			nResourceString = "MAX_HIDVOL_SIZE_GB";
		else if (bDevice)
			nResourceString = "DEVICE_FREE_GB";
		else
			nResourceString = "DISK_FREE_GB";
	}

	if (bHiddenVol && !bHiddenVolHost)	// If it's a hidden volume
	{
		_snwprintf (szTmp2, sizeof szTmp2 / 2, GetString (nResourceString), ((double) lDiskFree->QuadPart) / nMultiplier);
		SetWindowTextW (GetDlgItem (hwndTextBox, IDC_SIZEBOX), szTmp2);
	}
	else
		_snwprintf (szTmp2, sizeof szTmp2 / 2, GetString (nResourceString), lpszDrive, ((double) lDiskFree->QuadPart) / nMultiplier);

	SetWindowTextW (hwndTextBox, szTmp2);

	if (lDiskFree->QuadPart % (__int64) BYTES_PER_MB != 0)
		nMultiplier = BYTES_PER_KB;

	return nMultiplier;
}

/// mantido original
void DisplaySizingErrorText (HWND hwndTextBox)
{
	wchar_t szTmp[1024];

	if (translateWin32Error (szTmp, sizeof (szTmp) / sizeof(szTmp[0])))
	{
		wchar_t szTmp2[1024];
		wsprintfW (szTmp2, L"%s\n%s", GetString ("CANNOT_CALC_SPACE"), szTmp);
		SetWindowTextW (hwndTextBox, szTmp2);
	}
	else
	{
		SetWindowText (hwndTextBox, "");
	}
}
/// mantido original
void EnableDisableFileNext (HWND hComboBox, HWND hMainButton)
{
	LPARAM nIndex = SendMessage (hComboBox, CB_GETCURSEL, 0, 0);
	if (bHistory && nIndex == CB_ERR)
	{
		EnableWindow (hMainButton, FALSE);
		SetFocus (hComboBox);
	}
	else
	{
		EnableWindow (hMainButton, TRUE);
		SetFocus (hMainButton);
	}
}

// Returns TRUE if the file is a sparse file. If it's not a sparse file or in case of any error, returns FALSE.
/// mantido original
BOOL IsSparseFile (HWND hwndDlg)
{
	HANDLE hFile;
	BY_HANDLE_FILE_INFORMATION bhFileInfo;

	FILETIME ftLastAccessTime;
	BOOL bTimeStampValid = FALSE;

	BOOL retCode = FALSE;

	hFile = CreateFile (szFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		MessageBoxW (hwndDlg, GetString ("CANT_ACCESS_VOL"), lpszTitle, ICON_HAND);
		return FALSE;
	}

	if (bPreserveTimestamp)
	{
		/* Remember the container timestamp (used to reset file date and time of file-hosted
		   containers to preserve plausible deniability of hidden volumes)  */
		if (GetFileTime (hFile, NULL, &ftLastAccessTime, NULL) == 0)
		{
			bTimeStampValid = FALSE;
			MessageBoxW (hwndDlg, GetString ("GETFILETIME_FAILED_IMPLANT"), lpszTitle, MB_OK | MB_ICONEXCLAMATION);
		}
		else
			bTimeStampValid = TRUE;
	}

	bhFileInfo.dwFileAttributes = 0;

	GetFileInformationByHandle(hFile, &bhFileInfo);

	retCode = bhFileInfo.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE;

	if (bTimeStampValid)
	{
		// Restore the container timestamp (to preserve plausible deniability). 
		if (SetFileTime (hFile, NULL, &ftLastAccessTime, NULL) == 0)
			MessageBoxW (hwndDlg, GetString ("SETFILETIME_FAILED_PREP_IMPLANT"), lpszTitle, MB_OK | MB_ICONEXCLAMATION);
	}
	CloseHandle (hFile);
	return retCode;
}


// Note: GetFileVolSize is not to be used for devices (only for file-hosted volumes)
/// mantido original
BOOL GetFileVolSize (HWND hwndDlg, unsigned __int64 *size)
{
	LARGE_INTEGER fileSize;
	HANDLE hFile;

	FILETIME ftLastAccessTime;
	BOOL bTimeStampValid = FALSE;

	hFile = CreateFile (szFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		MessageBoxW (hwndDlg, GetString ("CANT_ACCESS_VOL"), lpszTitle, ICON_HAND);
		return FALSE;
	}

	if (bPreserveTimestamp)
	{
		/* Remember the container timestamp (used to reset file date and time of file-hosted
		   containers to preserve plausible deniability of hidden volumes)  */
		if (GetFileTime (hFile, NULL, &ftLastAccessTime, NULL) == 0)
		{
			bTimeStampValid = FALSE;
			MessageBoxW (hwndDlg, GetString ("GETFILETIME_FAILED_IMPLANT"), lpszTitle, MB_OK | MB_ICONEXCLAMATION);
		}
		else
			bTimeStampValid = TRUE;
	}

	if (GetFileSizeEx(hFile, &fileSize) == 0)
	{
		MessageBoxW (hwndDlg, GetString ("CANT_GET_VOLSIZE"), lpszTitle, ICON_HAND);
		if (bTimeStampValid)
		{
			// Restore the container timestamp (to preserve plausible deniability). 
			if (SetFileTime (hFile, NULL, &ftLastAccessTime, NULL) == 0)
				MessageBoxW (hwndDlg, GetString ("SETFILETIME_FAILED_PREP_IMPLANT"), lpszTitle, MB_OK | MB_ICONEXCLAMATION);
		}
		CloseHandle (hFile);
		return FALSE;
	}

	if (bTimeStampValid)
	{
		// Restore the container timestamp (to preserve plausible deniability). 
		if (SetFileTime (hFile, NULL, &ftLastAccessTime, NULL) == 0)
			MessageBoxW (hwndDlg, GetString ("SETFILETIME_FAILED_PREP_IMPLANT"), lpszTitle, MB_OK | MB_ICONEXCLAMATION);
	}
	CloseHandle (hFile);
	*size = fileSize.QuadPart;
	return TRUE;
}

/// mantido original
BOOL QueryFreeSpace (HWND hwndDlg, HWND hwndTextBox, BOOL display)
{
	if (bHiddenVol && !bHiddenVolHost)	// If it's a hidden volume
	{
		LARGE_INTEGER lDiskFree;
		char szTmp[TC_MAX_PATH];

		lDiskFree.QuadPart = nMaximumHiddenVolSize;

		if (display)
			PrintFreeSpace (hwndTextBox, szTmp, &lDiskFree);

		return TRUE;
	}
	else if (bDevice == FALSE)
	{
		char root[TC_MAX_PATH];
		ULARGE_INTEGER free;

		if (!GetVolumePathName (szFileName, root, sizeof (root)))
		{
			handleWin32Error (hwndDlg);
			return FALSE;
		}

		if (!GetDiskFreeSpaceEx (root, &free, 0, 0))
		{
			if (display)
				DisplaySizingErrorText (hwndTextBox);

			return FALSE;
		}
		else
		{
			LARGE_INTEGER lDiskFree;
			lDiskFree.QuadPart = free.QuadPart;

			if (display)
				PrintFreeSpace (hwndTextBox, root, &lDiskFree);

			return TRUE;
		}
	}
	else
	{
		DISK_GEOMETRY driveInfo;
		PARTITION_INFORMATION diskInfo;
		BOOL piValid = FALSE;
		BOOL gValid = FALSE;

		// Query partition size
		piValid = GetPartitionInfo (szDiskFile, &diskInfo);
		gValid = GetDriveGeometry (szDiskFile, &driveInfo);

		if (!piValid && !gValid)
		{
			if (display)
				DisplaySizingErrorText (hwndTextBox);

			return FALSE;
		}

		if (gValid && driveInfo.BytesPerSector != 512)
		{
			Error ("LARGE_SECTOR_UNSUPPORTED");
			return FALSE;
		}

		if (piValid)
		{
			nVolumeSize = diskInfo.PartitionLength.QuadPart;

			if(display)
				nMultiplier = PrintFreeSpace (hwndTextBox, szDiskFile, &diskInfo.PartitionLength);

			nUIVolumeSize = diskInfo.PartitionLength.QuadPart / nMultiplier;

			if (nVolumeSize == 0)
			{
				if (display)
					SetWindowTextW (hwndTextBox, GetString ("EXT_PARTITION"));

				return FALSE;
			}
		}
		else
		{
			LARGE_INTEGER lDiskFree;

			// Drive geometry info is used only when GetPartitionInfo() fails
			lDiskFree.QuadPart = driveInfo.Cylinders.QuadPart * driveInfo.BytesPerSector *
				driveInfo.SectorsPerTrack * driveInfo.TracksPerCylinder;

			nVolumeSize = lDiskFree.QuadPart;

			if (display)
				nMultiplier = PrintFreeSpace (hwndTextBox, szDiskFile, &lDiskFree);

			nUIVolumeSize = lDiskFree.QuadPart / nMultiplier;
		}

		return TRUE;
	}
}

/// mantido original
static BOOL FinalPreTransformPrompts (void)
{
	int x;
	wchar_t szTmp[4096];
	int driveNo;
	WCHAR deviceName[MAX_PATH];

	strcpy ((char *)deviceName, szFileName);
	ToUNICODE ((char *)deviceName);

	driveNo = GetDiskDeviceDriveLetter (deviceName);

	if (!(bHiddenVol && !bHiddenVolHost))	// Do not ask for permission to overwrite an existing volume if we're creating a hidden volume within it
	{
		wchar_t drive[128];
		wchar_t volumeLabel[128];
		wchar_t *type;
		BOOL bTmpIsPartition = FALSE;

		type = GetPathType (szFileName, !bInPlaceEncNonSys, &bTmpIsPartition);

		if (driveNo != -1)
		{
			if (!GetDriveLabel (driveNo, volumeLabel, sizeof (volumeLabel)))
				volumeLabel[0] = 0;

			swprintf_s (drive, sizeof (drive)/2, volumeLabel[0] ? L" (%hc: '%s')" : L" (%hc:%s)", 'A' + driveNo, volumeLabel[0] ? volumeLabel : L"");
		}
		else
		{
			drive[0] = 0;
			volumeLabel[0] = 0;
		}

		if (bHiddenOS && bHiddenVolHost)
			swprintf (szTmp, GetString ("OVERWRITEPROMPT_DEVICE_HIDDEN_OS_PARTITION"), szFileName, drive);
		else
			swprintf (szTmp, GetString (bInPlaceEncNonSys ? "NONSYS_INPLACE_ENC_CONFIRM" : "OVERWRITEPROMPT_DEVICE"), type, szFileName, drive);


		x = MessageBoxW (MainDlg, szTmp, lpszTitle, YES_NO | MB_ICONWARNING | (bInPlaceEncNonSys ? MB_DEFBUTTON1 : MB_DEFBUTTON2));
		if (x != IDYES)
			return FALSE;


		if (driveNo != -1 && bTmpIsPartition && !bInPlaceEncNonSys)
		{
			float percentFreeSpace = 100.0;
			__int64 occupiedBytes = 0;

			// Do a second check. If we find that the partition contains more than 1GB of data or more than 12%
			// of its space is occupied, we will display an extra warning, however, this time it won't be a Yes/No
			// dialog box (because users often ignore such dialog boxes).

			if (GetStatsFreeSpaceOnPartition (szFileName, &percentFreeSpace, &occupiedBytes, TRUE) != -1)
			{
				if (occupiedBytes > BYTES_PER_GB && percentFreeSpace < 99.99	// "percentFreeSpace < 99.99" is needed because an NTFS filesystem larger than several terabytes can have more than 1GB of data in use, even if there are no files stored on it.
					|| percentFreeSpace < 88)		// A 24-MB NTFS filesystem has 11.5% of space in use even if there are no files stored on it.
				{
					wchar_t tmpMcMsg [8000];
					wchar_t tmpMcOption1 [500];
					wchar_t tmpMcOptionCancel [50];

					wcscpy (tmpMcMsg, GetString("OVERWRITEPROMPT_DEVICE_SECOND_WARNING_LOTS_OF_DATA"));
					wcscpy (tmpMcOption1, GetString("ERASE_FILES_BY_CREATING_VOLUME"));
					wcscpy (tmpMcOptionCancel, GetString("CANCEL"));

					wcscat (tmpMcMsg, L"\n\n");
					wcscat (tmpMcMsg, GetString("DRIVE_LETTER_ITEM"));
					swprintf_s (szTmp, sizeof (szTmp)/2, L"%hc:", 'A' + driveNo);
					wcscat (tmpMcMsg, szTmp);

					wcscat (tmpMcMsg, L"\n");
					wcscat (tmpMcMsg, GetString("LABEL_ITEM"));
					wcscat (tmpMcMsg, volumeLabel[0] != 0 ? volumeLabel : GetString("NOT_APPLICABLE_OR_NOT_AVAILABLE"));

					wcscat (tmpMcMsg, L"\n");
					wcscat (tmpMcMsg, GetString("SIZE_ITEM"));
					GetSizeString (nVolumeSize, szTmp);
					wcscat (tmpMcMsg, szTmp);

					wcscat (tmpMcMsg, L"\n");
					wcscat (tmpMcMsg, GetString("PATH_ITEM"));
					wcscat (tmpMcMsg, deviceName);

					wchar_t *tmpStr[] = {L"", tmpMcMsg, tmpMcOption1, tmpMcOptionCancel, 0};
					switch (AskMultiChoice ((void **) tmpStr, TRUE))
					{
					case 1:
						// Proceed 

						// NOP
						break;

					default:
						return FALSE;
					}
				}
			}
		}
	}
	return TRUE;
}

/// mantido original
void HandleOldAssignedDriveLetter (void)
{
	if (bDevice)
	{
		// Handle assigned drive letter (if any)

		WCHAR deviceName[MAX_PATH];
		int driveLetter = -1;

		strcpy ((char *)deviceName, szDiskFile);
		ToUNICODE ((char *)deviceName);
		driveLetter = GetDiskDeviceDriveLetter (deviceName);

		if (!bHiddenVolHost
			&& !bHiddenOS
			&& driveLetter > 1)		// If a drive letter is assigned to the device, but not A: or B:
		{
			char rootPath[] = { driveLetter + 'A', ':', '\\', 0 };
			wchar_t szTmp[8192];

			swprintf (szTmp, GetString ("AFTER_FORMAT_DRIVE_LETTER_WARN"), rootPath[0], rootPath[0], rootPath[0], rootPath[0]);
			MessageBoxW (MainDlg, szTmp, lpszTitle, MB_ICONWARNING);
		}
	}
}


// Returns TRUE if it makes sense to ask the user whether he wants to store files larger than 4GB in the volume.
/// mantido original
static BOOL FileSize4GBLimitQuestionNeeded (void)
{
	uint64 dataAreaSize = GetVolumeDataAreaSize (bHiddenVol && !bHiddenVolHost, nVolumeSize);

	return (dataAreaSize > 4 * BYTES_PER_GB + TC_MIN_FAT_FS_SIZE
		&& dataAreaSize <= TC_MAX_FAT_FS_SIZE);
}


/* Except in response to the WM_INITDIALOG message, the dialog box procedure
   should return nonzero if it processes the message, and zero if it does
   not. - see DialogProc */
//********* Rotina PageDialogProc *********************************************
///
/// DLGProc das páginas (dialogos) chamadas pelo Wizard.
/// Alterada para tratar apenas INTRO_PAGE, PASSWQORD_PAGE e FORMAT_PAGE.
/// Demais páginas foram ignoradas, puladas.
///
/// \param DLGPROCPARAMS ? parametros padrao de um DLPROG

///
/// \return static LRESULT CALLBACK 
//*******************************************************************

static LRESULT CALLBACK PageDialogProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static char PageDebugId[128];
	WORD lw = LOWORD (wParam);
	WORD hw = HIWORD (wParam);

	hCurPage = hwndDlg;
	
	
	if (HB_BACK == NULL)
	  HB_BACK = CreateSolidBrush(RGB(241,241,241));

	switch (uMsg)
	{
	
	/*
	case WM_CTLCOLORDLG:
  	  return (LONG) HB_BACK;
    case WM_CTLCOLORSTATIC:
  	  return (LONG) HB_BACK;
*/
	case WM_INITDIALOG:
		LocalizeDialog (hwndDlg, "IDD_VOL_CREATION_WIZARD_DLG");

		sprintf (PageDebugId, "FORMAT_PAGE_%d", nCurPageNo);
		LastDialogId = PageDebugId;

		switch (nCurPageNo)
		{
		case INTRO_PAGE:

			
			HabilitarBotoes(hwndDlg);
			UpdateWizardModeControls (hwndDlg, WizardMode);
			break;

		case PASSWORD_PAGE:
			{
				//wchar_t str[1000];
				
				//char szTmp1[MAX_PASSWORD + 1];
				//char szTmp2[MAX_PASSWORD + 1];
				char szTmpTamanho[10];
				//int k = GetWindowTextLength (GetDlgItem (hwndDlg, IDC_PASSWORD));
				//bool bEnable = false;

				SendMessage (GetDlgItem (hwndDlg, IDC_HEADER), WM_SETFONT, (WPARAM) hTitleFont, (LPARAM) TRUE);
				HabilitarBotoes(hwndDlg);

				hPasswordInputField = GetDlgItem (hwndDlg, IDC_PASSWORD);
				hVerifyPasswordInputField = GetDlgItem (hwndDlg, IDC_VERIFY);
				SendMessage (GetDlgItem (hwndDlg, IDC_PASSWORD), EM_LIMITTEXT, MAX_PASSWORD, 0);
				SendMessage (GetDlgItem (hwndDlg, IDC_VERIFY), EM_LIMITTEXT, MAX_PASSWORD, 0);
				//Tamanho, no máximo 3 casas ...
				SendMessage (GetDlgItem (hwndDlg, IDC_SPIN), UDM_SETRANGE, 0, (LPARAM) MAKELONG ((short) 999, (short) 1));
				SendMessage (GetDlgItem (hwndDlg, IDC_TAMANHO), EM_LIMITTEXT, 3, 0);
				
				if (nUIVolumeSize == 0)
					nUIVolumeSize = 2;

				itoa(nUIVolumeSize, szTmpTamanho, 10);
				SendMessage (GetDlgItem (hwndDlg, IDC_TAMANHO), WM_SETTEXT, NULL, (LPARAM)szTmpTamanho);

				//SetWindowText (GetDlgItem (hwndDlg, IDC_PASSWORD), szRawPassword);
				//SetWindowText (GetDlgItem (hwndDlg, IDC_VERIFY), szVerify);

	            SetFocus (GetDlgItem (hwndDlg, IDC_PASSWORD));

				SetWindowText (GetDlgItem (GetParent (hwndDlg), IDC_BOX_TITLE), "Nova unidade, passo 1: Senha e Tamanho");
				
 			    HWND hBtAvancar = GetDlgItem (GetParent(hwndDlg), IDC_NEXT);
				EnableWindow (hBtAvancar,  false);
				SendMessage(hBtAvancar, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Avancar_c);
				volumePassword.Length = strlen ((char *) volumePassword.Text);

				SendMessage (GetDlgItem (hwndDlg, IDC_TAM_2), BM_SETCHECK, BST_CHECKED, 0);
				EnableWindow (GetDlgItem (hwndDlg, IDC_TAMANHO),  false);
				nUIVolumeSize = 2;
			}
			return 1;
			break;

		case FORMAT_PAGE:
			{
				BOOL bNTFSallowed = FALSE;
				BOOL bFATallowed = FALSE;
				BOOL bNoFSallowed = FALSE;
				char szTmp[255];

                SendMessage (GetDlgItem (hwndDlg, IDC_CHK_EXIBIRUNIDADE), BM_SETCHECK, BST_CHECKED, 0);
                
				SendMessage (GetDlgItem (hwndDlg, IDC_HEADER), WM_SETFONT, (WPARAM) hTitleFont, (LPARAM) TRUE);
                HabilitarBotoes(hwndDlg);
				EnableWindow (GetDlgItem (hwndDlg, IDC_ABORT_BUTTON), false);
                SendMessage(GetDlgItem (hwndDlg, IDC_ABORT_BUTTON), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Desistir_c);				
			
				SetTimer (GetParent (hwndDlg), TIMER_ID_RANDVIEW, TIMER_INTERVAL_RANDVIEW, NULL);

				hMasterKey = GetDlgItem (hwndDlg, IDC_DISK_KEY);
				hHeaderKey = GetDlgItem (hwndDlg, IDC_HEADER_KEY);
				hRandPool = GetDlgItem (hwndDlg, IDC_RANDOM_BYTES);

				SendMessage (GetDlgItem (hwndDlg, IDC_RANDOM_BYTES), WM_SETFONT, (WPARAM) hFixedDigitFont, (LPARAM) TRUE);
				SendMessage (GetDlgItem (hwndDlg, IDC_DISK_KEY), WM_SETFONT, (WPARAM) hFixedDigitFont, (LPARAM) TRUE);
				SendMessage (GetDlgItem (hwndDlg, IDC_HEADER_KEY), WM_SETFONT, (WPARAM) hFixedDigitFont, (LPARAM) TRUE);

				
				SetWindowText (GetDlgItem (GetParent (hwndDlg), IDC_BOX_TITLE), "Nova unidade, passo 2: Concluir");
                ShowWindow(GetDlgItem (GetParent (hwndDlg),IDC_NEXT) , SW_SHOW);
				EnableWindow (GetDlgItem (GetParent (hwndDlg),IDC_NEXT),  true);
                SendMessage(GetDlgItem (GetParent (hwndDlg),IDC_NEXT), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Avancar);				        
		        //SendMessage(GetDlgItem (GetParent (hwndDlg),IDC_NEXT), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Concluir);		
		        EnableWindow (GetDlgItem (GetParent (hwndDlg),IDCANCEL), true);
                SendMessage  (GetDlgItem (GetParent (hwndDlg),IDCANCEL), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Cancelar);				
				
				EnableWindow (GetDlgItem (hCurPage, IDC_CHK_EXIBIRUNIDADE), true);
				ShowWindow(GetDlgItem (hCurPage,IDC_STATIC_BAR), SW_HIDE);
				ShowWindow(GetDlgItem (hCurPage,IDT_HEADER_KEY), SW_HIDE);
				ShowWindow(GetDlgItem (hCurPage,IDC_PROGRESS_BAR), SW_HIDE);

				sprintf(szTmp, "Sua unidade de arquivos protegidos será criada com o tamanho de %i GB. Clique em 'Avançar' para finalizar o processo e iniciar seu uso.", nUIVolumeSize);
				SendMessage (GetDlgItem (hwndDlg, IDC_BOX_HELP), WM_SETTEXT, NULL, (LPARAM)szTmp);
				
				/* Quick/Dynamic */

				if (bHiddenVol)
				{
					quickFormat = !bHiddenVolHost;
					bSparseFileSwitch = FALSE;

					SetCheckBox (hwndDlg, IDC_QUICKFORMAT, quickFormat);
					SetWindowTextW (GetDlgItem (hwndDlg, IDC_QUICKFORMAT), GetString ((bDevice || !bHiddenVolHost) ? "IDC_QUICKFORMAT" : "SPARSE_FILE"));
					EnableWindow (GetDlgItem (hwndDlg, IDC_QUICKFORMAT), bDevice && bHiddenVolHost);
				}
				else
				{
					if (bDevice)
					{
						bSparseFileSwitch = FALSE;
						//SetWindowTextW (GetDlgItem (hwndDlg, IDC_QUICKFORMAT), GetString("IDC_QUICKFORMAT"));
						//EnableWindow (GetDlgItem (hwndDlg, IDC_QUICKFORMAT), TRUE);
					}
					else
					{
						char root[TC_MAX_PATH];
						DWORD fileSystemFlags = 0;

						//SetWindowTextW (GetDlgItem (hwndDlg, IDC_QUICKFORMAT), GetString("SPARSE_FILE"));

						/* Check if the host file system supports sparse files */

						if (GetVolumePathName (szFileName, root, sizeof (root)))
						{
							GetVolumeInformation (root, NULL, 0, NULL, NULL, &fileSystemFlags, NULL, 0);
							bSparseFileSwitch = fileSystemFlags & FILE_SUPPORTS_SPARSE_FILES;
						}
						else
							bSparseFileSwitch = FALSE;

						//EnableWindow (GetDlgItem (hwndDlg, IDC_QUICKFORMAT), bSparseFileSwitch);
					}
				}

				//SendMessage (GetDlgItem (hwndDlg, IDC_SHOW_KEYS), BM_SETCHECK, showKeys ? BST_CHECKED : BST_UNCHECKED, 0);
				SetWindowText (GetDlgItem (hwndDlg, IDC_RANDOM_BYTES), showKeys ? "" : "********************************                                              ");
				SetWindowText (GetDlgItem (hwndDlg, IDC_HEADER_KEY), showKeys ? "" : "********************************                                              ");
				SetWindowText (GetDlgItem (hwndDlg, IDC_DISK_KEY), showKeys ? "" : "********************************                                              ");

				//SendMessage (GetDlgItem (hwndDlg, IDC_CLUSTERSIZE), CB_RESETCONTENT, 0, 0);
				//AddComboPairW (GetDlgItem (hwndDlg, IDC_CLUSTERSIZE), GetString ("DEFAULT"), 0);
				//AddComboPair (GetDlgItem (hwndDlg, IDC_CLUSTERSIZE), "0.5 KB", 1);
				//AddComboPair (GetDlgItem (hwndDlg, IDC_CLUSTERSIZE), "1 KB", 2);
				//AddComboPair (GetDlgItem (hwndDlg, IDC_CLUSTERSIZE), "2 KB", 4);
				//AddComboPair (GetDlgItem (hwndDlg, IDC_CLUSTERSIZE), "4 KB", 8);
				//AddComboPair (GetDlgItem (hwndDlg, IDC_CLUSTERSIZE), "8 KB", 16);
				//AddComboPair (GetDlgItem (hwndDlg, IDC_CLUSTERSIZE), "16 KB", 32);
				//AddComboPair (GetDlgItem (hwndDlg, IDC_CLUSTERSIZE), "32 KB", 64);
				//AddComboPair (GetDlgItem (hwndDlg, IDC_CLUSTERSIZE), "64 KB", 128);
				//SendMessage (GetDlgItem (hwndDlg, IDC_CLUSTERSIZE), CB_SETCURSEL, 0, 0);

				//EnableWindow (GetDlgItem (hwndDlg, IDC_CLUSTERSIZE), TRUE);

				/* Filesystems */

				bNTFSallowed = FALSE;
				bFATallowed = FALSE;
				bNoFSallowed = FALSE;

				//SendMessage (GetDlgItem (hwndDlg, IDC_FILESYS), CB_RESETCONTENT, 0, 0);

				//EnableWindow (GetDlgItem (hwndDlg, IDC_FILESYS), TRUE);

				uint64 dataAreaSize = GetVolumeDataAreaSize (bHiddenVol && !bHiddenVolHost, nVolumeSize);

				if (!CreatingHiddenSysVol())	
				{
					if (dataAreaSize >= TC_MIN_NTFS_FS_SIZE && dataAreaSize <= TC_MAX_NTFS_FS_SIZE)
					{
						AddComboPair (GetDlgItem (hwndDlg, IDC_FILESYS), "NTFS", FILESYS_NTFS);
						bNTFSallowed = TRUE;
					}

					if (dataAreaSize >= TC_MIN_FAT_FS_SIZE && dataAreaSize <= TC_MAX_FAT_FS_SIZE)
					{
						AddComboPair (GetDlgItem (hwndDlg, IDC_FILESYS), "FAT", FILESYS_FAT);
						bFATallowed = TRUE;
					}
				}
				else
				{
					// We're creating a hidden volume for a hidden OS, so we don't need to format it with
					// any filesystem (the entire OS will be copied to the hidden volume sector by sector).
					//EnableWindow (GetDlgItem (hwndDlg, IDC_FILESYS), FALSE);
					//EnableWindow (GetDlgItem (hwndDlg, IDC_CLUSTERSIZE), FALSE);
				}

				if (!bHiddenVolHost)
				{
					//AddComboPairW (GetDlgItem (hwndDlg, IDC_FILESYS), GetString ("NONE"), FILESYS_NONE);
					bNoFSallowed = TRUE;
				}

				//EnableWindow (GetDlgItem (GetParent (hwndDlg), IDC_NEXT), TRUE);

				if (fileSystem == FILESYS_NONE)	// If no file system has been previously selected
				{
					// Set default file system

					if (bFATallowed && !(nNeedToStoreFilesOver4GB == 1 && bNTFSallowed))
						fileSystem = FILESYS_FAT;
					else if (bNTFSallowed)
						fileSystem = FILESYS_NTFS;
					else if (bNoFSallowed)
						fileSystem = FILESYS_NONE;
					else
					{
						//AddComboPair (GetDlgItem (hwndDlg, IDC_FILESYS), "---", 0);
						//EnableWindow (GetDlgItem (GetParent (hwndDlg), IDC_NEXT), FALSE);
					}
				}

				//SendMessage (GetDlgItem (hwndDlg, IDC_FILESYS), CB_SETCURSEL, 0, 0);
				//SelectAlgo (GetDlgItem (hwndDlg, IDC_FILESYS), (int *) &fileSystem);

				//EnableWindow (GetDlgItem (hwndDlg, IDC_ABORT_BUTTON), FALSE);

				//SetWindowTextW (GetDlgItem (GetParent (hwndDlg), IDC_NEXT), GetString ("FORMAT"));
				//SetWindowTextW (GetDlgItem (GetParent (hwndDlg), IDC_PREV), GetString ("PREV"));

				//EnableWindow (GetDlgItem (GetParent (hwndDlg), IDC_PREV), TRUE);

				//SetFocus (GetDlgItem (GetParent (hwndDlg), IDC_NEXT));
			}
			break;

		case FORMAT_FINISHED_PAGE:
			{
				if (!bHiddenVolHost && bHiddenVol && !bHiddenVolFinished)
				{
					wchar_t msg[4096];

					nNeedToStoreFilesOver4GB = -1;

					if (bHiddenOS)
					{
						wchar_t szMaxRecomOuterVolFillSize[100];

						__int64 maxRecomOuterVolFillSize = 0;

						// Determine the maximum recommended total size of files that can be copied to the outer volume
						// while leaving enough space for the hidden volume, which must contain a clone of the OS

						maxRecomOuterVolFillSize = nVolumeSize - GetSystemPartitionSize(); 

						// -50% reserve for filesystem "peculiarities"
						maxRecomOuterVolFillSize /= 2;	

						swprintf (szMaxRecomOuterVolFillSize, L"%I64d %s", maxRecomOuterVolFillSize / BYTES_PER_MB, GetString ("MB"));

						swprintf (msg, GetString ("HIDVOL_HOST_FILLING_HELP_SYSENC"), hiddenVolHostDriveNo + 'A', szMaxRecomOuterVolFillSize);			
					}
					else
						swprintf (msg, GetString ("HIDVOL_HOST_FILLING_HELP"), hiddenVolHostDriveNo + 'A');

					//SetWindowTextW (GetDlgItem (hwndDlg, IDC_BOX_HELP), msg);
					//SetWindowTextW (GetDlgItem (GetParent (hwndDlg), IDC_BOX_TITLE), GetString ("HIDVOL_HOST_FILLING_TITLE"));
				}
				else 
				{
					//if (bHiddenOS)
					//	SetWindowTextW (GetDlgItem (hwndDlg, IDC_BOX_HELP), GetString ("SYSENC_HIDDEN_VOL_FORMAT_FINISHED_HELP"));
					//else
					//{
						//SetWindowTextW (GetDlgItem (hwndDlg, IDC_BOX_HELP), GetString (bInPlaceEncNonSys ? "NONSYS_INPLACE_ENC_FINISHED_INFO" : "FORMAT_FINISHED_HELP"));
						//bConfirmQuit = FALSE;
					//}

					//SetWindowTextW (GetDlgItem (GetParent (hwndDlg), IDC_BOX_TITLE), GetString (bHiddenVol ? "HIDVOL_FORMAT_FINISHED_TITLE" : "FORMAT_FINISHED_TITLE"));
				}


				//SetWindowTextW (GetDlgItem (GetParent (hwndDlg), IDC_NEXT), GetString ("NEXT"));
				//SetWindowTextW (GetDlgItem (GetParent (hwndDlg), IDC_PREV), GetString ("PREV"));
				//EnableWindow (GetDlgItem (GetParent (hwndDlg), IDC_NEXT), FALSE);
				//EnableWindow (GetDlgItem (GetParent (hwndDlg), IDC_PREV), FALSE);

				//if ((!bHiddenVol || bHiddenVolFinished) && !bHiddenOS)
				//SetWindowTextW (GetDlgItem (GetParent (hwndDlg), IDCANCEL), GetString ("EXIT"));
			}
			break;
		}
		return 0;

	case WM_HELP:
		OpenPageHelp (GetParent (hwndDlg), nCurPageNo);
		return 1;

	case TC_APPMSG_PERFORM_POST_SYSENC_WMINIT_TASKS:
		AfterSysEncProgressWMInitTasks (hwndDlg);
		return 1;

	case WM_COMMAND:

		if (nCurPageNo == INTRO_PAGE && hw==0)
		{
			switch (lw)
			{
			  case IDC_MONTAR_VOLUME:
                int x;
				//BOOL tmpbDevice;
				BOOL mounted;
				//--> BOTÃO next DESABILITADO --> BM_CLICK não vai funcionar
				//PostMessage(GetDlgItem(GetParent (hwndDlg), IDC_NEXT), BM_CLICK, 0,0);
				//SHGetFolderPath (NULL, CSIDL_MYDOCUMENTS, NULL, 0, szFileName);
				//strcat (szFileName, "\\ArquivosSeguros.it");			
				//CreateFullVolumePath (szDiskFile, szFileName, &tmpbDevice);			
				if (!LerConfiguracoes())
				{
					Error ("ERROR_CONFIG_FILE");
					NormalCursor ();
					return 1;
				}
				if (IsMountedVolume (szDiskFile))
				{
					Error ("ALREADY_MOUNTED");
					NormalCursor ();
					return 1;
				}
                //Arquivo já existe, montar volume direto;
                x = _access (szDiskFile, 06);
			    if (x == 0 || errno != ENOENT)
				{
				  wchar_t szDriveLetter[3];
				  //wchar_t szMsg[8192];
				  char szTmp[255];

				  			  				  
				  if (!AskVolumePassword (hwndDlg, &volumePassword, "Acessar Unidade", FALSE))
				    return 1;				  
                  
				  Silent = true;
				  mounted = MontarVolume(GetParent (hwndDlg), szDriveLetter);
				  Silent = false;
                  
				  if (mounted)
				  { 
                      sprintf(szTmp, "%c:\\", szDriveLetter[0]);
					  SetVolumeLabelA(szTmp, "Unid_Segura");
					  GravarConfiguracoes("MONTADO", "TRUE");
					  GravarConfiguracoes("LIBERADO", "FALSE");
					  GravarConfiguracoes("UNIDADE", szTmp);
					  //swprintf (szMsg, GetString ("FORMAT_FINISHED_INFO"), szDriveLetter)
					  sprintf(szTmp, "Unidade de arquivos protegidos %c: disponível para acesso, através do Windows Explorer.", szDriveLetter[0]);
					  MessageBox (hwndDlg, (LPCSTR) szTmp, (LPCSTR) "Central de proteção de arquivos", MB_ICONEXCLAMATION);
					  
					  sprintf(szTmp, "ATENÇÃO: A unidade de arquivos protegidos está aberta, e deverá ser fechada caso não esteja sendo utilizada. Isso garante a privacidade dos arquivos confidenciais.");
					  MessageBox (hwndDlg, (LPCSTR) szTmp, (LPCSTR) "Central de proteção de arquivos", MB_ICONEXCLAMATION);
					  
					  MessageBox(hwndDlg, TC_ALERTA_SENHA, MB_OK | MB_ICONINFORMATION);
					  if (g_bExibirUnidade)
					  {
					     sprintf(szTmp, "Explorer.exe %c:\\", szDriveLetter[0]);
					     WinExec(szTmp, SW_SHOWNORMAL);
					  }
					  
				  }
				
				//	  Info ("FORMAT_FINISHED_INFO");
				  else
				  {
					//Error ("PASSWORD_WRONG");
					MessageBox (hwndDlg, (LPCSTR)"Erro ao acessar a unidade de arquivos protegidos. Verifique sua senha e tente novamente.", (LPCSTR)"Central de proteção de arquivos", ICON_HAND);
					NormalCursor ();
				    GravarConfiguracoes("MONTADO", "FALSE");

					return 1;
				  }
				}
				else //Arquivo não existe, entrar no Wizar para criar nova Unidade
				{				
                    MessageBox (MainDlg, (LPCSTR)"A unidade de proteção de arquivos ainda não existe, siga os passos a seguir para criá-la e iniciar o seu uso", (LPCSTR)"Central de proteção de arquivos", MB_ICONEXCLAMATION);
					PostMessage(GetParent (hwndDlg), WM_COMMAND, MAKEWPARAM(IDC_NEXT, BN_CLICKED), (LPARAM) GetDlgItem(GetParent (hwndDlg), IDC_NEXT));                 
					//PostMessage(GetParent (hwndDlg), WM_COMMAND, (WPARAM) 0, (LPARAM) GetDlgItem(GetParent (hwndDlg), IDC_NEXT));                 
				}
				 HabilitarBotoes(hwndDlg);
                 return 1;				
			  case IDC_DESMONTAR_VOLUME:
				if (!LerConfiguracoes())
				{
					Error ("ERROR_CONFIG_FILE");
					NormalCursor ();
					return 1;
				}
				
				ChecaUnidadeVazia();
			    
				DismountAll (GetParent (hwndDlg), TRUE, FALSE, UNMOUNT_MAX_AUTO_RETRIES, UNMOUNT_AUTO_RETRY_DELAY);
			    MessageBox (MainDlg, (LPCSTR) "Unidade de arquivos protegidos fechada com sucesso. Para acessá-la novamente, selecione a opção 'Acessar unidade' na tela principal do CPA.", (LPCSTR) "Central de proteção de arquivos", MB_ICONEXCLAMATION);
			    
				GravarConfiguracoes("MONTADO", "FALSE");
                GravarConfiguracoes("LIBERADO", "TRUE");

				//Info ("MOUNTED_VOLUMES_AUTO_DISMOUNTED");
				HabilitarBotoes(hwndDlg);
				return 1;
			  case IDC_ALTERAR_SENHA:
				if (!LerConfiguracoes())
				{
					Error ("ERROR_CONFIG_FILE");
					NormalCursor ();
					return 1;
				}
				if (!FileExists(szDiskFile))
				{
					Error ("ERROR_FILE_NOT_EXISTS");
					NormalCursor ();
					return 1;
				}
				pwdChangeDlgMode = PCDM_CHANGE_PASSWORD;
                ChangePassword(GetParent (hwndDlg));
				return 1;
			  case IDC_EXCLUIR_UNIDADE:
				  LerConfiguracoes();
				  if (strcmp(szConfigVazio, "TRUE"))
				  {				  
					  MessageBox(hwndDlg, "Existem arquivos na unidade de arquivos protegidos. Antes de remover a unidade de arquivos protegidos, remova todos os arquivos, ou movimente-os para outra pasta. Caso os arquivos sejam movimentados para outra pasta, seu conteúdo não permanecerá criptografado (protegido).", "Central de proteção de arquivos", MB_OK | MB_ICONSTOP);
				  }
				  else //vazio, checar consistencia dos flags... se entrou aqui, não esta amontada...
				  {
					 if (strcmp(szConfigLiberado, "TRUE") || //se liberado not TRUE ou 
						 strcmp(szConfigMontado, "FALSE")) //Montado not false
					 {
						 MessageBox(hwndDlg, "Imposível remover unidade. Acesse a unidade, remova todos os seus arquivos e tente novamente.", "Central de proteção de arquivos", MB_OK | MB_ICONSTOP);
					 }
					 else
					 {
                         if (MessageBox(hwndDlg, "ATENÇÃO: A unidade de arquivos protegidos será removida, deseja continuar? (Caso seja necessária sua utilização, ela deverá ser criada novamente).", "Central de proteção de arquivos", MB_OKCANCEL | MB_ICONINFORMATION ) == IDOK)						 
						 {
							 if (!AskVolumePassword (hwndDlg, &volumePassword, "Remover unidade", FALSE))
				               return 1;
							 else
							 {
								 wchar_t szDriveLetter[3];
								 //checar se senha digita é correta --> AskVoumePassword já verifica?
								 Silent = true;
				                 mounted = MontarVolume(hwndDlg, szDriveLetter);
				                 //Silent = false;
								 if (mounted)
								   DismountAll (GetParent (hwndDlg), TRUE, FALSE, UNMOUNT_MAX_AUTO_RETRIES, UNMOUNT_AUTO_RETRY_DELAY);
								 else
								 {
                                     MessageBox(hwndDlg, "Senha incorreta. Impossível remover unidade.", "Central de proteção de arquivos", MB_OK | MB_ICONINFORMATION );						 						
									 return 1;
								 }

								 if (DeleteFile(szDiskFile))
								 {
								   GravarConfiguracoes("LIBERADO", "TRUE");
								   GravarConfiguracoes("MONTADO", "FALSE");
								   GravarConfiguracoes("VAZIO", "TRUE");
								   GravarConfiguracoes("UNIDADE", "-");
								   MessageBox(hwndDlg, "Unidade de arquivos protegidos removida com sucesso.", "Central de proteção de arquivos", MB_OK | MB_ICONINFORMATION);
								   HabilitarBotoes(hwndDlg);
								 }
								 else
								   MessageBox(hwndDlg, "Imposível remover unidade. Acesse a unidade, remova todos os seus arquivos e tente novamente.", "Central de proteção de arquivos", MB_OK | MB_ICONSTOP);
							 }
						}

					 }
				  }
                  return 1;
			  case IDC_AJUDA:
					InitHelpFileName();
					//OpenPageHelp (hwndDlg, nCurPageNo);
					int r = (int)ShellExecute (NULL, "open", szHelpFile, NULL, NULL, SW_SHOWNORMAL);
 					if (r == ERROR_FILE_NOT_FOUND)
					{
						r = (int)ShellExecute (NULL, "open", szHelpFile2, NULL, NULL, SW_SHOWNORMAL);
						if (r == ERROR_FILE_NOT_FOUND)
						  MessageBox(hwndDlg, "Arquivo de ajuda não encontrado", "Central de proteção de arquivos", MB_OK | MB_ICONSTOP);
					}
					return 1;
/*
                case IDC_ALTERAR_NOME:
				if (!LerConfiguracoes())
				{
					Error ("ERROR_CONFIG_FILE");
					NormalCursor ();
					return 1;
				}
				if (IsMountedVolume (szDiskFile))
				{
					Warning ("MOUNTED_NOPWCHANGE");
					NormalCursor ();
					return 1;
				}
				if (!FileExists(szDiskFile))
				{
					Error ("ERROR_FILE_NOT_EXISTS");
					NormalCursor ();
					return 1;
				}
				RenomearVolume(hwndDlg);
				
				return 1;
*/
			}

		}

		if ( (lw == IDC_TAM_2 || lw == IDC_TAM_5 || lw == IDC_TAM_10 || lw == IDC_TAM_20 || lw == IDC_TAM_OUTRO) && nCurPageNo == PASSWORD_PAGE)
	    {            
			EnableWindow (GetDlgItem (hwndDlg, IDC_TAMANHO),  GetCheckBox (hwndDlg, IDC_TAM_OUTRO));
            switch (lw)
			{
			  case IDC_TAM_2   : nUIVolumeSize = 2; break;
              case IDC_TAM_5  : nUIVolumeSize  = 5; break;
              case IDC_TAM_10  : nUIVolumeSize = 10; break;
              case IDC_TAM_20 : nUIVolumeSize  = 20; break;
			}

			if (lw == IDC_TAM_OUTRO && GetCheckBox (hwndDlg, IDC_TAM_OUTRO))
			  SetFocus (GetDlgItem (hwndDlg, IDC_TAMANHO));
			else
			  SetFocus (GetDlgItem (hwndDlg, lw));
			return 1;
		}

		if (lw == IDC_ABORT_BUTTON && nCurPageNo == FORMAT_PAGE && hw == 0)
		{
				  
			if (MessageBox (hwndDlg, "Deseja realmente interromper a criação da unidade de proteção?", (LPCSTR) "Central de proteção de arquivos", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 ) == IDYES)
			{
				EnableWindow (GetDlgItem (hwndDlg, IDC_ABORT_BUTTON), false);
                SendMessage(GetDlgItem (hwndDlg, IDC_ABORT_BUTTON), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Desistir_c);				
				EnableWindow (GetDlgItem (GetParent (hwndDlg),IDC_NEXT),  true);
		        SendMessage(GetDlgItem (GetParent (hwndDlg),IDC_NEXT), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Concluir);		
		        EnableWindow (GetDlgItem (GetParent (hwndDlg),IDCANCEL), true);
                SendMessage  (GetDlgItem (GetParent (hwndDlg),IDCANCEL), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Cancelar);						
				bVolTransformThreadCancel = TRUE;
			}
			return 1;
		}
				
		if (hw == EN_CHANGE && nCurPageNo == PASSWORD_PAGE)
	    {
		
				char szTmp1[MAX_PASSWORD + 1];
				char szTmp2[MAX_PASSWORD + 1];
				char szTmpTamanho[10]; //limitado a tres, mas s e aumentar no edit ... já fica certo aqui
				int k = GetWindowTextLength (GetDlgItem (hwndDlg, IDC_PASSWORD));
				bool bEnable = false;

				GetWindowText (GetDlgItem (hwndDlg, IDC_PASSWORD), szTmp1, sizeof (szTmp1));
	            GetWindowText (GetDlgItem (hwndDlg, IDC_VERIFY), szTmp2, sizeof (szTmp2));
                
				if (GetCheckBox (hwndDlg, IDC_TAM_OUTRO))
				{
				  GetWindowText (GetDlgItem (hwndDlg, IDC_TAMANHO), szTmpTamanho, sizeof (szTmpTamanho));
				  nUIVolumeSize = atoi(szTmpTamanho);
				}

				if ((nUIVolumeSize == 0) || (strcmp (szTmp1, szTmp2) != 0))
					bEnable = FALSE;
				else
				{
					if (k >= MIN_PASSWORD)
						bEnable = TRUE;
					else
						bEnable = FALSE;
				}

				HWND hBtAvancar = GetDlgItem (GetParent(hwndDlg), IDC_NEXT);
				if (bEnable)
				{	
					EnableWindow (hBtAvancar,  true);
					SendMessage(hBtAvancar, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Avancar);
				}
				else
				{
					EnableWindow (hBtAvancar,  false);
					SendMessage(hBtAvancar, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Avancar_c);
				}

		volumePassword.Length = strlen ((char *) volumePassword.Text);

		return 1;
	    }

	}
	return 0;
}

/* Except in response to the WM_INITDIALOG and WM_ENDSESSION messages, the dialog box procedure
   should return nonzero if it processes the message, and zero if it does not. - see DialogProc */
//********* Rotina MainDialogProc *********************************************
///
/// DLGProc do diálogo principal do aplicativo
/// Alterada para tratar apenas INTRO_PAGE, PASSWQORD_PAGE e FORMAT_PAGE.
/// Demais páginas foram ignoradas, puladas.
///
/// \param DLGPROCPARAMS ? parametros padrao de um DLPROG

///
/// \return static LRESULT CALLBACK 
//*******************************************************************
static LRESULT CALLBACK MainDialogProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WORD lw = LOWORD (wParam);
	WORD hw = HIWORD (wParam);

	if (HB_BACK == NULL)
	  HB_BACK = CreateSolidBrush(RGB(241,241,241));

	int nNewPageNo = nCurPageNo;

	switch (uMsg)
	{
	
	/*
	case WM_CTLCOLORDLG:
  	  return (LONG) HB_BACK;
    case WM_CTLCOLORSTATIC:
  	  return (LONG) HB_BACK;
	*/

	case WM_INITDIALOG:
		{
			MainDlg = hwndDlg;
			InitDialog (hwndDlg);
			LocalizeDialog (hwndDlg, "IDD_VOL_CREATION_WIZARD_DLG");

			if (IsTrueCryptInstallerRunning())
				AbortProcess ("TC_INSTALLER_IS_RUNNING");

			// Resize the bitmap if the user has a non-default DPI 
			if (ScreenDPI != USER_DEFAULT_SCREEN_DPI)
			{
				hbmWizardBitmapRescaled = RenderBitmap (MAKEINTRESOURCE (IDB_ILUSTRACAO),
					GetDlgItem (hwndDlg, IDC_BITMAP_WIZARD),
					0, 0, 0, 0, FALSE, FALSE);
			}

			LoadSettings (hwndDlg);

			LoadDefaultKeyFilesParam ();
			RestoreDefaultKeyFilesParam ();

			SysEncMultiBootCfg.NumberOfSysDrives = -1;
			SysEncMultiBootCfg.MultipleSystemsOnDrive = -1;
			SysEncMultiBootCfg.BootLoaderLocation = -1;
			SysEncMultiBootCfg.BootLoaderBrand = -1;
			SysEncMultiBootCfg.SystemOnBootDrive = -1;

			try
			{
				BootEncStatus = BootEncObj->GetStatus();
			}
			catch (Exception &e)
			{
				e.Show (hwndDlg);
				Error ("ERR_GETTING_SYSTEM_ENCRYPTION_STATUS");
				EndMainDlg (MainDlg);
				return 0;
			}

			SendMessage (GetDlgItem (hwndDlg, IDC_BOX_TITLE), WM_SETFONT, (WPARAM) hTitleFont, (LPARAM) TRUE);
			
			//SetWindowTextW (hwndDlg, lpszTitle);

			ExtractCommandLine (hwndDlg, (char *) lParam);

			if (ComServerMode)
			{
				InitDialog (hwndDlg);

				if (!ComServerFormat ())
				{
					handleWin32Error (hwndDlg);
					exit (1);
				}
				exit (0);
			}

			SHGetFolderPath (NULL, CSIDL_MYDOCUMENTS, NULL, 0, szRescueDiskISO);
			strcat (szRescueDiskISO, "\\TrueCrypt Rescue Disk.iso");

#ifdef _DEBUG
			// For faster testing
			//sprintf (szVerify, "%s", "q");
			//sprintf (szRawPassword, "%s", "q");
#endif

			PostMessage (hwndDlg, TC_APPMSG_PERFORM_POST_WMINIT_TASKS, 0, 0);
		}
		return 0;

	case WM_SYSCOMMAND:
		if (lw == IDC_ABOUT)
		{
			DialogBoxW (hInst, MAKEINTRESOURCEW (IDD_ABOUT_DLG), hwndDlg, (DLGPROC) AboutDlgProc);
			return 1;
		}
		return 0;

	case WM_TIMER:

		switch (wParam)
		{
		case TIMER_ID_RANDVIEW:

			if (WizardMode == WIZARD_MODE_SYS_DEVICE
				|| bInPlaceEncNonSys)
			{
				DisplayRandPool (hRandPoolSys, showKeys);
			}
			else
			{
				unsigned char tmp[17];
				char tmp2[43];
				int i;

				if (!showKeys) 
					return 1;

				RandpeekBytes (tmp, sizeof (tmp));

				tmp2[0] = 0;

				for (i = 0; i < sizeof (tmp); i++)
				{
					char tmp3[8];
					sprintf (tmp3, "%02X", (int) (unsigned char) tmp[i]);
					strcat (tmp2, tmp3);
				}

				tmp2[32] = 0;

				SetWindowText (GetDlgItem (hCurPage, IDC_RANDOM_BYTES), tmp2);

				burn (tmp, sizeof(tmp));
				burn (tmp2, sizeof(tmp2));
			}
			return 1;

		case TIMER_ID_SYSENC_PROGRESS:
			{
				// Manage system encryption/decryption and update related GUI

				try
				{
					BootEncStatus = BootEncObj->GetStatus();
				}
				catch (Exception &e)
				{
					KillTimer (MainDlg, TIMER_ID_SYSENC_PROGRESS);

					try
					{
						BootEncObj->AbortSetup ();
					}
					catch (Exception &e)
					{
						e.Show (hwndDlg);
					}

					e.Show (hwndDlg);
					Error ("ERR_GETTING_SYSTEM_ENCRYPTION_STATUS");
					EndMainDlg (MainDlg);
					return 1;
				}

				if (BootEncStatus.SetupInProgress)
					UpdateSysEncProgressBar ();

				if (bSystemEncryptionInProgress != BootEncStatus.SetupInProgress)
				{
					bSystemEncryptionInProgress = BootEncStatus.SetupInProgress;

					UpdateSysEncProgressBar ();
					UpdateSysEncControls ();

					if (!bSystemEncryptionInProgress)
					{
						// The driver stopped encrypting/decrypting

						KillTimer (hwndDlg, TIMER_ID_SYSENC_PROGRESS);

						try
						{
							if (BootEncStatus.DriveMounted)	// If we had been really encrypting/decrypting (not just proceeding to deinstall)
								BootEncObj->CheckEncryptionSetupResult();
						}
						catch (SystemException &e)
						{
							if (!bTryToCorrectReadErrors
								&& SystemEncryptionStatus == SYSENC_STATUS_ENCRYPTING
								&& (e.ErrorCode == ERROR_CRC || e.ErrorCode == ERROR_IO_DEVICE))
							{
								bTryToCorrectReadErrors = (AskWarnYesNo ("ENABLE_BAD_SECTOR_ZEROING") == IDYES);

								if (bTryToCorrectReadErrors)
								{
									SysEncResume();
									return 1;
								}
							}
							else
							{
								e.Show (hwndDlg);
							}
						}
						catch (Exception &e)
						{
							e.Show (hwndDlg);
						}

						switch (SystemEncryptionStatus)
						{
						case SYSENC_STATUS_ENCRYPTING:

							if (BootEncStatus.ConfiguredEncryptedAreaStart == BootEncStatus.EncryptedAreaStart
								&& BootEncStatus.ConfiguredEncryptedAreaEnd == BootEncStatus.EncryptedAreaEnd)
							{
								// The partition/drive has been fully encrypted

								ManageStartupSeqWiz (TRUE, "");

								SetWindowTextW (GetDlgItem (hwndDlg, IDC_NEXT), GetString ("FINALIZE"));
								EnableWindow (GetDlgItem (hwndDlg, IDC_NEXT), TRUE);
								EnableWindow (GetDlgItem (hwndDlg, IDCANCEL), FALSE);
								EnableWindow (GetDlgItem (hCurPage, IDC_WIPE_MODE), FALSE);
								EnableWindow (GetDlgItem (hCurPage, IDC_PAUSE), FALSE);

								WipeHiddenOSCreationConfig();	// For extra conservative security

								ChangeSystemEncryptionStatus (SYSENC_STATUS_NONE);

								Info ("SYSTEM_ENCRYPTION_FINISHED");
								return 1;
							}
							break;

						case SYSENC_STATUS_DECRYPTING:

							if (!BootEncStatus.DriveEncrypted)
							{
								// The partition/drive has been fully decrypted

								try
								{
									// Finalize the process
									BootEncObj->Deinstall ();
								}
								catch (Exception &e)
								{
									e.Show (hwndDlg);
								}
					
								ManageStartupSeqWiz (TRUE, "");
								ChangeSystemEncryptionStatus (SYSENC_STATUS_NONE);

								SetWindowTextW (GetDlgItem (hwndDlg, IDC_NEXT), GetString ("FINALIZE"));
								EnableWindow (GetDlgItem (hwndDlg, IDC_NEXT), TRUE);
								EnableWindow (GetDlgItem (hwndDlg, IDCANCEL), FALSE);
								EnableWindow (GetDlgItem (hCurPage, IDC_PAUSE), FALSE);

								Info ("SYSTEM_DECRYPTION_FINISHED");

								return 1;
							}
							break;
						}
					}
				}
			}
			return 1;

		case TIMER_ID_NONSYS_INPLACE_ENC_PROGRESS:

			if (bInPlaceEncNonSys)
			{
				// Non-system in-place encryption

				if (!bVolTransformThreadRunning && !bVolTransformThreadToRun)
				{
					KillTimer (hwndDlg, TIMER_ID_NONSYS_INPLACE_ENC_PROGRESS);
				}

				UpdateNonSysInPlaceEncControls ();
			}
			return 1;

		case TIMER_ID_KEYB_LAYOUT_GUARD:
			if (SysEncInEffect ())
			{
				DWORD keybLayout = (DWORD) GetKeyboardLayout (NULL);

				/* Watch the keyboard layout */

				if (keybLayout != 0x00000409 && keybLayout != 0x04090409)
				{
					// Keyboard layout is not standard US

					WipePasswordsAndKeyfiles ();

					SetWindowText (GetDlgItem (hCurPage, IDC_PASSWORD), szRawPassword);
					SetWindowText (GetDlgItem (hCurPage, IDC_VERIFY), szVerify);

					keybLayout = (DWORD) LoadKeyboardLayout ("00000409", KLF_ACTIVATE);

					if (keybLayout != 0x00000409 && keybLayout != 0x04090409)
					{
						KillTimer (hwndDlg, TIMER_ID_KEYB_LAYOUT_GUARD);
						Error ("CANT_CHANGE_KEYB_LAYOUT_FOR_SYS_ENCRYPTION");
						EndMainDlg (MainDlg);
						return 1;
					}

					bKeyboardLayoutChanged = TRUE;

					wchar_t szTmp [4096];
					wcscpy (szTmp, GetString ("KEYB_LAYOUT_CHANGE_PREVENTED"));
					wcscat (szTmp, L"\n\n");
					wcscat (szTmp, GetString ("KEYB_LAYOUT_SYS_ENC_EXPLANATION"));
					MessageBoxW (MainDlg, szTmp, lpszTitle, MB_ICONWARNING | MB_SETFOREGROUND | MB_TOPMOST);
				}

				/* Watch the right Alt key (which is used to enter various characters on non-US keyboards) */

				if (bKeyboardLayoutChanged && !bKeybLayoutAltKeyWarningShown)
				{
					if (GetAsyncKeyState (VK_RMENU) < 0)
					{
						bKeybLayoutAltKeyWarningShown = TRUE;

						wchar_t szTmp [4096];
						wcscpy (szTmp, GetString ("ALT_KEY_CHARS_NOT_FOR_SYS_ENCRYPTION"));
						wcscat (szTmp, L"\n\n");
						wcscat (szTmp, GetString ("KEYB_LAYOUT_SYS_ENC_EXPLANATION"));
						MessageBoxW (MainDlg, szTmp, lpszTitle, MB_ICONINFORMATION  | MB_SETFOREGROUND | MB_TOPMOST);
					}
				}
			}
			return 1;

		case TIMER_ID_SYSENC_DRIVE_ANALYSIS_PROGRESS:

			if (bSysEncDriveAnalysisInProgress)
			{
				UpdateProgressBarProc (GetTickCount() - SysEncDriveAnalysisStart);

				if (GetTickCount() - SysEncDriveAnalysisStart > SYSENC_DRIVE_ANALYSIS_ETA)
				{
					// It's taking longer than expected -- reinit the progress bar
					SysEncDriveAnalysisStart = GetTickCount ();
					InitProgressBar (SYSENC_DRIVE_ANALYSIS_ETA, 0, FALSE, FALSE, FALSE, TRUE);
				}

				ArrowWaitCursor ();
			}
			else
			{
				KillTimer (hwndDlg, TIMER_ID_SYSENC_DRIVE_ANALYSIS_PROGRESS);
				UpdateProgressBarProc (SYSENC_DRIVE_ANALYSIS_ETA);
				Sleep (1500);	// User-friendly GUI

				if (bSysEncDriveAnalysisTimeOutOccurred)
					Warning ("SYS_DRIVE_SIZE_PROBE_TIMEOUT");

				LoadPage (hwndDlg, SYSENC_DRIVE_ANALYSIS_PAGE + 1);
			}
			return 1;

		case TIMER_ID_WIPE_PROGRESS:

			// Manage device wipe and update related GUI

			if (bHiddenOS && IsHiddenOSRunning())
			{
				// Decoy system partition wipe 

				DecoySystemWipeStatus decoySysPartitionWipeStatus;

				try
				{
					decoySysPartitionWipeStatus = BootEncObj->GetDecoyOSWipeStatus();
					BootEncStatus = BootEncObj->GetStatus();
				}
				catch (Exception &e)
				{
					KillTimer (MainDlg, TIMER_ID_WIPE_PROGRESS);

					try
					{
						BootEncObj->AbortDecoyOSWipe ();
					}
					catch (Exception &e)
					{
						e.Show (hwndDlg);
					}

					e.Show (hwndDlg);
					EndMainDlg (MainDlg);
					return 1;
				}

				if (decoySysPartitionWipeStatus.WipeInProgress)
				{
					ArrowWaitCursor ();

					UpdateWipeProgressBar ();
				}

				if (bDeviceWipeInProgress != decoySysPartitionWipeStatus.WipeInProgress)
				{
					bDeviceWipeInProgress = decoySysPartitionWipeStatus.WipeInProgress;

					UpdateWipeProgressBar ();
					UpdateWipeControls ();

					if (!bDeviceWipeInProgress)
					{
						// The driver stopped wiping

						KillTimer (hwndDlg, TIMER_ID_WIPE_PROGRESS);

						try
						{
							BootEncObj->CheckDecoyOSWipeResult();
						}
						catch (Exception &e)
						{
							e.Show (hwndDlg);
							AbortProcessSilent();
						}

						if (BootEncStatus.ConfiguredEncryptedAreaEnd == decoySysPartitionWipeStatus.WipedAreaEnd)
						{
							// Decoy system partition has been fully wiped

							ChangeHiddenOSCreationPhase (TC_HIDDEN_OS_CREATION_PHASE_WIPED);

							SetWindowTextW (GetDlgItem (MainDlg, IDCANCEL), GetString ("EXIT"));
							EnableWindow (GetDlgItem (MainDlg, IDCANCEL), TRUE);
							EnableWindow (GetDlgItem (MainDlg, IDC_PREV), FALSE);
							EnableWindow (GetDlgItem (MainDlg, IDC_NEXT), FALSE);
							EnableWindow (GetDlgItem (hCurPage, IDC_ABORT_BUTTON), FALSE);

							Info ("WIPE_FINISHED_DECOY_SYSTEM_PARTITION");

							TextInfoDialogBox (TC_TBXID_DECOY_OS_INSTRUCTIONS);

							return 1;
						}
					}
				}
			}
			else
			{
				// Regular device wipe (not decoy system partition wipe)

				//Info ("WIPE_FINISHED");
			}
			return 1;
		}

		return 0;


	case TC_APPMSG_PERFORM_POST_WMINIT_TASKS:

		AfterWMInitTasks (hwndDlg);
		return 1;

	case TC_APPMSG_FORMAT_FINISHED:
		{
			BOOL mounted;
			char tmp[RNG_POOL_SIZE*2+1];
            wchar_t szDriveLetter[3];

			EnableWindow (GetDlgItem (hCurPage, IDC_ABORT_BUTTON), FALSE);
			EnableWindow (GetDlgItem (hwndDlg, IDC_PREV), FALSE);
			//EnableWindow (GetDlgItem (hwndDlg, IDHELP), TRUE);
			EnableWindow (GetDlgItem (hwndDlg, IDCANCEL), TRUE);
			EnableWindow (GetDlgItem (hwndDlg, IDC_NEXT), FALSE);
			SetFocus (GetDlgItem (hwndDlg, IDCANCEL));

			if (nCurPageNo == FORMAT_PAGE)
				KillTimer (hwndDlg, TIMER_ID_RANDVIEW);

			// Attempt to wipe the GUI fields showing portions of randpool, of the master and header keys
			memset (tmp, 'X', sizeof(tmp));
			tmp [sizeof(tmp)-1] = 0;
			SetWindowText (hRandPool, tmp);
			SetWindowText (hMasterKey, tmp);
			SetWindowText (hHeaderKey, tmp);
			
			Silent = true; 
			SetFileAttributes(szDiskFile, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
			mounted = MontarVolume(hwndDlg, szDriveLetter);
			if (mounted)
			{
                char szTmp[255];
				
				sprintf(szTmp, "Unidade de arquivos protegidos %c: disponível para acesso, através do Windows Explorer.", szDriveLetter[0]);
				MessageBox (hwndDlg, (LPCSTR) szTmp, (LPCSTR) "Central de proteção de arquivos", MB_ICONEXCLAMATION);
								  
                sprintf(szTmp,"ATENÇÃO: A unidade %c: de arquivos protegidos faz parte do disco rígido C:. Se o disco rígido deste computador for apagado ou formatado, todos os arquivos que estiverem armazenados na unidade de arquivos protegidos serão perdidos.", szDriveLetter[0]);
				MessageBox (hwndDlg, (LPCSTR) szTmp, (LPCSTR) "Central de proteção de arquivos", MB_ICONEXCLAMATION);

				sprintf(szTmp, "ATENÇÃO: A unidade de arquivos protegidos está aberta, e deverá ser fechada caso não esteja sendo utilizada. Isso garante a privacidade dos arquivos confidenciais.");
			    MessageBox (hwndDlg, (LPCSTR) szTmp, (LPCSTR) "Central de proteção de arquivos", MB_ICONEXCLAMATION);

				sprintf(szTmp, "%c:\\", szDriveLetter[0]);
			    SetVolumeLabelA(szTmp, "Unid_Segura");
				GravarConfiguracoes("MONTADO", "TRUE");
                GravarConfiguracoes("LIBERADO", "FALSE");
				GravarConfiguracoes("UNIDADE", szTmp);
                if (g_bExibirUnidade)
  			    {
			      sprintf(szTmp, "Explorer.exe %c:\\", szDriveLetter[0]);
			      WinExec(szTmp, SW_SHOWNORMAL);
			    }
			}
			else
				MessageBox (hwndDlg, (LPCSTR)"Erro ao acessar a unidade de arquivos protegidos. Verifique sua senha e tente novamente.", (LPCSTR)"Central de proteção de arquivos", ICON_HAND);
			    //Error("PASSWORD_WRONG");
            Silent =false;            

			//Encerrar aqui --> verificar se houve erro ...
			//PostMessage (hwndDlg, TC_APPMSG_FORMAT_USER_QUIT, 0, 0);
			LoadPage (hwndDlg, INTRO_PAGE);
			return 1;
			
		}
		return 1;

	case TC_APPMSG_NONSYS_INPLACE_ENC_FINISHED:

		// A partition has just been fully encrypted in place

		KillTimer (hwndDlg, TIMER_ID_NONSYS_INPLACE_ENC_PROGRESS);

		LoadPage (hwndDlg, NONSYS_INPLACE_ENC_ENCRYPTION_FINISHED_PAGE);

		return 1;

	case TC_APPMSG_VOL_TRANSFORM_THREAD_ENDED:

		if (bInPlaceEncNonSys)
		{
			// In-place encryption was interrupted/paused (did not finish)

			KillTimer (hwndDlg, TIMER_ID_NONSYS_INPLACE_ENC_PROGRESS);

			UpdateNonSysInPlaceEncControls ();
		}
		else
		{
			// Format has been aborted (did not finish)

			//EnableWindow (GetDlgItem (hCurPage, IDC_QUICKFORMAT), (bDevice || bSparseFileSwitch) && !(bHiddenVol && !bHiddenVolHost));
			//EnableWindow (GetDlgItem (hCurPage, IDC_FILESYS), TRUE);
			//EnableWindow (GetDlgItem (hCurPage, IDC_CLUSTERSIZE), TRUE);
			//EnableWindow (GetDlgItem (hwndDlg, IDC_PREV), TRUE);
			//EnableWindow (GetDlgItem (hwndDlg, IDHELP), TRUE);
			//EnableWindow (GetDlgItem (hwndDlg, IDCANCEL), TRUE);
			//EnableWindow (GetDlgItem (hCurPage, IDC_ABORT_BUTTON), FALSE);
			//EnableWindow (GetDlgItem (hwndDlg, IDC_NEXT), TRUE);
			//SendMessage (GetDlgItem (hCurPage, IDC_PROGRESS_BAR), PBM_SETPOS, 0, 0L);
			//SetFocus (GetDlgItem (hwndDlg, IDC_NEXT));
			LoadPage (hwndDlg, PASSWORD_PAGE);
			//LoadPage (hwndDlg, INTRO_PAGE);
		}

		NormalCursor ();
		return 1;

	case WM_HELP:

		OpenPageHelp (hwndDlg, nCurPageNo);
		return 1;

	case TC_APPMSG_FORMAT_USER_QUIT:

		if (nCurPageNo == NONSYS_INPLACE_ENC_ENCRYPTION_PAGE
			&& (bVolTransformThreadRunning || bVolTransformThreadToRun || bInPlaceEncNonSysResumed))
		{
			// Non-system encryption in progress
			if (AskNoYes ("NONSYS_INPLACE_ENC_DEFER_CONFIRM") == IDYES)
			{
				NonSysInplaceEncPause ();

				EndMainDlg (hwndDlg);
				return 1;
			}
			else
				return 1;	// Disallow close
		}
		else if (bVolTransformThreadRunning || bVolTransformThreadToRun)
		{
			// Format (non-in-place encryption) in progress
			
			if (MessageBoxW (MainDlg, L"Interromper a criação da nova unidade?", L"Central de proteção de arquivos", MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
			{
				bVolTransformThreadCancel = TRUE;

				EndMainDlg (hwndDlg);
				return 1;
			}
			else
				return 1;	// Disallow close
		}
		else if ((nCurPageNo == SYSENC_ENCRYPTION_PAGE || nCurPageNo == SYSENC_PRETEST_RESULT_PAGE)
			&& SystemEncryptionStatus != SYSENC_STATUS_NONE
			&& InstanceHasSysEncMutex ())
		{
			// System encryption/decryption in progress

			if (AskYesNo (SystemEncryptionStatus == SYSENC_STATUS_DECRYPTING ? 
				"SYSTEM_DECRYPTION_DEFER_CONFIRM" : "SYSTEM_ENCRYPTION_DEFER_CONFIRM") == IDYES)
			{
				if (nCurPageNo == SYSENC_PRETEST_RESULT_PAGE)
					TextInfoDialogBox (TC_TBXID_SYS_ENC_RESCUE_DISK);

				try
				{
					BootEncStatus = BootEncObj->GetStatus();

					if (BootEncStatus.SetupInProgress)
					{
						BootEncObj->AbortSetupWait ();
						Sleep (200);
						BootEncStatus = BootEncObj->GetStatus();
					}

					if (!BootEncStatus.SetupInProgress)
					{
						EndMainDlg (MainDlg);
						return 1;
					}
					else
					{
						Error ("FAILED_TO_INTERRUPT_SYSTEM_ENCRYPTION");
						return 1;	// Disallow close
					}
				}
				catch (Exception &e)
				{
					e.Show (hwndDlg);
				}
				return 1;	// Disallow close
			}
			else
				return 1;	// Disallow close
		}
		else if (bConfirmQuitSysEncPretest)
		{
			if (AskWarnNoYes (bHiddenOS ? "CONFIRM_CANCEL_HIDDEN_OS_CREATION" : "CONFIRM_CANCEL_SYS_ENC_PRETEST") == IDNO)
				return 1;	// Disallow close
		}
		else if (bConfirmQuit)
		{
			if (AskWarnNoYes ("CONFIRM_EXIT_UNIVERSAL") == IDNO)
				return 1;	// Disallow close
		}
        
        if (IsMountedVolume (szDiskFile))          
            if(MessageBoxW (MainDlg, L"ATENÇÃO: A unidade de arquivos protegidos está aberta, e deverá ser fechada caso não esteja sendo utilizada. Isso garante a privacidade dos arquivos confidenciais. \n Deseja encerrar o CPA?" , L"Central de proteção de arquivos", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2) == IDNO)
                return 1; // Disallow close
            
		
		if (hiddenVolHostDriveNo > -1)
		{
			CloseVolumeExplorerWindows (hwndDlg, hiddenVolHostDriveNo);
			UnmountVolume (hwndDlg, hiddenVolHostDriveNo, FALSE);
		}

		EndMainDlg (hwndDlg);
		return 1;

	case WM_COMMAND:

		//if (lw == IDC_AJUDA && hw == 0)//			IDHELP)
		//{
		//	InitHelpFileName();
		//	//OpenPageHelp (hwndDlg, nCurPageNo);
		//	int r = (int)ShellExecute (NULL, "open", szHelpFile, NULL, NULL, SW_SHOWNORMAL);
 	//        if (r == ERROR_FILE_NOT_FOUND)
	 //       {
		//		r = (int)ShellExecute (NULL, "open", szHelpFile2, NULL, NULL, SW_SHOWNORMAL);
  //              if (r == ERROR_FILE_NOT_FOUND)
		//		  MessageBox(hwndDlg, "Arquivo de ajuda não encontrado", "Central de proteção de arquivos", MB_OK | MB_ICONSTOP);
		//	}
		//	return 1;
		//}
		if (lw == IDCANCEL && hw == 0)
		{
			
			if (nCurPageNo == INTRO_PAGE)
			{
			  PostMessage (hwndDlg, TC_APPMSG_FORMAT_USER_QUIT, 0, 0);
			}
			else
			  LoadPage (hwndDlg, INTRO_PAGE);
			
			return 1;
		}
		else if ( (lw == IDC_NEXT) && (hw==0) ) //buttonUP e DOWN para picture no BN_CLICKED
		{	
			if (nCurPageNo == INTRO_PAGE)
			{
				nNewPageNo = PASSWORD_PAGE - 1;
				//lw = IDC_NEXT; //forçar para continuar
				
			}					
			else if (nCurPageNo == PASSWORD_PAGE)
			{
				
				nNewPageNo = FORMAT_PAGE - 1;
				MessageBox(hwndDlg, TC_ALERTA_SENHA, MB_OK | MB_ICONINFORMATION);

//int x = _access (szDiskFile, 06);
//if (x == 0 || errno != ENOENT)

                Randinit ();
				//EnableWindow (GetDlgItem (GetParent (hwndDlg), IDC_NEXT), TRUE);
			    //EnableWindow (GetDlgItem (GetParent (hwndDlg), IDC_PREV), FALSE);
			    //
			    //PASSO 1: intro_page: --> WIZARD_MODE_FILE_CONTAINER:
				//
			    WaitCursor ();
				CloseSysEncMutex ();
				ChangeWizardMode (WIZARD_MODE_FILE_CONTAINER);
				bHiddenOS = FALSE;
				bInPlaceEncNonSys = FALSE;		
				//nNewPageNo = VOLUME_TYPE_PAGE - 1;	// Skip irrelevant pages
				
				//
				//PASSO2 : VOLUME_TYPE_PAGE --> STANDARD Volume
				//
				bHiddenVol = FALSE;
				bHiddenVolHost = FALSE;
				bHiddenVolDirect = FALSE;					
				
				//nNewPageNo = VOLUME_LOCATION_PAGE - 1;		// Skip the hidden volume creation wizard mode selection

                //
				// passo 3: VOLUME_LOCATION_PAGE
				//

				if (!LerConfiguracoes())
				{
					Error ("CONFIG_FILE");
					NormalCursor ();
					return 1;
				}

				if (IsMountedVolume (szDiskFile))
				{
					Error ("ALREADY_MOUNTED");
					NormalCursor ();
					return 1;
				}

				bHistory = false;
				
				//
				// PASSO 4: CIPHER_PAGE
				//
				//LPARAM nIndex;
				//nIndex = SendMessage (GetDlgItem (hCurPage, IDC_COMBO_BOX), CB_GETCURSEL, 0, 0);
				//AES: nIndex = 0; --> Itemdata = Aes = 1;
				//nVolumeEA = SendMessage (GetDlgItem (hCurPage, IDC_COMBO_BOX), CB_GETITEMDATA, nIndex, 0);
                nVolumeEA = 1; //--> Default

				//nIndex = SendMessage (GetDlgItem (hCurPage, IDC_COMBO_BOX_HASH_ALGO), CB_GETCURSEL, 0, 0);
				//hash_algo = SendMessage (GetDlgItem (hCurPage, IDC_COMBO_BOX_HASH_ALGO), CB_GETITEMDATA, nIndex, 0);
				//Algo = RIPEMD160 --> nindex =0; ItemData = 0;
                hash_algo = DEFAULT_HASH_ALGORITHM; //1 --> ver enum
				RandSetHashFunction (hash_algo);

                //
				// PASSO 5: Volume size
				//               
				nVolumeSize = nUIVolumeSize * BYTES_PER_GB;  	        
				
				//
				// PASSO 6: PASSWORD
				//

				VerifyPasswordAndUpdate (hwndDlg, GetDlgItem (MainDlg, IDC_NEXT),
					GetDlgItem (hCurPage, IDC_PASSWORD),
					GetDlgItem (hCurPage, IDC_VERIFY),
					volumePassword.Text,
					szVerify,
					KeyFilesEnable && FirstKeyFile!=NULL && !SysEncInEffect());

				volumePassword.Length = strlen ((char *) volumePassword.Text);

				if (volumePassword.Length > 0)
				{
					// Password character encoding
					if (!CheckPasswordCharEncoding (GetDlgItem (hCurPage, IDC_PASSWORD), NULL))
					{
						Error ("UNSUPPORTED_CHARS_IN_PWD");
						return 1;
					}
					// Check password length (do not check if it's for an outer volume).
					else if (!bHiddenVolHost
						&& !CheckPasswordLength (hwndDlg, GetDlgItem (hCurPage, IDC_PASSWORD)))
					{
						return 1;
					}
				}

				// Store the password in case we need to restore it after keyfile is applied to it
				GetWindowText (GetDlgItem (hCurPage, IDC_PASSWORD), szRawPassword, sizeof (szRawPassword));

				if (!SysEncInEffect ()) 
				{
					if (KeyFilesEnable)
					{
						WaitCursor ();
						KeyFilesApply (&volumePassword, FirstKeyFile);
						NormalCursor ();
					}

				}
				else
				{
					KillTimer (hwndDlg, TIMER_ID_KEYB_LAYOUT_GUARD);

					if (bKeyboardLayoutChanged)
					{
						// Restore the original keyboard layout
						if (LoadKeyboardLayout (OrigKeyboardLayout, KLF_ACTIVATE | KLF_SUBSTITUTE_OK) == NULL) 
							Warning ("CANNOT_RESTORE_KEYBOARD_LAYOUT");
						else
							bKeyboardLayoutChanged = FALSE;
					}

					nNewPageNo = SYSENC_COLLECTING_RANDOM_DATA_PAGE - 1;	// Skip irrelevant pages
				}

				if (bInPlaceEncNonSys)
				{
					nNewPageNo = NONSYS_INPLACE_ENC_RAND_DATA_PAGE - 1;		// Skip irrelevant pages
				}
				else if (WizardMode != WIZARD_MODE_SYS_DEVICE
					&& !FileSize4GBLimitQuestionNeeded () 
					|| CreatingHiddenSysVol())		// If we're creating a hidden volume for a hidden OS, we don't need to format it with any filesystem (the entire OS will be copied to the hidden volume sector by sector).
				{
					nNewPageNo = FORMAT_PAGE - 1;				// Skip irrelevant pages
				}
			}

			else if ((nCurPageNo == FORMAT_PAGE) && (hw == 0) )
			{
				/* Format start  (the 'Next' button has been clicked on the Format page) */
                g_bExibirUnidade = IsButtonChecked(GetDlgItem (hCurPage, IDC_CHK_EXIBIRUNIDADE));
				
				EnableWindow (GetDlgItem (hCurPage, IDC_ABORT_BUTTON), TRUE);
                SendMessage(GetDlgItem (hCurPage, IDC_ABORT_BUTTON), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Desistir);				
				SetFocus (GetDlgItem (hCurPage, IDC_ABORT_BUTTON));
				
				EnableWindow (GetDlgItem (MainDlg, IDC_NEXT), false);
                SendMessage(GetDlgItem (MainDlg, IDC_NEXT), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Avancar_c);				
                //SendMessage(GetDlgItem (MainDlg, IDC_NEXT), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Concluir_c);				
				EnableWindow (GetDlgItem (MainDlg, IDCANCEL), false);
                SendMessage(GetDlgItem (MainDlg, IDCANCEL), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) Hbmp_Cancelar_c);				
				
                EnableWindow (GetDlgItem (hCurPage, IDC_CHK_EXIBIRUNIDADE), false);
				ShowWindow(GetDlgItem (hCurPage,IDC_STATIC_BAR), SW_SHOW);
				ShowWindow(GetDlgItem (hCurPage,IDT_HEADER_KEY), SW_SHOW);
				ShowWindow(GetDlgItem (hCurPage,IDC_PROGRESS_BAR), SW_SHOW);

				LastDialogId = "FORMAT_IN_PROGRESS";
				ArrowWaitCursor ();
				_beginthread (volTransformThreadFunction, 0, MainDlg);

				return 1;
			}

			else if (nCurPageNo == FORMAT_FINISHED_PAGE)
			{
				if (!bHiddenVol || bHiddenVolFinished)
				{
					/* Wizard loop restart */

					if (bHiddenOS)
					{
						if (!ChangeWizardMode (WIZARD_MODE_SYS_DEVICE))
							return 1;

						// Hidden volume for hidden OS has been created. Now we will prepare our boot loader
						// that will handle the OS cloning. 
						try
						{
							WaitCursor();

							BootEncObj->PrepareHiddenOSCreation (nVolumeEA, FIRST_MODE_OF_OPERATION_ID, hash_algo);
						}
						catch (Exception &e)
						{
							e.Show (MainDlg);
							NormalCursor();
							return 1;
						}

						bHiddenVol = FALSE;

						LoadPage (hwndDlg, SYSENC_PRETEST_INFO_PAGE);
					}
					//else
						//LoadPage (hwndDlg, INTRO_PAGE);

					SetWindowTextW (GetDlgItem (MainDlg, IDCANCEL), GetString ("CANCEL"));
					bHiddenVolFinished = FALSE;
					WipePasswordsAndKeyfiles ();

					return 1;
				}
				else
				{
					/* We're going to scan the bitmap of the hidden volume host (in the non-Direct hidden volume wizard mode) */
					int retCode;
					WaitCursor ();

					if (hiddenVolHostDriveNo != -1)		// If the hidden volume host is mounted
					{
						BOOL tmp_result;

						// Dismount the hidden volume host (in order to remount it as read-only subsequently)
						CloseVolumeExplorerWindows (hwndDlg, hiddenVolHostDriveNo);
						while (!(tmp_result = UnmountVolume (hwndDlg, hiddenVolHostDriveNo, FALSE)))
						{
							if (MessageBoxW (hwndDlg, GetString ("CANT_DISMOUNT_OUTER_VOL"), lpszTitle, MB_RETRYCANCEL | MB_ICONERROR | MB_SETFOREGROUND) != IDRETRY)
							{
								// Cancel
								NormalCursor();
								return 1;
							}
						}
						if (tmp_result)		// If dismounted
							hiddenVolHostDriveNo = -1;
					}

					if (hiddenVolHostDriveNo < 0)		// If the hidden volume host is not mounted
					{
						// Remount the hidden volume host as read-only (to ensure consistent and secure
						// results of the volume bitmap scanning)
						switch (MountHiddenVolHost (hwndDlg, szDiskFile, &hiddenVolHostDriveNo, &volumePassword, TRUE))
						{
						case ERR_NO_FREE_DRIVES:
							MessageBoxW (hwndDlg, GetString ("NO_FREE_DRIVE_FOR_OUTER_VOL"), lpszTitle, ICON_HAND);
							NormalCursor ();
							return 1;

						case ERR_VOL_MOUNT_FAILED:
						case ERR_PASSWORD_WRONG:
							NormalCursor ();
							return 1;

						case 0:

							/* Hidden volume host successfully mounted as read-only */

							// Verify that the outer volume contains a suitable file system, retrieve cluster size, and 
							// scan the volume bitmap
							if (!IsAdmin () && IsUacSupported ())
								retCode = UacAnalyzeHiddenVolumeHost (hwndDlg, &hiddenVolHostDriveNo, GetVolumeDataAreaSize (FALSE, nHiddenVolHostSize), &realClusterSize, &nbrFreeClusters);
							else
								retCode = AnalyzeHiddenVolumeHost (hwndDlg, &hiddenVolHostDriveNo, GetVolumeDataAreaSize (FALSE, nHiddenVolHostSize), &realClusterSize, &nbrFreeClusters);

							switch (retCode)
							{
							case -1:	// Fatal error
								CloseVolumeExplorerWindows (hwndDlg, hiddenVolHostDriveNo);

								if (UnmountVolume (hwndDlg, hiddenVolHostDriveNo, FALSE))
									hiddenVolHostDriveNo = -1;

								AbortProcessSilent ();
								break;

							case 0:		// Unsupported file system (or other non-fatal error which has already been reported)
								NormalCursor ();
								return 1;

							case 1:		// Success
								{
									BOOL tmp_result;

									// Determine the maximum possible size of the hidden volume
									if (DetermineMaxHiddenVolSize (hwndDlg) < 1)
									{
										NormalCursor ();
										goto ovf_end;
									}

									/* Maximum possible size of the hidden volume successfully determined */

									// Dismount the hidden volume host
									while (!(tmp_result = UnmountVolume (hwndDlg, hiddenVolHostDriveNo, FALSE)))
									{
										if (MessageBoxW (hwndDlg, GetString ("CANT_DISMOUNT_OUTER_VOL"), lpszTitle, MB_RETRYCANCEL) != IDRETRY)
										{
											// Cancel
											NormalCursor ();
											goto ovf_end;
										}
									}

									// Prevent having to recreate the outer volume due to inadvertent exit
									bConfirmQuit = TRUE;

									hiddenVolHostDriveNo = -1;

									nNewPageNo = HIDDEN_VOL_HOST_PRE_CIPHER_PAGE;

									// Clear the outer volume password
									WipePasswordsAndKeyfiles ();

									EnableWindow (GetDlgItem (MainDlg, IDC_NEXT), TRUE);
									NormalCursor ();

								}
								break;
							}
							break;
						}
					}
				}
			}

			else if (nCurPageNo == DEVICE_WIPE_PAGE)
			{
				if (AskWarnOkCancel (bHiddenOS && IsHiddenOSRunning() ? "CONFIRM_WIPE_START_DECOY_SYS_PARTITION" : "CONFIRM_WIPE_START") == IDOK)
				{
					WipeStart ();
					ArrowWaitCursor();
				}
				return 1;
			}

			LoadPage (hwndDlg, nNewPageNo + 1);
ovf_end:
			lw = 0;
			return 1;
		}
		else if ((lw == IDC_PREV) && (hw==0) )
		{
			
			if (nCurPageNo == CIPHER_PAGE)
			{
				LPARAM nIndex;
				nIndex = SendMessage (GetDlgItem (hCurPage, IDC_COMBO_BOX), CB_GETCURSEL, 0, 0);
				nVolumeEA = SendMessage (GetDlgItem (hCurPage, IDC_COMBO_BOX), CB_GETITEMDATA, nIndex, 0);

				nIndex = SendMessage (GetDlgItem (hCurPage, IDC_COMBO_BOX_HASH_ALGO), CB_GETCURSEL, 0, 0);
				hash_algo = SendMessage (GetDlgItem (hCurPage, IDC_COMBO_BOX_HASH_ALGO), CB_GETITEMDATA, nIndex, 0);

				RandSetHashFunction (hash_algo);

				if (WizardMode == WIZARD_MODE_SYS_DEVICE)
				{
					if (nMultiBoot > 1)
						nNewPageNo = SYSENC_MULTI_BOOT_OUTCOME_PAGE + 1;	// Skip irrelevant pages
					else
						nNewPageNo = SYSENC_MULTI_BOOT_MODE_PAGE + 1;		// Skip irrelevant pages
				}
				else if (!bHiddenVol)
					nNewPageNo = (bDevice ? DEVICE_TRANSFORM_MODE_PAGE : VOLUME_LOCATION_PAGE) + 1;	
				else if (bHiddenVolHost)
					nNewPageNo = HIDDEN_VOL_HOST_PRE_CIPHER_PAGE + 1;		// Skip the info on the hidden volume
			}

			else if (nCurPageNo == PASSWORD_PAGE)
			{
				// Store the password in case we need to restore it after keyfile is applied to it
				GetWindowText (GetDlgItem (hCurPage, IDC_PASSWORD), szRawPassword, sizeof (szRawPassword));

				VerifyPasswordAndUpdate (hwndDlg, GetDlgItem (MainDlg, IDC_NEXT),
					GetDlgItem (hCurPage, IDC_PASSWORD),
					GetDlgItem (hCurPage, IDC_VERIFY),
					volumePassword.Text,
					szVerify,
					KeyFilesEnable && FirstKeyFile!=NULL && !SysEncInEffect ());

				volumePassword.Length = strlen ((char *) volumePassword.Text);

				nNewPageNo = SIZE_PAGE + 1;		// Skip the hidden volume host password page

				if (SysEncInEffect ())
				{
					nNewPageNo = CIPHER_PAGE + 1;				// Skip irrelevant pages

					KillTimer (hwndDlg, TIMER_ID_KEYB_LAYOUT_GUARD);

					if (bKeyboardLayoutChanged)
					{
						// Restore the original keyboard layout
						if (LoadKeyboardLayout (OrigKeyboardLayout, KLF_ACTIVATE | KLF_SUBSTITUTE_OK) == NULL) 
							Warning ("CANNOT_RESTORE_KEYBOARD_LAYOUT");
						else
							bKeyboardLayoutChanged = FALSE;
					}
				}
				else if (bInPlaceEncNonSys)
					nNewPageNo = CIPHER_PAGE + 1;
			}

			else if (nCurPageNo == FORMAT_PAGE)
			{
				char tmp[RNG_POOL_SIZE*2+1];

				KillTimer (hwndDlg, TIMER_ID_RANDVIEW);

				// Attempt to wipe the GUI fields showing portions of randpool, of the master and header keys
				memset (tmp, 'X', sizeof(tmp));
				tmp [sizeof(tmp)-1] = 0;
				SetWindowText (hRandPool, tmp);
				SetWindowText (hMasterKey, tmp);
				SetWindowText (hHeaderKey, tmp);
                
                
                
				if (WizardMode != WIZARD_MODE_SYS_DEVICE)
				{
					// Skip irrelevant pages

					if (FileSize4GBLimitQuestionNeeded ()
						&& !CreatingHiddenSysVol()		// If we're creating a hidden volume for a hidden OS, we don't need to format it with any filesystem (the entire OS will be copied to the hidden volume sector by sector).
						&& !bInPlaceEncNonSys)
					{
						nNewPageNo = FILESYS_PAGE + 1;
					}
					else
						nNewPageNo = PASSWORD_PAGE + 1;		
				}
			}

			LoadPage (hwndDlg, nNewPageNo - 1);

			return 1;
		}
		return 0;

	case WM_ENDSESSION:
		EndMainDlg (MainDlg);
		localcleanup ();
		return 0;

	case WM_CLOSE:
		PostMessage (hwndDlg, TC_APPMSG_FORMAT_USER_QUIT, 0, 0);
		return 1;
	}

	return 0;
}

/// mantido original
void ExtractCommandLine (HWND hwndDlg, char *lpszCommandLine)
{
	char **lpszCommandLineArgs;	/* Array of command line arguments */
	int nNoCommandLineArgs;	/* The number of arguments in the array */

	if (_stricmp (lpszCommandLine, "-Embedding") == 0)
	{
		ComServerMode = TRUE;
		return;
	}

	/* Extract command line arguments */
	nNoCommandLineArgs = Win32CommandLine (lpszCommandLine, &lpszCommandLineArgs);
	if (nNoCommandLineArgs > 0)
	{
		int i;

		for (i = 0; i < nNoCommandLineArgs; i++)
		{
			enum
			{
				OptionHistory,
				OptionNoIsoCheck,
				OptionQuit,
				OptionTokenLib,
				CommandResumeSysEncLogOn,
				CommandResumeSysEnc,
				CommandDecryptSysEnc,
				CommandEncDev,
				CommandHiddenSys,
				CommandResumeInplaceLogOn,
				CommandResumeHiddenSys,
				CommandSysEnc,
				CommandResumeInplace,
			};

			argument args[]=
			{
				{ OptionHistory,				"/history",			"/h", FALSE },
				{ OptionNoIsoCheck,				"/noisocheck",		"/n", FALSE },
				{ OptionQuit,					"/quit",			"/q", FALSE },
				{ OptionTokenLib,				"/tokenlib",		NULL, FALSE },

				{ CommandResumeSysEncLogOn,		"/acsysenc",		"/a", TRUE },
				{ CommandResumeSysEnc,			"/csysenc",			"/c", TRUE },
				{ CommandDecryptSysEnc,			"/dsysenc",			"/d", TRUE },
				{ CommandEncDev,				"/encdev",			"/e", TRUE },
				{ CommandHiddenSys,				"/isysenc",			"/i", TRUE },	
				{ CommandResumeInplaceLogOn,	"/prinplace",		"/p", TRUE },
				{ CommandResumeHiddenSys,		"/risysenc",		"/r", TRUE },	
				{ CommandSysEnc,				"/sysenc",			"/s", TRUE },	
				{ CommandResumeInplace,			"/zinplace",		"/z", TRUE }
			};

			argumentspec as;

			int nArgPos;
			int x;

			if (lpszCommandLineArgs[i] == NULL)
				continue;

			as.args = args;
			as.arg_cnt = sizeof(args)/ sizeof(args[0]);
			
			x = GetArgumentID (&as, lpszCommandLineArgs[i], &nArgPos);

			switch (x)
			{
			case CommandSysEnc:
				// Encrypt system partition/drive (passed by Mount if system encryption hasn't started or to reverse decryption)

				// From now on, we should be the only instance of the TC wizard allowed to deal with system encryption
				if (CreateSysEncMutex ())
				{
					bDirectSysEncMode = TRUE;
					bDirectSysEncModeCommand = SYSENC_COMMAND_ENCRYPT;
					ChangeWizardMode (WIZARD_MODE_SYS_DEVICE);
				}
				else
				{
					Warning ("SYSTEM_ENCRYPTION_IN_PROGRESS_ELSEWHERE");
					exit(0);
				}

				break;

			case CommandDecryptSysEnc:
				// Decrypt system partition/drive (passed by Mount, also to reverse encryption in progress, when paused)

				// From now on, we should be the only instance of the TC wizard allowed to deal with system encryption
				if (CreateSysEncMutex ())
				{
					bDirectSysEncMode = TRUE;
					bDirectSysEncModeCommand = SYSENC_COMMAND_DECRYPT;
					ChangeWizardMode (WIZARD_MODE_SYS_DEVICE);
				}
				else
				{
					Warning ("SYSTEM_ENCRYPTION_IN_PROGRESS_ELSEWHERE");
					exit(0);
				}
				break;

			case CommandHiddenSys:
				// Create a hidden operating system (passed by Mount when the user selects System -> Create Hidden Operating System)

				// From now on, we should be the only instance of the TC wizard allowed to deal with system encryption
				if (CreateSysEncMutex ())
				{
					bDirectSysEncMode = TRUE;
					bDirectSysEncModeCommand = SYSENC_COMMAND_CREATE_HIDDEN_OS;
					ChangeWizardMode (WIZARD_MODE_SYS_DEVICE);
				}
				else
				{
					Warning ("SYSTEM_ENCRYPTION_IN_PROGRESS_ELSEWHERE");
					exit(0);
				}

				break;

			case CommandResumeHiddenSys:
				// Resume process of creation of a hidden operating system (passed by Wizard when the user needs to UAC-elevate the whole wizard process)

				// From now on, we should be the only instance of the TC wizard allowed to deal with system encryption
				if (CreateSysEncMutex ())
				{
					bDirectSysEncMode = TRUE;
					bDirectSysEncModeCommand = SYSENC_COMMAND_CREATE_HIDDEN_OS_ELEV;
					ChangeWizardMode (WIZARD_MODE_SYS_DEVICE);
				}
				else
				{
					Warning ("SYSTEM_ENCRYPTION_IN_PROGRESS_ELSEWHERE");
					exit(0);
				}

				break;

			case CommandResumeSysEnc:
				// Resume previous system-encryption operation (passed by Mount) e.g. encryption, decryption, or pretest 

				// From now on, we should be the only instance of the TC wizard allowed to deal with system encryption
				if (CreateSysEncMutex ())
				{
					bDirectSysEncMode = TRUE;
					bDirectSysEncModeCommand = SYSENC_COMMAND_RESUME;
					ChangeWizardMode (WIZARD_MODE_SYS_DEVICE);
				}
				else
				{
					Warning ("SYSTEM_ENCRYPTION_IN_PROGRESS_ELSEWHERE");
					exit(0);
				}
				break;

			case CommandResumeSysEncLogOn:
				// Same as csysenc but passed only by the system (from the startup sequence)

				// From now on, we should be the only instance of the TC wizard allowed to deal with system encryption
				if (CreateSysEncMutex ())
				{
					bDirectSysEncMode = TRUE;
					bDirectSysEncModeCommand = SYSENC_COMMAND_STARTUP_SEQ_RESUME;
					ChangeWizardMode (WIZARD_MODE_SYS_DEVICE);
				}
				else
				{
					Warning ("SYSTEM_ENCRYPTION_IN_PROGRESS_ELSEWHERE");
					exit(0);
				}
				break;

			case CommandEncDev:
				// Resume process of creation of a non-sys-device-hosted volume (passed by Wizard when the user needs to UAC-elevate)
				DirectDeviceEncMode = TRUE;
				break;

			case CommandResumeInplace:
				// Resume interrupted process of non-system in-place encryption of a partition
				DirectNonSysInplaceEncResumeMode = TRUE;
				break;

			case CommandResumeInplaceLogOn:
				// Ask the user whether to resume interrupted process of non-system in-place encryption of a partition
				// This switch is passed only by the system (from the startup sequence).
				DirectPromptNonSysInplaceEncResumeMode = TRUE;
				break;

			case OptionNoIsoCheck:
				bDontVerifyRescueDisk = TRUE;
				break;

			case OptionHistory:
				{
					char szTmp[8];
					GetArgumentValue (lpszCommandLineArgs, nArgPos, &i, nNoCommandLineArgs,
						     szTmp, sizeof (szTmp));
					if (!_stricmp(szTmp,"y") || !_stricmp(szTmp,"yes"))
					{
						bHistory = TRUE;
						bHistoryCmdLine = TRUE;
					}

					if (!_stricmp(szTmp,"n") || !_stricmp(szTmp,"no"))
					{
						bHistory = FALSE;
						bHistoryCmdLine = TRUE;
					}
				}
				break;
				
			case OptionTokenLib:
				if (GetArgumentValue (lpszCommandLineArgs, nArgPos, &i, nNoCommandLineArgs, SecurityTokenLibraryPath, sizeof (SecurityTokenLibraryPath)) == HAS_ARGUMENT)
					InitSecurityTokenLibrary();
				else
					Error ("COMMAND_LINE_ERROR");

				break;

			case OptionQuit:
				{
					// Used to indicate non-install elevation
					char szTmp[32];
					GetArgumentValue (lpszCommandLineArgs, nArgPos, &i, nNoCommandLineArgs, szTmp, sizeof (szTmp));
				}
				break;

			default:
				DialogBoxParamW (hInst, MAKEINTRESOURCEW (IDD_COMMANDHELP_DLG), hwndDlg, (DLGPROC)
						CommandHelpDlgProc, (LPARAM) &as);

				exit(0);
			}
		}
	}

	/* Free up the command line arguments */
	while (--nNoCommandLineArgs >= 0)
	{
		free (lpszCommandLineArgs[nNoCommandLineArgs]);
	}
}


/// mantido original
int DetermineMaxHiddenVolSize (HWND hwndDlg)
{
	__int64 nbrReserveBytes;

	if (nbrFreeClusters * realClusterSize < TC_MIN_HIDDEN_VOLUME_SIZE)
	{
		MessageBoxW (hwndDlg, GetString ("NO_SPACE_FOR_HIDDEN_VOL"), lpszTitle, ICON_HAND);
		UnmountVolume (hwndDlg, hiddenVolHostDriveNo, FALSE);
		AbortProcessSilent ();
	}

	// Add a reserve (in case the user mounts the outer volume and creates new files
	// on it by accident or OS writes some new data behind his or her back, such as
	// System Restore etc.)
	nbrReserveBytes = GetVolumeDataAreaSize (FALSE, nHiddenVolHostSize) / 200;
	if (nbrReserveBytes > BYTES_PER_MB * 10)
		nbrReserveBytes = BYTES_PER_MB * 10;

	// Compute the final value

	nMaximumHiddenVolSize = nbrFreeClusters * realClusterSize - TC_HIDDEN_VOLUME_HOST_FS_RESERVED_END_AREA_SIZE - nbrReserveBytes;
	nMaximumHiddenVolSize -= nMaximumHiddenVolSize % SECTOR_SIZE;		// Must be a multiple of the sector size

	if (nMaximumHiddenVolSize < TC_MIN_HIDDEN_VOLUME_SIZE)
	{
		MessageBoxW (hwndDlg, GetString ("NO_SPACE_FOR_HIDDEN_VOL"), lpszTitle, ICON_HAND);
		UnmountVolume (hwndDlg, hiddenVolHostDriveNo, FALSE);
		AbortProcessSilent ();
	}

	// Prepare the hidden volume size parameters
	if (nMaximumHiddenVolSize < BYTES_PER_MB)
		nMultiplier = BYTES_PER_KB;
	else if (nMaximumHiddenVolSize < BYTES_PER_GB)
		nMultiplier = BYTES_PER_MB;
	else
		nMultiplier = BYTES_PER_GB;

	nUIVolumeSize = 0;								// Set the initial value for the hidden volume size input field to the max
	nVolumeSize = nUIVolumeSize * nMultiplier;		// Chop off possible remainder

	return 1;
}


// Tests whether the file system of the given volume is suitable to host a hidden volume,
// retrieves the cluster size, and scans the volume cluster bitmap. In addition, checks
// the TrueCrypt volume format version and the type of volume.
/// mantido original
int AnalyzeHiddenVolumeHost (HWND hwndDlg, int *driveNo, __int64 hiddenVolHostSize, int *realClusterSize, __int64 *pnbrFreeClusters)
{
	HANDLE hDevice;
	DWORD bytesReturned;
	DWORD dwSectorsPerCluster, dwBytesPerSector, dwNumberOfFreeClusters, dwTotalNumberOfClusters;
	DWORD dwResult;
	int result;
	char szFileSystemNameBuffer[256];
	char tmpPath[7] = {'\\','\\','.','\\',*driveNo + 'A',':',0};
	char szRootPathName[4] = {*driveNo + 'A', ':', '\\', 0};
	BYTE readBuffer[SECTOR_SIZE*2];
	LARGE_INTEGER offset, offsetNew;
	VOLUME_PROPERTIES_STRUCT volProp;

	memset (&volProp, 0, sizeof(volProp));
	volProp.driveNo = *driveNo;
	if (!DeviceIoControl (hDriver, TC_IOCTL_GET_VOLUME_PROPERTIES, &volProp, sizeof (volProp), &volProp, sizeof (volProp), &dwResult, NULL) || dwResult == 0)
	{
		handleWin32Error (hwndDlg);
		Error ("CANT_ACCESS_OUTER_VOL");
		goto efsf_error;
	}

	if (volProp.volFormatVersion < TC_VOLUME_FORMAT_VERSION)
	{
		// We do not support creating hidden volumes within volumes created by TrueCrypt 5.1a or earlier.
		Error ("ERR_VOL_FORMAT_BAD");
		return 0;
	}

	if (volProp.hiddenVolume)
	{
		// The user entered a password for a hidden volume
		Error ("ERR_HIDDEN_NOT_NORMAL_VOLUME");
		return 0;
	}

	if (volProp.volumeHeaderFlags & TC_HEADER_FLAG_NONSYS_INPLACE_ENC
		|| volProp.volumeHeaderFlags & TC_HEADER_FLAG_ENCRYPTED_SYSTEM)
	{
		Warning ("ERR_HIDDEN_VOL_HOST_ENCRYPTED_INPLACE");
		return 0;
	}

	hDevice = CreateFile (tmpPath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		MessageBoxW (hwndDlg, GetString ("CANT_ACCESS_OUTER_VOL"), lpszTitle, ICON_HAND);
		goto efsf_error;
	}

	offset.QuadPart = 0;

	if (SetFilePointerEx (hDevice, offset, &offsetNew, FILE_BEGIN) == 0)
	{
		handleWin32Error (hwndDlg);
		goto efs_error;
	}

	result = ReadFile(hDevice, &readBuffer, (DWORD) SECTOR_SIZE, &bytesReturned, NULL);

	if (result == 0)
	{
		handleWin32Error (hwndDlg);
		MessageBoxW (hwndDlg, GetString ("CANT_ACCESS_OUTER_VOL"), lpszTitle, ICON_HAND);
		goto efs_error;
	}

	CloseHandle (hDevice);
	hDevice = INVALID_HANDLE_VALUE;

	// Determine file system type

	GetVolumeInformation(szRootPathName, NULL, 0, NULL, NULL, NULL, szFileSystemNameBuffer, sizeof(szFileSystemNameBuffer));

	// The Windows API sometimes fails to indentify the file system correctly so we're using "raw" analysis too.
	if (!strncmp (szFileSystemNameBuffer, "FAT", 3)
		|| (readBuffer[0x36] == 'F' && readBuffer[0x37] == 'A' && readBuffer[0x38] == 'T')
		|| (readBuffer[0x52] == 'F' && readBuffer[0x53] == 'A' && readBuffer[0x54] == 'T'))
	{
		// FAT12/FAT16/FAT32

		// Retrieve the cluster size
		*realClusterSize = ((int) readBuffer[0xb] + ((int) readBuffer[0xc] << 8)) * (int) readBuffer[0xd];	

		// Get the map of the clusters that are free and in use on the outer volume.
		// The map will be scanned to determine the size of the uninterrupted block of free
		// space (provided there is any) whose end is aligned with the end of the volume.
		// The value will then be used to determine the maximum possible size of the hidden volume.

		return ScanVolClusterBitmap (hwndDlg,
			driveNo,
			hiddenVolHostSize / *realClusterSize,
			pnbrFreeClusters);
	}
	else if (!strncmp (szFileSystemNameBuffer, "NTFS", 4))
	{
		// NTFS

		if (nCurrentOS == WIN_2000)
		{
			Error("HIDDEN_VOL_HOST_UNSUPPORTED_FILESYS_WIN2000");
			return 0;
		}

		if (bHiddenVolDirect && GetVolumeDataAreaSize (FALSE, hiddenVolHostSize) <= TC_MAX_FAT_FS_SIZE)
			Info ("HIDDEN_VOL_HOST_NTFS");

		if (!GetDiskFreeSpace(szRootPathName, 
			&dwSectorsPerCluster, 
			&dwBytesPerSector, 
			&dwNumberOfFreeClusters, 
			&dwTotalNumberOfClusters))
		{
			handleWin32Error (hwndDlg);
			Error ("CANT_GET_OUTER_VOL_INFO");
			return -1;
		};

		*realClusterSize = dwBytesPerSector * dwSectorsPerCluster;

		// Get the map of the clusters that are free and in use on the outer volume.
		// The map will be scanned to determine the size of the uninterrupted block of free
		// space (provided there is any) whose end is aligned with the end of the volume.
		// The value will then be used to determine the maximum possible size of the hidden volume.

		return ScanVolClusterBitmap (hwndDlg,
			driveNo,
			hiddenVolHostSize / *realClusterSize,
			pnbrFreeClusters);
	}
	else
	{
		// Unsupported file system

		Error ((nCurrentOS == WIN_2000) ? "HIDDEN_VOL_HOST_UNSUPPORTED_FILESYS_WIN2000" : "HIDDEN_VOL_HOST_UNSUPPORTED_FILESYS");
		return 0;
	}

efs_error:
	CloseHandle (hDevice);

efsf_error:
	CloseVolumeExplorerWindows (hwndDlg, *driveNo);

	return -1;
}


// Mounts a volume within which the user intends to create a hidden volume
/// mantido original
int MountHiddenVolHost (HWND hwndDlg, char *volumePath, int *driveNo, Password *password, BOOL bReadOnly)
{
	MountOptions mountOptions;
	ZeroMemory (&mountOptions, sizeof (mountOptions));

	*driveNo = GetLastAvailableDrive ();

	if (*driveNo == -1)
	{
		*driveNo = -2;
		return ERR_NO_FREE_DRIVES;
	}

	mountOptions.ReadOnly = bReadOnly;
	mountOptions.Removable = ConfigReadInt ("MountVolumesRemovable", FALSE);
	mountOptions.ProtectHiddenVolume = FALSE;
	mountOptions.PreserveTimestamp = bPreserveTimestamp;
	mountOptions.PartitionInInactiveSysEncScope = FALSE;
	mountOptions.UseBackupHeader = FALSE;

	if (MountVolume (hwndDlg, *driveNo, volumePath, password, FALSE, TRUE, &mountOptions, FALSE, TRUE) < 1)
	{
		*driveNo = -3;
		return ERR_VOL_MOUNT_FAILED;
	}
	return 0;
}


/* Gets the map of the clusters that are free and in use on a volume that is to host
   a hidden volume. The map is scanned to determine the size of the uninterrupted
   area of free space (provided there is any) whose end is aligned with the end
   of the volume. The value will then be used to determine the maximum possible size
   of the hidden volume. */
//mantido original
int ScanVolClusterBitmap (HWND hwndDlg, int *driveNo, __int64 nbrClusters, __int64 *nbrFreeClusters)
{
	PVOLUME_BITMAP_BUFFER lpOutBuffer;
	STARTING_LCN_INPUT_BUFFER lpInBuffer;

	HANDLE hDevice;
	DWORD lBytesReturned;
	BYTE rmnd;
	char tmpPath[7] = {'\\','\\','.','\\', *driveNo + 'A', ':', 0};

	DWORD bufLen;
	__int64 bitmapCnt;

	hDevice = CreateFile (tmpPath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		MessageBoxW (hwndDlg, GetString ("CANT_ACCESS_OUTER_VOL"), lpszTitle, ICON_HAND);
		goto vcmf_error;
	}

 	bufLen = (DWORD) (nbrClusters / 8 + 2 * sizeof(LARGE_INTEGER));
	bufLen += 100000 + bufLen/10;	// Add reserve

	lpOutBuffer = (PVOLUME_BITMAP_BUFFER) malloc (bufLen);

	if (lpOutBuffer == NULL)
	{
		MessageBoxW (hwndDlg, GetString ("ERR_MEM_ALLOC"), lpszTitle, ICON_HAND);
		goto vcmf_error;
	}

	lpInBuffer.StartingLcn.QuadPart = 0;

	if ( !DeviceIoControl (hDevice,
		FSCTL_GET_VOLUME_BITMAP,
		&lpInBuffer,
		sizeof(lpInBuffer),
		lpOutBuffer,
		bufLen,  
		&lBytesReturned,
		NULL))
	{
		handleWin32Error (hwndDlg);
		MessageBoxW (hwndDlg, GetString ("CANT_GET_CLUSTER_BITMAP"), lpszTitle, ICON_HAND);

		goto vcm_error;
	}

	rmnd = (BYTE) (lpOutBuffer->BitmapSize.QuadPart % 8);

	if ((rmnd != 0) 
	&& ((lpOutBuffer->Buffer[lpOutBuffer->BitmapSize.QuadPart / 8] & ((1 << rmnd)-1) ) != 0))
	{
		*nbrFreeClusters = 0;
	}
	else
	{
		*nbrFreeClusters = lpOutBuffer->BitmapSize.QuadPart;
		bitmapCnt = lpOutBuffer->BitmapSize.QuadPart / 8;

		// Scan the bitmap from the end
		while (--bitmapCnt >= 0)
		{
			if (lpOutBuffer->Buffer[bitmapCnt] != 0)
			{
				// There might be up to 7 extra free clusters in this byte of the bitmap. 
				// These are ignored because there is always a cluster reserve added anyway.
				*nbrFreeClusters = lpOutBuffer->BitmapSize.QuadPart - ((bitmapCnt + 1) * 8);	
				break;
			}
		}
	}

	CloseHandle (hDevice);
	free(lpOutBuffer);
	return 1;

vcm_error:
	CloseHandle (hDevice);
	free(lpOutBuffer);

vcmf_error:
	return -1;
}


// Wipe the hidden OS config flag bits in the MBR
/// mantido original
static BOOL WipeHiddenOSCreationConfig (void)
{
	if (!IsHiddenOSRunning())
	{
		try
		{
			WaitCursor();
			finally_do ({ NormalCursor(); });

			BootEncObj->WipeHiddenOSCreationConfig();
		}
		catch (Exception &e)
		{
			e.Show (MainDlg);
			return FALSE;
		}
	}

	return TRUE;
}


// Tasks that need to be performed after the WM_INITDIALOG message for the SYSENC_ENCRYPTION_PAGE dialog is
// handled should be done here (otherwise the UAC prompt causes the GUI to be only half-rendered). 
/// mantido original
static void AfterSysEncProgressWMInitTasks (HWND hwndDlg)
{
	try
	{
		switch (SystemEncryptionStatus)
		{
		case SYSENC_STATUS_ENCRYPTING:

			if (BootEncStatus.ConfiguredEncryptedAreaStart == BootEncStatus.EncryptedAreaStart
				&& BootEncStatus.ConfiguredEncryptedAreaEnd == BootEncStatus.EncryptedAreaEnd)
			{
				// The partition/drive had been fully encrypted

				ManageStartupSeqWiz (TRUE, "");
				WipeHiddenOSCreationConfig();	// For extra conservative security
				ChangeSystemEncryptionStatus (SYSENC_STATUS_NONE);

				Info ("SYSTEM_ENCRYPTION_FINISHED");
				EndMainDlg (MainDlg);
				return;
			}
			else
			{
				SysEncResume ();
			}

			break;

		case SYSENC_STATUS_DECRYPTING:
			SysEncResume ();
			break;

		default:

			// Unexpected mode here -- fix the inconsistency

			ManageStartupSeqWiz (TRUE, "");
			ChangeSystemEncryptionStatus (SYSENC_STATUS_NONE);
			EndMainDlg (MainDlg);
			InconsistencyResolved (SRC_POS);
			return;
		}
	}
	catch (Exception &e)
	{
		e.Show (hwndDlg);
		EndMainDlg (MainDlg);
		return;
	}

	InitSysEncProgressBar ();

	UpdateSysEncProgressBar ();

	UpdateSysEncControls ();
}


// Tasks that need to be performed after the WM_INITDIALOG message is handled must be done here. 
// For example, any tasks that may invoke the UAC prompt (otherwise the UAC dialog box would not be on top).
/// mantido original
static void AfterWMInitTasks (HWND hwndDlg)
{
	// Note that if bDirectSysEncModeCommand is not SYSENC_COMMAND_NONE, we already have the mutex.
	
	// SYSENC_COMMAND_DECRYPT has the highest priority because it also performs uninstallation (restores the
	// original contents of the first drive cylinder, etc.) so it must be attempted regardless of the phase
	// or content of configuration files.
	if (bDirectSysEncModeCommand == SYSENC_COMMAND_DECRYPT)
	{
		if (IsHiddenOSRunning())
		{
			Warning ("CANNOT_DECRYPT_HIDDEN_OS");
			AbortProcessSilent();
		}

		// Add the wizard to the system startup sequence
		ManageStartupSeqWiz (FALSE, "/acsysenc");

		ChangeSystemEncryptionStatus (SYSENC_STATUS_DECRYPTING);
		LoadPage (hwndDlg, SYSENC_ENCRYPTION_PAGE);
		return;
	}


	if (SystemEncryptionStatus == SYSENC_STATUS_ENCRYPTING
		|| SystemEncryptionStatus == SYSENC_STATUS_DECRYPTING)
	{
		try
		{
			BootEncStatus = BootEncObj->GetStatus();

			if (!BootEncStatus.DriveMounted)
			{
				if (!BootEncStatus.DeviceFilterActive)
				{
					// This is an inconsistent state. SystemEncryptionStatus should never be SYSENC_STATUS_ENCRYPTING
					// or SYSENC_STATUS_DECRYPTING when the drive filter is not active. Possible causes: 1) corrupted
					// or stale config file, 2) corrupted system

					// Fix the inconsistency
					ManageStartupSeqWiz (TRUE, "");
					ChangeSystemEncryptionStatus (SYSENC_STATUS_NONE);
					EndMainDlg (MainDlg);
					InconsistencyResolved (SRC_POS);
					return;
				}
				else if (bDirectSysEncMode)
				{
					// This is an inconsistent state. We have a direct system encryption command, 
					// SystemEncryptionStatus is SYSENC_STATUS_ENCRYPTING or SYSENC_STATUS_DECRYPTING, the
					// system drive is not 'mounted' and drive filter is active.  Possible causes: 1) The drive had
					// been decrypted in the pre-boot environment. 2) The OS is not located on the lowest partition,
					// the drive is to be fully encrypted, but the user rebooted before encryption reached the 
					// system partition and then pressed Esc in the boot loader screen. 3) Corrupted or stale config
					// file. 4) Damaged system.
					
					Warning ("SYSTEM_ENCRYPTION_SCHEDULED_BUT_PBA_FAILED");
					EndMainDlg (MainDlg);
					return;
				}
			}
		}
		catch (Exception &e)
		{
			e.Show (MainDlg);
		}
	}


	if (SystemEncryptionStatus != SYSENC_STATUS_PRETEST)
	{
		// Handle system encryption command line arguments (if we're not in the Pretest phase).
		// Note that if bDirectSysEncModeCommand is not SYSENC_COMMAND_NONE, we already have the mutex.
		// Also note that SYSENC_COMMAND_DECRYPT is handled above.

		switch (bDirectSysEncModeCommand)
		{
		case SYSENC_COMMAND_RESUME:
		case SYSENC_COMMAND_STARTUP_SEQ_RESUME:

			if (bDirectSysEncModeCommand == SYSENC_COMMAND_STARTUP_SEQ_RESUME
				&& AskWarnYesNo ("SYSTEM_ENCRYPTION_RESUME_PROMPT") == IDNO)
			{
				EndMainDlg (MainDlg);
				return;
			}

			if (SysEncryptionOrDecryptionRequired ())
			{
				if (SystemEncryptionStatus != SYSENC_STATUS_ENCRYPTING
					&& SystemEncryptionStatus != SYSENC_STATUS_DECRYPTING)
				{
					// If the config file with status was lost or not written correctly, we
					// don't know whether to encrypt or decrypt (but we know that encryption or
					// decryption is required). Ask the user to select encryption, decryption, 
					// or cancel
					if (!ResolveUnknownSysEncDirection ())
					{
						EndMainDlg (MainDlg);
						return;
					}
				}

				LoadPage (hwndDlg, SYSENC_ENCRYPTION_PAGE);
				return;
			}
			else
			{
				// Nothing to resume
				Warning ("NOTHING_TO_RESUME");
				EndMainDlg (MainDlg);

				return;
			}
			break;

		case SYSENC_COMMAND_ENCRYPT:

			if (SysDriveOrPartitionFullyEncrypted (FALSE))
			{
				Info ("SYS_PARTITION_OR_DRIVE_APPEARS_FULLY_ENCRYPTED");
				EndMainDlg (MainDlg);
				return;
			}

			if (SysEncryptionOrDecryptionRequired ())
			{
				// System partition/drive encryption process already initiated but is incomplete.
				// If we were encrypting, resume the process directly. If we were decrypting, reverse 
				// the process and start encrypting.

				ChangeSystemEncryptionStatus (SYSENC_STATUS_ENCRYPTING);
				LoadPage (hwndDlg, SYSENC_ENCRYPTION_PAGE);
				return;
			}
			else
			{
				// Initiate the Pretest preparation phase
				if (!SwitchWizardToSysEncMode ())
				{
					bDirectSysEncMode = FALSE;
					EndMainDlg (MainDlg);
				}
				return;
			}

			break;

		case SYSENC_COMMAND_CREATE_HIDDEN_OS_ELEV:
		case SYSENC_COMMAND_CREATE_HIDDEN_OS:

			if (!SwitchWizardToHiddenOSMode ())
			{
				bDirectSysEncMode = FALSE;
				EndMainDlg (MainDlg);
			}
			return;
		}
	}


	if (!bDirectSysEncMode
		|| bDirectSysEncMode && SystemEncryptionStatus == SYSENC_STATUS_NONE)
	{
		// Handle system encryption cases where the wizard did not start even though it
		// was added to the startup sequence, as well as other weird cases and "leftovers"

		if (SystemEncryptionStatus != SYSENC_STATUS_NONE
			&& SystemEncryptionStatus != SYSENC_STATUS_PRETEST
			&& SysEncryptionOrDecryptionRequired ())
		{
			// System encryption/decryption had been in progress and did not finish

			if (CreateSysEncMutex ())	// If no other instance is currently taking care of system encryption
			{
				if (AskWarnYesNo ("SYSTEM_ENCRYPTION_RESUME_PROMPT") == IDYES)
				{
					bDirectSysEncMode = TRUE;
					ChangeWizardMode (WIZARD_MODE_SYS_DEVICE);
					LoadPage (hwndDlg, SYSENC_ENCRYPTION_PAGE);
					return;
				}
				else
					CloseSysEncMutex ();
			}
		}

		else if (SystemEncryptionStatus == SYSENC_STATUS_PRETEST)
		{
			// System pretest had been in progress but we were not launched during the startup seq

			if (CreateSysEncMutex ())	// If no other instance is currently taking care of system encryption
			{
				// The pretest has "priority handling"
				bDirectSysEncMode = TRUE;
				ChangeWizardMode (WIZARD_MODE_SYS_DEVICE);

				/* Do not return yet -- the principal pretest handler is below. */
			}
		}

		else if ((SystemEncryptionStatus == SYSENC_STATUS_NONE || SystemEncryptionStatus == SYSENC_STATUS_DECRYPTING)
			&& !BootEncStatus.DriveEncrypted 
			&& (BootEncStatus.DriveMounted || BootEncStatus.VolumeHeaderPresent))
		{
			// The pretest may have been in progress but we can't be sure (it is not in the config file).
			// Another possibility is that the user had finished decrypting the drive, but the config file
			// was not correctly updated. In both cases the best thing we can do is remove the header and 
			// deinstall. Otherwise, the result might be some kind of deadlock.

			if (CreateSysEncMutex ())	// If no other instance is currently taking care of system encryption
			{
				WaitCursor ();

				ForceRemoveSysEnc();

				InconsistencyResolved (SRC_POS);

				NormalCursor();
				CloseSysEncMutex ();
			}
		}
	}

	if (bDirectSysEncMode && CreateSysEncMutex ())
	{
		// We were launched either by Mount or by the system (startup sequence). Most of such cases should have 
		// been handled above already. Here we handle only the pretest phase (which can also be a hidden OS 
		// creation phase actually) and possible inconsistencies.

		switch (SystemEncryptionStatus)
		{
		case SYSENC_STATUS_PRETEST:
			{
				unsigned int hiddenOSCreationPhase = DetermineHiddenOSCreationPhase();

				bHiddenOS = (hiddenOSCreationPhase != TC_HIDDEN_OS_CREATION_PHASE_NONE);

				// Evaluate the results of the system encryption pretest (or of the hidden OS creation process)

				try
				{
					BootEncStatus = BootEncObj->GetStatus();
				}
				catch (Exception &e)
				{
					e.Show (hwndDlg);
					Error ("ERR_GETTING_SYSTEM_ENCRYPTION_STATUS");
					EndMainDlg (MainDlg);
					return;
				}

				if (BootEncStatus.DriveMounted)
				{
					/* Pretest successful or hidden OS has been booted during the process of hidden OS creation. */

					switch (hiddenOSCreationPhase)
					{
					case TC_HIDDEN_OS_CREATION_PHASE_NONE:

						// Pretest successful (or the hidden OS has been booted for the first time since the user started installing a new decoy OS)

						if (IsHiddenOSRunning())
						{
							// The hidden OS has been booted for the first time since the user started installing a
							// new decoy OS (presumably, our MBR config flags have been erased).
							
							// As for things we are responsible for, the process of hidden OS creation is completed
							// (the rest is up to the user).

							ManageStartupSeqWiz (TRUE, "");
							ChangeSystemEncryptionStatus (SYSENC_STATUS_NONE);

							EndMainDlg (MainDlg);
							
							return;
						}

						// Pretest successful (no hidden operating system involved)

						LoadPage (hwndDlg, SYSENC_PRETEST_RESULT_PAGE);
						return;

					case TC_HIDDEN_OS_CREATION_PHASE_WIPING:

						// Hidden OS has been booted when we are supposed to wipe the original OS

						LoadPage (hwndDlg, SYSENC_HIDDEN_OS_INITIAL_INFO_PAGE);
						return;

					case TC_HIDDEN_OS_CREATION_PHASE_WIPED:

						// Hidden OS has been booted and the original OS wiped. Now the user is required to install a new, decoy, OS.

						TextInfoDialogBox (TC_TBXID_DECOY_OS_INSTRUCTIONS);

						EndMainDlg (MainDlg);
						return;

					default:

						// Unexpected/unknown status
						ReportUnexpectedState (SRC_POS);
						EndMainDlg (MainDlg);
						return;
					}
				}
				else
				{
					BOOL bAnswerTerminate = FALSE, bAnswerRetry = FALSE;

					/* Pretest failed 
					or hidden OS cloning has been interrupted (and non-hidden OS is running)
					or wiping of the original OS has not been started (and non-hidden OS is running) */

					if (hiddenOSCreationPhase == TC_HIDDEN_OS_CREATION_PHASE_NONE)
					{
						// Pretest failed (no hidden operating system involved)

						if (AskWarnYesNo ("BOOT_PRETEST_FAILED_RETRY") == IDYES)
						{
							// User wants to retry the pretest
							bAnswerTerminate = FALSE;
							bAnswerRetry = TRUE;
						}
						else
						{
							// User doesn't want to retry the pretest
							bAnswerTerminate = TRUE;
							bAnswerRetry = FALSE;
						}
					}
					else
					{
						// Hidden OS cloning was interrupted or wiping of the original OS has not been started
						
						char *tmpStr[] = {0,
							hiddenOSCreationPhase == TC_HIDDEN_OS_CREATION_PHASE_WIPING ? "OS_WIPING_NOT_FINISHED_ASK" : "HIDDEN_OS_CREATION_NOT_FINISHED_ASK",
							"HIDDEN_OS_CREATION_NOT_FINISHED_CHOICE_RETRY",
							"HIDDEN_OS_CREATION_NOT_FINISHED_CHOICE_TERMINATE",
							"HIDDEN_OS_CREATION_NOT_FINISHED_CHOICE_ASK_LATER",
							0};

						switch (AskMultiChoice ((void **) tmpStr, FALSE))
						{
						case 1:
							// User wants to restart and continue/retry
							bAnswerTerminate = FALSE;
							bAnswerRetry = TRUE;
							break;

						case 2:
							// User doesn't want to retry but wants to terminate the entire process of hidden OS creation
							bAnswerTerminate = TRUE;
							bAnswerRetry = FALSE;
							break;

						default:
							// User doesn't want to do anything now
							bAnswerTerminate = FALSE;
							bAnswerRetry = FALSE;
						}
					}


					if (bAnswerRetry)
					{
						// User wants to restart and retry the pretest (or hidden OS creation)

						// We re-register the driver for boot because the user may have selected
						// "Last Known Good Configuration" from the Windows boot menu.
						// Note that we need to do this even when creating a hidden OS (because 
						// the hidden OS needs our boot driver and it will be a clone of this OS).
						try
						{
							BootEncObj->RegisterBootDriver (bHiddenOS ? true : false);
						}
						catch (Exception &e)
						{
							e.Show (NULL);
						}

						if (AskWarnYesNo ("CONFIRM_RESTART") == IDYES)
						{
							EndMainDlg (MainDlg);

							try
							{
								BootEncObj->RestartComputer ();
							}
							catch (Exception &e)
							{
								e.Show (hwndDlg);
							}

							return;
						}

						EndMainDlg (MainDlg);
						return;
					}
					else if (bAnswerTerminate)
					{
						// User doesn't want to retry pretest (or OS cloning), but to terminate the entire process

						try
						{
							BootEncObj->Deinstall ();
						}
						catch (Exception &e)
						{
							e.Show (hwndDlg);
						}

						ManageStartupSeqWiz (TRUE, "");
						ChangeSystemEncryptionStatus (SYSENC_STATUS_NONE);
						EndMainDlg (MainDlg);
						return;
					}
					else 
					{
						// User doesn't want to take any action now

						AbortProcessSilent();
					}
				}
			}
			break;

		default:

			// Unexpected progress status -- fix the inconsistency

			ManageStartupSeqWiz (TRUE, "");
			ChangeSystemEncryptionStatus (SYSENC_STATUS_NONE);
			EndMainDlg (MainDlg);
			InconsistencyResolved (SRC_POS);
			return;
		}
	}
	else
	{
		if (DirectDeviceEncMode)
		{
			SwitchWizardToNonSysDeviceMode();
			return;
		}

		if (DirectPromptNonSysInplaceEncResumeMode
			&& !bInPlaceEncNonSysPending)
		{
			// This instance of the wizard has been launched via the system startup sequence to prompt for resume of
			// a non-system in-place encryption process. However, no config file indicates that any such process
			// has been interrupted. This inconsistency may occur, for example, when the process is finished
			// but the wizard is not removed from the startup sequence because system encryption is in progress.
			// Therefore, we remove it from the startup sequence now if possible.

			if (!IsNonInstallMode () && SystemEncryptionStatus == SYSENC_STATUS_NONE)
				ManageStartupSeqWiz (TRUE, "");

			AbortProcessSilent ();
		}

		if (DirectNonSysInplaceEncResumeMode)
		{
			SwitchWizardToNonSysInplaceEncResumeMode();
			return;
		}
		else if (DirectPromptNonSysInplaceEncResumeMode)
		{
			if (NonSysInplaceEncInProgressElsewhere ())
				AbortProcessSilent ();

			if (AskWarnYesNo ("NONSYS_INPLACE_ENC_RESUME_PROMPT") == IDYES)
				SwitchWizardToNonSysInplaceEncResumeMode();
			else
				AbortProcessSilent ();

			return;
		}
		else if (bInPlaceEncNonSysPending
			&& !NonSysInplaceEncInProgressElsewhere ()
			&& AskWarnYesNo ("NONSYS_INPLACE_ENC_RESUME_PROMPT") == IDYES)
		{
			SwitchWizardToNonSysInplaceEncResumeMode();
			return;
		}

		LoadPage (hwndDlg, INTRO_PAGE);
	}
}

//********* Rotina WINMAIN *********************************************
///
/// Inicializa LOOP de mensagens e chama DLG Principal
///
/// \param MainProcParam : Parametros padrao de WINMAIN
///
/// \return int WINAPI
//*******************************************************************
int WINAPI WINMAIN (HINSTANCE hInstance, HINSTANCE hPrevInstance, char *lpszCommandLine, int nCmdShow)
{
	int status;
	atexit (localcleanup);

	VirtualLock (&volumePassword, sizeof(volumePassword));
	VirtualLock (szVerify, sizeof(szVerify));
	VirtualLock (szRawPassword, sizeof(szRawPassword));

	VirtualLock (MasterKeyGUIView, sizeof(MasterKeyGUIView));
	VirtualLock (HeaderKeyGUIView, sizeof(HeaderKeyGUIView));

	VirtualLock (randPool, sizeof(randPool));
	VirtualLock (lastRandPool, sizeof(lastRandPool));
	VirtualLock (outRandPoolDispBuffer, sizeof(outRandPoolDispBuffer));

	VirtualLock (&szFileName, sizeof(szFileName));
	VirtualLock (&szDiskFile, sizeof(szDiskFile));

	try
	{
		BootEncObj = new BootEncryption (NULL);
	}
	catch (Exception &e)
	{
		e.Show (NULL);
	}

	if (BootEncObj == NULL)
		AbortProcess ("INIT_SYS_ENC");

	InitCommonControls ();
	InitApp (hInstance, lpszCommandLine);

	nPbar = IDC_PROGRESS_BAR;

	if (Randinit ())
		AbortProcess ("INIT_RAND");

	RegisterRedTick(hInstance);

	/* Allocate, dup, then store away the application title */
	lpszTitle = GetString ("IDD_VOL_CREATION_WIZARD_DLG");

	status = DriverAttach ();
	if (status != 0)
	{
		if (status == ERR_OS_ERROR)
			handleWin32Error (NULL);
		else
			handleError (NULL, status);

		AbortProcess ("NODRIVER");
	}

	if (!AutoTestAlgorithms())
		AbortProcess ("ERR_SELF_TESTS_FAILED");

	/* Create the main dialog box */
	DialogBoxParamW (hInstance, MAKEINTRESOURCEW (IDD_VOL_CREATION_WIZARD_DLG), NULL, (DLGPROC) MainDialogProc, 
		(LPARAM)lpszCommandLine);

	return 0;
}

/*
CString g_szProgramFolder;
CString g_szCmdLine;

BOOL RunAsAdministrator(LPSTR lpszFileName, LPSTR lpszDirectory)
{
    SHELLEXECUTEINFOA TempInfo = {0};

    TempInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    TempInfo.fMask = 0;
    TempInfo.hwnd = NULL;
    TempInfo.lpVerb = "runas";
    TempInfo.lpFile = lpszFileName;
    TempInfo.lpParameters = "runasadmin";
    TempInfo.lpDirectory = lpszDirectory;
    TempInfo.nShow = SW_NORMAL;

    BOOL bRet = ::ShellExecuteExA(&TempInfo);

    return bRet;
}

BOOL CMFCApplication::InitInstance()
{
    char szCurFolder[1024] = { 0, };
    GetModuleFileName(GetModuleHandle(NULL), szCurFolder, 1023);

    CString szFullPath = szCurFolder;
    szFullPath = szFullPath.Left(szFullPath.ReverseFind('\\'));

    g_szProgramFolder = szFullPath;

    CCommandLineInfo oCmdLineInfo;
    ParseCommandLine(oCmdLineInfo);

    g_szCmdLine = oCmdLineInfo.m_strFileName;

    if(IsVista()) {
        if(stricmp(g_szCmdLine, "elevation") == 0) {
            char szCmdLine[1024] = { 0, };
            char szCurFileName[1024] = { 0, };
            GetModuleFileName(GetModuleHandle(NULL), szCurFileName, 1023);

            BOOL bRet = 
                RunAsAdministrator( szCurFileName, (LPSTR)(LPCTSTR)g_szProgramFolder );

            if(bRet == TRUE) {
                return FALSE;
            }
        } else if(stricmp(g_szCmdLine, "runasadmin") == 0) {
            //
            // go trough!.
            //
        } else {
            char szCmdLine[1024] = { 0, };
            char szCurFileName[1024] = { 0, };
            GetModuleFileName(GetModuleHandle(NULL), szCurFileName, 1023);

            sprintf(szCmdLine, "\"%s\" elevation", szCurFileName);

            WinExec(szCmdLine, SW_SHOW);

            return FALSE;
        }
    }
    .
    .
    .
}
*/
