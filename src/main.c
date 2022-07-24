#include <gtk/gtk.h>

#include "ftpapp.h"

int main(int argc, char *argv[])
{
	return g_application_run(G_APPLICATION(ftp_app_new()), argc, argv);
}
