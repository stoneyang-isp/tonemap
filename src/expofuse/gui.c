#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include<pthread.h>

#include "expofuse.h"

typedef struct{
	GtkWidget *file_chooser;
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *reset_button;
	GtkWidget *exportbutton;
	GtkWidget *table;
	GtkWidget *vbox;
	GtkWidget *imagen;
	GdkPixbuf *img;
	GtkWidget *s_contrast;
	GtkWidget *s_saturation;
	GtkWidget *s_exposeness;
	GtkWidget *s_sigma;
	GtkWidget *s_post_contrast;
	GSList    *files;
	//pthread_t worker;

	int k, n_samples;
	char* folder;
	Matrix** weights;
	ColorImage* fused_image;
	ColorImage** color_images;
	ColorImage** color_images_large;
	const char* output_name;
	double contrast_weight;
	double saturation_weight;
	double exposeness_weight;
	double sigma;
	double post_contrast_weight;
} GUI;

/*typedef struct{
	GUI* gui;
	int value;
} params;

void* worker_draw(void* p){
	GUI* gui=((params*)p)->gui;
	int val=((params*)p)->value;
	gtk_widget_set_sensitive(gui->button,FALSE);
	int n_channels = gdk_pixbuf_get_n_channels(gui->img);
	int width = gdk_pixbuf_get_width(gui->img);
	int height = gdk_pixbuf_get_height(gui->img);
	int rowstride = gdk_pixbuf_get_rowstride(gui->img);
	guchar* im=gdk_pixbuf_get_pixels(gui->img);
	int x,y,n;
	for(n=0;n<1000;n++) for(y=0;y<height;y++) for(x=0;x<width;x++){
		guchar* p = im + y*rowstride + x*n_channels;
		p[0] = val;
		p[1] = val;
		p[2] = 0;
	}
	gtk_widget_hide(gui->imagen);
	gtk_widget_show(gui->imagen);
	gtk_widget_set_sensitive(gui->button,TRUE);
	return NULL;
}*/

static void flush_gtk() {
  while (gtk_events_pending()) gtk_main_iteration();
}

void ColorImage2pixbuf(ColorImage* src, GdkPixbuf* dst){
	int n_channels = gdk_pixbuf_get_n_channels(dst);
	int width = gdk_pixbuf_get_width(dst);
	if(width > src->R->cols) width = src->R->cols;
	int height = gdk_pixbuf_get_height(dst);
	if(height > src->R->rows) height = src->R->rows;
	int rowstride = gdk_pixbuf_get_rowstride(dst);
	guchar* im=gdk_pixbuf_get_pixels(dst);
	int x,y;
	 for(y=0;y<height;y++) for(x=0;x<width;x++){
		guchar* p = im + y*rowstride + x*n_channels;
		p[0] = (guchar)(ELEM(src->R,y,x)*(ELEM(src->R,y,x)<0?-1:1)*255.0);
		p[1] = (guchar)(ELEM(src->G,y,x)*(ELEM(src->G,y,x)<0?-1:1)*255.0);
		p[2] = (guchar)(ELEM(src->B,y,x)*(ELEM(src->B,y,x)<0?-1:1)*255.0);
	}
}

static void update(GtkWidget *widget, GUI* gui){
	gui->contrast_weight =      gtk_range_get_value(GTK_RANGE(gui->s_contrast));
	gui->saturation_weight =    gtk_range_get_value(GTK_RANGE(gui->s_saturation));
	gui->exposeness_weight =    gtk_range_get_value(GTK_RANGE(gui->s_exposeness));
	gui->sigma =                gtk_range_get_value(GTK_RANGE(gui->s_sigma));
	gui->post_contrast_weight = gtk_range_get_value(GTK_RANGE(gui->s_post_contrast));
	
	gui->weights = ConstructWeights(gui->color_images,
	                                gui->n_samples,
	                                gui->contrast_weight,
	                                gui->saturation_weight,
	                                gui->exposeness_weight,
	                                gui->sigma);
	gui->fused_image = Fusion(gui->color_images, gui->weights, gui->n_samples);
	TruncateColorImage(gui->fused_image);
	SContrast(gui->fused_image, gui->post_contrast_weight);
	ColorImage2pixbuf(gui->fused_image, gui->img);
	//SaveColorImage(gui->fused_image,gui->output_name);
	gtk_widget_hide(gui->imagen);
	gtk_widget_show(gui->imagen);
	/*params* param=malloc(sizeof(params));
	param->gui=gui;
	param->value=val;
	pthread_create( &(gui->worker), NULL, worker_draw, (void*) param);
	pthread_join(gui->worker,NULL);*/
}

