#include "XCPAMainDlg.h"
#include <wx/msgdlg.h>
#include <wx/dir.h>
#include <wx/app.h>
#include "DlgAcessarunidade.h"
#include "DlgNovaUnidade.h"
#include <stdlib.h>
#include <wx/bitmap.h>
#include "DlgTrocarSenha.h"
#include "XCPAApp.h"

//(*InternalHeaders(XCPAMainDlg)
#include <wx/bitmap.h>
#include <wx/intl.h>
#include <wx/image.h>
#include <wx/string.h>
//*)

//(*IdInit(XCPAMainDlg)
const long XCPAMainDlg::ID_STATICBITMAP1 = wxNewId();
const long XCPAMainDlg::ID_BT_ACESSAR = wxNewId();
const long XCPAMainDlg::ID_BT_FECHAR = wxNewId();
const long XCPAMainDlg::ID_BT_ALTERARSENHA = wxNewId();
const long XCPAMainDlg::ID_BT_REMOVER = wxNewId();
const long XCPAMainDlg::ID_BT_AJUDA = wxNewId();
const long XCPAMainDlg::ID_PANEL1 = wxNewId();
const long XCPAMainDlg::ID_STATICLINE1 = wxNewId();
const long XCPAMainDlg::ID_BITMAPBUTTON6 = wxNewId();
//*)

BEGIN_EVENT_TABLE(XCPAMainDlg,wxDialog)
	//(*EventTable(XCPAMainDlg)
	//*)
END_EVENT_TABLE()

