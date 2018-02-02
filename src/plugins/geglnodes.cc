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
// Copyright (C) 2017 by Tom Lechner
//


#include "../language.h"
#include "../interfaces/nodeinterface.h"
#include "geglnodes.h"


#include <iostream>
#define DBG


using namespace std;


/*! dl entry point for laidout
 */
Laidout::PluginBase *GetPlugin()
{
	return new Laidout::GeglNodesPluginNS::GeglNodesPlugin();
}




namespace Laidout {
namespace GeglNodesPluginNS {


//-------------------------------- Helper funcs --------------------------

Laxkit::MenuInfo *GetGeglOps();
Laidout::ObjectDef gegl_types;


void XMLOut(GeglNode *gegl_node, const char *nodename)
{ 
    gchar *str = gegl_node_to_xml(gegl_node, ".");
    cout <<"\nXML for "<<nodename<<":\n"<<str<<endl;
    g_free(str);
}


int GValueToValue(GValue *gv, Value **v_Ret)
{
	// ***
	return 0;
}

/*! gvtype is the desired type as reported by gegl.
 * gv_ret should be at default blank GValue, with no specific type.
 *
 * Return 0 for success, or nonzero error.
 */
int ValueToGValue(Value *v, const char *gvtype, GValue *gv_ret, GeglNode *node, const char *property)
{
	if (!v) return 1;

	int vtype = v->type();

	if (!strcmp(gvtype, "gboolean")) {
		int isnum=0;
		int i = getNumberValue(v, &isnum);
		if (i) i=1;
		if (isnum) {
			g_value_init (gv_ret, G_TYPE_BOOLEAN);
			g_value_set_boolean(gv_ret, i);
			return 0;
		}
	} else if (!strcmp(gvtype, "gdouble")) {
		int isnum=0;
		double i = getNumberValue(v, &isnum);
		if (isnum) {
			g_value_init (gv_ret, G_TYPE_DOUBLE);
			g_value_set_double(gv_ret, i);
			return 0;
		}
	} else if (!strcmp(gvtype, "gint")) {
		int isnum=0;
		int i = getNumberValue(v, &isnum);
		if (isnum) {
			g_value_init (gv_ret, G_TYPE_INT);
			g_value_set_int(gv_ret, i);
			return 0;
		}
	} else if (!strcmp(gvtype, "gchararray")) {
		if (vtype == VALUE_Int || vtype == VALUE_Real || vtype == VALUE_Boolean) {
			 //convert numbers to strings
			char str[20]; str[0]='\0';
			v->getValueStr(str,20);
			g_value_init (gv_ret, G_TYPE_STRING);
			g_value_set_string(gv_ret, str);
			return 0;

		} else {
			if (vtype == VALUE_String) {
				StringValue *s = dynamic_cast<StringValue*>(v);
				if (s->str) {
					g_value_init (gv_ret, G_TYPE_STRING);
					g_value_set_string(gv_ret, s->str);
					return 0;
				} else return 2;
			}
			return 3;
		}

		return 4;

	}

	return 100;
}

/*! Return 0 for able to set, else nonzero error.
 */
int ValueToProperty(Value *v, const char *gvtype, GeglNode *node, const char *property)
{
	int vtype = v->type();

	if (!strcmp(gvtype, "GeglColor")) {
		if (vtype == VALUE_Color) {
			ColorValue *col = dynamic_cast<ColorValue*>(v);
			GeglColor  *color = gegl_color_new (NULL);
			gegl_color_set_rgba(color, col->color.Red(), col->color.Green(), col->color.Blue(), col->color.Alpha());
			gegl_node_set(node, property, color, NULL);
			g_object_unref (color);
			return 0;
		}

	} else if (vtype == VALUE_EnumVal) {
		 //incoming data as well as the gegl property must be the same enum type
		GValue gv = G_VALUE_INIT;
		gegl_node_get_property(node, property, &gv);

		//check for existing property being some kind of enum
		GType gtype = G_VALUE_TYPE(&gv);
		int success = 0;

		if (G_TYPE_IS_ENUM(gtype)) {

			EnumValue *ev = dynamic_cast<EnumValue*>(v);
			if (!strcmp(ev->GetObjectDef()->name, gvtype)) {
				int e = ev->value;

				g_value_set_enum(&gv, e);
				gegl_node_set_property(node, property, &gv);
				success = 1;

			} else {
				DBG cerr <<" WARNING! trying to set enum "<<gvtype<<" with wrong enum type "<<ev->GetObjectDef()->name<<endl;
			}
		}

		g_value_unset(&gv);
		if (success) return 0;

	//} else if (vtype == VALUE_) {
	}

	return 100;
}

/*! Convert a gegl xml string to a NodeGroup.
 */
int XMLStringToLaidoutNodes(const char *str, NodeGroup *parent, Laxkit::ErrorLog *log)
{
	// ***
	return 1;
}

/*! Read in a node expected to be a node with a known op type.
 */
GeglLaidoutNode *ReadNode(Attribute *att, Laxkit::ErrorLog *log)
{
	if (!att) return NULL;

	 //first, figure out what type of node we are.
	const char *op = att->findValue("operation");
	if (!op) {
		if (log) log->AddMessage(_("Missing gegl operation"), ERROR_Fail);
		return NULL;
	}

	GeglLaidoutNode *node = new GeglLaidoutNode(op);
	if (node->operation == NULL) {
		if (log) {
			char scratch[strlen(_("Unknown gegl operation %s")) + strlen(op)+1];
			sprintf(scratch, _("Unknown gegl operation %s"), op);
			log->AddMessage(scratch, ERROR_Fail);
		}
		delete node;
		return NULL;
	}

	 //read in any params that are contained as xml properties
	const char *name, *pname; 
	//const char *value, *name, *pname; 
	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		//value = att->attributes.e[c]->value;

		if (!strcmp(name, "operation")) continue;

		// ***
	}


