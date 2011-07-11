#include "PnlConcluir.h"

//(*InternalHeaders(PnlConcluir)
#include <wx/font.h>
#include <wx/intl.h>
#include <wx/string.h>
//*)

//(*IdInit(PnlConcluir)
const long PnlConcluir::ID_STATICTEXT1 = wxNewId();
const long PnlConcluir::ID_STATICTEXT3 = wxNewId();
const long PnlConcluir::ID_GAUGE1 = wxNewId();
const long PnlConcluir::ID_PANEL1 = wxNewId();
const long PnlConcluir::ID_STATICTEXT2 = wxNewId();
const long PnlConcluir::ID_CHECKBOX1 = wxNewId();
const long PnlConcluir::ID_chkExibirUnidade = wxNewId();
const long PnlConcluir::ID_PANEL2 = wxNewId();
//*)

BEGIN_EVENT_TABLE(PnlConcluir,wxPanel)
	//(*EventTable(PnlConcluir)
	//*)
END_EVENT_TABLE()

PnlConcluir::PnlConcluir(wxWindow* parent,wxWindowID id,const wxPoint& pos,const wxSize& size)
{
	//(*Initialize(PnlConcluir)
	Create(parent, wxID_ANY, wxDefaultPosition, wxSize(424,176), wxSIMPLE_BORDER|wxTAB_TRAVERSAL, _T("wxID_ANY"));
	Panel2 = new wxPanel(this, ID_PANEL2, wxPoint(0,0), wxSize(408,288), wxTAB_TRAVERSAL, _T("ID_PANEL2"));
	StaticText1 = new wxStaticText(Panel2, ID_STATICTEXT1, _("Nova unidade, passo 2: Concluir"), wxPoint(48,16), wxDefaultSize, 0, _T("ID_STATICTEXT1"));
	wxFont StaticText1Font(12,wxSWISS,wxFONTSTYLE_NORMAL,wxBOLD,false,_T("Sans"),wxFONTENCODING_DEFAULT);
	StaticText1->SetFont(StaticText1Font);
	Panel1 = new wxPanel(Panel2, ID_PANEL1, wxPoint(8,48), wxSize(384,87), wxSIMPLE_BORDER|wxTAB_TRAVERSAL, _T("ID_PANEL1"));
	StaticText3 = new wxStaticText(Panel1, ID_STATICTEXT3, _("Andamento da criação da nova unidade:"), wxPoint(8,16), wxDefaultSize, 0, _T("ID_STATICTEXT3"));
	Gauge1 = new wxGauge(Panel1, ID_GAUGE1, 100, wxPoint(8,40), wxSize(368,28), 0, wxDefaultValidator, _T("ID_GAUGE1"));
	txLblConcluir = new wxStaticText(Panel2, ID_STATICTEXT2, _("Sua unidade de arquivos protegidos será criada com o tamanho\nde %d GB.  Clique em \'Concluir\' para finalizar o processo e iniciar\nseu uso."), wxPoint(24,160), wxDefaultSize, 0, _T("ID_STATICTEXT2"));
	wxFont txLblConcluirFont(8,wxSWISS,wxFONTSTYLE_NORMAL,wxNORMAL,false,_T("Sans"),wxFONTENCODING_DEFAULT);
	txLblConcluir->SetFont(txLblConcluirFont);
	chkExibirUnidade = new wxCheckBox(Panel2, ID_chkExibirUnidade, _("Exibir unidade de arquivos protegidos"), wxPoint(24,216), wxDefaultSize, 0, wxDefaultValidator, _T("ID_chkExibirUnidade"));
	chkExibirUnidade->SetValue(true);
	//*)
	position = 0;
	Gauge1->SetValue(0);
}

PnlConcluir::~PnlConcluir()
{
	//(*Destroy(PnlConcluir)
	//*)
}

void PnlConcluir::IncGauge(int step)
{
    if (position >= iRange)
      position = 1;

    Gauge1->SetValue(position);
    position += step;
    Gauge1->Refresh();

}
