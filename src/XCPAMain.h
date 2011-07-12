/***************************************************************
 * Name:      XCPAMain.cpp
 * Purpose:   Code for Application Frame
 * Author:    gustavo ()
 * Created:   2009-02-02
 * Copyright: gustavo ()
 * License:
 **************************************************************/

#include "XCPAMain.h"
#include <wx/msgdlg.h>
#include <wx/dir.h>
#include "DlgAcessarunidade.h"
#include "DlgNovaUnidade.h"
#include <stdlib.h>
#include <wx/bitmap.h>
#include "DlgTrocarSenha.h"

//(*InternalHeaders(XCPADialog)
#include <wx/bitmap.h>
#include <wx/intl.h>
#include <wx/image.h>
#include <wx/string.h>
//*)

//helper functions
enum wxbuildinfoformat {
    short_f, long_f };

wxString wxbuildinfo(wxbuildinfoformat format)
{
    wxString wxbuild(wxVERSION_STRING);

    if (format == long_f )
    {
#if defined(__WXMSW__)
        wxbuild << _T("-Windows");
#elif defined(__UNIX__)
        wxbuild << _T("-Linux");
#endif

#if wxUSE_UNICODE
        wxbuild << _T("-Unicode build");
#else
        wxbuild << _T("-ANSI build");
#endif // wxUSE_UNICODE
    }

    return wxbuild;
}

//(*IdInit(XCPADialog)
const long XCPADialog::ID_STATICBITMAP1 = wxNewId();
const long XCPADialog::ID_BT_ACESSAR = wxNewId();
const long XCPADialog::ID_BT_FECHAR = wxNewId();
const long XCPADialog::ID_BT_ALTERARSENHA = wxNewId();
const long XCPADialog::ID_BT_REMOVER = wxNewId();
const long XCPADialog::ID_BT_AJUDA = wxNewId();
const long XCPADialog::ID_BT_CANCELAR = wxNewId();
//*)

BEGIN_EVENT_TABLE(XCPADialog,wxDialog)
    //(*EventTable(XCPADialog)
    //*)
END_EVENT_TABLE()