	Attribute *content = att->find("content:");

	for (int c=0; c<content->attributes.n; c++) {
		name  = content->attributes.e[c]->name;
		//value = content->attributes.e[c]->value;

		if (!strcmp(name, "params")) {

			Attribute *params = content->attributes.e[c];
			for (int c2=0; c2<params->attributes.n; c2++) {
				name  = params->attributes.e[c2]->name;
				//value = params->attributes.e[c2]->value;

				if (!strcmp(name, "param")) {
					//need to look up the type for the param, convert to a value from the string

					pname = params->attributes.e[c]->findValue("name");
					if (pname) {
					}
				}
			}

		} else if (!strcmp(name, "node")) {
			 //we have subnodes that need to connect
			// ***

		} else if (!strcmp(name, "clone")) {
			//const char *ref = param->attributes.e[c]->findValue("ref");
			// ***
		}
	}

	return node;
}

/*! Construct a chain of nodes that are subatts of att.
 * All the new nodes are placed as children of parent. If parent is NULL,
 * then create and use a new NodeGroup as parent. Return parent on success.
 * Returns NULL if nodes cannot be constructed or some other error.
 */
NodeGroup *ReadNodes(NodeGroup *parent, Attribute *att, Laxkit::ErrorLog *log)
{
	if (!att) return NULL;

	const char *value, *name, *pname;
	const char *op = NULL;
	NodeGroup *group = parent;

	 //first, figure out what type of node we are. Null operation means we are just a blank parent group
	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name, "operation")) {
			op = value;
		} else {
			 //probably has properties as attributes, not subelements
			// ***
		}
	}

	GeglLaidoutNode *node = new GeglLaidoutNode(op);
	if (node->operation == NULL) {
		if (log) {
			char scratch[strlen(_("Unknown gegl operation %s")) + strlen(op)+1];
			sprintf(scratch, _("Unknown gegl operation %s"), op);
			log->AddMessage(scratch, ERROR_Fail);
		}
		delete node;
		return NULL;
	}

	Attribute *params = att->find("params");
	if (params) params = params->find("content:");
	if (params) {
		for (int c=0; c<params->attributes.n; c++) {
			name  = params->attributes.e[c]->name;
			value = params->attributes.e[c]->value;

			if (!strcmp(name, "param")) {
				//need to look up the type for the param, convert to a value from the string

				pname = params->attributes.e[c]->findValue("name");
				if (pname) {
				}

			} else if (!strcmp(name, "node")) {
				 //we have subnodes that need to connect

			} else if (!strcmp(name, "clone")) {
				//const char *ref = param->attributes.e[c]->findValue("ref");
			}
		}
	}

	return group;
}

/*! Read in a file to an Attribute, and pass to ReadNodes().
 */
NodeGroup *XMLFileToLaidoutNodes(const char *file, NodeGroup *parent, Laxkit::ErrorLog *log)
{
	Attribute att;
	Attribute *ret = XMLFileToAttribute(&att, file, NULL);
	if (!ret) return NULL;

	//parse the att
	Attribute *att2 = att.find("gegl");
	if (att2) att2 = att2->find("content:");
	if (!att2) {
		if (log) log->AddMessage(_("Missing gegl node info"), ERROR_Fail);
		return NULL;
	}

	NodeGroup *group = new NodeGroup();
	NodeGroup *group_ret = ReadNodes(group, att2, log);
	if (!group_ret) delete group;
	return group;
}

/*! Using gegl methods, parse the file to a new GeglNode.
 * Note the returned node needs to be g_object_unref (node) when done.
 * Use this with GeglNodesToLaidoutNodes() to fully transform to NodeBase objects.
 */
GeglNode *XMLFileToGeglNodes(const char *file, Laxkit::ErrorLog *log)
{
	GeglNode *node = NULL;

	try {
		DBG cerr << "test-gegl, reading in "<<file<<endl;
		node = gegl_node_new_from_file(file);
	} catch (exception &e) {
		cerr << "Gegl node read in from "<<file<<" error: "<<e.what()<<endl;
		if (log) log->AddMessage(e.what(), ERROR_Fail);
	}
	return node;
}

/*! Put all children from gegl into parent. If parent == NULL, then return a new NodeGroup.
 * If children_only, then if gegl is a no-op graph holder, add children of gegl only, not gegl itself.
 * Otherwise add the whole thing under a fresh NodeGroup.
 */
