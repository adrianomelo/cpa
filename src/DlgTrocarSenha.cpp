
#include "DlgTrocarSenha.h"
#include "XCPAMainDlg.h"
#include <wx/msgdlg.h>
#include <wx/app.h>

//(*InternalHeaders(DlgTrocarSenha)
#include <wx/bitmap.h>
#include <wx/font.h>
#include <wx/intl.h>
#include <wx/image.h>
#include <wx/string.h>
//*)

//(*IdInit(DlgTrocarSenha)
const long DlgTrocarSenha::ID_TX_SENHAATUAL = wxNewId();
const long DlgTrocarSenha::ID_TX_NOVASENHA = wxNewId();
const long DlgTrocarSenha::ID_TX_CONFIRMARNOVASENHA = wxNewId();
const long DlgTrocarSenha::ID_STATICTEXT1 = wxNewId();
const long DlgTrocarSenha::ID_STATICTEXT2 = wxNewId();
const long DlgTrocarSenha::ID_STATICTEXT3 = wxNewId();
const long DlgTrocarSenha::ID_BT_OK = wxNewId();
const long DlgTrocarSenha::ID_BT_SAIR = wxNewId();
const long DlgTrocarSenha::ID_STATICTEXT4 = wxNewId();
const long DlgTrocarSenha::ID_STATICTEXT6 = wxNewId();
const long DlgTrocarSenha::ID_STATICTEXT7 = wxNewId();
const long DlgTrocarSenha::ID_STATICTEXT5 = wxNewId();
//*)

BEGIN_EVENT_TABLE(DlgTrocarSenha,wxDialog)
	//(*EventTable(DlgTrocarSenha)
	//*)
END_EVENT_TABLE()

