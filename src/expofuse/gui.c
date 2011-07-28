#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include<pthread.h>
//using namespace std;

typedef struct{
	GtkWidget* file_chooser;
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *imagen;
	GdkPixbuf *img;
	GtkWidget *s_contrast;
	GtkWidget *s_saturation;
	GtkWidget *s_exposeness;
	GtkWidget *s_sigma;
	pthread_t worker;
} GUI;

typedef struct{
	GUI* gui;
	int value;
} params;

/*void* worker_draw(void* p){
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

static void update(GtkWidget *widget, GUI* gui){
	gdouble value=gtk_range_get_value(GTK_RANGE(gui->s_contrast));
	int val = value*(255);
	g_print("%d\n",val);
	int n_channels = gdk_pixbuf_get_n_channels(gui->img);
	int width = gdk_pixbuf_get_width(gui->img);
	int height = gdk_pixbuf_get_height(gui->img);
	int rowstride = gdk_pixbuf_get_rowstride(gui->img);
	guchar* im=gdk_pixbuf_get_pixels(gui->img);
	int x,y;
	 for(y=0;y<height;y++) for(x=0;x<width;x++){
		guchar* p = im + y*rowstride + x*n_channels;
		p[0] = val;
		p[1] = val;
		p[2] = 0;
	}
	gtk_widget_hide(gui->imagen);
	gtk_widget_show(gui->imagen);
	
	/*params* param=malloc(sizeof(params));
	param->gui=gui;
	param->value=val;
	pthread_create( &(gui->worker), NULL, worker_draw, (void*) param);
	pthread_join(gui->worker,NULL);*/
}

void file(GFile* f, GUI* gui){
	g_print("%s\n",  g_file_get_path(f));
}
void choose_file(GtkDialog *dialog,gint resp, GUI* gui){
	if(resp==GTK_RESPONSE_CANCEL) gtk_main_quit();
	else{
		GSList* files = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));
		g_slist_foreach(files, (GFunc)file, gui);
		gtk_widget_destroy(GTK_WIDGET(dialog));
		gtk_widget_show_all(gui->window);
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
	
	gui.file_chooser = gtk_file_chooser_dialog_new("Seleccionar Imagenes", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	g_signal_connect(gui.file_chooser, "response", G_CALLBACK(choose_file), &gui);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(gui.file_chooser), TRUE);
	GtkFileFilter* filter = gtk_file_filter_new();
	gtk_file_filter_add_pixbuf_formats(filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(gui.file_chooser), filter);
	gtk_widget_show(gui.file_chooser);

	gui.img =    gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 640, 480);
	gui.imagen = gtk_image_new_from_pixbuf(gui.img);
	gui.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gui.button = gtk_button_new_with_label("Update");
	gui.s_contrast   =  gtk_hscale_new_with_range(0., 2., .01); gtk_range_set_value(GTK_RANGE(gui.s_contrast), 1.); gtk_scale_set_value_pos(GTK_SCALE(gui.s_contrast), GTK_POS_LEFT);
	gui.s_saturation =  gtk_hscale_new_with_range(0., 2., .01); gtk_range_set_value(GTK_RANGE(gui.s_saturation), 1.); gtk_scale_set_value_pos(GTK_SCALE(gui.s_saturation), GTK_POS_LEFT);
	gui.s_exposeness =  gtk_hscale_new_with_range(0., 2., .01); gtk_range_set_value(GTK_RANGE(gui.s_exposeness), 1.); gtk_scale_set_value_pos(GTK_SCALE(gui.s_exposeness), GTK_POS_LEFT);
	gui.s_sigma      =  gtk_hscale_new_with_range(0., 1., .01); gtk_range_set_value(GTK_RANGE(gui.s_sigma), .2); gtk_scale_set_value_pos(GTK_SCALE(gui.s_sigma), GTK_POS_LEFT);

	g_signal_connect(gui.window, "delete-event", G_CALLBACK(delete_event), &gui);
	g_signal_connect(gui.window, "destroy", G_CALLBACK(destroy), &gui);
	g_signal_connect(gui.button, "clicked", G_CALLBACK(update), &gui);
	//g_signal_connect(gui.scale, "value-changed", G_CALLBACK(draw), &gui);


	gtk_container_set_border_width(GTK_CONTAINER(gui.window), 10);
	GtkWidget* vbox = gtk_vbox_new(FALSE,10);
	GtkWidget* table = gtk_table_new(4,3,FALSE);
	GtkWidget* contrast = gtk_label_new("Contrast");
	GtkWidget* saturation = gtk_label_new("Saturation");
	GtkWidget* exposeness = gtk_label_new("Exposeness");
	GtkWidget* sigma = gtk_label_new("Sigma");
	gtk_container_add(GTK_CONTAINER(gui.window), vbox);
	gtk_container_add(GTK_CONTAINER(vbox), gui.imagen);
	gtk_container_add(GTK_CONTAINER(vbox), table);
	gtk_table_attach(GTK_TABLE(table), contrast  ,       0, 1, 0, 1, GTK_FILL, GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(table), gui.s_contrast  , 1, 2, 0, 1, GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(table), saturation,       0, 1, 1, 2, GTK_FILL, GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(table), gui.s_saturation, 1, 2, 1, 2, GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(table), exposeness,       0, 1, 2, 3, GTK_FILL, GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(table), gui.s_exposeness, 1, 2, 2, 3, GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(table), sigma     ,       0, 1, 3, 4, GTK_FILL, GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(table), gui.s_sigma     , 1, 2, 3, 4, GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 10, 3);
	gtk_table_attach(GTK_TABLE(table), gui.button,       2, 3, 0, 4, GTK_FILL, GTK_EXPAND|GTK_FILL, 14, 14);

	//gtk_widget_show_all(gui.window);

	gtk_main();

	return 0;
}