NodeGroup* GeglNodesToLaidoutNodes(GeglNode *gegl, NodeGroup *parent, bool children_only, Laxkit::ErrorLog *log)
{
	GSList *children = gegl_node_get_children(gegl);

	RefPtrStack<NodeBase> nodes;
	if (!parent) {
		parent = new NodeGroup;
		parent->InstallColors(new NodeColors(), 1);
		parent->colors->Font(anXApp::app->defaultlaxfont, 0);
	}

	 //first, create a NodeBase to correspond to the child's operation
	for (GSList *child = children; child; child = child->next) {
		//*** //need to translate the properties out of gegl into laidout
		GeglLaidoutNode *newnode = new GeglLaidoutNode((GeglNode*)child->data);
		newnode->InstallColors(parent->colors, 0);
		nodes.push(newnode);
		parent->nodes.push(newnode);
		newnode->dec_count();
	}


	 //once we have a list of NodeBases, we need to connect them as per the actual gegl connections
	GeglNode *gnode;
	GeglLaidoutNode *glonode;
	GeglNode **consumers;
	const gchar **consumerpads;

	for (int c=0; c<nodes.n; c++) {
		 //we only connect forward links, so as not to duplicate effort
		glonode = dynamic_cast<GeglLaidoutNode*>(nodes.e[c]);
		gnode = (glonode ? glonode->gegl : NULL);
		if (!gnode) continue;

		gchar **padnames = gegl_node_list_output_pads(gnode);
		if (padnames) {
			for (int c2=0; padnames[c2]; c2++) {
				gchar *padname = padnames[c2];
				NodeProperty *fromprop = glonode->FindProperty(padname);
				//if (!prop) {  <- let it crash if not found.. should not happen anyway
				//	DBG cerr << " *** warning! seem to be missing a defined gegl output pad "<<padname<<" on "<<glonode->Label()<<endl;
				//	continue;
				//}

				int num = gegl_node_get_consumers(gnode, padname, &consumers, &consumerpads);
				if (num) {
					for (int c3=0; c3<num; c3++) {
						GeglLaidoutNode *to = NULL;
						for (int c4=0; c4<nodes.n; c4++) {
							 //we only connect forward links, so as not to duplicate effort
							to = dynamic_cast<GeglLaidoutNode*>(nodes.e[c4]);
							if (to && to->gegl == consumers[c3]) break;
							to = NULL;
						}
						NodeProperty *toprop = (to ? to->FindProperty(consumerpads[c3]) : NULL);
						if (toprop) parent->Connect(fromprop, toprop);
						//if (toprop) parent->Connect(glonode, to, fromprop, toprop);
						else {
							DBG cerr << " *** warning! couldn't find a to property "<<consumerpads[c3]<<" on "<<(to?to->Label():"(none)")<<" from "<<glonode->Label()<<endl;
						}
					}
					g_free(consumers);
					g_free(consumerpads);
				}

			}
			g_strfreev(padnames);
		}
	}

	for (int c=0; c<nodes.n; c++) {
		nodes.e[c]->UpdatePreview();
		nodes.e[c]->Wrap();
	}
	for (int c=0; c<nodes.n; c++) {
//		//reparent?
//		if (GeglLaidoutNode::masternode == NULL) {
//			 //this is an arbitrary total parent to all kids
//			GeglLaidoutNode::masternode = gegl_node_new();
//		}
//		gegl_node_add_child(masternode, nodes.e[c]->gegl);

		//parent->NoOverlap(nodes.e[c], 2*parent->colors->font->textheight());
		parent->NoOverlap(nodes.e[c], 20);
	}

	g_slist_free(children);

	return parent;
}

/*! Attempt to put full size in in_this.
 * If gegl is unbounded, then use a 100 x 100 box.
 * If new_if_different and in_this != NULL and dimensions differ, then return a new LaxImage.
 */
LaxImage *GeglToLaxImage(GeglNode *gegl, LaxImage *in_this, bool new_if_different)
{
	GeglRectangle rect = gegl_node_get_bounding_box (gegl);
	if (rect.width <=0 || rect.height <= 0 || rect.width > 100000 || rect.height > 100000) {
		 //probably unbounded, arbitrarily select a little window onto the data
		rect.x      = 0;
		rect.y      = 0;
		rect.width  = 100;
		rect.height = 100;
	}


	LaxImage *image = in_this;
	double scale = 1;
	if (new_if_different && !image && (image->w() != rect.width || image->h() != rect.height)) {
		image = create_new_image(rect.width, rect.height);
		scale = image->w() / (double)rect.width;
	}
	unsigned char *buffer = image->getImageBuffer(); //bgra

	int ibufw = image->w();
	int ibufh = image->h();

	GeglRectangle  orect;
	orect.x = orect.y = 0;
	orect.width  = ibufw;
	orect.height = ibufh;

	gegl_node_blit (gegl,
					scale,
					&orect,
					babl_format("R'G'B'A u8"),
					buffer,
					GEGL_AUTO_ROWSTRIDE,
					GEGL_BLIT_DEFAULT);

	//need to flip r and b
	int i=0;
	unsigned char t;
	for (int y=0; y<ibufh; y++) {
		for (int x=0; x<ibufw; x++) {
			t = buffer[i+2];
			buffer[i+2] = buffer[i];
			buffer[i] = t;
			i += 4;
		}
	}

	image->doneWithBuffer(buffer);
	return image;
}

GeglNode *LaxImageToGegl(LaxImage *image, GeglNode *to_this)
{
	GeglNode *gegl = to_this;


	return gegl;
}


//-------------------------------- GeglLaidoutNode --------------------------


GeglNode *GeglLaidoutNode::masternode = NULL;

Laxkit::SingletonKeeper GeglLaidoutNode::op_menu;


GeglLaidoutNode::GeglLaidoutNode(const char *oper)
{
	gegl      = NULL;
	operation = NULL;
	op        = NULL; //don't delete this! it lives within op_menu
	SetOperation(oper);

	//gegl = gegl_node_new();
}

/*! Takes node and builds a NodeBase out of it. node should not be NULL.
 */
GeglLaidoutNode::GeglLaidoutNode(GeglNode *node)
{
	gegl = node;
	if (gegl) g_object_ref(gegl);
	op = NULL;
	operation = NULL;
	SetOperation(gegl_node_get_operation(gegl));
}

GeglLaidoutNode::~GeglLaidoutNode()
{
	delete[] operation;
	if (gegl) g_object_unref (gegl);
}

int GeglLaidoutNode::UpdateProperties()
{
	//*** gegl_node_process(gegl);
	return 1;
}

/*! Return 1 for saving nodes, display nodes.
 * 0 otherwise.
 */
int GeglLaidoutNode::IsSaveNode()
{
	const char *savenodes[] = {
		"gegl:exr-save",
		"gegl:ff-save",
		"gegl:gegl-buffer-save",
		"gegl:jpg-save",
		"gegl:npy-save",
		"gegl:png-save",
		"gegl:ppm-save",
		"gegl:rgbe-save",
		"gegl:save",
		"gegl:save-pixbuf",
		"gegl:tiff-save",
		"gegl:webp-save",
		NULL
	};

	for (int c=0; savenodes[c]!=NULL; c++) {
		if (!strcmp(operation, savenodes[c])) return 1;
	}

	return 0;
}

