/* -*- c-file-style: "ruby"; indent-tabs-mode: nil -*- */
/*
 *  Copyright (C) 2011  Ruby-GNOME2 Project Team
 *  Copyright (C) 2002-2004 Masao Mutoh
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA
 */

#include "rbgdk3private.h"

#define RG_TARGET_NAMESPACE mSelection

static VALUE
rg_s_owner_set(int argc, VALUE *argv, G_GNUC_UNUSED VALUE self)
{
    VALUE owner, selection, time, send_event;
    int ret;

    if (argc == 4){
        rb_scan_args(argc, argv, "40", &owner, &selection, &time, &send_event);
        ret = gdk_selection_owner_set(RVAL2GDKWINDOW(owner), 
                                      RVAL2ATOM(selection), 
                                      NUM2UINT(time), RVAL2CBOOL(send_event));
    } else {
      VALUE display = Qnil;
      rb_scan_args(argc, argv, "50", &display, &owner, &selection, &time, &send_event);
      ret = gdk_selection_owner_set_for_display(RVAL2GDKDISPLAYOBJECT(display),
                                                RVAL2GDKWINDOW(owner), 
                                                RVAL2ATOM(selection), 
                                                NUM2UINT(time), RVAL2CBOOL(send_event));
    }
    return CBOOL2RVAL(ret);
}

static VALUE
rg_s_owner_get(int argc, VALUE *argv, G_GNUC_UNUSED VALUE self)
{
    VALUE selection;

    if (argc == 1) {
        rb_scan_args(argc, argv, "10", &selection);
        return GOBJ2RVAL(gdk_selection_owner_get(RVAL2ATOM(selection)));
    } else {
      VALUE display = Qnil;
      rb_scan_args(argc, argv, "20", &display, &selection);
      return GOBJ2RVAL(gdk_selection_owner_get_for_display(RVAL2GDKDISPLAYOBJECT(display),
                                                           RVAL2ATOM(selection)));
    }
}

static VALUE
rg_s_convert(VALUE self, VALUE requestor, VALUE selection, VALUE target, VALUE time)
{
    gdk_selection_convert(RVAL2GDKWINDOW(requestor), 
                          RVAL2ATOM(selection), 
                          RVAL2ATOM(target), NUM2INT(time));
    return self;
}

static VALUE
rg_s_property_get(G_GNUC_UNUSED VALUE self, VALUE requestor)
{
    guchar *data;
    GdkAtom prop_type;
    gint prop_format;
    VALUE ary;

    gdk_selection_property_get(RVAL2GDKWINDOW(requestor), &data, 
                               &prop_type, &prop_format);

    ary = rb_ary_new3(3, CSTR2RVAL((const char*)data), GDKATOM2RVAL(prop_type), 
                      INT2NUM(prop_format));
    g_free(data);
    return ary;
}

static VALUE
rg_s_send_notify(int argc, VALUE *argv, VALUE self)
{
    VALUE requestor, selection, target, property, time;

    if (argc == 5) {
        rb_scan_args(argc, argv, "50", &requestor, &selection, &target, &property, &time);
        gdk_selection_send_notify(RVAL2GDKWINDOW(requestor), RVAL2ATOM(selection),
                                  RVAL2ATOM(target), 
                                  NIL_P(property) ? GDK_NONE : RVAL2ATOM(property), 
                                  NUM2INT(time));
    } else {
      VALUE display = Qnil;
      rb_scan_args(argc, argv, "60", &display, &requestor, &selection, &target, &property, &time);
      gdk_selection_send_notify_for_display(RVAL2GDKDISPLAYOBJECT(display),
                                            RVAL2GDKWINDOW(requestor), RVAL2ATOM(selection),
                                            RVAL2ATOM(target), 
                                            NIL_P(property) ? GDK_NONE : RVAL2ATOM(property), 
                                            NUM2INT(time));
    }
    return self;
}

void
Init_gdk_selection(VALUE mGdk)
{
    VALUE RG_TARGET_NAMESPACE = rb_define_module_under(mGdk, "Selection");

    RG_DEF_SMETHOD(owner_set, -1);
    RG_DEF_SMETHOD(owner_get, -1);
    RG_DEF_SMETHOD(convert, 4);
    RG_DEF_SMETHOD(property_get, 1);
    RG_DEF_SMETHOD(send_notify, -1);

    /* Constants */
    rb_define_const(RG_TARGET_NAMESPACE, "PRIMARY", GDKATOM2RVAL(GDK_SELECTION_PRIMARY));
    rb_define_const(RG_TARGET_NAMESPACE, "SECONDARY", GDKATOM2RVAL(GDK_SELECTION_SECONDARY));
    rb_define_const(RG_TARGET_NAMESPACE, "CLIPBOARD", GDKATOM2RVAL(GDK_SELECTION_CLIPBOARD));

    /* GdkSelectionType */
    rb_define_const(RG_TARGET_NAMESPACE, "TYPE_ATOM", GDKATOM2RVAL(GDK_SELECTION_TYPE_ATOM));
    rb_define_const(RG_TARGET_NAMESPACE, "TYPE_BITMAP", GDKATOM2RVAL(GDK_SELECTION_TYPE_BITMAP));
    rb_define_const(RG_TARGET_NAMESPACE, "TYPE_COLORMAP", GDKATOM2RVAL(GDK_SELECTION_TYPE_COLORMAP));
    rb_define_const(RG_TARGET_NAMESPACE, "TYPE_DRAWABLE", GDKATOM2RVAL(GDK_SELECTION_TYPE_DRAWABLE));
    rb_define_const(RG_TARGET_NAMESPACE, "TYPE_INTEGER", GDKATOM2RVAL(GDK_SELECTION_TYPE_INTEGER));
    rb_define_const(RG_TARGET_NAMESPACE, "TYPE_PIXMAP", GDKATOM2RVAL(GDK_SELECTION_TYPE_PIXMAP));
    rb_define_const(RG_TARGET_NAMESPACE, "TYPE_WINDOW", GDKATOM2RVAL(GDK_SELECTION_TYPE_WINDOW));
    rb_define_const(RG_TARGET_NAMESPACE, "TYPE_STRING", GDKATOM2RVAL(GDK_SELECTION_TYPE_STRING));

    /* GdkTarget */
    rb_define_const(RG_TARGET_NAMESPACE, "TARGET_BITMAP", GDKATOM2RVAL(GDK_TARGET_BITMAP));
    rb_define_const(RG_TARGET_NAMESPACE, "TARGET_COLORMAP", GDKATOM2RVAL(GDK_TARGET_COLORMAP));
    rb_define_const(RG_TARGET_NAMESPACE, "TARGET_DRAWABLE", GDKATOM2RVAL(GDK_TARGET_DRAWABLE));
    rb_define_const(RG_TARGET_NAMESPACE, "TARGET_PIXMAP", GDKATOM2RVAL(GDK_TARGET_PIXMAP));
    rb_define_const(RG_TARGET_NAMESPACE, "TARGET_STRING", GDKATOM2RVAL(GDK_TARGET_STRING));

}
