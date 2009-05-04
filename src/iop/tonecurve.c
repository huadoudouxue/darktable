
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "iop/tonecurve.h"
#include "gui/histogram.h"
#include "develop/develop.h"
#include "control/control.h"
#include "gui/gtk.h"

#define DT_GUI_CURVE_EDITOR_INSET 5
#define DT_GUI_CURVE_INFL .3f

#ifndef M_PI
# define M_PI		3.14159265358979323846	/* pi */
#endif

void init(dt_iop_module_t *module)
{
  module->data = malloc(sizeof(dt_iop_tonecurve_data_t));
  module->params = malloc(sizeof(dt_iop_tonecurve_params_t));
  module->gui_data = NULL;
  dt_iop_tonecurve_data_t *d = (dt_iop_tonecurve_data_t *)module->data;
  d->curve = gegl_curve_new(0.0, 1.0);
  (void)gegl_curve_add_point(d->curve, 0.0,  0.0);
  (void)gegl_curve_add_point(d->curve, 0.08, 0.08);
  (void)gegl_curve_add_point(d->curve, 0.4,  0.4);
  (void)gegl_curve_add_point(d->curve, 0.6,  0.6);
  (void)gegl_curve_add_point(d->curve, 0.92, 0.92);
  (void)gegl_curve_add_point(d->curve, 1.0,  1.0);
  strncpy(d->input_pad, "input", 20);
  strncpy(d->output_pad, "output", 20);

  d->node = gegl_node_new_child(module->dev->gegl, "operation", "gegl:contrast-curve", "sampling-points", 65535, "curve", d->curve, NULL);
  d->node_preview = gegl_node_new_child(module->dev->gegl, "operation", "gegl:contrast-curve", "sampling-points", 512, "curve", d->curve, NULL);
}

void cleanup(dt_iop_module_t *module)
{
  dt_iop_tonecurve_data_t *d = (dt_iop_tonecurve_data_t *)module->data;
  gegl_node_remove_child(module->dev->gegl, d->node);
  gegl_node_remove_child(module->dev->gegl, d->node_preview);
  g_free(d->curve);
  free(module->data);
  module->data = NULL;
  free(module->gui_data);
  module->gui_data = NULL;
  free(module->params);
  module->params = NULL;
}

void gui_update(struct dt_iop_module_t *self)
{
  // pull in new params
}

void gui_reset(struct dt_iop_module_t *self)
{
  dt_iop_tonecurve_data_t *d = (dt_iop_tonecurve_data_t *)self->data;
  gegl_curve_set_point(d->curve, 0, 0.0,  0.0);
  gegl_curve_set_point(d->curve, 1, 0.08, 0.08);
  gegl_curve_set_point(d->curve, 2, 0.4,  0.4);
  gegl_curve_set_point(d->curve, 3, 0.6,  0.6);
  gegl_curve_set_point(d->curve, 4, 0.92, 0.92);
  gegl_curve_set_point(d->curve, 5, 1.0,  1.0);
}

void gui_init(struct dt_iop_module_t *self)
{
  self->gui_data = malloc(sizeof(dt_iop_tonecurve_params_t));
  dt_iop_tonecurve_gui_data_t *c = (dt_iop_tonecurve_gui_data_t *)self->gui_data;

  c->mouse_x = c->mouse_y = -1.0;
  c->selected = -1; c->selected_offset = 0.0;
  c->dragging = 0;

  g_signal_connect (G_OBJECT (self->widget), "expose-event",
                    G_CALLBACK (dt_iop_tonecurve_expose), c);
  g_signal_connect (G_OBJECT (self->widget), "button-press-event",
                    G_CALLBACK (dt_iop_tonecurve_button_press), c);
  g_signal_connect (G_OBJECT (self->widget), "button-release-event",
                    G_CALLBACK (dt_iop_tonecurve_button_release), c);
  g_signal_connect (G_OBJECT (self->widget), "motion-notify-event",
                    G_CALLBACK (dt_iop_tonecurve_motion_notify), c);
  g_signal_connect (G_OBJECT (self->widget), "leave-notify-event",
                    G_CALLBACK (dt_iop_tonecurve_leave_notify), c);
  // init gtk stuff
  self->widget = GTK_WIDGET(gtk_vbox_new(FALSE, 0));
  c->area = GTK_DRAWING_AREA(gtk_drawing_area_new());
  gtk_drawing_area_size(c->area, 200, 200);
  gtk_box_pack_start(GTK_BOX(self->widget), GTK_WIDGET(c->area), TRUE, TRUE, 0);
  c->hbox = GTK_HBOX(gtk_hbox_new(FALSE, 0));
  gtk_box_pack_start(GTK_BOX(self->widget), GTK_WIDGET(c->hbox), FALSE, FALSE, 0);
  c->label = GTK_LABEL(gtk_label_new("presets"));
  gtk_box_pack_start(GTK_BOX(c->hbox), GTK_WIDGET(c->label), FALSE, FALSE, 5);
  c->presets = GTK_COMBO_BOX(gtk_combo_box_new_text());
  gtk_combo_box_append_text(GTK_COMBO_BOX(c->presets), "linear");
  gtk_combo_box_append_text(GTK_COMBO_BOX(c->presets), "med contrast");
  gtk_combo_box_append_text(GTK_COMBO_BOX(c->presets), "high contrast");
  gtk_box_pack_end(GTK_BOX(c->hbox), GTK_WIDGET(c->presets), FALSE, FALSE, 5);
}