static void reset_params(GtkWidget *widget, GUI* gui){
  gtk_range_set_value(GTK_RANGE(gui->s_contrast), 1.);
	gtk_range_set_value(GTK_RANGE(gui->s_saturation), 1.);
	gtk_range_set_value(GTK_RANGE(gui->s_exposeness), 1.);
	gtk_range_set_value(GTK_RANGE(gui->s_sigma), .2);
	gtk_range_set_value(GTK_RANGE(gui->s_post_contrast), 0.);
	update(gui->window, gui);
}

void file_large(GFile* f, GUI* gui){
	gui->color_images_large[gui->k] = LoadColorImage(g_file_get_path(f), 0);
	gui->k++;
}
static void export(GtkWidget *widget, GUI* gui) {
  Matrix** weights;
  ColorImage* fused_image;
  update(gui->window, gui);
  GtkWidget *dialog;
     
  dialog = gtk_file_chooser_dialog_new ("Save File", NULL,
      GTK_FILE_CHOOSER_ACTION_SAVE,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
  if (gui->folder) gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), gui->folder);
  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "fused_image.jpg");

  if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    
    gtk_widget_destroy(dialog);
    GtkWidget* message = gtk_message_dialog_new(GTK_WINDOW(gui->window),
                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                 GTK_MESSAGE_INFO,
                                 GTK_BUTTONS_CANCEL,
                                 "Saving to %s...", filename);
    gtk_widget_show(message);
    flush_gtk();
    
    // Load Large Images
    gui->color_images_large = malloc(sizeof(ColorImage*)*gui->n_samples);
    gui->k = 0;
		g_slist_foreach(gui->files, (GFunc)file_large, gui);
		
		flush_gtk();
		
		// Fuse
		weights = ConstructWeights(gui->color_images_large,
		                           gui->n_samples,
                               gui->contrast_weight,
                               gui->saturation_weight,
                               gui->exposeness_weight,
                               gui->sigma);
    flush_gtk();
	  fused_image = Fusion(gui->color_images_large, weights, gui->n_samples);
	  flush_gtk();
	  TruncateColorImage(fused_image);

	  // Apply post-processing S contrast curve
	  SContrast(fused_image, gui->post_contrast_weight);

	  // Save
	  flush_gtk();
	  SaveColorImage(fused_image, filename);

    g_free(filename);
    gtk_widget_destroy(message);
  } else {
    gtk_widget_destroy(dialog);
  }
}

void file(GFile* f, GUI* gui){
	//g_print("%s\n",  g_file_get_path(f));
	gui->color_images[gui->k] = LoadColorImage(g_file_get_path(f), 460);
	gui->k++;
}
void choose_file(GtkDialog *dialog,gint resp, GUI* gui){
	if(resp==GTK_RESPONSE_CANCEL) gtk_main_quit();
	else{
		gui->files = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));
		gui->folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
		gui->n_samples = g_slist_length(gui->files);
		gui->k=0;
	
		gui->color_images = malloc(sizeof(ColorImage*)*gui->n_samples);
		g_slist_foreach(gui->files, (GFunc)file, gui);
		gtk_widget_destroy(GTK_WIDGET(dialog));

		gui->img = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
		                          gui->color_images[0]->R->cols,
		                          gui->color_images[0]->R->rows);
		gui->imagen = gtk_image_new_from_pixbuf(gui->img);
		gtk_container_add(GTK_CONTAINER(gui->vbox), gui->imagen);
		gtk_container_add(GTK_CONTAINER(gui->vbox), gui->table);
		gtk_widget_show_all(gui->window);
		update(gui->window,gui);
	}
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data){
	return FALSE;
}

/* Another callback */
static void destroy(GtkWidget *widget, gpointer data){
	gtk_main_quit();
}

