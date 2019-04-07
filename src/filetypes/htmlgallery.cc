//
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2019 by Tom Lechner
//


#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>
#include <lax/fileutils.h>
#include <lax/utf8string.h>

#include "../language.h"
#include "../laidout.h"
#include "../stylemanager.h"
#include "../printing/psout.h"
#include "htmlgallery.h"
#include "../impositions/singles.h"
#include "../drawdata.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



namespace Laidout {

//
//
// This will render spreads to images, and output an index.html and a json description file.
// The json is structured like this:
//
//{
// "info": {
//     "metaTitle":"The Astrobales of Venus, a 24-ish hour comic by Tom Lechner.",
//     "metaDescription":"The Astrobales of Venus, a 24-ish hour comic by Tom Lechner.",
//     "metaKeywords":"Art, Alternative,Comics, Dream, surreal, Venus",
//     "title":"The Astrolabes of Venus",
//     "byline":"by Tom Lechner",
//     "date":"2017",
//     "blurb":"This is a <a href=\"http://www.24hourcomicsday.com/\">24 hour comic</a>, 24 pages almost done in 24 hours. I actually spent 27 hours on it, due to a badly timed 3 hour nap around hour 19."
// },
// "images":[
//     {"file":"images/p21.jpg", "w":720, "h":800, "thumb":"images/p21-s.jpg", "pw":135, "ph":150 },
//     {"file":"images/p22.jpg", "w":714, "h":800, "thumb":"images/p22-s.jpg", "pw":134, "ph":150 },
//     {"file":"images/p23.jpg", "w":717, "h":800, "thumb":"images/p23-s.jpg", "pw":134, "ph":150 },
//     {"file":"images/p24.jpg", "w":724, "h":800, "thumb":"images/p24-s.jpg", "pw":136, "ph":150 }
//   ]
//}
//
// The index.html takes in an html template, and replaces template tags structured like
//   <!--SOME-TAG-->
//
// ... need to build and maintain list of the tags, and save as export config
//
//  <!--META-TITLE-->
//  <!--ICON-->
//  <!--STYLE-CSS-->
//
//  <meta property="og:title" content="<--TITLE-->">
//  <meta property="og:description" content="<!--META-DESCRIPTION-->">
//  <meta property="og:image" content="<!--META-IMAGE-->">
//  <meta property="og:url" content="<!--META-URL-->">
//  <meta property="og:type" content="website">
//  <meta property="og:site_name" content="Tom Lechner's Art">
//  <meta name="twitter:image" content="<!--META-IMAGE-->">
//  <meta name="twitter:card" content="summary">
//  <meta name="twitter:site" content="@TomsArtStuff">
//  <meta name="twitter:image:alt" content="<!--TITLE-->">
//  <div class="images_title"><!--TITLE--></div>
//  <div class="images_byline"><!--BYLINE--></div>
//  <div class="images_date"><!--DATE--></div>
//  <div class="images_blurb"><!--BLURB--></div>
//
// <!--EXTRA-HEADER-->
// <!--INITIAL-BODY-->
// <!--FINAL-BODY-->
//






//--------------------------------- install html filter

//! Tells the Laidout application that there's a new filter in town.
void InstallHtmlFilter()
{
	HtmlGalleryExportFilter *htmlout=new HtmlGalleryExportFilter;
	htmlout->GetObjectDef();
	laidout->PushExportFilter(htmlout);
}


//----------------------------- HtmlGalleryExportConfig -----------------------------
/*! \class HtmlGalleryExportConfig
 * \brief Holds extra config for html export.
 */
class HtmlGalleryExportConfig : public DocumentExportConfig
{
 public:
	char *image_format;
	char *html_template_file;
	int use_transparent_bg;
	int width, height;
	int img_max_width, img_max_height;

	LaxFiles::Attribute template_vars;