void gui_cleanup(struct dt_iop_module_t *self)
{
  // dt_iop_tonecurve_gui_data_t *c = (dt_iop_tonecurve_gui_data_t *)self->gui_data;
  // destroy gtk stuff.
  gtk_widget_destroy(self->widget);
  free(self->gui_data);
  self->gui_data = NULL;
}

void get_output_pad(struct dt_iop_module_t *self, GeglNode **node, const gchar **pad)
{
  dt_iop_tonecurve_data_t *d = (dt_iop_tonecurve_data_t *)self->data;
  *node = d->node;
  *pad = d->output_pad;
}

void get_input_pad (struct dt_iop_module_t *self, GeglNode **node, const gchar **pad)
{
  dt_iop_tonecurve_data_t *d = (dt_iop_tonecurve_data_t *)self->data;
  *node = d->node;
  *pad = d->input_pad;
}

void get_preview_output_pad(struct dt_iop_module_t *self, GeglNode **node, const gchar **pad)
{
  dt_iop_tonecurve_data_t *d = (dt_iop_tonecurve_data_t *)self->data;
  *node = d->node_preview;
  *pad = d->output_pad;
}

void get_preview_input_pad (struct dt_iop_module_t *self, GeglNode **node, const gchar **pad)
{
  dt_iop_tonecurve_data_t *d = (dt_iop_tonecurve_data_t *)self->data;
  *node = d->node_preview;
  *pad = d->input_pad;
}

gboolean dt_iop_tonecurve_leave_notify(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
  dt_iop_module_t *self = (dt_iop_module_t *)user_data;
  dt_iop_tonecurve_gui_data_t *c = (dt_iop_tonecurve_gui_data_t *)self->gui_data;
  c->mouse_x = c->mouse_y = -1.0;
  gtk_widget_queue_draw(widget);
  return TRUE;
}

gboolean dt_iop_tonecurve_expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
  dt_iop_module_t *self = (dt_iop_module_t *)user_data;
  dt_iop_tonecurve_gui_data_t *c = (dt_iop_tonecurve_gui_data_t *)self->gui_data;
  dt_iop_tonecurve_data_t *d = (dt_iop_tonecurve_data_t *)self->data;
  dt_iop_tonecurve_params_t *p = (dt_iop_tonecurve_params_t *)self->params;
  const int inset = DT_GUI_CURVE_EDITOR_INSET;
  int width = widget->allocation.width, height = widget->allocation.height;
  cairo_surface_t *cst = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *cr = cairo_create(cst);
  // clear bg
  cairo_set_source_rgb (cr, .2, .2, .2);
  cairo_paint(cr);

  cairo_translate(cr, inset, inset);
  width -= 2*inset; height -= 2*inset;
  
#if 0
  // draw shadow around
  float alpha = 1.0f;
  for(int k=0;k<inset;k++)
  {
    cairo_rectangle(cr, -k, -k, width + 2*k, height + 2*k);
    cairo_set_source_rgba(cr, 0, 0, 0, alpha);
    alpha *= 0.6f;
    cairo_fill(cr);
  }
#else
  cairo_set_line_width(cr, 1.0);
  cairo_set_source_rgb (cr, .1, .1, .1);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_stroke(cr);
#endif

  cairo_set_source_rgb (cr, .3, .3, .3);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill(cr);

