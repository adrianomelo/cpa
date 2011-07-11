#include <wx/msgdlg.h>
#include "DlgNovaUnidade.h"
#include "PnlConcluir.h"
#include <wx/process.h>
#include <wx/wxchar.h>
#include "XCPAMainDlg.h"
#include <errno.h>
#include <sys/statfs.h>

//(*InternalHeaders(DlgNovaUnidade)
#include <wx/bitmap.h>
#include <wx/font.h>
#include <wx/intl.h>
#include <wx/image.h>
#include <wx/string.h>
//*)

//(*IdInit(DlgNovaUnidade)
const long DlgNovaUnidade::ID_TX_SENHA = wxNewId();
const long DlgNovaUnidade::ID_TX_CONFIRMA = wxNewId();
const long DlgNovaUnidade::ID_RADIOBOX1 = wxNewId();
const long DlgNovaUnidade::ID_SP_TAM_OUTRO = wxNewId();
const long DlgNovaUnidade::ID_STATICTEXT1 = wxNewId();
const long DlgNovaUnidade::ID_STATICTEXT2 = wxNewId();
const long DlgNovaUnidade::ID_STATICTEXT3 = wxNewId();
const long DlgNovaUnidade::ID_STATICTEXT4 = wxNewId();
const long DlgNovaUnidade::ID_PANEL1 = wxNewId();
const long DlgNovaUnidade::ID_BT_CANCELAR = wxNewId();
const long DlgNovaUnidade::ID_BT_AVANCAR = wxNewId();
const long DlgNovaUnidade::ID_BT_CONCLUIR = wxNewId();
const long DlgNovaUnidade::ID_TIMER1 = wxNewId();
//*)

BEGIN_EVENT_TABLE(DlgNovaUnidade,wxDialog)
	//(*EventTable(DlgNovaUnidade)
	//*)
	EVT_END_PROCESS(wxID_ANY, DlgNovaUnidade::OnProcessTerm)
	EVT_TIMER(ID_TIMER1, DlgNovaUnidade::OnTimer1Trigger)
	EVT_PAINT(DlgNovaUnidade::OnPanel1Paint)
END_EVENT_TABLE()


