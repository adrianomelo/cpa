#ifndef DLGACESSARUNIDADE_H
#define DLGACESSARUNIDADE_H

//(*Headers(DlgAcessarunidade)
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/bmpbuttn.h>
#include <wx/dialog.h>
//*)

class DlgAcessarunidade: public wxDialog
{
	public:

		DlgAcessarunidade(wxWindow* parent);
		virtual ~DlgAcessarunidade();

		//(*Declarations(DlgAcessarunidade)
		wxBitmapButton* btCancelar;
		wxStaticText* StaticText2;
		wxTextCtrl* edSenha;
		wxCheckBox* chkExibirUnidade;
		wxStaticText* lblTitulo;
		wxBitmapButton* BtOk;
		//*)

	protected:

		//(*Identifiers(DlgAcessarunidade)
		static const long ID_LBL_TITULO;
		static const long ID_STATICTEXT2;
		static const long ID_TXT_SENHA;
		static const long ID_BT_CANCELAR;
		static const long ID_BT_OK;
		static const long ID_chkExibirUnidade;
		//*)

	private:

		//(*Handlers(DlgAcessarunidade)
		void OnQuit(wxCommandEvent& event);
		void OnBitmapButton1Click(wxCommandEvent& event);
		void OnBtOkClick(wxCommandEvent& event);
		void OnBtOkClick1(wxCommandEvent& event);
		void OnInit(wxInitDialogEvent& event);
		void OnedSenhaText(wxCommandEvent& event);
		//*)

		DECLARE_EVENT_TABLE()
};

#endif
