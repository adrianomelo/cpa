#ifndef DLGNOVAUNIDADE_H
#define DLGNOVAUNIDADE_H

#include "PnlConcluir.h"
#include "myProcess.h"

//(*Headers(DlgNovaUnidade)
#include <wx/stattext.h>
#include <wx/radiobox.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/panel.h>
#include <wx/bmpbuttn.h>
#include <wx/dialog.h>
#include <wx/timer.h>
//*)

class DlgNovaUnidade: public wxDialog
{
	public:

		DlgNovaUnidade(wxWindow* parent,wxWindowID id=wxID_ANY,const wxPoint& pos=wxDefaultPosition,const wxSize& size=wxDefaultSize);
		virtual ~DlgNovaUnidade();

		//(*Declarations(DlgNovaUnidade)
		wxStaticText* StaticText2;
		wxPanel* Panel1;
		wxStaticText* StaticText1;
		wxStaticText* StaticText3;
		wxBitmapButton* BtAvancar;
		wxTextCtrl* txConfirma;
		wxStaticText* lblTexto1;
		wxSpinCtrl* spTamOutro;
		wxRadioBox* RadioBox1;
		wxTextCtrl* txSenha;
		wxBitmapButton* BtConcluir;
		wxBitmapButton* BtCancelar;
		wxTimer Timer1;
		//*)

        wxString sSenha;
        bool mostrarUnidade;

	protected:

		//(*Identifiers(DlgNovaUnidade)
		static const long ID_TX_SENHA;
		static const long ID_TX_CONFIRMA;
		static const long ID_RADIOBOX1;
		static const long ID_SP_TAM_OUTRO;
		static const long ID_STATICTEXT1;
		static const long ID_STATICTEXT2;
		static const long ID_STATICTEXT3;
		static const long ID_STATICTEXT4;
		static const long ID_PANEL1;
		static const long ID_BT_CANCELAR;
		static const long ID_BT_AVANCAR;
		static const long ID_BT_CONCLUIR;
		static const long ID_TIMER1;
		//*)

		void OnProcessTerm(wxProcessEvent& event);

	private:

		//(*Handlers(DlgNovaUnidade)
		void OnBtAvancarClick(wxCommandEvent& event);
		void OnbtCancelarClick(wxCommandEvent& event);
		//void OnButton1Click(wxCommandEvent& event);
		void OnInit(wxInitDialogEvent& event);
		void OnTextCtrl3Text(wxCommandEvent& event);
		void OntxSenhaText(wxCommandEvent& event);
		void OnRadioBox1Select(wxCommandEvent& event);
		void OnBtAvancarClick1(wxCommandEvent& event);
		void OnBtConcluirClick(wxCommandEvent& event);
		void OnTimer1Trigger(wxTimerEvent& event);
		void OnPanel1Paint(wxPaintEvent& event);
		//*)

        bool ChecarEspacoLivre(unsigned long long itamanho);
        void CriarUnidade(void);
        bool bIniciou;

		int iStepGauge;
        int iTamanho;
        void TerminarProcesso(void);
        PnlConcluir * ppnl;
        myProcess * Process;
        long m_pid;
    public:
        wxString sNomeArq;
        wxString sPontoMontagem;

		DECLARE_EVENT_TABLE()
};

#endif