#if 0
  // TODO: gui curve and temporary data!
  if(c->mouse_y > 0 || c->dragging)
  {
    float oldx1, oldx2, oldy1, oldy2;
    // const float tmp = c->curve.m_anchors[c->selected].y;
    gegl_curve_get_point(d->curve, c->selected, &oldx1, &oldy1);
    if(c->selected == 2) gegl_curve_get_point(d->curve, 1, &oldx2, &oldy2);//tmp2 = c->curve.m_anchors[1].y;
    if(c->selected == 3) gegl_curve_get_point(d->curve, 4, &oldx2, &oldy2);
    // if(c->selected == 3) tmp2 = c->curve.m_anchors[4].y;
    if(c->selected == 2) d->curve.m_anchors[1].y = fminf(c->selected_min, fmaxf(0.0, d->curve.m_anchors[1].y + DT_GUI_CURVE_INFL*(c->selected_min - d->curve.m_anchors[2].y)));
    if(c->selected == 3) d->curve.m_anchors[4].y = fmaxf(c->selected_min, fminf(1.0, d->curve.m_anchors[4].y + DT_GUI_CURVE_INFL*(c->selected_min - d->curve.m_anchors[3].y)));
    d->curve.m_anchors[c->selected].y = c->selected_min;
    gegl_curve_calc_values(d->curve, 0.0, 1.0, DT_IOP_TONECURVE_RES, c->draw_min_xs, c->draw_min_ys);
    if(c->selected == 2) d->curve.m_anchors[1].y = fminf(c->selected_max, fmaxf(0.0, d->curve.m_anchors[1].y + DT_GUI_CURVE_INFL*(c->selected_max - d->curve.m_anchors[2].y)));
    if(c->selected == 3) d->curve.m_anchors[4].y = fmaxf(c->selected_max, fminf(1.0, d->curve.m_anchors[4].y + DT_GUI_CURVE_INFL*(c->selected_max - d->curve.m_anchors[3].y)));
    d->curve.m_anchors[c->selected].y = c->selected_max;
    gegl_curve_calc_values(d->curve, 0.0, 1.0, DT_IOP_TONECURVE_RES, c->draw_max_xs, c->draw_max_ys);
    d->curve.m_anchors[c->selected].y = tmp;
à    if(c->selected == 2) d->curve.m_anchors[1].y = tmp2;
    if(c->selected == 3) d->curve.m_anchors[4].y = tmp2;
  }
  gegl_curve_calc_values(d->curve, 0.0, 1.0, DT_IOP_TONECURVE_RES, c->draw_xs, c->draw_ys);
#endif

  // draw grid
  cairo_set_line_width(cr, .4);
  cairo_set_source_rgb (cr, .1, .1, .1);
  for(int k=1;k<4;k++)
  {
    cairo_move_to(cr, k/4.0*width, 0); cairo_line_to(cr, k/4.0*width, height);
    // cairo_move_to(cr, c->curve.m_anchors[k].x*width, 0); cairo_line_to(cr, c->curve.m_anchors[k].x*width, height);
    cairo_stroke(cr);
    cairo_move_to(cr, 0, k/4.0*height); cairo_line_to(cr, width, k/4.0*height);
    cairo_stroke(cr);
  }

  // draw selected cursor
  cairo_set_line_width(cr, 1.);
  cairo_translate(cr, 0, height);

  // draw lum h istogram in background
  dt_develop_t *dev = darktable.develop;
  uint32_t *hist, hist_max;
  hist = dev->histogram_pre;
  hist_max = dev->histogram_pre_max;
  if(hist_max > 0)
  {
    cairo_save(cr);
    cairo_scale(cr, 256.0/width, -height/(float)hist_max);
    cairo_set_source_rgba(cr, .2, .2, .2, 0.5);
    dt_gui_histogram_draw_8(cr, hist, 3);
    cairo_restore(cr);
  }
 
  if(c->mouse_y > 0 || c->dragging)
  {
    // draw min/max, if selected
    cairo_set_source_rgba(cr, .6, .6, .6, .5);
    cairo_move_to(cr, 0, 0);
    for(int k=0;k<DT_IOP_TONECURVE_RES;k++)   cairo_line_to(cr, k*width/(float)DT_IOP_TONECURVE_RES, - height*c->draw_min_ys[k]);
    for(int k=DT_IOP_TONECURVE_RES-2;k>0;k--) cairo_line_to(cr, k*width/(float)DT_IOP_TONECURVE_RES, - height*c->draw_max_ys[k]);
    cairo_close_path(cr);
    cairo_fill(cr);
    // draw mouse focus circle
    cairo_set_source_rgb(cr, .9, .9, .9);
    const float pos = DT_IOP_TONECURVE_RES * c->mouse_x/(float)width;
    int k = (int)pos; const float f = k - pos;
    if(k >= DT_IOP_TONECURVE_RES-1) k = DT_IOP_TONECURVE_RES - 2;
    float ht = -height*(f*c->draw_ys[k] + (1-f)*c->draw_ys[k+1]);
    cairo_arc(cr, c->mouse_x, ht+2.5, 4, 0, 2.*M_PI);
    cairo_stroke(cr);
  }

  // draw curve
  cairo_set_line_width(cr, 2.);
  cairo_set_source_rgb(cr, .9, .9, .9);
  // cairo_set_line_cap  (cr, CAIRO_LINE_CAP_SQUARE);
  cairo_move_to(cr, 0, 0);
  for(int k=0;k<DT_IOP_TONECURVE_RES;k++) cairo_line_to(cr, k*width/(float)DT_IOP_TONECURVE_RES, - height*c->draw_ys[k]);
  cairo_stroke(cr);

  cairo_destroy(cr);
  cairo_t *cr_pixmap = gdk_cairo_create(gtk_widget_get_window(widget));
  cairo_set_source_surface (cr_pixmap, cst, 0, 0);
  cairo_paint(cr_pixmap);
  cairo_destroy(cr_pixmap);
  cairo_surface_destroy(cst);
  return TRUE;
}

