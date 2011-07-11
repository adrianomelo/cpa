/***************************************************************
 * Name:      XCPAMain.h
 * Purpose:   Defines Application Frame
 * Author:    gustavo ()
 * Created:   2009-02-02
 * Copyright: gustavo ()
 * License:
 **************************************************************/

#ifndef XCPAMAIN_H
#define XCPAMAIN_H

#include "DlgNovaUnidade.h"

//(*Headers(XCPADialog)
#include <wx/sizer.h>
#include <wx/bmpbuttn.h>
#include <wx/statbmp.h>
#include <wx/dialog.h>
//*)

const wxString sNomeSistema   = _T("CPA - Central de proteÃ§Ã£o de arquivos");
const wxString sNomeArquivo   = _T("/home/gustavo/UnidadeSegura.arq");

const wxString sPontoMontagem = _T("//media//UnidadeSegura");

class XCPADialog: public wxDialog
{
    public:

        XCPADialog(wxWindow* parent,wxWindowID id = -1);
        virtual ~XCPADialog();

        // the PID of the last process we launched asynchronously
        long m_pidLast;

    private:

        //(*Handlers(XCPADialog)
        void OnQuit(wxCommandEvent& event);
        void OnAcessar(wxCommandEvent& event);
        void OnFechar(wxCommandEvent& event);
        void OnAlterar(wxCommandEvent& event);
        void OnRemover(wxCommandEvent& event);
        void OnAjuda(wxCommandEvent& event);
        void OnBtAlterarSenhaClick(wxCommandEvent& event);
        void OnBtRemoverUnidadeClick(wxCommandEvent& event);
        void OnBtAcessarClick1(wxCommandEvent& event);
        void OnBtFecharClick1(wxCommandEvent& event);
        void OnBtRemoverClick(wxCommandEvent& event);
        void OnBtAjudaClick1(wxCommandEvent& event);
        void OnBtCancelarClick(wxCommandEvent& event);
        void OnBtAlterarSenhaClick1(wxCommandEvent& event);
        void OnInit(wxInitDialogEvent& event);
        //*)

        //(*Identifiers(XCPADialog)
        static const long ID_STATICBITMAP1;
        static const long ID_BT_ACESSAR;
        static const long ID_BT_FECHAR;
        static const long ID_BT_ALTERARSENHA;
        static const long ID_BT_REMOVER;
        static const long ID_BT_AJUDA;
        static const long ID_BT_CANCELAR;
        //*)

        //(*Declarations(XCPADialog)
        wxBitmapButton* BtAlterarSenha;
        wxStaticBitmap* StaticBitmap1;
        wxBitmapButton* BtFechar;
        wxBoxSizer* BoxSizer2;
        wxBitmapButton* BtRemover;
        wxBoxSizer* BoxSizer1;
        wxBitmapButton* BtAjuda;
        wxBitmapButton* BtCancelar;
        wxBitmapButton* BtAcessar;
        //*)

        void HabilitarBotoes();
        bool UnidadeCriada();
        bool UnidadeMontada();
        bool DoAsyncExec(const wxString& cmd);
        wxString MontarCmdMontarUnidade();
        wxString sSenha;

        DECLARE_EVENT_TABLE()

};

#endif // XCPAMAIN_H