/*! Called whenever input properties are changed, and outputs need to be updated.
 */
int GeglLaidoutNode::Update()
{
	DBG cerr << "GeglLaidoutNode::Update()..."<<endl;

	int num_updated = 0;
	int errors = 0;
	int unhandled_props = 0;

	NodeProperty *prop;
	NodeConnection *connection;

	for (int c=0; c<properties.n; c++) {
		if (!properties.e[c]->IsInput()) continue;
		prop = properties.e[c];
		connection = prop->connections.n ? prop->connections.e[0] : NULL;

		 //turn Value back into GValue
		 //set it in the node

		const char *ptype = NULL;
		MenuItem *propspec = op->GetSubmenu()->e(c);
		if (!propspec) {
			if (connection) {
				 //is an input pad, we need to make sure it's connected
				GeglNode *prevnode = NULL;
				int pindex=-1;
				GeglLaidoutNode *prev = dynamic_cast<GeglLaidoutNode*>(prop->GetConnection(0, &pindex));
				if (prev && prev->gegl) {
					prevnode = prev->gegl;
					gegl_node_connect_to(prevnode, connection->fromprop->Name(),
										 gegl, prop->Name());
				} else {
					DBG cerr <<" *** Warning! error connected non-gegl input pad for "<<operation<<endl;
					errors++;
				}
			}

		} else {
			 //we are updating a from a non-gegl node property
			Value *v = properties.e[c]->GetData(); //<-not inc counted
			if (v) {
				ptype = propspec->GetString(1);

				if (ValueToProperty(v, ptype, gegl, propspec->name)==0) {
					//was possible to set directly to gegl without GValue setup

				} else {
					GValue gv = G_VALUE_INIT;

					if (ValueToGValue(v, ptype, &gv, gegl, prop->name) == 0) {
						 //more finicky method using GValue
						gegl_node_set_property(gegl, prop->name, &gv);
						num_updated++;

					} else {
						errors++;
						unhandled_props++;
						DBG cerr <<" *** Warning! error setting property "<<propspec->name<<" on gegl node "<<operation<<endl;
					}
					g_value_unset(&gv);
				}

			}
		}
	}

	if (errors == 0) {
		 //should do this ONLY if the node is a sink
		if (IsSaveNode()) {
			if (AutoProcess()) {
				DBG cerr <<"..........Attempting to process "<<operation<<endl;
				gegl_node_process(gegl);
				XMLOut(gegl, operation);
			} else {
				DBG cerr <<"....deferring gegl node process"<<endl;
			}
		}

		UpdatePreview();
		return NodeBase::Update(); //should trigger updates in outputs

	} 
	//else

	DBG cerr << " *** warning! "<<errors<<" errors encountered in GeglLaidoutNode::Update()! "<<Name<<endl;

	return -1;
}

/*! Checks for true on last property if it's named "AutoProcess" and contains a BooleanValue.
 * If no such property, assume yes.
 */
int GeglLaidoutNode::AutoProcess()
{
	if (properties.n == 0) return 1;
	NodeProperty *autoprocess = FindProperty("AutoProcess");
	if (autoprocess) {
		BooleanValue *b = dynamic_cast<BooleanValue*>(autoprocess->GetData());
		if (b) {
			if (!b->i) return 0;

			// *** need to check boundedness
			return 1;
		}
	}
//	----
//	if (!strcmp(properties.e[properties.n-1]->name, "AutoProcess")) {
//		BooleanValue *b = dynamic_cast<BooleanValue*>(properties.e[properties.n-1]->GetData());
//		if (b) return b->i;
//	}
	return 1;
}

/*! Return box.nonzerobounds(), which is true when box has _valid_ non zero bounds.
 */
int GeglLaidoutNode::GetRect(Laxkit::DoubleBBox &box)
{
	GeglRectangle rect = gegl_node_get_bounding_box (gegl);
	box.minx = rect.x;
	box.maxx = rect.x + rect.width;
	box.miny = rect.y;
	box.maxy = rect.y + rect.height;
	return box.nonzerobounds();
}

/*! Return 0 for nothing done, or 1 for preview updated.
 */
int GeglLaidoutNode::UpdatePreview()
{

	if (preview_area_height < 0) preview_area_height = 3*colors->font->textheight();

	GeglRectangle rect = gegl_node_get_bounding_box (gegl);
	if (rect.width <=0 || rect.height <= 0 || rect.width > 100000 || rect.height > 100000) {
		 //probably unbounded, arbitrarily select a little window onto the data
		rect.x      = 0;
		rect.y      = 0;
		rect.width  = 100;
		rect.height = 100;
	}


	//double aspect = (double)rect.width / rect.height;
	int bufw = rect.width;
	int bufh = rect.height;
	int maxwidth = (width > 0 ? width : 3*colors->font->textheight());
	int maxheight = preview_area_height;

	 //first determine a smallish size for the preview image, adjust total_preview if necessary.
	 //fit inside a rect this->width x maxdim
	int ibufw = bufw, ibufh = bufh;
	double scale  = (double)maxwidth  / bufw;
	double scaley = (double)maxheight / bufh;
	if (scaley > scale) {
		scaley = scale;
	}
	ibufw = bufw * scale;
	ibufh = bufh * scale;
	if (ibufw==0) ibufw = 1;
	if (ibufh==0) ibufh = 1;


	bool needtowrap = false;
	if (!total_preview) needtowrap = true;
	if (total_preview && (ibufw != total_preview->w() || ibufh != total_preview->h())) {
		total_preview->dec_count();
		total_preview = NULL;
		needtowrap = true;
	}

	if (!total_preview) { 
		total_preview = create_new_image(ibufw, ibufh);
	}

	unsigned char *buffer = total_preview->getImageBuffer(); //bgra


	GeglRectangle  orect;
	orect.x = orect.y = 0;
	orect.width  = ibufw;
	orect.height = ibufh;

	gegl_node_blit (gegl,
					ibufw/(double)rect.width,
					&orect,
					babl_format("R'G'B'A u8"),
					buffer,
					GEGL_AUTO_ROWSTRIDE,
					GEGL_BLIT_DEFAULT);

	//need to flip r and b
	int i=0;
	unsigned char t;
	for (int y=0; y<ibufh; y++) {
		for (int x=0; x<ibufw; x++) {
			t = buffer[i+2];
			buffer[i+2] = buffer[i];
			buffer[i] = t;
			i += 4;
		}
	}

	total_preview->doneWithBuffer(buffer);

	if (needtowrap) Wrap();

	return 1;
}

