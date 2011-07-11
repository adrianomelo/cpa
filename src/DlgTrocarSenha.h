#ifndef DLGTROCARSENHA_H
#define DLGTROCARSENHA_H

//(*Headers(DlgTrocarSenha)
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/dialog.h>
//*)

class DlgTrocarSenha: public wxDialog
{
	public:

		DlgTrocarSenha(wxWindow* parent,wxWindowID id=wxID_ANY);
		virtual ~DlgTrocarSenha();

		//(*Declarations(DlgTrocarSenha)
		wxStaticText* StaticText2;
		wxStaticText* StaticText6;
		wxStaticText* StaticText1;
		wxStaticText* StaticText3;
		wxTextCtrl* TxSenhaAtual;
		wxStaticText* StaticText5;
		wxBitmapButton* BtSair;
		wxBitmapButton* BtOK;
		wxStaticText* lblAlertaSenha;
		wxStaticText* StaticText4;
		wxTextCtrl* TxConfirmarNovaSenha;
		wxTextCtrl* TxNovaSenha;
		//*)

	protected:

		//(*Identifiers(DlgTrocarSenha)
		static const long ID_TX_SENHAATUAL;
		static const long ID_TX_NOVASENHA;
		static const long ID_TX_CONFIRMARNOVASENHA;
		static const long ID_STATICTEXT1;
		static const long ID_STATICTEXT2;
		static const long ID_STATICTEXT3;
		static const long ID_BT_OK;
		static const long ID_BT_SAIR;
		static const long ID_STATICTEXT4;
		static const long ID_STATICTEXT6;
		static const long ID_STATICTEXT7;
		static const long ID_STATICTEXT5;
		//*)

	private:

		//(*Handlers(DlgTrocarSenha)
		void OnInit(wxInitDialogEvent& event);
		void OnBtSairClick(wxCommandEvent& event);
		void OnBtOKClick(wxCommandEvent& event);
		void OnTxConfirmarNovaSenhaText(wxCommandEvent& event);
		void OnTxNovaSenhaText(wxCommandEvent& event);
		void OnTxSenhaAtualText(wxCommandEvent& event);
		//*)

        bool ExecMudarSenha();
    public:
        wxString sNomeArq;
        wxString sPontoMontagem;
		DECLARE_EVENT_TABLE()
};

#endif