	HtmlGalleryExportConfig();
	HtmlGalleryExportConfig(DocumentExportConfig *config);
	virtual ~HtmlGalleryExportConfig();
	virtual const char *whattype() { return "HtmlGalleryExportConfig"; }
	virtual ObjectDef* makeObjectDef();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute * dump_out_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};

//! Set the filter to the Html export filter stored in the laidout object.
HtmlGalleryExportConfig::HtmlGalleryExportConfig()
{
	image_format = newstr("jpg");
	use_transparent_bg = false;
	width = height = 0;
	html_template_file = nullptr;

	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"HtmlGallery")) {
			filter = laidout->exportfilters.e[c];
			break; 
		}
	}
}

HtmlGalleryExportConfig::HtmlGalleryExportConfig(DocumentExportConfig *config)
  : DocumentExportConfig(config)
{
	HtmlGalleryExportConfig *conf = dynamic_cast<HtmlGalleryExportConfig*>(config);
	if (conf) {
		image_format = newstr(conf->image_format);
		use_transparent_bg = conf->use_transparent_bg;
		width = conf->width;
		height = conf->height;
		makestr(html_template_file, conf->html_template_file);

	} else {
		html_template_file = nullptr;
		image_format = newstr("jpg");
		use_transparent_bg = false;
		width = height = 0;
	}
}


HtmlGalleryExportConfig::~HtmlGalleryExportConfig()
{
	delete[] image_format;
	delete[] html_template_file;
}

void HtmlGalleryExportConfig::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		DocumentExportConfig::dump_out(f,indent,-1,context);
		fprintf(f,"%simage_format  jpg  #file format of exported images to use. Default is jpg\n",spc);
		fprintf(f,"%stransparent  #use a transparent background, not a rendered color.\n",spc);
		fprintf(f,"%swidth  0  #width of resulting image. 0 means auto calculate from dpi.\n",spc);
		fprintf(f,"%sheight 0  #height of resulting image. 0 means auto calculate from dpi.\n",spc);
		return;
	}

	DocumentExportConfig::dump_out(f,indent,what,context);
	fprintf(f,"%simage_format  %s\n",spc,image_format);
	if (use_transparent_bg) fprintf(f,"%stransparent\n",spc);
	fprintf(f,"%swidth  %d\n",spc, width);
	fprintf(f,"%sheight %d\n",spc, height);
	if (html_template_file) fprintf(f,"%shtml_template_file %s\n",spc, html_template_file);
}

LaxFiles::Attribute *HtmlGalleryExportConfig::dump_out_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	att=DocumentExportConfig::dump_out_atts(att,flag,context);
	att->push("image_format", image_format);
	att->push("transparent", use_transparent_bg ? "yes" : "no");
	att->push("width", width);
	att->push("height", height);
	if (html_template_file) att->push("html_template_file", html_template_file);
	return att;
}

void HtmlGalleryExportConfig::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	DocumentExportConfig::dump_in_atts(att,flag,context);

	char *value, *name;
	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name, "image_format")) {
			makestr(image_format, value);

		} else if (!strcmp(name, "html_template_file")) {
			makestr(html_template_file, value);

		} else if (!strcmp(name, "transparent")) {
			use_transparent_bg = BooleanAttribute(value);

		} else if (!strcmp(name, "width")) {
			IntAttribute(value, &width, NULL);

		} else if (!strcmp(name, "height")) {
			IntAttribute(value, &height, NULL);

		}
	}
}

