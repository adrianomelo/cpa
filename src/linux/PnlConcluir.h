#ifndef PNLCONCLUIR_H
#define PNLCONCLUIR_H

//(*Headers(PnlConcluir)
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/gauge.h>
//*)

class PnlConcluir: public wxPanel
{
	public:

		PnlConcluir(wxWindow* parent,wxWindowID id=wxID_ANY,const wxPoint& pos=wxDefaultPosition,const wxSize& size=wxDefaultSize);
		virtual ~PnlConcluir();

		//(*Declarations(PnlConcluir)
		wxGauge* Gauge1;
		wxCheckBox* CheckBox3;
		wxCheckBox* CheckBox2;
		wxPanel* Panel1;
		wxStaticText* StaticText1;
		wxStaticText* StaticText3;
		wxStaticText* txLblConcluir;
		wxCheckBox* chkExibirUnidade;
		wxPanel* Panel2;
		//*)

	protected:

		//(*Identifiers(PnlConcluir)
		static const long ID_STATICTEXT1;
		static const long ID_STATICTEXT3;
		static const long ID_GAUGE1;
		static const long ID_PANEL1;
		static const long ID_STATICTEXT2;
		static const long ID_CHECKBOX1;
		static const long ID_chkExibirUnidade;
		static const long ID_PANEL2;
		//*)

	private:

		//(*Handlers(PnlConcluir)
		void OnButton1Click(wxCommandEvent& event);
		//*)
	public:
	  void IncGauge(int step);
	  int position;
	  int iRange;


		DECLARE_EVENT_TABLE()
};

#endif
