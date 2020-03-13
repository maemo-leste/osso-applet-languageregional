#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>

int main (int argc, char** argv)
{
    FILE* newlocalefile = NULL;
    char* language = NULL;
    char* region = NULL;
    char* contents = NULL;
    size_t contents_len = 0;
    int exitcode;
    int am_root;
    int unused;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <language> <region>\n", argv[0]);
        exit(1);
    }

    am_root = !getpid();

    language = argv[1];
    region = argv[2];

    newlocalefile = fopen("/var/tmp/.locale.new", "w");

    if (!newlocalefile) {
        exit(3);
    }

    contents = g_strconcat(
         "#!/bin/sh\n\nunset LC_ALL\n",
         "export LANG=", language, "\n",
         "export LC_TIME=", language, "\n",
         "export LC_MESSAGES=", language, "\n",
         "export LC_NUMERIC=", region, "\n",
         "export LC_MONETARY=", region, "\n",
         "export LC_PAPER=", region, "\n",
         "export LC_NAME=", region, "\n",
         "export LC_ADDRESS=", region, "\n",
         "export LC_TELEPHONE=", region, "\n",
         "export LC_MEASUREMENT=", region, "\n",
         "export LC_IDENTIFICATION=", region, "\n",
         NULL);
    contents_len = strlen(contents);

    if (fwrite(contents, sizeof(char), contents_len, newlocalefile) != contents_len) {
        fclose(newlocalefile);
        g_free(contents);
        exit(2);
    }
    fflush(newlocalefile);
    fsync(fileno(newlocalefile));
    fclose(newlocalefile);
    g_free(contents);

    if (am_root) {
        exitcode = system("/bin/mv -f /var/tmp/.locale.new /etc/osso-af-init/locale");
        if (exitcode != 0) {
            g_warning("Moving '%s' to '%s' failed", "/var/tmp/.locale.new", "/etc/osso-af-init/locale");
            exitcode = 3;
        }
    } else {
        exitcode = system("sudo /bin/mv -f /var/tmp/.locale.new /etc/osso-af-init/locale");
        if (exitcode != 0) {
            g_warning("Moving '%s' to '%s' failed", "/var/tmp/.locale.new", "/etc/osso-af-init/locale");
            exitcode = 3;
        }
    }

    if (am_root) {
        unused = system("bin/chmod 0755 /etc/osso-af-init/locale");
        unused = system("bin/chown 0.0 /etc/osso-af-init/locale");
    } else {
        unused = system("sudo /bin/chmod 0755 /etc/osso-af-init/locale");
        unused = system("sudo /bin/chown 0.0 /etc/osso-af-init/locale");
    }
    (void)unused;

    return exitcode;
}
