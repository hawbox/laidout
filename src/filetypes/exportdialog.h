//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <lax/lineedit.h>
#include <lax/checkbox.h>
#include <lax/messagebox.h>

#include "filefilters.h"


#define EXPORT_COMMAND (1<<16)
	
//---------------------------- ConfigEventData -------------------------
class ConfigEventData : public Laxkit::EventData
{
 public:
	DocumentExportConfig *config;
	ConfigEventData(DocumentExportConfig *c);
	virtual ~ConfigEventData();
};

//---------------------------- ExportDialog -------------------------
class ExportDialog : public Laxkit::RowFrame
{
 protected:
	DocumentExportConfig *config;
	
	int overwriteok;
	unsigned long dialog_style;
	int tofile, cur, max, min;
	Laxkit::LineEdit *fileedit,*filesedit,*printstart,*printend,*command;
	Laxkit::CheckBox *filecheck,*filescheck,*commandcheck;
	Laxkit::CheckBox *printall,*printcurrent,*printrange;
	ExportFilter *filter;

	virtual void changeTofile(int t);
	virtual void changeRangeTarget(int t);
	virtual void configBounds();
	virtual void overwriteCheck();
	virtual int send();

	virtual void start(int s);
	virtual int  start();
	virtual void end(int e);
	virtual int  end();
	virtual void findMinMax();
	virtual int updateExt();
 public:
	ExportDialog(unsigned long nstyle,Window nowner,const char *nsend,
				 Document *doc, ExportFilter *nfilter, const char *file, int layout,
				 int pmin, int pmax, int pcur);
	virtual ~ExportDialog();
	virtual int init();
	virtual int CharInput(unsigned int ch, unsigned int state);
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
	virtual int DataEvent(Laxkit::EventData *data,const char *mes);
};



#endif