XCPADialog::XCPADialog(wxWindow* parent,wxWindowID id)
{
    //(*Initialize(XCPADialog)
    wxBoxSizer* BoxSizer3;
    
    Create(parent, wxID_ANY, _("Central de proteÃ§Ã£o de arquivos"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE, _T("wxID_ANY"));
    SetClientSize(wxSize(579,387));
    BoxSizer1 = new wxBoxSizer(wxHORIZONTAL);
    StaticBitmap1 = new wxStaticBitmap(this, ID_STATICBITMAP1, wxBitmap(wxImage(_T("/home/gustavo/XCPA/imagens/ilustracao.jpg")).Rescale(wxSize(232,286).GetWidth(),wxSize(232,286).GetHeight())), wxDefaultPosition, wxSize(232,286), 0, _T("ID_STATICBITMAP1"));
    BoxSizer1->Add(StaticBitmap1, 1, wxALL|wxALIGN_TOP|wxALIGN_CENTER_HORIZONTAL, 5);
    BoxSizer2 = new wxBoxSizer(wxVERTICAL);
    BtAcessar = new wxBitmapButton(this, ID_BT_ACESSAR, wxBitmap(wxImage(_T("/home/gustavo/XCPA/imagens/acessar-pasta.jpg"))), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_ACESSAR"));
    BtAcessar->SetBitmapDisabled(wxBitmap(wxImage(_T("/home/gustavo/XCPA/imagens/acessar-pasta_cinza.jpg"))));
    BtAcessar->SetDefault();
    BoxSizer2->Add(BtAcessar, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    BtFechar = new wxBitmapButton(this, ID_BT_FECHAR, wxBitmap(wxImage(_T("/home/gustavo/XCPA/imagens/fechar-unidade.jpg"))), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_FECHAR"));
    BtFechar->SetBitmapDisabled(wxBitmap(wxImage(_T("/home/gustavo/XCPA/imagens/fechar-unidade_cinza.jpg"))));
    BoxSizer2->Add(BtFechar, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    BtAlterarSenha = new wxBitmapButton(this, ID_BT_ALTERARSENHA, wxBitmap(wxImage(_T("/home/gustavo/XCPA/imagens/alterar-senha.jpg"))), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_ALTERARSENHA"));
    BtAlterarSenha->SetBitmapDisabled(wxBitmap(wxImage(_T("/home/gustavo/XCPA/imagens/alterar-senha_cinza.jpg"))));
    BoxSizer2->Add(BtAlterarSenha, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    BtRemover = new wxBitmapButton(this, ID_BT_REMOVER, wxBitmap(wxImage(_T("/home/gustavo/XCPA/imagens/remover-unidade.jpg"))), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_REMOVER"));
    BtRemover->SetBitmapDisabled(wxBitmap(wxImage(_T("/home/gustavo/XCPA/imagens/remover-unidade_cinza.jpg"))));
    BtRemover->SetDefault();
    BoxSizer2->Add(BtRemover, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    BtAjuda = new wxBitmapButton(this, ID_BT_AJUDA, wxBitmap(wxImage(_T("/home/gustavo/XCPA/imagens/botao-ajuda2.jpg"))), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_AJUDA"));
    BoxSizer2->Add(BtAjuda, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    BoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    BtCancelar = new wxBitmapButton(this, ID_BT_CANCELAR, wxBitmap(wxImage(_T("/home/gustavo/XCPA/imagens/cancelar.bmp"))), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BT_CANCELAR"));
    BtCancelar->SetDefault();
    BoxSizer3->Add(BtCancelar, 1, wxALL|wxALIGN_RIGHT|wxALIGN_BOTTOM, 0);
    BoxSizer2->Add(BoxSizer3, 1, wxALL|wxALIGN_RIGHT|wxALIGN_BOTTOM, 5);
    BoxSizer1->Add(BoxSizer2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 4);
    SetSizer(BoxSizer1);
    BoxSizer1->SetSizeHints(this);
    
    Connect(ID_BT_ACESSAR,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&XCPADialog::OnBtAcessarClick1);
    Connect(ID_BT_FECHAR,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&XCPADialog::OnBtFecharClick1);
    Connect(ID_BT_ALTERARSENHA,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&XCPADialog::OnBtAlterarSenhaClick1);
    Connect(ID_BT_REMOVER,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&XCPADialog::OnBtRemoverClick);
    Connect(ID_BT_AJUDA,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&XCPADialog::OnBtAjudaClick1);
    Connect(ID_BT_CANCELAR,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&XCPADialog::OnBtCancelarClick);
    Connect(wxID_ANY,wxEVT_INIT_DIALOG,(wxObjectEventFunction)&XCPADialog::OnInit);
    //*)
}

XCPADialog::~XCPADialog()
{
    //(*Destroy(XCPADialog)
    //*)
}

void XCPADialog::OnQuit(wxCommandEvent& event)
{
    Close();
}

void XCPADialog::OnBtAcessarClick1(wxCommandEvent& event)
{
    int code;
    bool bCriou = false;
    wxString scmdMontarUnidade;
    wxString smsg;

    if (! wxFileExists(sNomeArquivo))
    {
        DlgNovaUnidade  dlgNova(0);
        dlgNova.SetTitle(sNomeSistema);
        if (dlgNova.ShowModal() == wxID_OK)
        {
            bCriou = true;
            sSenha = dlgNova.sSenha;
        }
        else
        {
            dlgNova.Destroy();
            return;
        }
        dlgNova.Destroy();
    }

    if (!bCriou)
    {
      DlgAcessarunidade dlg(0);
      dlg.SetTitle(sNomeSistema);
      if (dlg.ShowModal() != wxID_OK)
        return;
      sSenha = dlg.edSenha->GetValue();
    }

    //mkdir( (char *) sPontoMontagem.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
    mkdir( sPontoMontagem.mb_str(), S_IRWXU | S_IRWXG | S_IRWXO );

    scmdMontarUnidade = MontarCmdMontarUnidade();
    code = wxExecute(scmdMontarUnidade, wxEXEC_SYNC);

    if (code)
    {
      rmdir( sPontoMontagem.mb_str() );
      smsg = _T("Erro ao acessar unidade, Tente novamente. Processo ");
      //smsg += wxString::Format("%s", scmdMontarUnidade.c_str());
      smsg += _T(" terminou com cÃ³digo de saÃ­da: ");
      smsg += wxString::Format(_T("%d"), code);
    }
    else
    {
      smsg = _T("Unidade de arquivos seguros ");
      if (bCriou)
        smsg += _T(" criada e ");
      smsg += _T(" montada com sucesso em: ");
      smsg += sPontoMontagem;

    }
    wxMessageBox(smsg , sNomeSistema);
    HabilitarBotoes();
}

void XCPADialog::OnBtFecharClick1(wxCommandEvent& event)
{
    wxString smsg;
    int code = 0;
    wxDir * d = new wxDir();

    if (d->Exists(sPontoMontagem))
    {
        wxString scmd = _T("sudo truecrypt -t -d --non-interactive ");
        scmd += sPontoMontagem;
        code = wxExecute(scmd, wxEXEC_SYNC);
        rmdir( sPontoMontagem.mb_str() );
    }
    smsg = (code)?_T("Erro ao fechar unidade. Feche todos os arquios e tente novamente."):_T("Unidade liberada com sucesso!");
    wxMessageBox(smsg, sNomeSistema);
    delete d;
    HabilitarBotoes();
}

void XCPADialog::OnBtRemoverClick(wxCommandEvent& event)
{
    if (wxFileExists(sNomeArquivo))
    {
      std::remove(sNomeArquivo.mb_str());
      wxMessageBox(_T("Unidade de arquivos protegidos removida com sucesso."), sNomeSistema);
    }
    HabilitarBotoes();
}

void XCPADialog::OnBtAjudaClick1(wxCommandEvent& event)
{
    //Shelexec
    wxMessageBox(_("Ajuda"), sNomeSistema);
}

void XCPADialog::OnBtCancelarClick(wxCommandEvent& event)
{
    Close();
}

void XCPADialog::OnBtAlterarSenhaClick1(wxCommandEvent& event)
{

   wxString smsg;
   int code;
//#if defined(__WXMSW__) || defined(__WXPM__)
//    wxBitmap bmp("IDB_BITMAP1", wxBITMAP_TYPE_RESOURCE);
//#else // Unix
//    wxBitmap bmp(IDB_BITMAP1, wxBITMAP_TYPE_XPM);
//#endif

//    wxBitmap bmp(".//imagens//acessar-pasta.xpm", wxBITMAP_TYPE_XPM);
//    BtRemover->SetBitmapLabel(bmp);

    DlgTrocarSenha dlg(0);
    dlg.SetTitle(sNomeSistema);
    if (dlg.ShowModal() != wxID_OK)
        return;

    wxString sSenhaAtual = dlg.TxSenhaAtual->GetValue();
    wxString sNovaSenha = dlg.TxNovaSenha->GetValue();

    wxString scmd = _T("sudo truecrypt -t -C -p ");
    scmd +=  sSenhaAtual;
    scmd += _T(" --new-password=");
    scmd += sNovaSenha;
    scmd += _T(" --non-interactive ");
    scmd += sNomeArquivo;
    code = wxExecute(scmd, wxEXEC_SYNC);
    if (code)
    {
      smsg = _T("Erro ao alterar a senha da unidade. Processo ");
      //smsg += wxString::Format(_T("%s"), scmd.c_str());
      smsg += _T(" terminou com cÃ³digo de saÃ­da: ");
      smsg += wxString::Format(_T("%d"), code);
    }
    else
      smsg = _T("Senha alterada com sucesso!");

    wxMessageBox(smsg , sNomeSistema);
}

void XCPADialog::OnInit(wxInitDialogEvent& event)
{

  HabilitarBotoes();
}

wxString XCPADialog::MontarCmdMontarUnidade()
{
    wxString scmd;
    int temp;
//    wxString tmp (spPontoMontagem, wxConvUTF8);

    temp = umask(0);

    //mkdir( (char *) sPontoMontagem.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
    //mkdir( sPontoMontagem.mb_str(), S_IRWXU | S_IRWXG | S_IRWXO );

    scmd = _T("sudo truecrypt -t --mount -p ");
    scmd += sSenha;
    scmd += _T(" --non-interactive ");
    scmd += sNomeArquivo;
    scmd += _T(" ");
    scmd += sPontoMontagem;
    return scmd;

}

bool XCPADialog::UnidadeMontada()
{
  wxDir * d = new wxDir();
  bool bUnidadeMontada;

    //melhorar, trcar pelo truecrypt -l e ver codigo de retorno.
    bUnidadeMontada = d->Exists(sPontoMontagem);
    delete d;

    return bUnidadeMontada;
}

bool XCPADialog::UnidadeCriada()
{
    return wxFileExists(sNomeArquivo);
}

void XCPADialog::HabilitarBotoes()
{
    if (UnidadeCriada())
    {
        if (UnidadeMontada()) //criada e montada
        {
            BtAcessar->Disable();
            BtFechar->Enable();
            BtAlterarSenha->Disable();
            BtRemover->Disable();
        }
        else //criada mas nÃ£o montada
        {
            BtAcessar->Enable();
            BtFechar->Disable();
            BtAlterarSenha->Enable();
            BtRemover->Enable();
        }
    }
    else
    {
            BtAcessar->Enable();
            BtFechar->Disable();
            BtAlterarSenha->Disable();
            BtRemover->Disable();
    }
    return;
}

/*
bool XCPA::Dialog UnidadeVazia()
{
    return true;
}
*/