ObjectDef *HtmlGalleryExportConfig::makeObjectDef()
{
    ObjectDef *def=stylemanager.FindDef("HtmlGalleryExportConfig");
	if (def) {
		def->inc_count();
		return def;
	}

    ObjectDef *exportdef=stylemanager.FindDef("ExportConfig");
    if (!exportdef) {
        exportdef=makeExportConfigDef();
		stylemanager.AddObjectDef(exportdef,1);
		exportdef->inc_count(); // *** this saves a crash, but why?!?!?
    }

 
	def=new ObjectDef(exportdef,"HtmlGalleryExportConfig",
            _("Html Gallery Export Configuration"),
            _("Settings to export a document to an html gallery."),
            "class",
            NULL,NULL,
            NULL,
            0, //new flags
            NULL,
            NULL);


     //define parameters
    def->push("transparent",
            _("Transparent"),
            _("Render background as transparent, not as paper color."),
            "boolean",
            NULL,    //range
            "true", //defvalue
            0,      //flags
            NULL); //newfunc

    def->push("image_format",
            _("File format"),
            _("What file format to export as."),
            "string",
            NULL,   //range
            "jpg", //defvalue
            0,     //flags
            NULL);//newfunc
 
    def->push("width",
            _("Width"),
            _("Pixel width of resulting image. 0 means autocalculate"),
            "int",
            NULL,   //range
            "0", //defvalue
            0,     //flags
            NULL);//newfunc

    def->push("height",
            _("Height"),
            _("Pixel height of resulting image. 0 means autocalculate"),
            "int",
            NULL,   //range
            "0", //defvalue
            0,     //flags
            NULL);//newfunc

    def->push("html_template_file",
            _("Index template file"),
            _("Index template file"),
            "string",
            NULL,   //range
            NULL, //defvalue
            0,     //flags
            NULL);//newfunc

	stylemanager.AddObjectDef(def,0);
	return def;
}

Value *HtmlGalleryExportConfig::dereference(const char *extstring, int len)
{
	if (len<0) len=strlen(extstring);

	if (!strncmp(extstring,"image_format",8)) {
		return new StringValue(image_format);

	} else if (!strncmp(extstring,"transparent",8)) {
		return new BooleanValue(use_transparent_bg);

	} else if (!strncmp(extstring,"width",8)) {
		return new IntValue(width);

	} else if (!strncmp(extstring,"height",8)) {
		return new IntValue(height);

	} else if (!strncmp(extstring,"html_template_file",8)) {
		return new StringValue(html_template_file);

	}
	return DocumentExportConfig::dereference(extstring,len);
}

int HtmlGalleryExportConfig::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n()==1) {
        const char *str=ext->e(0);
        int isnum;
        double d;
        if (str) {
			if (!strcmp(str,"image_format")) {
				StringValue *str=dynamic_cast<StringValue*>(v);
				if (!str) return 0;
				makestr(image_format, str->str);
                return 1;

			} else if (!strcmp(str,"transparent")) {
                d=getNumberValue(v, &isnum);
                if (!isnum) return 0;
                use_transparent_bg = (d==0 ? false : true);
                return 1;

			} else if (!strcmp(str,"width")) {
				int w=getNumberValue(v, &isnum);
                if (!isnum) return 0;
				width=w;
                return 1;

			} else if (!strcmp(str,"height")) {
				int h=getNumberValue(v, &isnum);
                if (!isnum) return 0;
				height=h;
                return 1;

			} else if (!strcmp(str,"html_template_file")) {
				StringValue *str = dynamic_cast<StringValue*>(v);
				if (!str) return 0;
				makestr(html_template_file, str->str);
                return 1;

			}
		}
	}

	return DocumentExportConfig::assign(ext,v);
}


//------------------------------------ HtmlGalleryExportFilter ----------------------------------
	
/*! \class HtmlGalleryExportFilter
 * \brief Filter for exporting pages to images via a Displayer.
 *
 * Uses HtmlGalleryExportConfig.
 */


HtmlGalleryExportFilter::HtmlGalleryExportFilter()
{
	flags = FILTER_MANY_FILES
			|FILTER_MULTIPAGE;
			;
}

/*! \todo if other image formats get implemented, then this would return
 *    the proper extension for that image type
 */
const char *HtmlGalleryExportFilter::DefaultExtension()
{
	return "html";
}

const char *HtmlGalleryExportFilter::VersionName()
{
	return _("Html Gallery");
}


Value *newHtmlGalleryExportConfig()
{
	HtmlGalleryExportConfig *o=new HtmlGalleryExportConfig;
	ObjectValue *v=new ObjectValue(o);
	o->dec_count();
	return v;
}

/*! \todo *** this needs a going over for safety's sake!! track down ref counting
 */