/*! Sever gegl connection if the connection is to an input pad of this.
 */
int GeglLaidoutNode::Disconnected(NodeConnection *connection, int to_side)
{
	if (connection->to == this) {
		 //remove something connected to an input pad
		int propindex = properties.findindex(connection->toprop);
		if (propindex < 0) return 0; // not found!
		if (propindex >= op->GetSubmenu()->n()) {
			//is not a simpleproperty, but an input or output pad
			GeglLaidoutNode *othernode = dynamic_cast<GeglLaidoutNode*>(connection->from);
			if (!othernode) return 0; // *** for now assume it has to be a gegl connection
			gegl_node_disconnect(gegl, connection->toprop->Name());
		}
	} else {
		 //never mind about outgoing, should be taken care of elsewhere
	}

	return 0;
}

int GeglLaidoutNode::Connected(NodeConnection *connection)
{
	if (connection->to == this) {
		int propindex = properties.findindex(connection->toprop);
		if (propindex < 0) return 0; // not found!
		if (propindex >= op->GetSubmenu()->n()) {
			//is not a simple property, but an input or output pad
			GeglLaidoutNode *othernode = dynamic_cast<GeglLaidoutNode*>(connection->from);
			if (!othernode) {
				 //check for connected proxy
				if (connection->fromprop->frompropproxy) {
					NodeProperty *proxy = connection->fromprop->frompropproxy;

					if (proxy->IsInput() && proxy->connections.n == 1) {
						othernode = dynamic_cast<GeglLaidoutNode*>(proxy->connections.e[0]->from);
					}
				}
			}
			if (!othernode) return 0; // *** for now assume it has to be a gegl connection
			gegl_node_connect_to(othernode->gegl, connection->fromprop->Name(), gegl, connection->toprop->Name());
		}
	}
	return 0;
}

int GeglLaidoutNode::SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att)
{
    return NodeBase::SetPropertyFromAtt(propname, att);
}

#define GEGLNODE_INPUT   (-1)
#define GEGLNODE_OUTPUT  (-2)
#define GEGLNODE_PREVIEW (-3)
#define GEGLNODE_SWITCH  (-4)

/*! Set to op, and grab the spec for the properties.
 * Note this should only be called once. It currently does not properly update
 * existing structure to a new op structure. If gegl exists already, it is assumed
 * it was passed in with proper type and we just need to grab and construct property uis.
 *
 * Return 0 for success, or nonzero for error.
 */
