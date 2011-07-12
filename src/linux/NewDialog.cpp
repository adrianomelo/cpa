/*
 * ------------------------------------------------
 * Copyright © 2010 Itautec S/A
 * ------------------------------------------------
 * Produto:		CPA
 * Módulo:		NewDialog.cpp
 * Descrição:	Implementação para criação de uma nova janela de diálogo
 * Data:		05/02/2010
 * Autor:		Gustavo R. de Castro
 *-------------------------------------------------
 */

#include "NewDialog.h"

//(*InternalHeaders(NewDialog)
#include <wx/intl.h>
#include <wx/string.h>


BEGIN_EVENT_TABLE(NewDialog,wxDialog)
	//(*EventTable(NewDialog)
	//*)
END_EVENT_TABLE()


/*
 * Método:		NewDialog
 * Descrição:	Construtor da classe
 * Data:		05/02/2010
 * Autor:		Gustavo R. de Castro
 * Entrada:		parent - referência para a janela pai
 * 				id - id da janela
 * 				pos - posição da janela
 * 				size - tamanho da janelas
 */
NewDialog::NewDialog(wxWindow* parent,wxWindowID id,const wxPoint& pos,const wxSize& size)
{
	//(*Initialize(NewDialog)
	Create(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE, _T("id"));
	SetClientSize(wxDefaultSize);
	Move(wxDefaultPosition);
	//*)
}

/*
 * Método:		~NewDialog
 * Descrição:	Destrutor da classe
 * Data:		05/02/2010
 * Autor:		Gustavo R. de Castro
 */
NewDialog::~NewDialog()
{
	//(*Destroy(NewDialog)
	//*)
}