int createHtmlGalleryExportConfig(ValueHash *context,ValueHash *parameters,Value **value_ret,ErrorLog &log)
{
	HtmlGalleryExportConfig *config=new HtmlGalleryExportConfig;

	ValueHash *pp=parameters;
	if (pp==NULL) pp=new ValueHash;

	Value *vv=NULL, *v=NULL;
	vv=new ObjectValue(config);
	pp->push("exportconfig",vv);

	int status=createExportConfig(context,pp, &v, log);
	if (status==0 && v && v->type()==VALUE_Object) config=dynamic_cast<HtmlGalleryExportConfig *>(((ObjectValue *)v)->object);

	 //assign proper base filter
	if (config) for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"HtmlGallery")) {
			config->filter=laidout->exportfilters.e[c];
			break;
		}
	}

	char error[100];
	int err=0;
	try {
		int i, e;

		 //---image_format
		const char *str=parameters->findString("image_format",-1,&e);
		if (e==0) { if (str) makestr(config->image_format, str); }
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"image_format"); throw error; }

		 //---use_transparent_bg
		i=parameters->findInt("transparent",-1,&e);
		if (e==0) config->use_transparent_bg=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"transparent"); throw error; }

		 //---width
		double d=parameters->findIntOrDouble("width",-1,&e);
		if (e==0) config->width=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"width"); throw error; }

		 //---height
		d = parameters->findIntOrDouble("height",-1,&e);
		if (e==0) config->height=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"height"); throw error; }

	} catch (const char *str) {
		log.AddMessage(str,ERROR_Fail);
		err=1;
	}
	//if (error) delete[] error;

	if (value_ret && err==0) {
		if (config) {
			*value_ret=new ObjectValue(config);
		} else *value_ret=NULL;
	}
	if (config) config->dec_count();


	if (pp!=parameters) delete pp;

	return 0;
}

DocumentExportConfig *HtmlGalleryExportFilter::CreateConfig(DocumentExportConfig *fromconfig)
{
	return new HtmlGalleryExportConfig(fromconfig);
}

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 *
 * \todo implement background color. currently defaults to transparent
 */
ObjectDef *HtmlGalleryExportFilter::GetObjectDef()
{
	ObjectDef *styledef;
	styledef=stylemanager.FindDef("HtmlGalleryExportConfig");
	if (styledef) return styledef; 

	HtmlGalleryExportConfig config;
	styledef = config.GetObjectDef();

	return styledef;
}




/*! Save the document as html viewer with image files with optional transparency:
 *  1. make web size images
 *  2. make preview images
 *  3. create and populate directory for html gallery:
 *      //index.html
 *      //images/ *.jpg
 *      //images/ *-s.jpg
 *      //styles/style.css
 *      //styles/controller.js
 * 
 * Return 0 for success, or nonzero error.
 * 
 * Currently uses an HtmlGalleryExportConfig.
 */