int GeglLaidoutNode::SetOperation(const char *oper)
{
	if (oper==NULL) return 1;
	if (oper!=NULL && operation!=NULL && !strcmp(oper, operation)) return 0;

	MenuInfo *menu = GetGeglOps();

	int i = menu->findIndex(oper);
	if (i<0) return 2; //unknown op!

	MenuItem *opitem = menu->e(i);

	if (opitem->info == -1) {
		 //op property info hasn't been grabbed yet, so grab the property info and install in the GetGeglOps() menu
		guint n_props = 0;

		GParamSpec **specs = gegl_operation_list_properties (oper, &n_props);

		opitem->info = n_props;

		menu->SubMenu("", i);

		 //add properties to the op list: name, type, nickname, blurb
		for (unsigned int c2=0; c2<n_props; c2++) {
			const gchar *nick  = g_param_spec_get_nick  (specs[c2]);
			const gchar *blurb = g_param_spec_get_blurb (specs[c2]);
			const gchar *ptype = g_type_name(specs[c2]->value_type);

			DBG cerr << "    prop: "<<specs[c2]->name <<','<< ptype <<','<< (nick?nick:"(no nick)")<<','<< (blurb?blurb:"(no blurb)")<<endl;

			menu->AddItem(specs[c2]->name, 0, -1);     // [0]
			menu->AddDetail(ptype, NULL);              // [1]
			menu->AddDetail(nick ? nick : "", NULL);   // [2]
			menu->AddDetail(blurb ? blurb : "", NULL); // [3]


			//guint nn = 0;
			//gchar **keys = gegl_operation_list_property_keys (operations[i], specs[c2]->name, &nn);
			//
			//for (int c3=0; c3<nn; c3++) {
			//	const gchar *val = gegl_param_spec_get_property_key(specs[c2], keys[c3]);
			//	printf("      property key: %s, val: %s\n", keys[c3], val);
			//}
			//
			//g_free(keys);
		}

		if (specs) g_free(specs);

		menu->EndSubMenu();
	}

	makestr(operation, oper);
	makestr(Name, operation);

//	if (gegl && strcmp(operation, gegl_node_get_operation(gegl))) {
//		 //remove if operation mismatch... not sure what changing the op through gegl does to existing properties and connections
//		g_object_unref (gegl);
//		gegl = NULL;
//	}

	if (GeglLaidoutNode::masternode == NULL) {
		 //this is an arbitrary total parent to all kids
		GeglLaidoutNode::masternode = gegl_node_new();
	}

	if (!gegl) gegl = gegl_node_new_child(GeglLaidoutNode::masternode,
								"operation", operation,
								NULL);

	 //set up non input/output properties
	op = opitem;
	MenuInfo *opmenu = op->GetSubmenu();
	MenuItem *prop;
	const char *proptype;
	//NodeProperty::PropertyTypes propwhat;
	bool linkable;
	Value *v;

	makestr  (type, "Gegl/");
	const char *categories = op->GetDetail(2)->name;
	if (!isblank(categories)) {
		//use final category
		const char *cat = strrchr(categories, ':');
		if (cat) cat++; else cat = categories;
		appendstr(type, cat);
		appendstr(type, "/");
	}
	appendstr(type, operation);

	for (int c=0; c<opmenu->n(); c++) {
		prop = opmenu->e(c);
		proptype = prop->GetString(1);
		v = NULL;
		//propwhat = NodeProperty::PROP_Input;
		linkable = true;

		if (proptype) {
			 //set default values
			GValue gv = G_VALUE_INIT;
			gegl_node_get_property(gegl, prop->name, &gv);

            if (!strcmp(proptype, "gboolean")) {
                bool boolean = 0;
                if (G_VALUE_HOLDS_BOOLEAN(&gv)) { boolean = g_value_get_boolean(&gv); }
				v = new BooleanValue(boolean); 
				//---
				//gegl_node_get(gegl, prop->name, &boolean, NULL);

            } else if (!strcmp(proptype, "gint")) {
                int vv=0;
                if (G_VALUE_HOLDS_INT(&gv)) { vv = g_value_get_int(&gv); }
				v = new IntValue(vv);

            } else if (!strcmp(proptype, "gdouble")) {
                double vv=0;
                if (G_VALUE_HOLDS_DOUBLE(&gv)) { vv = g_value_get_double(&gv); }
				v = new DoubleValue(vv);

            } else if (!strcmp(proptype, "gchararray")) {
                const gchar *vv = NULL;
                if (G_VALUE_HOLDS_STRING(&gv)) {
					vv = g_value_get_string(&gv);
				}
				v = new StringValue(vv);

			} else if (!strcmp(proptype, "GeglColor" )) {
				//v = new ColorValue("#000");
				//-----
				GeglColor *color = NULL;
				gegl_node_get(gegl, prop->name, &color, NULL);
				double r=0,g=0,b=0,a=1.0;
				gegl_color_get_rgba(color, &r, &g, &b, &a);
				v = new ColorValue(r,g,b,a);

			} else {
				//propwhat = NodeProperty::PROP_Input; //for now, block any linking to both enums and unknown types
				linkable = false; //for now, block any linking to both enums and unknown types

				GType gtype = G_VALUE_TYPE(&gv);

				if (G_TYPE_IS_ENUM(gtype)) {
					 //gegl has a whole slew of enums that should
					 //be caught here
					if (G_VALUE_HOLDS_ENUM(&gv)) {
						DBG cerr <<"gvalue holds enum"<<endl;
					}

					ObjectDef *def = gegl_types.FindDef(proptype);
					if (!def) {

						GEnumClass *enum_class = (GEnumClass*)g_type_class_ref (gtype);
						GEnumValue *enum_value;

						def = new ObjectDef(NULL, proptype, proptype, NULL, "enum", NULL,NULL);

						DBG cerr << "gegl enum: "<<proptype << ", min,max: " << enum_class->minimum<< ", "<< enum_class->maximum<<endl;
						for (unsigned int c2=0; c2<enum_class->n_values; c2++) {
							enum_value = &enum_class->values[c2];
							DBG cerr <<"  enum value: "<<enum_value->value<<"  "
							DBG  <<enum_value->value_name<<"  "
							DBG  <<enum_value->value_nick<<endl;

							def->pushEnumValue(enum_value->value_name, enum_value->value_nick, NULL);
						}

						gegl_types.push(def, 1);

						g_type_class_unref (enum_class);
					}

					int vv = g_value_get_enum(&gv);
					v = new EnumValue(def, vv);

				//} else if (!strcmp(proptype, "gpointer"                )) {
				//	//this might be BablFormat. these do not appear to be distinguished exactly

				//} else if (!strcmp(proptype, "GeglCurve"               )) {
				//	GeglPath *curve = NULL;
				//	gegl_node_get(gegl, prop->name, &curve, NULL);
				//	CurveValue *locurve = new CurveValue;
				//	locurve->Reset(true);
				//	unsigned int npoints = gegl_curve_num_points(path);
				//	double x,y;
				//	for (int c2=0; c2<npoints; c2++) {
				//		gegl_curve_get_point(curve, c2, &x, &y);
				//		locurve->AddPoint(x,y);
				//	}
				//	v = locurve;
                //
				//} else if (!strcmp(proptype, "GeglPath"                )) {
				//	GeglCurve *path = NULL;
				//	gegl_node_get(gegl, prop->name, &path, NULL);
				//	LPathsData *pathobject = new LPathsData;
				//	char *pathstr = gegl_path_to_string(path);
				//	pathobject->appendSvg(pathstr);
				//	g_free(pathstr);
				//	v = pathobject;

				//} else if (!strcmp(proptype, "GeglAudioFragment"       )) {
				//} else if (!strcmp(proptype, "GeglBuffer"              )) {
				//} else if (!strcmp(proptype, "GdkPixbuf"               )) {
				//} else if (!strcmp(proptype, "GeglNode"                )) {
				} else {
					DBG cerr << proptype << " in gegl is unknown type here!"<<endl;
				}

			}

			g_value_unset(&gv);
		}

		AddProperty(new NodeProperty(NodeProperty::PROP_Input, linkable, prop->name, v,1, prop->GetString(2),prop->GetString(3), c));
	}

	 //set up input pads
	gchar **pads = gegl_node_list_input_pads(gegl);
	if (pads) {
		for (int c=0; pads[c]!=NULL; c++) {
			AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, pads[c],  NULL,1, pads[c] ,NULL, GEGLNODE_INPUT));
		}
		g_strfreev(pads);
	}

	 //set up output pads
	pads = gegl_node_list_output_pads(gegl);
	if (pads) {
		for (int c=0; pads[c]!=NULL; c++) {
			AddProperty(new NodeProperty(NodeProperty::PROP_Output,  true, pads[c],  NULL,1, pads[c] ,NULL, GEGLNODE_OUTPUT));
		}
		g_strfreev(pads);
	}

	 //add an extra toggle for auto saving, so we can optionally not have to be constantly processing as things change
	 //default to no, since process during loading, for instance can do bad things
	if (IsSaveNode()) {
		AddProperty(new NodeProperty(NodeProperty::PROP_Block, false, "AutoProcess",  new BooleanValue(false),1, _("Auto Save") ,NULL, GEGLNODE_SWITCH));
	}

	return 0;
}