gboolean dt_iop_tonecurve_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
  dt_iop_module_t *self = (dt_iop_module_t *)user_data;
  dt_iop_tonecurve_gui_data_t *c = (dt_iop_tonecurve_gui_data_t *)self->gui_data;
  const int inset = DT_GUI_CURVE_EDITOR_INSET;
  int height = widget->allocation.height - 2*inset, width = widget->allocation.width - 2*inset;
  if(!c->dragging) c->mouse_x = CLAMP(event->x - inset, 0, width);
  c->mouse_y = CLAMP(event->y - inset, 0, height);

  // TODO: port to gegl curve!
#if 0
  if(c->dragging)
  {
    float f = c->selected_y - (c->mouse_y-c->selected_offset)/height;
    f = fmaxf(c->selected_min, fminf(c->selected_max, f));
    if(c->selected == 2) c->curve.m_anchors[1].y = fminf(f, fmaxf(0.0, c->curve.m_anchors[1].y + DT_GUI_CURVE_INFL*(f - c->curve.m_anchors[2].y)));
    if(c->selected == 3) c->curve.m_anchors[4].y = fmaxf(f, fminf(1.0, c->curve.m_anchors[4].y + DT_GUI_CURVE_INFL*(f - c->curve.m_anchors[3].y)));
    c->curve.m_anchors[c->selected].y = f;
    DT_CTL_SET_IMAGE(tonecurve_y[c->selected], f);
    if(c->selected == 2) DT_CTL_SET_IMAGE(tonecurve_y[1], c->curve.m_anchors[1].y);
    if(c->selected == 3) DT_CTL_SET_IMAGE(tonecurve_y[4], c->curve.m_anchors[4].y);
    dt_dev_add_history_item(darktable.develop, "tonecurve");
  }
  else
  {
    float pos = (event->x - inset)/width;
    float min = 100.0;
    int nearest = 0;
    for(int k=1;k<c->curve.m_numAnchors-1;k++)
    {
      float dist = (pos - c->curve.m_anchors[k].x); dist *= dist;
      if(dist < min) { min = dist; nearest = k; }
    }
    c->selected = nearest;
    c->selected_y = c->curve.m_anchors[c->selected].y;
    c->selected_offset = c->mouse_y;
    // c->selected_min = .5f*(c->selected_y + c->curve.m_anchors[c->selected-1].y);
    // c->selected_max = .5f*(c->selected_y + c->curve.m_anchors[c->selected+1].y);
    const float f = 0.8f;
    c->selected_min = fmaxf(c->selected_y - 0.2f, (1.-f)*c->selected_y + f*c->curve.m_anchors[c->selected-1].y);
    c->selected_max = fminf(c->selected_y + 0.2f, (1.-f)*c->selected_y + f*c->curve.m_anchors[c->selected+1].y);
    if(c->selected == 1) c->selected_max *= 0.7;
    if(c->selected == 4) c->selected_min = 1.0 - 0.7*(1.0 - c->selected_min);
  }
#endif
  gtk_widget_queue_draw(widget);
  gint x, y;
  gdk_window_get_pointer(event->window, &x, &y, NULL);
  return TRUE;
}

gboolean dt_iop_tonecurve_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{ // set active point
  dt_iop_module_t *self = (dt_iop_module_t *)user_data;
  dt_iop_tonecurve_gui_data_t *c = (dt_iop_tonecurve_gui_data_t *)self->gui_data;
  c->dragging = 1;
  return TRUE;
}

gboolean dt_iop_tonecurve_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  dt_iop_module_t *self = (dt_iop_module_t *)user_data;
  dt_iop_tonecurve_gui_data_t *c = (dt_iop_tonecurve_gui_data_t *)self->gui_data;
  c->dragging = 0;
  return TRUE;
}

