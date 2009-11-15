//
// $Id$
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2009 by Tom Lechner
//

#include "reimpose.h"
#include "../language.h"
#include "../laidout.h"

/*! \ingroup api */
StyleDef *makeReimposeStyleDef()
{
	 //define base
	StyleDef *sd=new StyleDef(NULL,"Reimpose",
			_("Reimpose"),
			_("Replace a document's imposition"),
			Element_Function,
			NULL,NULL,
			NULL,
			0, //new flags
			NULL,
			ReImposeFunction);

	 //define parameters
	sd->push("document",
			_("Document"),
			_("The document to reimpose. Pulled from context if none given"),
			Element_String, //major hack shortcut
			NULL, //range
			"0",  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("imposition",
			_("New Imposition"),
			_("The new imposition to use"),
			Element_String, //Element_DynamicEnum, ***
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("scalepages",
			_("Scale Pages"),
			_("Whether to scale the old pages to fit the new pages"),
			Element_Boolean,
			NULL,
			"yes",
			0,NULL);
	sd->push("papersize",
			_("Paper Size"),
			_("New paper size to use in the new imposition. Defaults to the old paper size."),
			Element_String,
			NULL,
			NULL,
			0,NULL);
	sd->push("createnew",
			_("Create new"),
			_("Reimpose to a new document"),
			Element_Boolean,
			NULL,
			"no",
			0,NULL);

	return sd;
}

//! General function to reimpose a document.
/*! \ingroup api
 *
 * Return 0 for success, -1 for success with warnings, or 1 for unredeemable failure.
 *
 * Something like "doc=Document(); doc.Reimpose(...)" would not go through this function
 * but instead call Document::FunctionCall("Reimpose",context,parameters).
 *
 * parameters MUST have the exact order of available parameters as specified in the
 * StyleDef for this function. This is normally done in Calculator::parseParameters().
 *
 * \todo *** this is major hack stage, needs to be automated more, with automatic parsing
 *    based on StyleDef objects
 */
int ReImposeFunction(ValueHash *context, 
					 ValueHash *parameters,
					 Value **value_ret,
					 char **error_ret)
{
	 //grab document from context, and call doc->reimpose() using parameters from the styledef;
	if (!parameters) return 1;
	if (!context) return 1;

	 //we need:
	 //  1. a document
	 //  2. a new imposition object
	 //  3. optional new paper size
	 //  4. whether to scale pages if they don't fit the new papersize
	 //  5. whether to impose to a new document

//--------- in an ideal world, it would be as easy as:
//	 //1.
//	Document *doc=dynamic_cast<Document*>(parameters->findObject(NULL,0));
//	if (!doc) doc=dynamic_cast<Document*>context->findObject("document");
//	if (!doc) return 1;
//
//	 //2.
//	Imposition *imp=NULL;
//	imp=dynamic_cast<Imposition*>(parameters->findObject(NULL,1));
//	if (!imp) return 2;
//
//	 //3. 
//	int scalepages=parameters->findInt(NULL,2);
//
//	 //4.
//	PaperStyle *paper=dynamic_cast<PaperStyle*>(parameters->findObject(NULL,3));
//
//
//	if (paper) imp->SetPaperSize(paper);
//	if (doc->ReImpose(imp,scalepages)==0) return 0;
//	return 10; //error reimposing!
//----------------------------------

	 //1.
	const char *str=NULL;

	Document *doc=NULL;
	str=parameters->findString("document");
	if (str) {
		 //look in parameters
		if (!str) return 2; //no doc to reimpose!
		int c;
		for (c=0; c<laidout->project->docs.n; c++) {
			if (!strcmp(str,laidout->project->docs.e[c]->filename)) break;
		}
		if (c==laidout->project->docs.n) return 3; //doc not found!
		doc=laidout->project->docs.e[c]->doc;
	}
	if (!doc) doc=dynamic_cast<Document*>(context->findObject("document"));
	if (!doc) return 1; //no doc to impose!

	 //2.
	Imposition *imp=NULL;
	str=parameters->findString("imposition");
	if (!str) return 4; //no new imposition!
	int c2;
	for (c2=0; c2<laidout->impositionpool.n; c2++) {
		if (!strncasecmp(str,laidout->impositionpool.e[c2]->Stylename(),strlen(laidout->impositionpool.e[c2]->Stylename()))) {
			imp=laidout->impositionpool.e[c2];
			break;
		}
	}
	if (!imp || c2==laidout->impositionpool.n) return 5; //no imposition to use!
	imp=(Imposition *)imp->duplicate();

	 //3.
	PaperStyle *paper=NULL;
	str=parameters->findString("papersize");
	if (!str) {
		//***use a papersize that is big enough to fit;
		cout <<"*** must implement auto compute paper size for reimposition"<<endl;
		//figure out w and h of current document
		//step through list of papers, find one that will fit either WxH or HxW
		//if no fit, then custom size
	} else {
		 //establish papersize;
		for (c2=0; c2<laidout->papersizes.n; c2++) {
			if (!strncasecmp(str,laidout->papersizes.e[c2]->name,strlen(laidout->papersizes.e[c2]->name))) {
				paper=laidout->papersizes.e[c2];
				break;
			}
		}
		if (paper) imp->SetPaperSize(paper); // makes a duplicate of paper
	}
	if (!paper) { return 5; delete imp; }

	 //4.
	//***scale pages to fit
	int scalepages=parameters->findInt("scalepages");

	int c=doc->ReImpose(imp, scalepages);

	if (value_ret) *value_ret=NULL;
	return c;
}