DlgNovaUnidade::DlgNovaUnidade(wxWindow* parent,wxWindowID id,const wxPoint& pos,const wxSize& size)
{
	//(*Initialize(DlgNovaUnidade)
	Create(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE, _T("id"));
	SetClientSize(wxSize(411,344));
	Move(wxDefaultPosition);
	Panel1 = new wxPanel(this, ID_PANEL1, wxPoint(0,8), wxSize(408,288), wxSIMPLE_BORDER|wxTAB_TRAVERSAL, _T("ID_PANEL1"));
	txSenha = new wxTextCtrl(Panel1, ID_TX_SENHA, wxEmptyString, wxPoint(136,48), wxSize(263,25), wxTE_PASSWORD, wxDefaultValidator, _T("ID_TX_SENHA"));
	txConfirma = new wxTextCtrl(Panel1, ID_TX_CONFIRMA, wxEmptyString, wxPoint(136,80), wxSize(263,25), wxTE_PASSWORD, wxDefaultValidator, _T("ID_TX_CONFIRMA"));
	wxString __wxRadioBoxChoices_1[5] =
	{
		_("2"),
		_("5"),
		_("10"),
		_("20"),
		_("outro:")
	};
	RadioBox1 = new wxRadioBox(Panel1, ID_RADIOBOX1, _("Tamanho da unidade (em GB)"), wxPoint(8,121), wxSize(392,48), 5, __wxRadioBoxChoices_1, 1, 0, wxDefaultValidator, _T("ID_RADIOBOX1"));
	spTamOutro = new wxSpinCtrl(Panel1, ID_SP_TAM_OUTRO, _T("1"), wxPoint(240,136), wxSize(95,25), 0, 1, 100, 1, _T("ID_SP_TAM_OUTRO"));
	spTamOutro->SetValue(_T("1"));
	spTamOutro->Disable();
	StaticText1 = new wxStaticText(Panel1, ID_STATICTEXT1, _("Senha:"), wxPoint(80,56), wxDefaultSize, 0, _T("ID_STATICTEXT1"));
	StaticText2 = new wxStaticText(Panel1, ID_STATICTEXT2, _("Confirmar senha:"), wxPoint(16,88), wxDefaultSize, 0, _T("ID_STATICTEXT2"));
	StaticText3 = new wxStaticText(Panel1, ID_STATICTEXT3, _("Nova unidade, passo 1: Senha e Tamanho"), wxPoint(8,8), wxSize(391,19), wxALIGN_CENTRE, _T("ID_STATICTEXT3"));
	wxFont StaticText3Font(12,wxSWISS,wxFONTSTYLE_NORMAL,wxBOLD,false,_T("Sans"),wxFONTENCODING_DEFAULT);
	StaticText3->SetFont(StaticText3Font);
	lblTexto1 = new wxStaticText(Panel1, ID_STATICTEXT4, _("É muito importante utilizar uma senha segura. Evite utilizar palavras\nque possam ser encontradas nos dicionários, ou mesmo,\ncombinações destas palavras. Senhas não devem conter nomes e\nnem datas de nascimento. Não devem ser de fácil adivinhação.\n\nAtenção também ao tamanho selecionado para a unidade de \narquivos protegidos, pois o espaço utilizado aqui será subtraído do \ntotal disponível."), wxPoint(8,176), wxSize(100,56), wxVSCROLL, _T("ID_STATICTEXT4"));
	wxFont lblTexto1Font(8,wxSWISS,wxFONTSTYLE_NORMAL,wxNORMAL,false,_T("Sans"),wxFONTENCODING_DEFAULT);
	lblTexto1->SetFont(lblTexto1Font);
	BtCancelar = new wxBitmapButton(this, ID_BT_CANCELAR, wxBitmap(wxImage(_T("/usr/share/cpa/imagens/cancelar.bmp"))), wxPoint(312,304), wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_CANCELAR"));
	BtAvancar = new wxBitmapButton(this, ID_BT_AVANCAR, wxBitmap(wxImage(_T("/usr/share/cpa/imagens/avancar.jpg"))), wxPoint(216,304), wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_AVANCAR"));
	BtAvancar->SetBitmapDisabled(wxBitmap(wxImage(_T("/usr/share/cpa/imagens/avancar-cinza.jpg"))));
	BtAvancar->SetDefault();
	BtAvancar->Disable();
	BtConcluir = new wxBitmapButton(this, ID_BT_CONCLUIR, wxBitmap(wxImage(_T("/usr/share/cpa/imagens/concluir.jpg"))), wxPoint(120,304), wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_CONCLUIR"));
	Timer1.SetOwner(this, ID_TIMER1);
	Timer1.Start(1, false);

	Connect(ID_TX_SENHA,wxEVT_COMMAND_TEXT_UPDATED,(wxObjectEventFunction)&DlgNovaUnidade::OntxSenhaText);
	Connect(ID_TX_CONFIRMA,wxEVT_COMMAND_TEXT_UPDATED,(wxObjectEventFunction)&DlgNovaUnidade::OnTextCtrl3Text);
	Connect(ID_RADIOBOX1,wxEVT_COMMAND_RADIOBOX_SELECTED,(wxObjectEventFunction)&DlgNovaUnidade::OnRadioBox1Select);
	//Panel1->Connect(ID_PANEL1,wxEVT_PAINT,(wxObjectEventFunction)&DlgNovaUnidade::OnPanel1Paint,0,this);
	Connect(ID_BT_CANCELAR,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&DlgNovaUnidade::OnbtCancelarClick);
	Connect(ID_BT_AVANCAR,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&DlgNovaUnidade::OnBtAvancarClick1);
	Connect(ID_BT_CONCLUIR,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&DlgNovaUnidade::OnBtConcluirClick);
	Connect(ID_TIMER1,wxEVT_TIMER,(wxObjectEventFunction)&DlgNovaUnidade::OnTimer1Trigger);
	Connect(wxID_ANY,wxEVT_INIT_DIALOG,(wxObjectEventFunction)&DlgNovaUnidade::OnInit);
	//*)


	ppnl = new PnlConcluir(this);
}
DlgNovaUnidade::~DlgNovaUnidade()
{
	//(*Destroy(DlgNovaUnidade)
	//*)
	if (ppnl)
	  delete ppnl;
	if(Timer1.IsRunning())
	  Timer1.Stop();
    if(Process)
	 delete Process;
}
void DlgNovaUnidade::OnbtCancelarClick(wxCommandEvent& event)
{
       EndModal(wxID_CANCEL);
}

void DlgNovaUnidade::OnInit(wxInitDialogEvent& event)
{
   ppnl->Hide();
   Panel1->Show();
   ppnl->Panel1->Hide();
   BtConcluir->Hide();
   BtAvancar->Show();

   bIniciou = false;
   iStepGauge = 1;
   if(Timer1.IsRunning())
	  Timer1.Stop();
    Process = 0L;
}
void DlgNovaUnidade::OnTextCtrl3Text(wxCommandEvent& event)
{
    wxString tmp;
    tmp = txSenha->GetValue();

    if  (  (tmp.Len() > 0 ) &&
           (txSenha->GetValue() == txConfirma->GetValue())
        )
    {
        BtAvancar->Enable();
    }
    else
    BtAvancar->Disable();
}
void DlgNovaUnidade::OntxSenhaText(wxCommandEvent& event)
{
    OnTextCtrl3Text(event);
}
void DlgNovaUnidade::OnRadioBox1Select(wxCommandEvent& event)
{
    //RadioBox1->GetSelection()
    if (RadioBox1->GetSelection()==4)
      spTamOutro->Enable();
    else
      spTamOutro->Disable();
}
void DlgNovaUnidade::OnBtAvancarClick1(wxCommandEvent& event)
{
    wxString smsg ("ATENÇÃO. Cuidado para não perder a senha, pois não existe nenhuma maneira de recuperá-la. O acesso aos arquivos, que fazem parte da unidade de arquivos protegidos, depende exclusivamente da senha de acesso.", wxConvUTF8);
    //smsg = wxString::FromUTF8("ATENÇÃO. Cuidado para não esquecer esta senha, pois não existe nenhuma maneira de recuperá-la caso você a esqueça.");
    wxMessageBox(smsg , sCaptionAviso, wxOK, this);
    sSenha = txConfirma->GetValue();
    if(sSenha.Len() < 10)
    {
        smsg = _T("SENHA CURTA. Senhas muito curtas são fáceis de quebrar. Deseja continuar mesmo assim?");
        int answer = wxMessageBox(smsg , sCaptionAviso, wxYES_NO, this);
        if (answer != wxYES)
          return;
    }
    switch(RadioBox1->GetSelection())
    {
        case 0: iTamanho = 2; iStepGauge  = 1; break;
        case 1: iTamanho = 5; iStepGauge  = 1; break;
        case 2: iTamanho = 10;iStepGauge  = 1; break;
        case 3: iTamanho = 20; iStepGauge = 1; break;
        case 4: iTamanho = spTamOutro->GetValue(); iStepGauge = 1; break;
        default: iTamanho = 2; iStepGauge = 1; break;
    }
    ppnl->iRange = iTamanho * 100;
    ppnl->Gauge1->SetRange(ppnl->iRange);
    //Timer1.SetInterval(1000);
    Panel1->Hide();
    BtAvancar->Hide();
    BtConcluir->SetPosition(BtAvancar->GetPosition());
    BtConcluir->Show();

    ppnl->SetPosition(Panel1->GetPosition());
    ppnl->SetSize(Panel1->GetSize());
    smsg = _T("Sua unidade de arquivos protegidos será criada com o tamanho \n");
    smsg += wxString::Format(_T("de %d GB.  Clique em 'Concluir' para finalizar o processo e iniciar\n"), iTamanho);
    smsg += _T("seu uso.");
    ppnl->txLblConcluir->SetLabel(smsg);
    ppnl->Gauge1->SetValue(0);
    ppnl->Show(true);
}

void DlgNovaUnidade::OnBtConcluirClick(wxCommandEvent& event)
{
   ppnl->Panel1->Show();
   mostrarUnidade = ppnl->chkExibirUnidade->IsChecked();
   if (!Process)
     Process = new myProcess();
      //Process = new myProcess(this);
    BtConcluir->Disable();
    BtCancelar->Disable();
    Process->Terminou = false;
    this->Timer1.Start(1000, false);
    CriarUnidade();
}

void DlgNovaUnidade::OnProcessTerm(wxProcessEvent& event)
{
    TerminarProcesso();
}

void DlgNovaUnidade::TerminarProcesso()
{
       Timer1.Stop();
       delete Process;
       Process = 0L;
       BtConcluir->Enable();
       BtCancelar->Enable();
       this->EndModal(wxID_OK);
}
void DlgNovaUnidade::OnTimer1Trigger(wxTimerEvent& event)
{
    this->ppnl->IncGauge(iStepGauge);

    DlgNovaUnidade::Refresh();
    this->ppnl->Refresh();
    this->ppnl->Update();
    if (! wxProcess::Exists(m_pid))
    {
        TerminarProcesso();
    }
}

void DlgNovaUnidade::CriarUnidade(void)
{
        wxString scmdCriarUnidade;

        unsigned long long iMultiplicador = 1073741824LL; //BYTES_PER_GB 1024 * 1024 * 1024
        //unsigned long long iMultiplicador = 1024 * 1024 ; //BYTES PER MB
        //Process = new myProcess();
        //sudo
        scmdCriarUnidade = _T("truecrypt -t -c --filesystem=FAT ");
        scmdCriarUnidade += wxString::Format(_T("--size=%llu "), iTamanho * iMultiplicador);
        scmdCriarUnidade +=_T(" -p ");
        scmdCriarUnidade += wxString::Format(_T("%s"), sSenha.c_str());
        scmdCriarUnidade +=_T(" --volume-type=normal --encryption=AES --hash=RIPEMD-160 --non-interactive ");
        scmdCriarUnidade += sNomeArq;
        //scmdCriarUnidade += _T(" > /dev/null 2>/dev/null");
        //wxMessageBox(scmdCriarUnidade , sNomeSistema);
        //wxExecute(scmdCriarUnidade, wxEXEC_SYNC);
        if (! ChecarEspacoLivre (iTamanho * iMultiplicador))
        {
            wxMessageBox(_T("Espaço em disco insuficiente. Processo interrompido."), sCaptionAviso, wxOK, this);
            return;
        }
        m_pid = wxExecute(scmdCriarUnidade, wxEXEC_ASYNC, Process);

        if ( (!m_pid) || ! wxProcess::Exists(m_pid))
         wxMessageBox(_T("Erro ao executar o processo. Tente novamente."), sCaptionAviso, wxOK, this);

        return;
}

/*
 * Método:		OnPanel1Paint
 * Descrição:	Atualizar o painel
 * Autor:		Gustavo R. de Castro
 * Data:		05/02/2010
 * Entrada:		event - referência para o evento
 */
void DlgNovaUnidade::OnPanel1Paint(wxPaintEvent& event)
{
    this->Refresh();
    this->ppnl->Refresh();
    event.Skip();
}


/*
 * Método: 		ChecarEspacoLivre
 * Descrição: 	Checar espaço livre
 * Autor:		Gustavo R. de Castro
 * Data:		05/02/2010
 * Entrada:		piTamanho - espaço a ser verificado a disponibilidade
 * Saída:		bool - disponível ou não
 */
bool DlgNovaUnidade::ChecarEspacoLivre ( unsigned long long piTamanho)
{
//    static void df(char *s, int always) {
//static int ok = EXIT_SUCCESS;

    struct statfs st;
    int always = 0;
    char *s = "/";
    unsigned long long espacoLivre;
    if (statfs(s, &st) < 0) {
        //fprintf(stderr, "%s: %s\n", s, strerror(errno));
  //      ok = EXIT_FAILURE;
         return false;
    } else {
        if (st.f_blocks == 0 && !always)
            return false;

        //printf("%s: %lldK total, %lldK used, %lldK available (block size %d)\n",
        //       s,
        //       ((long long)st.f_blocks * (long long)st.f_bsize) / 1024,
        //       ((long long)(st.f_blocks - (long long)st.f_bfree) * st.f_bsize) / 1024,
        //       ((long long)st.f_bfree * (long long)st.f_bsize) / 1024,
        //       (int) st.f_bsize);

        espacoLivre = ((long long)st.f_bfree * (long long)st.f_bsize) / 1024 ;
        piTamanho = piTamanho / 1024;
        return espacoLivre * 0.7 > piTamanho;
    }
//}

}
