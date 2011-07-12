#include "DlgAcessarunidade.h"

//(*InternalHeaders(DlgAcessarunidade)
#include <wx/bitmap.h>
#include <wx/font.h>
#include <wx/intl.h>
#include <wx/image.h>
#include <wx/string.h>
//*)

//(*IdInit(DlgAcessarunidade)
const long DlgAcessarunidade::ID_LBL_TITULO = wxNewId();
const long DlgAcessarunidade::ID_STATICTEXT2 = wxNewId();
const long DlgAcessarunidade::ID_TXT_SENHA = wxNewId();
const long DlgAcessarunidade::ID_BT_CANCELAR = wxNewId();
const long DlgAcessarunidade::ID_BT_OK = wxNewId();
const long DlgAcessarunidade::ID_chkExibirUnidade = wxNewId();
//*)

BEGIN_EVENT_TABLE(DlgAcessarunidade,wxDialog)
	//(*EventTable(DlgAcessarunidade)
	//*)
END_EVENT_TABLE()

DlgAcessarunidade::DlgAcessarunidade(wxWindow* parent)
{
	//(*Initialize(DlgAcessarunidade)
	Create(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE, _T("wxID_ANY"));
	SetClientSize(wxSize(400,165));
	lblTitulo = new wxStaticText(this, ID_LBL_TITULO, _("Acessar unidade"), wxPoint(112,16), wxSize(184,23), wxALIGN_CENTRE, _T("ID_LBL_TITULO"));
	wxFont lblTituloFont(14,wxSWISS,wxFONTSTYLE_NORMAL,wxBOLD,false,_T("Sans"),wxFONTENCODING_DEFAULT);
	lblTitulo->SetFont(lblTituloFont);
	StaticText2 = new wxStaticText(this, ID_STATICTEXT2, _("Senha:"), wxPoint(16,64), wxDefaultSize, 0, _T("ID_STATICTEXT2"));
	edSenha = new wxTextCtrl(this, ID_TXT_SENHA, wxEmptyString, wxPoint(72,56), wxSize(312,25), wxTE_PASSWORD, wxDefaultValidator, _T("ID_TXT_SENHA"));
	btCancelar = new wxBitmapButton(this, ID_BT_CANCELAR, wxBitmap(wxImage(_T("/usr/share/cpa/imagens/cancelar.bmp"))), wxPoint(296,128), wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_CANCELAR"));
	BtOk = new wxBitmapButton(this, ID_BT_OK, wxBitmap(wxImage(_T("/usr/share/cpa/imagens/ok.jpg"))), wxPoint(200,128), wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_OK"));
	BtOk->SetDefault();
	chkExibirUnidade = new wxCheckBox(this, ID_chkExibirUnidade, _("Exibir unidade de arquivos protegidos"), wxPoint(72,96), wxDefaultSize, 0, wxDefaultValidator, _T("ID_chkExibirUnidade"));
	chkExibirUnidade->SetValue(true);

	Connect(ID_TXT_SENHA,wxEVT_COMMAND_TEXT_UPDATED,(wxObjectEventFunction)&DlgAcessarunidade::OnedSenhaText);
	Connect(ID_BT_CANCELAR,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&DlgAcessarunidade::OnBitmapButton1Click);
	Connect(ID_BT_OK,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&DlgAcessarunidade::OnBtOkClick1);
	Connect(wxID_ANY,wxEVT_INIT_DIALOG,(wxObjectEventFunction)&DlgAcessarunidade::OnInit);
	//*)
}

DlgAcessarunidade::~DlgAcessarunidade()
{
	//(*Destroy(DlgAcessarunidade)
	//*)
}

void DlgAcessarunidade::OnBitmapButton1Click(wxCommandEvent& event)
{
      EndModal(wxID_CANCEL);
}


void DlgAcessarunidade::OnBtOkClick1(wxCommandEvent& event)
{
       EndModal(wxID_OK);
}

void DlgAcessarunidade::OnInit(wxInitDialogEvent& event)
{
}

void DlgAcessarunidade::OnedSenhaText(wxCommandEvent& event)
{
}