int HtmlGalleryExportFilter::Out(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	HtmlGalleryExportConfig *out=dynamic_cast<HtmlGalleryExportConfig *>(context);
	if (!out) {
		log.AddMessage(_("Wrong type of output config!"),ERROR_Fail);
		return 1;
	}

	Document *doc = out->doc;
	//int start     =out->start;
	//int end       =out->end;
	//int layout    =out->layout;
	if (!filename) filename = out->filename;
	
	 //we must have something to export...
	if (!doc && !out->limbo) {
		//|| !doc->imposition || !doc->imposition->paper)...
		log.AddMessage(_("Nothing to export!"),ERROR_Fail);
		return 1;
	}

	Utf8String file;
	if (!filename) {
		if (!doc || isblank(doc->saveas)) {
			log.AddError(_("Cannot save without a filename."));
			return 3;
		}

		file = doc->saveas;
		file.Append("_html");

		filename = file.c_str();
	}

	int ftype = file_exists(filename, 1, NULL);
	if (ftype != 0 && ftype != S_IFDIR) {
		log.AddError("File exists and is not a directory!");
		return 4;
	}
	if (ftype == 0) {
		// create directory
		check_dirs(filename, true);
	}

	Utf8String scratch;
	scratch.Sprintf("%s/images", filename);
	check_dirs(scratch.c_str(), true);

	InterfaceManager *imanager=InterfaceManager::GetDefault(true);
	Displayer *dp = imanager->GetDisplayer(DRAWS_Hires);
	DoubleBBox bounds;

	scratch.Sprintf("%s/index.html", filename);
	FILE *htmlout = fopen(scratch.c_str(), "w");
	if (!htmlout) {
		log.AddError("Could not open index.html for writing!");
		return 5;
	}

	fprintf(htmlout, "<html>\n<head>\n<title>%s</title>\n</head>\n<body>\n", filename);

	try {

		int start = out->start;
		int end = out->end;
		if (end < 0) end = (doc ? doc->imposition->NumSpreads(out->layout) : start);
		for (int sc = start; sc <= end; sc++) {

			 //spread objects
			Spread *spread = NULL;
			if (doc) {
				spread = doc->imposition->Layout(out->layout, sc);
			}

			int width  = out->width;
			int height = out->height;

			if (out->crop.validbounds()) {
				 //use crop to override any bounds derived from PaperGroup
				bounds.setbounds(&out->crop);

			} else	if (out->papergroup) {
				out->papergroup->FindPaperBBox(&bounds);
			}

			if (!bounds.validbounds() && out->limbo) {
				bounds.setbounds(out->limbo);
			}

			if (!bounds.validbounds()) {
				throw _("Invalid output bounds! Either set crop or papergroup.");
			}

			//now bounds is the real bounds on a papergroup/view space.
			//We need to figure out how big an image to use
			if (width>0 && height>0) {
				//nothing to do, dimensions already specified

			} else if (width == 0 && height == 0) {
				 //both zero means use dpi to compute
				double dpi=300; // *** maybe make this an option??
				if (out->papergroup) dpi=out->papergroup->GetBasePaper(0)->dpi;
				width =bounds.boxwidth ()*dpi;
				height=bounds.boxheight()*dpi;

			} else if (width == 0) {
				 //compute based on height, which must be zonzero
				width = height/bounds.boxheight()*bounds.boxwidth();

			} else { //if (height == 0) {
				 //compute based on width, which must be zonzero
				height = width/bounds.boxwidth()*bounds.boxheight();

			}

			if (width==0 || height==0) {
				if (width==0)  throw _( "Null width, nothing to output!");
				throw _("Null height, nothing to output!");
			}

			 //set displayer port to the proper area.
			 //The area in bounds must map to [0..width, 0..height]
			dp->CreateSurface(width, height);
			dp->PushAxes();
			dp->defaultRighthanded(true);
			dp->NewTransform(1,0,0,-1,0,-height);

			//dp->SetSpace(0,width, 0,height);
			dp->SetSpace(bounds.minx,bounds.maxx, bounds.miny,bounds.maxy);
			dp->Center(bounds.minx,bounds.maxx, bounds.miny,bounds.maxy);
			//dp->defaultRighthanded(false);

			 //now output everything
			if (!out->use_transparent_bg) {
				 //fill output with an appropriate background color
				 // *** this should really color papers according to their characteristics
				 // *** and have a default for non-transparent limbo color
				if (out->papergroup && out->papergroup->papers.n) {
					dp->NewBG(&out->papergroup->papers.e[0]->color);
				} else dp->NewBG(1.0, 1.0, 1.0);

				dp->ClearWindow();
			}

			//DBG dp->NewFG(.5,.5,.5);
			//DBG dp->drawline(bounds.minx,bounds.miny, bounds.maxx,bounds.maxy);
			//DBG dp->drawline(bounds.minx,bounds.maxy, bounds.maxx,bounds.miny);

			 //limbo objects
			if (out->limbo) DrawData(dp, out->limbo, NULL,NULL,DRAW_HIRES);
			//if (out->limbo) imanager->DrawData(dp, out->limbo, NULL,NULL,DRAW_HIRES);
			
			 //papergroup objects
			if (out->papergroup && out->papergroup->objs.n()) {
				for (int c=0; c<out->papergroup->objs.n(); c++) {
					  //imanager->DrawData(dp, out->papergroup->objs.e(c), NULL,NULL,DRAW_HIRES);
					  DrawData(dp, out->papergroup->objs.e(c), NULL,NULL,DRAW_HIRES);
				}
			}

			if (spread) {
				dp->BlendMode(LAXOP_Over);

				 // draw the page's objects and margins
				Page *page=NULL;
				int pagei=-1;
				flatpoint p;
				SomeData *sd=NULL;
				for (int c=0; c<spread->pagestack.n(); c++) {
					DBG cerr <<" drawing from pagestack.e["<<c<<"], which has page "<<spread->pagestack.e[c]->index<<endl;
					page=spread->pagestack.e[c]->page;
					pagei=spread->pagestack.e[c]->index;

					if (!page) { // try to look up page in doc using pagestack->index
						if (spread->pagestack.e[c]->index>=0 && spread->pagestack.e[c]->index<doc->pages.n) {
							page=spread->pagestack.e[c]->page=doc->pages.e[pagei];
						}
					}

					//if (spread->pagestack.e[c]->index<0) {
					if (!page) continue;

					 //else we have a page, so draw it all
					sd=spread->pagestack.e[c]->outline;
					dp->PushAndNewTransform(sd->m()); // transform to page coords
					
					if (page->pagestyle->flags&PAGE_CLIPS) {
						 // setup clipping region to be the page
						dp->PushClip(1);
						SetClipFromPaths(dp,sd,dp->Getctm());
					}
					
					 //*** debuggging: draw X over whole page...
					//DBG dp->NewFG(255,0,0);
					//DBG dp->drawrline(flatpoint(sd->minx,sd->miny), flatpoint(sd->maxx,sd->miny));
					//DBG dp->drawrline(flatpoint(sd->maxx,sd->miny), flatpoint(sd->maxx,sd->maxy));
					//DBG dp->drawrline(flatpoint(sd->maxx,sd->maxy), flatpoint(sd->minx,sd->maxy));
					//DBG dp->drawrline(flatpoint(sd->minx,sd->maxy), flatpoint(sd->minx,sd->miny));
					//DBG dp->drawrline(flatpoint(sd->minx,sd->miny), flatpoint(sd->maxx,sd->maxy));
					//DBG dp->drawrline(flatpoint(sd->maxx,sd->miny), flatpoint(sd->minx,sd->maxy));
			

					 // Draw all the page's objects.
					for (int c2=0; c2<page->layers.n(); c2++) {
						DBG cerr <<"  num layers in page: "<<page->n()<<", num objs:"<<page->e(c2)->n()<<endl;
						DBG cerr <<"  Layer "<<c2<<", objs.n="<<page->e(c2)->n()<<endl;
						//imanager->DrawData(dp, page->e(c2), NULL,NULL,DRAW_HIRES);
						DrawData(dp, page->e(c2),NULL,NULL,DRAW_HIRES);
					}
					
					if (page->pagestyle->flags&PAGE_CLIPS) {
						 //remove clipping region
						dp->PopClip();
					}

					dp->PopAxes(); // remove page transform
				} //foreach in pagestack
			} //if spread

			//DBG dp->NewFG(.2,.2,.5);
			//DBG dp->LineWidth(.25);
			//DBG dp->drawline(bounds.minx,bounds.miny, bounds.maxx,bounds.maxy);
			//DBG dp->drawline(bounds.minx,bounds.maxy, bounds.maxx,bounds.miny);

			dp->PopAxes(); //initial dp protection

			LaxImage *img = dp->GetSurface();

			scratch.Sprintf("%s/images/%03d.%s", filename, sc, out->image_format);
			int err = img->Save(scratch.c_str(), out->image_format);
			if (err) {
				log.AddError(_("Could not save the image"));
			}
			img->dec_count();

			fprintf(htmlout, "<img src=\"images/%s\">\n", lax_basename(scratch.c_str()));

		} //each spread

	} catch(const char *err) {
		log.AddError(err);
		fclose(htmlout);
		return 2;
	}

	fprintf(htmlout, "</body>\n</html>");
	fclose(htmlout);

	if (log.Errors()) return -1;

	return 0; 
}


} // namespace Laidout