//------------------------------- Gegl funcs ----------------------------

/*! Query what gegl ops are available. Properties are not scanned here, only operations.
 */
Laxkit::MenuInfo *GetGeglOps()
{
	MenuInfo *menu = dynamic_cast<MenuInfo*>(GeglLaidoutNode::op_menu.GetObject());
	if (menu) return menu;

	menu = new MenuInfo;
	GeglLaidoutNode::op_menu.SetObject(menu, 1);


    guint   n_operations;
    gchar **operations = gegl_list_operations (&n_operations);
	MenuItem *item, *item_compat;

	DBG cerr <<"gegl operations:"<<endl;

    for (unsigned int i=0; i < n_operations; i++) { 
        DBG cerr << "  operator: " << (operations[i]?operations[i]:"?") << endl;

		menu->AddItem(operations[i], i, -1); 
		item = menu->Top();
		item->info = -1;

		const char *compat_name = gegl_operation_get_key(operations[i], "compat-name");
		item_compat = NULL;
		if (compat_name) {
			DBG cerr <<"compat-name for "<<operations[i]<<": "<<compat_name<<endl;

			menu->AddItem(compat_name, i, -1); 
			item_compat = menu->Top();
			item_compat->info = -1;
		}

		
		guint nkeys = 0;
		gchar **keys = gegl_operation_list_keys (operations[i], &nkeys);
		const gchar *categories = NULL;
		const gchar *description = NULL;
		for (guint c=0; c<nkeys; c++) {
			const char *value = gegl_operation_get_key(operations[i], keys[c]);

			if (!strcmp(keys[c], "source")) value="...code...";
			DBG if (!strcmp(keys[c], "cl-source")) cerr <<"    operation key: "<<keys[c]<<": "<<"(...source code...)"<<endl;
			DBG else cerr <<"    operation key: "<<keys[c]<<": "<<value<<endl;

			if (!strcmp(keys[c], "categories")) {
				categories = value;
			} else if (!strcmp(keys[c], "description")) {
				description = value;
			//} else if (!strcmp(keys[c], "title")) { //localized display name?
			}
		}

		 //add description as first detail
		item->AddDetail(description ? description : NULL);
		if (item_compat) item_compat->AddDetail(description ? description : NULL);

		 //add categories as second detail
		item->AddDetail(categories ? categories : NULL);
		if (item_compat) item_compat->AddDetail(categories ? categories : NULL);

		g_free(keys);

		//GType gtype = gegl_operation_gtype_from_name(operations[i]);
		//GeglOperationClass *opclass = g_type_class_ref (gtype);
		//if (opclass) {
		//	const char *compat_name = gegl_operation_class_get_key(opclass, "compat-name");
		//	if (compat_name) {
		//    DBG cerr <<"compat-name: "<<compat_name<<endl;
		//	}
        //  
		//	g_object_unref (opclass);
		//}
    }

    g_free (operations); 

	return menu;
}

Laxkit::anObject *newGeglLaidoutNode(int p, Laxkit::anObject *ref)
{
	MenuInfo *menu = GetGeglOps();
	MenuItem *op   = menu->e(p);
	if (!op) return NULL;

	GeglLaidoutNode *node = new GeglLaidoutNode(op->name);
	return node;
}

void RegisterNodes(Laxkit::ObjectFactory *factory)
{
	MenuInfo *ops = GetGeglOps();

	char str[200];
	char *categories;
	MenuItem *op;
	for (int c=0; c<ops->n(); c++) {
		op = ops->e(c);
		categories = op->GetDetail(2)->name;

		if (isblank(categories)) {
			sprintf(str, "Gegl/%s", ops->e(c)->name);
			factory->DefineNewObject(getUniqueNumber(), str, newGeglLaidoutNode, NULL, c);

		} else {
			int n = 0;
			char **cats = split(categories, ':', &n);

			for (int c2=0; c2<n; c2++) {
				if (isblank(cats[c2])) continue;

				sprintf(str, "Gegl/%s/%s", cats[c2], op->name);
				factory->DefineNewObject(getUniqueNumber(), str, newGeglLaidoutNode, NULL, c);
			}
			deletestrs(cats, n);
		}
	}
}



//-------------------------------- GeglNodesPlugin --------------------------

/*! class GeglNodesPlugin
 *
 */

GeglNodesPlugin::GeglNodesPlugin()
{
	DBG cerr <<"GeglNodesPlugin constructor"<<endl;
}

GeglNodesPlugin::~GeglNodesPlugin()
{
//	if (GeglLaidoutNode::masternode != NULL) {
//		g_object_unref (GeglLaidoutNode::masternode);
//		GeglLaidoutNode::masternode = NULL;
//	}

    //gegl_exit ();
}