DlgTrocarSenha::DlgTrocarSenha(wxWindow* parent,wxWindowID id)
{
	//(*Initialize(DlgTrocarSenha)
	Create(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE, _T("wxID_ANY"));
	SetClientSize(wxSize(400,333));
	TxSenhaAtual = new wxTextCtrl(this, ID_TX_SENHAATUAL, wxEmptyString, wxPoint(192,40), wxSize(192,23), wxTE_PASSWORD, wxDefaultValidator, _T("ID_TX_SENHAATUAL"));
	TxNovaSenha = new wxTextCtrl(this, ID_TX_NOVASENHA, wxEmptyString, wxPoint(192,80), wxSize(192,23), wxTE_PASSWORD, wxDefaultValidator, _T("ID_TX_NOVASENHA"));
	TxConfirmarNovaSenha = new wxTextCtrl(this, ID_TX_CONFIRMARNOVASENHA, wxEmptyString, wxPoint(192,112), wxSize(192,23), wxTE_PASSWORD, wxDefaultValidator, _T("ID_TX_CONFIRMARNOVASENHA"));
	StaticText1 = new wxStaticText(this, ID_STATICTEXT1, _("Senha atual:"), wxPoint(96,48), wxDefaultSize, 0, _T("ID_STATICTEXT1"));
	StaticText2 = new wxStaticText(this, ID_STATICTEXT2, _("Nova senha:"), wxPoint(96,88), wxDefaultSize, 0, _T("ID_STATICTEXT2"));
	StaticText3 = new wxStaticText(this, ID_STATICTEXT3, _("Confirmar nova senha:"), wxPoint(32,120), wxDefaultSize, 0, _T("ID_STATICTEXT3"));
	BtOK = new wxBitmapButton(this, ID_BT_OK, wxBitmap(wxImage(_T("/usr/share/cpa/imagens/ok.jpg"))), wxPoint(200,296), wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_OK"));
	BtOK->SetBitmapDisabled(wxBitmap(wxImage(_T("/usr/share/cpa/imagens/ok-cinza.jpg"))));
	BtOK->Disable();
	BtSair = new wxBitmapButton(this, ID_BT_SAIR, wxBitmap(wxImage(_T("/usr/share/cpa/imagens/cancelar.bmp"))), wxPoint(296,296), wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_SAIR"));
	BtSair->SetDefault();
	StaticText4 = new wxStaticText(this, ID_STATICTEXT4, _("Alterar senha da unidade"), wxPoint(64,8), wxSize(232,19), 0, _T("ID_STATICTEXT4"));
	wxFont StaticText4Font(14,wxSWISS,wxFONTSTYLE_NORMAL,wxBOLD,false,_T("Sans"),wxFONTENCODING_DEFAULT);
	StaticText4->SetFont(StaticText4Font);
	StaticText5 = new wxStaticText(this, ID_STATICTEXT6, _("É de extrema importância utilizar uma senha segura. Evite utilizar \npalavras que possam ser encontradas nos dicionários, ou mesmo, \ncombinações destas palavras. Senhas não devem conter nomes\ne nem datas de nascimento. Não devem ser de fácil adivinhação."), wxPoint(8,176), wxSize(384,56), 0, _T("ID_STATICTEXT6"));
	wxFont StaticText5Font(8,wxSWISS,wxFONTSTYLE_NORMAL,wxNORMAL,false,_T("Sans"),wxFONTENCODING_DEFAULT);
	StaticText5->SetFont(StaticText5Font);
	StaticText6 = new wxStaticText(this, ID_STATICTEXT7, _("Atenção, este processo poderá demorar alguns minutos e não deverá\nser interrompido."), wxPoint(8,144), wxSize(368,24), 0, _T("ID_STATICTEXT7"));
	wxFont StaticText6Font(8,wxSWISS,wxFONTSTYLE_NORMAL,wxNORMAL,false,_T("Sans"),wxFONTENCODING_DEFAULT);
	StaticText6->SetFont(StaticText6Font);
	lblAlertaSenha = new wxStaticText(this, ID_STATICTEXT5, _("Processo em execução!\nAguarde enquanto a troca de senha é executada.\nNão interompa."), wxPoint(8,232), wxSize(384,56), wxST_NO_AUTORESIZE|wxALIGN_CENTRE|wxSIMPLE_BORDER|wxDOUBLE_BORDER, _T("ID_STATICTEXT5"));
	wxFont lblAlertaSenhaFont(10,wxSWISS,wxFONTSTYLE_ITALIC,wxBOLD,false,_T("Sans"),wxFONTENCODING_DEFAULT);
	lblAlertaSenha->SetFont(lblAlertaSenhaFont);

	Connect(ID_TX_SENHAATUAL,wxEVT_COMMAND_TEXT_UPDATED,(wxObjectEventFunction)&DlgTrocarSenha::OnTxSenhaAtualText);
	Connect(ID_TX_NOVASENHA,wxEVT_COMMAND_TEXT_UPDATED,(wxObjectEventFunction)&DlgTrocarSenha::OnTxNovaSenhaText);
	Connect(ID_TX_CONFIRMARNOVASENHA,wxEVT_COMMAND_TEXT_UPDATED,(wxObjectEventFunction)&DlgTrocarSenha::OnTxConfirmarNovaSenhaText);
	Connect(ID_BT_OK,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&DlgTrocarSenha::OnBtOKClick);
	Connect(ID_BT_SAIR,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&DlgTrocarSenha::OnBtSairClick);
	Connect(wxID_ANY,wxEVT_INIT_DIALOG,(wxObjectEventFunction)&DlgTrocarSenha::OnInit);
	//*)
}

DlgTrocarSenha::~DlgTrocarSenha()
{
	//(*Destroy(DlgTrocarSenha)
	//*)
}


void DlgTrocarSenha::OnInit(wxInitDialogEvent& event)
{
    lblAlertaSenha->Hide();
}

void DlgTrocarSenha::OnBtSairClick(wxCommandEvent& event)
{
       EndModal(wxID_CANCEL);
}

void DlgTrocarSenha::OnBtOKClick(wxCommandEvent& event)
{
    wxString smsg("o", wxConvUTF8);
    bool bSucesso;
    if (TxNovaSenha->GetValue().Len() < 15)
    {
        smsg = _T("SENHA CURTA. Senhas muito curtas são fáceis de quebrar. Deseja continuar mesmo assim?");
        int answer = wxMessageBox(smsg , sCaptionAviso, wxYES_NO, this);
        if (answer != wxYES)
          return;
    }

    smsg = _T("ATENÇÃO. Cuidado para não perder a senha, pois não existe nenhuma maneira de recuperá-la. O acesso aos arquivos, que fazem parte da unidade de arquivos protegidos, depende exclusivamente da senha de acesso.");
    wxMessageBox(smsg , sCaptionAviso, wxOK, this);


    lblAlertaSenha->Show();
    lblAlertaSenha->Refresh();
    BtOK->Disable();
    BtSair->Disable();
    this->Refresh();
    ::wxYield();
    bSucesso = ExecMudarSenha();
    BtOK->Enable();
    BtSair->Enable();
    if (bSucesso)
          EndModal(wxID_OK);
    else
          EndModal(wxID_ABORT);
}

void DlgTrocarSenha::OnTxConfirmarNovaSenhaText(wxCommandEvent& event)
{
    wxString tmp;
    tmp = TxSenhaAtual->GetValue();

    if  (  (tmp.Len() > 0 ) && (TxNovaSenha->GetValue().Len() > 0) &&
           (TxNovaSenha->GetValue() == TxConfirmarNovaSenha->GetValue())
        )
    {
        BtOK->Enable();
    }
    else
      BtOK->Disable();
}

void DlgTrocarSenha::OnTxNovaSenhaText(wxCommandEvent& event)
{
    OnTxConfirmarNovaSenhaText(event);
}

void DlgTrocarSenha::OnTxSenhaAtualText(wxCommandEvent& event)
{
    OnTxConfirmarNovaSenhaText(event);
}

bool DlgTrocarSenha::ExecMudarSenha()
{
    wxString sSenhaAtual = TxSenhaAtual->GetValue();
    wxString sNovaSenha  =  TxNovaSenha->GetValue();
    int code;
    //wxString scmd = _T("sudo truecrypt -t -C -p ");
    wxString scmd = _T("truecrypt -t -C -p ");
    scmd +=  sSenhaAtual;
    scmd += _T(" --new-password=");
    scmd += sNovaSenha;
    scmd += _T(" --non-interactive ");
    scmd += sNomeArq;
    code = wxExecute(scmd, wxEXEC_SYNC);

    return (code == 0);
}
