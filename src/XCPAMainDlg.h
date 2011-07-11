#ifndef XCPAMAINDLG_H
#define XCPAMAINDLG_H

//(*Headers(XCPAMainDlg)
#include <wx/statline.h>
#include <wx/panel.h>
#include <wx/bmpbuttn.h>
#include <wx/statbmp.h>
#include <wx/dialog.h>
//*)

const wxString sNomeSistema ( _T("Central de proteção de arquivos"), wxConvUTF8);  // = _T("Central de protecao de arquivos");
const wxString sCaptionAviso   = _T("CPA - Aviso do sistema");
//const wxString sPontoMontagem = _T("/media/UnidadeSegura");
//wxString sPontoMontagem;// = _T("/home/gustavo/UnidadeSegura");

class XCPAMainDlg: public wxDialog
{
	public:

		XCPAMainDlg(wxWindow* parent,wxWindowID id=wxID_ANY,const wxPoint& pos=wxDefaultPosition,const wxSize& size=wxDefaultSize);
		virtual ~XCPAMainDlg();

		//(*Declarations(XCPAMainDlg)
		wxBitmapButton* BtAcessarUnidade;
		wxBitmapButton* BtAlterarSenha;
		wxStaticBitmap* StaticBitmap1;
		wxBitmapButton* BtFechar;
		wxPanel* Panel1;
		wxBitmapButton* BtRemover;
		wxBitmapButton* BitmapButton6;
		wxStaticLine* StaticLine1;
		wxBitmapButton* BtAjuda;
		//*)

	protected:

		//(*Identifiers(XCPAMainDlg)
		static const long ID_STATICBITMAP1;
		static const long ID_BT_ACESSAR;
		static const long ID_BT_FECHAR;
		static const long ID_BT_ALTERARSENHA;
		static const long ID_BT_REMOVER;
		static const long ID_BT_AJUDA;
		static const long ID_PANEL1;
		static const long ID_STATICLINE1;
		static const long ID_BITMAPBUTTON6;
		//*)

	private:

		//(*Handlers(XCPAMainDlg)
		void OnbtAcessarUnidadeClick(wxCommandEvent& event);
		void OnBtFecharClick(wxCommandEvent& event);
		void OnBtAlterarSenhaClick(wxCommandEvent& event);
		void OnBtRemoverClick(wxCommandEvent& event);
		void OnBtAjudaClick(wxCommandEvent& event);
		void OnBitmapButton6Click(wxCommandEvent& event);
		void OnInit(wxInitDialogEvent& event);
		void OnPanel1Paint(wxPaintEvent& event);
		//*)

		void HabilitarBotoes();
        bool UnidadeCriada();
        bool UnidadeMontada();
        bool MontarUnidade (bool bPedirSenha, bool bRemover, bool *montarUnidade);
        bool FecharUnidade (bool bMensagem);
        bool UnidadeVazia(void);
        void MostrarUnidade(void);
        wxString MontarCmdMontarUnidade();
        wxString sSenha;
        bool mostrarUnidade;
public:
        wxString sNomeArq;
        wxString sPontoMontagem;
        void OnClose (wxCloseEvent& e);
		DECLARE_EVENT_TABLE()
};

#endif