unsigned long GeglNodesPlugin::WhatYouGot()
{
	 //or'd list of PluginBase::PluginBaseContents
	return PluginBase::PLUGIN_Nodes;
		//| PLUGIN_Panes
		//| PLUGIN_ImageImporters
		//| PLUGIN_TextImporters
		//| PLUGIN_ImportFilters
		//| PLUGIN_ExportFilters
		//| PLUGIN_DrawableObjects
		//| PLUGIN_Tools
		//| PLUGIN_Configs
		//| PLUGIN_Resources
		//| PLUGIN_Impositions
		//| PLUGIN_Actions
		//| PLUGIN_Interpreters
		//| PLUGIN_CalcModules
	  ;
}

/*! This should be the same as non-class PluginName() function.
 */
const char *GeglNodesPlugin::PluginName()
{
	return "Gegl Nodes";
}

const char *GeglNodesPlugin::Name()
{
	return _("Gegl Nodes");
}

const char *GeglNodesPlugin::Version()
{
	return "0.1";
}

/*! Return localized description.
 */
const char *GeglNodesPlugin::Description()
{
	return _("Provides gegl image functionality to nodes.");
}

const char *GeglNodesPlugin::Author()
{
	return "Laidout";
}

const char *GeglNodesPlugin::ReleaseDate()
{
	return "2017 12 01";
}

const char *GeglNodesPlugin::License()
{
	return "GPL3";
}

class GeglLoader : public Laidout::ObjectIO
{
  public:
	GeglLoader(GeglNodesPlugin *fromplugin) { plugin = fromplugin; }
	virtual ~GeglLoader() {}

	virtual const char *Author() { return "Laidout"; }
    virtual const char *FilterVersion() { return "0.1"; }

    virtual const char *Format() { return "xml"; }
    virtual const char *DefaultExtension()  { return "xml"; }
    virtual const char *Version() { return "0.1"; }
    virtual const char *VersionName() { return "Gegl XML"; }
    virtual ObjectDef *GetObjectDef() { return NULL; }

	virtual int Serializable(int what) { return 0; }
	virtual int CanImport(const char *file, const char *first100) { return true; } //if null, then return if in theory it can import
	virtual int CanExport(anObject *object) { return true; } //if null, then return if in theory it can export
	virtual int Import(const char *file, anObject **object_ret, anObject *context, Laxkit::ErrorLog &log);
	virtual int Export(const char *file, anObject *object,      anObject *context, Laxkit::ErrorLog &log);
};

int GeglLoader::Import(const char *file, anObject **object_ret, anObject *context, Laxkit::ErrorLog &log)
{
	GeglNode *gnode = XMLFileToGeglNodes(file, &log);
	if (gnode) {
		NodeGroup *parent = dynamic_cast<NodeGroup*>(context);
		NodeGroup *group = GeglNodesToLaidoutNodes(gnode, parent, true, &log);
		*object_ret = group;
		g_object_unref (gnode);
	}
	return log.Errors();
}

int GeglLoader::Export(const char *file, anObject *object, anObject *context, Laxkit::ErrorLog &log)
{
	NodeGroup *group = dynamic_cast<NodeGroup*>(object);
	if (!group) {
		log.AddMessage(_("Object not a NodeGroup in Export"), ERROR_Fail);
		return 1;
	}

	DBG cerr << " *** need to implement GeglLoader::Export()!"<<endl;
	log.AddMessage("need to implement GeglLoader::Export()!!", ERROR_Fail);
	return 1;
}

GeglLoader *theloader = NULL;


/*! Install stuff.
 */
int GeglNodesPlugin::Initialize()
{
	DBG cerr << "GeglNodesPlugin initializing..."<<endl;

	if (initialized) return 0;


	 //initialize gegl
	gegl_init (NULL,NULL); //(&argc, &argv);

	// WARNING!!! Gegl normally is LGPL3, but some operators are only available as
	//   part of a GPL3 license. GPL allows about 50 more operations
	g_object_set (gegl_config (),
                 "application-license", "GPL3",
			     NULL);

	ObjectFactory *node_factory = Laidout::NodeGroup::NodeFactory(true);

	RegisterNodes(node_factory);

	 //install loader
	theloader = new GeglLoader(this);
	Laidout::NodeGroup::InstallLoader(theloader, 0);

	DBG cerr << "GeglNodesPlugin initialized!"<<endl;
	initialized = 1;


//	//---------------TEST STUFF
//	DBG cerr <<"---start testing gegl xml"<<endl;
//	ErrorLog log;
//	GeglNode *gnode = XMLFileToGeglNodes("test-gegl.xml", &log);
//	if (gnode) {
//
//		//GSList *list = gegl_node_get_children(gnode);
//		//cerr << "Num nodes as children: "<<g_slist_length(list)<<endl;
//
//		NodeGroup *group = GeglNodesToLaidoutNodes(gnode, NULL, true, &log);
//		if (group) {
//			DBG cerr <<"xml to lo nodes:"<<endl;
//			group->dump_out(stdout, 2, 0, NULL);
//			FILE *f = fopen("nodes-TEMP-gegl.nodes", "w");
//			group->dump_out(f, 2, 0, NULL);
//			fclose(f);
//			group->dec_count();
//		}
//
//
//		//g_slist_free(list);
//		g_object_unref (gnode);
//	}
//	DBG dumperrorlog("gegl read xml test log:", log);
//	DBG cerr <<"---done testing gegl xml"<<endl;
//	//---------------end TEST STUFF


	return 0;
}

/*! Things that need to be done ONLY after Initialize(), but not a destructor.
 */
void GeglNodesPlugin::Finalize()
{
	if (GeglLaidoutNode::masternode != NULL) {
		g_object_unref (GeglLaidoutNode::masternode);
		GeglLaidoutNode::masternode = NULL;
	}

	if (theloader) {
		Laidout::NodeGroup::RemoveLoader(theloader);
		theloader->dec_count();
		theloader = NULL;
	}

    gegl_exit ();
}



} //namespace GeglNodesPluginNS
} //namespace Laidout