int main(int argc, char *argv[]){
	GUI gui;

	gtk_init (&argc, &argv);
	
	gui.output_name = "fused_image.jpg";
	gui.contrast_weight = 1.;
	gui.saturation_weight = 1.;
	gui.exposeness_weight = 1.;
	
	gui.file_chooser = gtk_file_chooser_dialog_new(
	    "Seleccionar Imagenes", NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
	    GTK_RESPONSE_ACCEPT, NULL);
	g_signal_connect(gui.file_chooser, "response", G_CALLBACK(choose_file), &gui);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(gui.file_chooser),TRUE);
	GtkFileFilter* filter = gtk_file_filter_new();
	gtk_file_filter_add_pixbuf_formats(filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(gui.file_chooser), filter);
	gtk_widget_show(gui.file_chooser);

	gui.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect(gui.window, "delete-event", G_CALLBACK(delete_event), &gui);
	g_signal_connect(gui.window, "destroy", G_CALLBACK(destroy), &gui);
	gtk_container_set_border_width(GTK_CONTAINER(gui.window), 10);
	
	gui.button = gtk_button_new_with_label("Update");
	g_signal_connect(gui.button, "clicked", G_CALLBACK(update), &gui);

	gui.reset_button = gtk_button_new_with_label("Reset");
	g_signal_connect(gui.reset_button, "clicked", G_CALLBACK(reset_params), &gui);
	
	gui.exportbutton = gtk_button_new_with_label("Export");
	g_signal_connect(gui.exportbutton, "clicked", G_CALLBACK(export), &gui);
	
	gui.s_contrast   =  gtk_hscale_new_with_range(0., 5., .01);
	gtk_range_set_value(GTK_RANGE(gui.s_contrast), 1.);
	gtk_scale_set_value_pos(GTK_SCALE(gui.s_contrast), GTK_POS_LEFT);
	
	gui.s_saturation =  gtk_hscale_new_with_range(0., 2., .01);
	gtk_range_set_value(GTK_RANGE(gui.s_saturation), 1.);
	gtk_scale_set_value_pos(GTK_SCALE(gui.s_saturation), GTK_POS_LEFT);
	
	gui.s_exposeness =  gtk_hscale_new_with_range(0., 2., .01);
	gtk_range_set_value(GTK_RANGE(gui.s_exposeness), 1.);
	gtk_scale_set_value_pos(GTK_SCALE(gui.s_exposeness), GTK_POS_LEFT);
	
	gui.s_sigma      =  gtk_hscale_new_with_range(0., 1., .01);
	gtk_range_set_value(GTK_RANGE(gui.s_sigma), .2);
	gtk_scale_set_value_pos(GTK_SCALE(gui.s_sigma), GTK_POS_LEFT);

	gui.s_post_contrast = gtk_hscale_new_with_range(0., 1., .01);
	gtk_range_set_value(GTK_RANGE(gui.s_post_contrast), 0.);
	gtk_scale_set_value_pos(GTK_SCALE(gui.s_post_contrast), GTK_POS_LEFT);

	gui.vbox = gtk_vbox_new(FALSE, 10);
	gui.table = gtk_table_new(5, 3, FALSE);
	GtkWidget* contrast = gtk_label_new("Contrast");
	GtkWidget* saturation = gtk_label_new("Saturation");
	GtkWidget* exposeness = gtk_label_new("Exposeness");
	GtkWidget* sigma = gtk_label_new("Sigma");
	GtkWidget* post_contrast = gtk_label_new("Post Contrast");
	gtk_container_add(GTK_CONTAINER(gui.window), gui.vbox);

	gtk_table_attach(GTK_TABLE(gui.table), contrast  ,          0, 1, 0, 1, GTK_FILL, GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(gui.table), gui.s_contrast,      1, 2, 0, 1, GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(gui.table), saturation,          0, 1, 1, 2, GTK_FILL, GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(gui.table), gui.s_saturation,    1, 2, 1, 2, GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(gui.table), exposeness,          0, 1, 2, 3, GTK_FILL, GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(gui.table), gui.s_exposeness,    1, 2, 2, 3, GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(gui.table), sigma,               0, 1, 3, 4, GTK_FILL, GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(gui.table), gui.s_sigma,         1, 2, 3, 4, GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(gui.table), post_contrast,       0, 1, 4, 5, GTK_FILL, GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(gui.table), gui.s_post_contrast, 1, 2, 4, 5, GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(gui.table), gui.button,          2, 3, 0, 2, GTK_FILL, GTK_EXPAND|GTK_FILL, 8, 3);
	gtk_table_attach(GTK_TABLE(gui.table), gui.reset_button,    2, 3, 2, 3, GTK_FILL, GTK_EXPAND|GTK_FILL, 8, 3);
	gtk_table_attach(GTK_TABLE(gui.table), gui.exportbutton,    2, 3, 3, 5, GTK_FILL, GTK_EXPAND|GTK_FILL, 8, 3);

	//gtk_widget_show_all(gui.window);

	gtk_main();

	return 0;
}