XCPAMainDlg::XCPAMainDlg(wxWindow* parent,wxWindowID id,const wxPoint& pos,const wxSize& size)
{
	//(*Initialize(XCPAMainDlg)
	Create(parent, wxID_ANY, _("Central de proteção de arquivos"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE, _T("wxID_ANY"));
	SetClientSize(wxSize(616,355));
	Panel1 = new wxPanel(this, ID_PANEL1, wxDLG_UNIT(this,wxPoint(8,3)), wxSize(584,290), wxSTATIC_BORDER|wxTAB_TRAVERSAL, _T("ID_PANEL1"));

	StaticBitmap1 = new wxStaticBitmap(Panel1, ID_STATICBITMAP1, wxBitmap(wxImage(_T("/usr/share/cpa/imagens/imagem01.jpg"))), wxPoint(10,7), wxDefaultSize, 0, _T("ID_STATICBITMAP1"));
	BtAcessarUnidade = new wxBitmapButton(Panel1, ID_BT_ACESSAR, wxBitmap(wxImage(_T("/usr/share/cpa/imagens/acessar-pasta.jpg"))), wxPoint(256,8), wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_ACESSAR"));
	BtAcessarUnidade->SetBitmapDisabled(wxBitmap(wxImage(_T("/usr/share/cpa/imagens/acessar-pasta_cinza.jpg"))));
	BtAcessarUnidade->SetDefault();
	BtFechar = new wxBitmapButton(Panel1, ID_BT_FECHAR, wxBitmap(wxImage(_T("/usr/share/cpa/imagens/fechar-unidade.jpg"))), wxDLG_UNIT(Panel1,wxPoint(128,30)), wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_FECHAR"));
	BtFechar->SetBitmapDisabled(wxBitmap(wxImage(_T("/usr/share/cpa/imagens/fechar-unidade_cinza.jpg"))));
	BtAlterarSenha = new wxBitmapButton(Panel1, ID_BT_ALTERARSENHA, wxBitmap(wxImage(_T("/usr/share/cpa/imagens/alterar-senha.jpg"))), wxDLG_UNIT(Panel1,wxPoint(128,56)), wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_ALTERARSENHA"));
	BtAlterarSenha->SetBitmapDisabled(wxBitmap(wxImage(_T("/usr/share/cpa/imagens/alterar-senha_cinza.jpg"))));
	BtAlterarSenha->SetDefault();
	BtRemover = new wxBitmapButton(Panel1, ID_BT_REMOVER, wxBitmap(wxImage(_T("/usr/share/cpa/imagens/remover-unidade.jpg"))), wxDLG_UNIT(Panel1,wxPoint(128,82)), wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_REMOVER"));
	BtRemover->SetBitmapDisabled(wxBitmap(wxImage(_T("/usr/share/cpa/imagens/remover-unidade_cinza.jpg"))));
	BtRemover->SetDefault();
	BtAjuda = new wxBitmapButton(Panel1, ID_BT_AJUDA, wxBitmap(wxImage(_T("/usr/share/cpa/imagens/botao-ajuda2.jpg"))), wxDLG_UNIT(Panel1,wxPoint(128,109)), wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_AJUDA"));
	BtAjuda->SetDefault();
	StaticLine1 = new wxStaticLine(this, ID_STATICLINE1, wxPoint(8,296), wxSize(600,16), wxLI_HORIZONTAL, _T("ID_STATICLINE1"));
	BitmapButton6 = new wxBitmapButton(this, ID_BITMAPBUTTON6, wxBitmap(wxImage(_T("/usr/share/cpa/imagens/cancelar.bmp"))), wxPoint(520,312), wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BITMAPBUTTON6"));
	BitmapButton6->SetDefault();

	Connect(ID_BT_ACESSAR,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&XCPAMainDlg::OnbtAcessarUnidadeClick);
	Connect(ID_BT_FECHAR,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&XCPAMainDlg::OnBtFecharClick);
	Connect(ID_BT_ALTERARSENHA,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&XCPAMainDlg::OnBtAlterarSenhaClick);
	Connect(ID_BT_REMOVER,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&XCPAMainDlg::OnBtRemoverClick);
	Connect(ID_BT_AJUDA,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&XCPAMainDlg::OnBtAjudaClick);
	Panel1->Connect(ID_PANEL1,wxEVT_PAINT,(wxObjectEventFunction)&XCPAMainDlg::OnPanel1Paint,0,this);
	Connect(ID_BITMAPBUTTON6,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&XCPAMainDlg::OnBitmapButton6Click);
	Connect(wxID_ANY,wxEVT_INIT_DIALOG,(wxObjectEventFunction)&XCPAMainDlg::OnInit);
	//*)
	Connect (-1, wxEVT_CLOSE_WINDOW,
             wxCloseEventHandler (XCPAMainDlg::OnClose),
             NULL, this);


}

/*
 * Método:		OnClose
 * Descrição:	Fechar janela principal de diálogo
 * Data:		05/02/2010
 * Autor:		Gustavo R. de Castro
 * Entrada:		e - evento
 */
void XCPAMainDlg::OnClose (wxCloseEvent& e)
{
    wxString  smsg ("", wxConvUTF8);
    if(e.CanVeto())
     if (UnidadeMontada())
     {
        smsg = _("ATENÇÃO. A unidade de arquivos protegidos está aberta e deverá ser fechada caso não esteja sendo utilizada. Isso garante a privacidade dos arquivos confidenciais.\n Deseja encerrar o CPA?");//, wxConvUTF8);
        int answer = wxMessageBox(smsg , sCaptionAviso, wxYES_NO, this);
        if (answer == wxYES)
        {
            Destroy();
        }
        else
            return;
     }
    else
    {
        Destroy();
    }
}

XCPAMainDlg::~XCPAMainDlg()
{
}

wxString XCPAMainDlg::MontarCmdMontarUnidade()
{
    wxString scmd;
    int temp;

    temp = umask(0);

    scmd = _T("sudo truecrypt -t --mount -p ");
    scmd += sSenha;
    scmd += _T(" --non-interactive ");
    scmd += sNomeArq;
    scmd += _T(" ");
    scmd += sPontoMontagem;
     return scmd;

}

bool XCPAMainDlg::UnidadeMontada()
{

  int code;
  wxString scmd;


    scmd = _T("sudo truecrypt -t -l --non-interactive ");
    scmd += sPontoMontagem;
    wxArrayString strOut = wxArrayString();
    code = wxExecute(scmd, strOut, 0);

    return (code==0);
}

bool XCPAMainDlg::UnidadeCriada()
{
    return wxFileExists(sNomeArq);
}

void XCPAMainDlg::HabilitarBotoes()
{
    if (UnidadeCriada())
    {
        if (UnidadeMontada()) //criada e montada
        {
            BtAcessarUnidade->Disable();
            BtFechar->Enable();
            BtAlterarSenha->Disable();
            BtRemover->Disable();
        }
        else //criada mas nÃ£o montada
        {
            BtAcessarUnidade->Enable();
            BtFechar->Disable();
            BtAlterarSenha->Enable();
            BtRemover->Enable();
        }
    }
    else
    {
            BtAcessarUnidade->Enable();
            BtFechar->Disable();
            BtAlterarSenha->Disable();
            BtRemover->Disable();
    }
    return;
}

void XCPAMainDlg::OnbtAcessarUnidadeClick(wxCommandEvent& event)
{
 //int code;
    bool bCriou = false;
    bool bMontou = false;
    bool mostrarUnidade = false;
    //wxString scmdMontarUnidade;
    wxString  smsg ("", wxConvUTF8);
    wxString sCaption;
    sCaption = sNomeSistema;
    sCaption += " ";

    if (! UnidadeCriada()) //wxFileExists(sNomeArq))
    {

        smsg = _("A unidade de arquivos protegidos ainda não existe. Siga os passos a seguir para criá-la e iniciar seu uso.");//, wxConvUTF8);

        wxMessageBox( smsg , sCaptionAviso, wxOK, this);

        DlgNovaUnidade  dlgNova(0);
        dlgNova.sNomeArq = sNomeArq;
        dlgNova.sPontoMontagem = sPontoMontagem;
        dlgNova.SetTitle(sCaption);

        //dlgNova.SetTitle(sNomeSistema);
        if (dlgNova.ShowModal() == wxID_OK)
        {
            bCriou = true;
            sSenha = dlgNova.sSenha;
            mostrarUnidade = dlgNova.mostrarUnidade;
        }
        else
        {
            dlgNova.Destroy();
            return;
        }
        dlgNova.Destroy();
    }

    if (! UnidadeCriada()) //wxFileExists(sNomeArq))
    {
       return; //mensagem de erro já foi exibida anteriormente
    }

    MontarUnidade (!bCriou, false, &mostrarUnidade);
    bMontou = UnidadeMontada();
    if (bMontou)
    {
      smsg = _T("Unidade de arquivos protegidos ");
      if (bCriou)
        smsg += _T(" criada e ");
      smsg += _T(" disponível para acesso em: ");
      smsg += sPontoMontagem;
      wxMessageBox(smsg , sCaptionAviso, wxOK, this);
      if (bCriou)
      {
        smsg = _T("ATENÇÃO. A unidade de arquivos protegidos faz parte do disco rí­gido. Se o disco rí­gido deste computador for apagado ou formatado, todos os arquivos que estiverem armazenados na Unidade de Arquivos Protegidos serão perdidos.");
        wxMessageBox(smsg , sCaptionAviso, wxOK, this);
      }
    }
    else //Criou ma nao conseguiu montar
    {
        smsg = _T("Unidade de arquivos protegidos está criada, porém não foi montada. Clique novamente no botão Acessar unidade para poder acessá-la.");
        wxMessageBox(smsg, sCaptionAviso, wxOK, this);
    }

    if(bMontou)
    {
       smsg = _T("ATENÇÃO. A unidade de arquivos protegidos está aberta e deverá ser fechada caso não esteja sendo utilizada. Isso garante a privacidade dos arquivos confidenciais.");
       wxMessageBox(smsg , sCaptionAviso, wxOK, this);

    }
    if (bMontou && !bCriou)
    {
       smsg = _T("ATENÇÃO. Cuidado para não perder a senha, pois não existe nenhuma maneira de recuperá-la. O acesso aos arquivos, que fazem parte da unidade de arquivos protegidos, depende exclusivamente da senha de acesso.");
       wxMessageBox(smsg , sCaptionAviso, wxOK, this);
    }

    if(bMontou && mostrarUnidade)
    {
        MostrarUnidade();
    }

    HabilitarBotoes();
}
void XCPAMainDlg::OnBtFecharClick(wxCommandEvent& event)
{
    FecharUnidade(true);
    HabilitarBotoes();
}
void XCPAMainDlg::OnBtAlterarSenhaClick(wxCommandEvent& event)
{
   wxString smsg ("o", wxConvUTF8);
   //int code;
   wxString sCaption;
   sCaption = sNomeSistema;
   sCaption += " ";

    DlgTrocarSenha dlg(0);
    dlg.sNomeArq = sNomeArq;
    dlg.sPontoMontagem = sPontoMontagem;
    dlg.SetTitle(sCaption);
    int iRetCode = dlg.ShowModal();
    if ( iRetCode == wxID_ABORT)
    {
      smsg = _T("Não foi possí­vel alterar a senha. Verifique a senha digitada e tente novamente.");
      wxMessageBox(smsg , sCaptionAviso, wxOK, this);
    }
    else if (iRetCode == wxID_OK)
    {
      smsg = _T("Senha da unidade de arquivos protegidos alterada com sucesso!");
      wxMessageBox(smsg , sCaptionAviso, wxOK, this);
    }
    return;
}
void XCPAMainDlg::OnBtRemoverClick(wxCommandEvent& event)
{
   wxString smsg;
   bool bUnidadeVazia;
   smsg = _T("ATENÇÃO. A unidade de arquivos protegidos será removida, deseja continuar? (Caso seja necessária sua utilização, ela deverá ser criada novamente).");
   int answer = wxMessageBox(smsg , sCaptionAviso, wxYES_NO, this);
   if (answer != wxYES)
       return;

    if (! MontarUnidade(true, true, false))
    {
     HabilitarBotoes(); //redundante, mas em caso de erro, poderá salvar o usuario
     return;
    }

    bUnidadeVazia = UnidadeVazia();

    FecharUnidade(false);

    if (! bUnidadeVazia)
    {
        smsg = _T("Unidade de arquivos protegidos contém um ou mais arquivos e por isto não pode ser removida. Exclua todos os arquivos e tente novamente.");
        wxMessageBox(smsg, sCaptionAviso, wxOK, this);
        HabilitarBotoes(); //redundante, mas em caso de algum erro, poderá salvar o usuário.
        return;
    }

    if (UnidadeCriada()) //wxFileExists(sNomeArq))
    {
      std::remove(sNomeArq.mb_str());
      wxMessageBox(_T("Unidade de arquivos protegidos removida com sucesso."), sCaptionAviso, wxOK, this);
    }
    HabilitarBotoes();
}
void XCPAMainDlg::OnBtAjudaClick(wxCommandEvent& event)
{
    //wxString spath;
    //spath =_T("/usr/share/cpa/ajuda/index.html");
    //::wxLaunchDefaultBrowser(spath);

    int pid;
    char *const helpfirefox[] = {"firefox", "/usr/share/cpa/ajuda/index.html", NULL};
    char *const helpkonqueror[] = {"konqueror", "/usr/share/cpa/ajuda/index.html", NULL};
    pid = fork();
    if (pid == 0)
    {
        if (execvp("firefox", helpfirefox) == -1)
            execvp("konqueror", helpkonqueror);
        exit(0);
    }
}

void XCPAMainDlg::OnBitmapButton6Click(wxCommandEvent& event)
{
        Close();
}

void XCPAMainDlg::OnInit(wxInitDialogEvent& event)
{
   char shome[255];
   wxString sAppPath;
   wxString sIcon;
   wxString spath;
   char buffer[255];
   getcwd(buffer, sizeof(buffer));
   sAppPath = wxString::Format("%s", buffer); //::wxGetCwd();

    strcpy(shome, getenv("HOME"));
    strcat(shome, "/.cpa");
    sNomeArq = wxString::Format("%s", shome);
    wxDir * d = new wxDir();
    if (! d->Exists(sNomeArq))
      mkdir( shome, S_IRWXU | S_IRWXG | S_IRWXO );


    strcat (shome, "/UnidadeSegura.it");
    sNomeArq = wxString::Format("%s", shome);


    strcpy(shome, getenv("HOME"));
    strcat (shome, "/UnidadeSegura");
    sPontoMontagem = wxString::Format("%s", shome);
    if (! d->Exists(sPontoMontagem))
      mkdir( sPontoMontagem, S_IRWXU | S_IRWXG | S_IRWXO );

    delete d;

    SetTitle(sNomeSistema);
    HabilitarBotoes();
    sIcon = _T("/usr/share/cpa/imagens/itautec.xpm");
    SetIcon(wxIcon(sIcon));

}

void XCPAMainDlg::OnPanel1Paint(wxPaintEvent& event)
{
}

/*
 * Método:		XCPAMainDlg::MontarUnidade
 * Descrição:	Monta uma unidade
 * Data:		05/02/2010
 * Autor:		Gustavo R. de Castro
 * Entrada:		bPedirSenha - pedir senha ou não
 * 				bremover	- remover partição
 * 				mostrarUnidade	- mostrar unidade
 * Saida:		true - conseguiu montar
 * 				false - não conseguiu
 */
bool XCPAMainDlg::MontarUnidade(bool bPedirSenha, bool bRemover, bool *mostrarUnidade)
{
   int code;
   wxString scmdMontarUnidade;
   wxString smsg;


   //apenas para evitar <2> no caption
   wxString sCaption;
   sCaption = sNomeSistema;
   sCaption += " ";

    if (bPedirSenha)
    {
      DlgAcessarunidade dlg(0);
      dlg.SetTitle(sCaption);
      if (bRemover)
      {
        dlg.lblTitulo->SetLabel(_T("Remover Unidade"));
        dlg.chkExibirUnidade->Show(false);
      }
      else
      {
        dlg.lblTitulo->SetLabel(_T("Acessar Unidade"));
        dlg.chkExibirUnidade->Show(true);
      }

      if (dlg.ShowModal() != wxID_OK)
        return false;
      sSenha = dlg.edSenha->GetValue();
      if(mostrarUnidade)
        *mostrarUnidade = dlg.chkExibirUnidade->IsChecked();
    }

    wxDir * d = new wxDir();
    if (! d->Exists(sPontoMontagem))
      mkdir( sPontoMontagem.mb_str(), S_IRWXU | S_IRWXG | S_IRWXO );
    delete d;

    if(! sSenha.IsEmpty())
    {
        scmdMontarUnidade = MontarCmdMontarUnidade();
        code = wxExecute(scmdMontarUnidade, wxEXEC_SYNC);
    }
    else
     code = -1;

    if (code)
    {
      rmdir( sPontoMontagem.mb_str() );
      smsg = _T("Erro ao acessar unidade. Verifique a senha digitada e tente novamente.");
      wxMessageBox(smsg, sCaptionAviso, wxOK, this);
      return false;
    }
    else
    {
      return true;
    }
}
bool XCPAMainDlg::FecharUnidade(bool bMensagem)
{
    wxString smsg;
    bool bFechada = false;
    int code = 0;

    if (UnidadeMontada())
    {
        wxString scmd = _T("sudo truecrypt -t -d --non-interactive ");
        code = wxExecute(scmd, wxEXEC_SYNC);
        bFechada = ! UnidadeMontada();
        if (bFechada)
          rmdir(sPontoMontagem.mb_str() );
    }

    if (bMensagem)
    {
      smsg = (bFechada)?_T("Unidade de arquivos protegidos fechada com sucesso! Para acessá-la novamente, selecione a opção 'Acessar unidade' na tela principal do CPA.") :
                        _T("Erro ao fechar a unidade. Feche todos os arquivos e tente novamente.");
      wxMessageBox(smsg, sCaptionAviso, wxOK, this);
    }
    return (bFechada);
}

/*
 * Método:		XCPAMainDlg::MostrarUnidade
 * Descrição:	Exibir unidade montada
 * Data:		05/02/2010
 * Autor:		Gustavo R. de Castro
 */
void XCPAMainDlg::MostrarUnidade()
{
    wxString scmd;
    scmd = _T("dolphin ");
    scmd += sPontoMontagem;
    wxExecute(scmd, wxEXEC_ASYNC, NULL);
}
bool XCPAMainDlg::UnidadeVazia()
{
    wxDir dir(sPontoMontagem);
    bool bAchou = false;

    if ( !dir.IsOpened() )
    {
        // deal with the error here - wxDir would already log an error message
        // explaining the exact reason of the failure
        return false;
    }

    //dir.HasFiles()mostrarUnidade
    //dir.HasSubDirs()

   wxString filename;
   if (dir.GetFirst(&filename,"*", wxDIR_FILES | wxDIR_DIRS | wxDIR_HIDDEN))
   do
   {
       //wxMessageBox(filename);
       if (  (filename != _T(".")) &&
             (filename != _T("..")) &&
             (filename.Find(_T(".Trash")) == -1)
          )
       {
         bAchou = true;
         break;
       }
   } while (dir.GetNext(&filename));

  return !bAchou;
}
