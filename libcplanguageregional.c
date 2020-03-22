#include <stdio.h>
#include <locale.h>
#include <libintl.h>
#include <langinfo.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>


#include <hildon-cp-plugin/hildon-cp-plugin-interface.h>
#include <hildon/hildon.h>
#include <gtk/gtk.h>


#define COLUMN_PRETTY 0
#define COLUMN_LANG 1

#define COLUMN_REGION_TRANS 0
#define COLUMN_REGION_NAME 1

#define COLUMN_LANGUAGE_TRANS 0
#define COLUMN_LANGUAGE_NAME 1

struct languageinfo {
    char* lang;
    char* pretty;
};

struct regioninfo {
    char* region;
    char* pretty;
};



GSList *get_language_list(void);
GSList *get_region_list(void);
void language_settings_changed(HildonPickerButton* button, gboolean unknown_bool);
void region_settings_changed(HildonPickerButton* button);
gchar* get_local_region_name(void);
gchar* get_local_language_name(void);


static GtkWidget* decimal_label = NULL;
static GtkWidget* thousands_label = NULL;
static GtkWidget* format_label = NULL;

static GtkWidget* language_popup_button = NULL;
static GtkWidget* region_popup_button = NULL;

static gchar* last_language = NULL;
static gchar* last_region = NULL;

char **locales_list = NULL;


void free_static_vars() {
    if (last_region) {
        g_free(last_region);
        last_region = NULL;
    }
    if (last_language) {
        g_free(last_language);
        last_language = NULL;
    }

    if (locales_list) {
        char** locales_iter = locales_list;
        while (*locales_iter != NULL) {
            free(*locales_iter);
            locales_iter++;
        }
        free(locales_list);
    }

    return;
}

GtkDialog* create_main_dialog(gpointer user_data, osso_context_t *osso){
    (void)osso;

    GtkWidget *widget = NULL;


    widget = gtk_widget_new(GTK_TYPE_DIALOG,
                            "transient-for",
                            GTK_WINDOW(user_data),
                            "destroy-with-parent", TRUE,
                            "resizable", TRUE,
                            "has-separator", FALSE,
                            "modal", TRUE,
                            NULL);

    const char* window_title = g_dgettext("osso-clock", "cpal_ti_language_and_regional_title");
    gtk_window_set_title(GTK_WINDOW(widget), window_title);

    GdkGeometry geom;
    geom.max_width = -1;
    geom.min_width = -1;
    geom.max_height = 280;
    geom.min_height = 280;
    gtk_window_set_geometry_hints(GTK_WINDOW(widget), widget, &geom, GDK_HINT_MAX_SIZE|GDK_HINT_MIN_SIZE);

    const char* save_text = g_dgettext("hildon-libs", "wdgt_bd_save");
    gtk_dialog_add_button(GTK_DIALOG(widget), save_text, -5); // TODO: -5

    GtkWidget* pannable = hildon_pannable_area_new();
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(widget)->vbox), pannable);
    gtk_widget_show(pannable);

    GtkWidget *vbox = gtk_vbox_new(0, 8);
    hildon_pannable_area_add_with_viewport((HildonPannableArea*)pannable, vbox);

    GtkSizeGroup *size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    /* Add device language touchable list + widget */
    const char* device_language_translated_text = g_dgettext("osso-clock", "cpal_fi_pr_device_language");
    char* current_lc_messages = setlocale(LC_MESSAGES, NULL);
    GtkListStore* list_store_lang = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

    GSList* language_list = get_language_list();
    if (!language_list || !language_list->data) {
        // XXX
    }

    GSList* language_list_iter = language_list;
    GtkTreeIter treeiter;
    GtkTreeIter* current_language_iter = NULL;

    while (language_list_iter && language_list_iter->data) {
        gtk_list_store_append(list_store_lang, &treeiter);

        struct languageinfo* info = (struct languageinfo*)(language_list_iter->data);
        gtk_list_store_set(list_store_lang, &treeiter,
                                COLUMN_PRETTY, info->pretty,
                                COLUMN_LANG, info->lang,
                                -1);

        if (!g_ascii_strcasecmp(info->lang, current_lc_messages)) {
            // This is the current language
            current_language_iter = gtk_tree_iter_copy(&treeiter);
            last_language = g_strdup(info->lang);
        }

        language_list_iter = language_list_iter->next;
    }

    // XXX: free language_glist

    HildonTouchSelector* hildon_touch_sel_lang = (HildonTouchSelector*)hildon_touch_selector_new();
    HildonTouchSelectorColumn *col = hildon_touch_selector_append_text_column(hildon_touch_sel_lang, GTK_TREE_MODEL(list_store_lang), TRUE);
    //g_object_set(G_OBJECT(col), "text-column"); // XXX

    hildon_touch_selector_set_column_selection_mode(hildon_touch_sel_lang, FALSE);

    if (current_language_iter) {
        hildon_touch_selector_select_iter(hildon_touch_sel_lang, 0, current_language_iter, FALSE);
        gtk_tree_iter_free(current_language_iter);
    }

    language_popup_button = gtk_widget_new(HILDON_TYPE_PICKER_BUTTON,
            "size", 4,
            "arrangement", 1,
            "title", device_language_translated_text,
            "touch-selector",
            hildon_touch_sel_lang,
            NULL);
    hildon_button_set_title_alignment(HILDON_BUTTON(language_popup_button), 0.0, 0.5);
    hildon_button_set_value_alignment(HILDON_BUTTON(language_popup_button), 0.0, 0.5);
    hildon_button_set_alignment(HILDON_BUTTON(language_popup_button), 0.0, 0.5, 1.0, 1.0);
    gtk_box_pack_start(GTK_BOX(vbox), language_popup_button, FALSE, FALSE, 0);
    GtkWidget *table_widget_lang = gtk_table_new(1, 2, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(table_widget_lang), FALSE); // XXX: we set it above, why unset it again?
    gtk_table_set_col_spacings(GTK_TABLE(table_widget_lang), 24);
    const char* table_label_date_format = g_dgettext("osso-clock", "cpal_fi_pr_info_date_format");
    GtkWidget* date_format_label = gtk_label_new(table_label_date_format);
    gtk_size_group_add_widget(size_group, date_format_label);

    format_label = gtk_label_new(NULL);

    gtk_misc_set_alignment(GTK_MISC(date_format_label), 0.0, 0.5);
    hildon_helper_set_logical_color(date_format_label, GTK_RC_FG, 0, "SecondaryTextColor");
    gtk_misc_set_alignment(GTK_MISC(format_label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_widget_lang), date_format_label, 0, 1, 0, 1, GTK_SHRINK, GTK_EXPAND, 13, 0);
    gtk_table_attach_defaults(GTK_TABLE(table_widget_lang), format_label, 1, 2, 0, 1);
    gtk_container_add(GTK_CONTAINER(vbox), table_widget_lang);

    const char* device_region_translated_text = g_dgettext("osso-clock", "cpal_fi_pr_info_date_format");
    char* current_lc_numeric = setlocale(LC_NUMERIC, NULL);
    GtkListStore* list_store_region = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

    GSList* region_list = get_region_list();
    if (!region_list || !region_list->data) {
        // XXX
    }

    GSList* region_list_iter = region_list;
    GtkTreeIter region_treeiter;
    GtkTreeIter* current_region_iter = NULL;

    while (region_list_iter && region_list_iter->data) {
        gtk_list_store_append(list_store_region, &region_treeiter);

        struct regioninfo* info = (struct regioninfo*)(region_list_iter->data);
        gtk_list_store_set(list_store_region, &region_treeiter,
                                COLUMN_PRETTY, info->pretty,
                                COLUMN_LANG, info->region,
                                -1);

        if (!g_ascii_strcasecmp(info->region, current_lc_numeric)) {
            // This is the current language
            current_region_iter = gtk_tree_iter_copy(&region_treeiter);
            last_region = g_strdup(info->region);
        }

        region_list_iter = region_list_iter->next;
    }
    // XXX: free region_glist

    HildonTouchSelector* hildon_touch_sel_region = (HildonTouchSelector*)hildon_touch_selector_new();
    HildonTouchSelectorColumn *region_col = hildon_touch_selector_append_text_column(hildon_touch_sel_region, GTK_TREE_MODEL(list_store_region), TRUE);
    //g_object_set(G_OBJECT(region_col), "text-column"); // XXX
    hildon_touch_selector_set_column_selection_mode(hildon_touch_sel_lang, FALSE);

    if (current_region_iter) {
        hildon_touch_selector_select_iter(hildon_touch_sel_region, 0, current_region_iter, FALSE);
        gtk_tree_iter_free(current_region_iter);
    }

    region_popup_button = gtk_widget_new(HILDON_TYPE_PICKER_BUTTON,
            "size", 4,
            "arrangement", 1,
            "title", device_region_translated_text,
            "touch-selector",
            hildon_touch_sel_region,
            NULL);
    hildon_button_set_title_alignment(HILDON_BUTTON(region_popup_button), 0.0, 0.5);
    hildon_button_set_value_alignment(HILDON_BUTTON(region_popup_button), 0.0, 0.5);
    hildon_button_set_alignment(HILDON_BUTTON(region_popup_button), 0.0, 0.5, 1.0, 1.0);
    gtk_box_pack_start(GTK_BOX(vbox), region_popup_button, FALSE, FALSE, 0);
    GtkWidget *table_widget_region = gtk_table_new(1, 2, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(table_widget_region), FALSE); // XXX: we set it above, why unset it again?
    gtk_table_set_col_spacings(GTK_TABLE(table_widget_region), 24);

    const char* table_decimal_sep_translated = g_dgettext("osso-clock", "cpal_fi_pr_info_decimal_point");
    GtkWidget* decimal_label_sep = gtk_label_new(table_decimal_sep_translated);
    gtk_size_group_add_widget(size_group, decimal_label_sep);

    decimal_label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(decimal_label_sep), 0.0, 0.5);
    hildon_helper_set_logical_color(decimal_label_sep, GTK_RC_FG, 0, "SecondaryTextColor");
    gtk_misc_set_alignment(GTK_MISC(decimal_label), 0.0, 0.5);

    gtk_table_attach(GTK_TABLE(table_widget_region), decimal_label_sep, 0, 1, 0, 1, GTK_SHRINK, GTK_EXPAND, 13, 0);
    gtk_table_attach_defaults(GTK_TABLE(table_widget_region), decimal_label, 1, 2, 0, 1);

    const char* table_thousands_sep_translated = g_dgettext("osso-clock", "cpal_fi_pr_info_thousands_separator");
    GtkWidget* thousands_label_sep = gtk_label_new(table_thousands_sep_translated);
    gtk_size_group_add_widget(size_group, thousands_label_sep);

    thousands_label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(thousands_label_sep), 0.0, 0.5);
    hildon_helper_set_logical_color(thousands_label_sep, GTK_RC_FG, 0, "SecondaryTextColor");
    gtk_misc_set_alignment(GTK_MISC(thousands_label), 0.0, 0.5);

    gtk_table_attach(GTK_TABLE(table_widget_region), thousands_label_sep, 0, 1, 1, 2, GTK_SHRINK, GTK_EXPAND, 13, 0);
    gtk_table_attach_defaults(GTK_TABLE(table_widget_region), thousands_label, 1, 2, 1, 2);


    gtk_container_add(GTK_CONTAINER(vbox), table_widget_region);
    (void)col;
    (void)region_col;

    gtk_widget_show_all(widget);

    /* TODO: Store signal connect results and close them later */
    g_signal_connect_data(language_popup_button, "value-changed", (GCallback)language_settings_changed, NULL, 0, G_CONNECT_AFTER);
    g_signal_connect_data(region_popup_button, "value-changed", (GCallback)region_settings_changed, NULL, 0, G_CONNECT_AFTER);

    region_settings_changed(HILDON_PICKER_BUTTON(region_popup_button));
    language_settings_changed(HILDON_PICKER_BUTTON(language_popup_button), TRUE);

    g_object_unref(size_group);

    return GTK_DIALOG(widget);
}


gint ask_confirm_reboot(GtkDialog* dialog) {
    (void)dialog;
    gint response;

    const char* confirmation_text_trans = g_dgettext("osso-clock","cpal_nc_changing_language");
    GtkWidget* note = hildon_note_new_confirmation(GTK_WINDOW(dialog), confirmation_text_trans);
    response = gtk_dialog_run(GTK_DIALOG(note));
    gtk_widget_destroy(note);

    return response;
}

static char** get_locales_list(void) {
    char** res = NULL;
    int res_cnt = 0;

    int exit_status;
    char* stdout_c;
    g_spawn_command_line_sync("localedef --list-archive", &stdout_c, NULL, &exit_status, NULL);
    if (exit_status != 0) {
        goto done_1;
    }

    char** lines = g_strsplit(stdout_c, "\n", 0);
    if (lines == NULL) {
        goto done_1;
    }

    char** line_iter = lines;
    while ((line_iter) && (*line_iter != NULL) && (strlen(*line_iter) > 1)) {
        char* ok;
        char* ppos = strstr(*line_iter, "utf8");
        if (ppos) {
            int pos = ppos - *line_iter;
            char* first_part = g_strndup(*line_iter, pos);

            ok = g_strdup_printf("%s%s", first_part, "utf-8");

            g_free(first_part);
        } else {
            ok = g_strdup(*line_iter);
        }
        res_cnt += 1;
        res = realloc(res, sizeof(char*) * res_cnt);
        res[res_cnt-1] = ok;
        line_iter++;
    }
    res = realloc(res, sizeof(char*) * res_cnt + 1);
    res[res_cnt] = NULL;

    g_strfreev(lines);

done_1:
    g_free(stdout_c);
    return res;
}


GSList *get_language_list(void) {
    GSList* resolved_languages = NULL;

    if (locales_list == NULL) {
        locales_list = get_locales_list();
    }

    char** locales_list_iter = locales_list;

    while (*locales_list_iter) {
        struct languageinfo *info = malloc(sizeof(struct languageinfo));

        info->lang= *locales_list_iter;

        char* old_locale = g_strdup(setlocale(LC_ADDRESS, NULL));
        setlocale(LC_ADDRESS, info->lang);
        char* lang_country = get_local_region_name();
        char* lang_name = get_local_language_name();
        info->pretty = g_strdup_printf("%s (%s)", lang_name, lang_country);
        g_free(lang_country);
        g_free(lang_name);
        setlocale(LC_ADDRESS, old_locale);
        g_free(old_locale);

        resolved_languages = g_slist_append(resolved_languages, info);

        locales_list_iter++;
    }

    return resolved_languages;
}

GSList *get_region_list(void) {
    GSList* resolved_regions = NULL;

    if (locales_list == NULL) {
        locales_list = get_locales_list();
    }

    char** locales_list_iter = locales_list;

    while (*locales_list_iter) {
        /* TODO: Make sure all the entries in gslist are freed, I guess */
        struct regioninfo *info = malloc(sizeof(struct regioninfo));

        info->region = *locales_list_iter;

        char* old_locale = g_strdup(setlocale(LC_ADDRESS, NULL));
        setlocale(LC_ADDRESS, info->region);
        info->pretty = get_local_region_name();
        setlocale(LC_ADDRESS, old_locale);
        g_free(old_locale);
        resolved_regions = g_slist_append(resolved_regions, info);

        locales_list_iter++;
    }


    return resolved_regions;
}

void language_settings_changed(HildonPickerButton* button, gboolean unknown_bool) {
    HildonTouchSelector* sel = hildon_picker_button_get_selector(button);
    GtkTreeIter iter;
    char *selected_language = NULL;
    char *selected_language_trans = NULL;

    if (!hildon_touch_selector_get_selected(sel, 0, &iter)) {
        g_warning("hildon_touch_selector_get_selected failed");
        return;
    }

    GtkTreeModel* model = hildon_touch_selector_get_model(sel, 0);
    if (!model) {
        g_warning("hildon_touch_selector_get_model failed");
        return;
    }

    gtk_tree_model_get(model, &iter, COLUMN_LANGUAGE_NAME, &selected_language, -1);

    if (!selected_language) {
        g_warning("cannot get selected language");
        return;
    }


    gtk_tree_model_get(model, &iter, COLUMN_LANGUAGE_TRANS, &selected_language_trans, -1);
    hildon_button_set_value(HILDON_BUTTON(language_popup_button), selected_language_trans);

    if (last_language)
        g_free(last_language);
    last_language = g_strdup(selected_language);

    char* old_language = g_strdup(setlocale(LC_MESSAGES, NULL));

    setlocale(LC_TIME, selected_language);
    setlocale(LC_MESSAGES, selected_language);

    struct timeval tv;
    struct tm* current_date_str;

    gettimeofday(&tv, NULL);
    const char* label_day_name_short = g_dgettext("hildon-libs", "wdgt_va_fulldate_day_name_short");
    current_date_str = localtime(&tv.tv_sec);

    char date_str[64];
    memset(date_str, 0, sizeof(date_str));
    strftime(date_str, 63, label_day_name_short, current_date_str);
    if (g_str_has_prefix(date_str, "wdgt")) {
        date_str[0] = '\0';
    }

    gtk_label_set_text(GTK_LABEL(format_label), date_str);

    setlocale(LC_TIME, old_language);
    setlocale(LC_MESSAGES, old_language);

    g_free(old_language);

    if (!unknown_bool) {
        /* XXX */
    }

    return;
}

void region_settings_changed(HildonPickerButton* button) {
    HildonTouchSelector* sel = hildon_picker_button_get_selector(button);
    GtkTreeIter iter;
    char* selected_region = NULL;
    char* selected_region_trans = NULL;

    if (!hildon_touch_selector_get_selected(sel, 0, &iter)) {
        g_warning("hildon_touch_selector_get_selected failed");
        return;
    }

    GtkTreeModel* model = hildon_touch_selector_get_model(sel, 0);
    if (!model) {
        g_warning("hildon_touch_selector_get_model failed");
        return;
    }

    gtk_tree_model_get(model, &iter, COLUMN_REGION_NAME, &selected_region, -1);

    if (!selected_region) {
        g_warning("cannot get selected region");
        return;
    }

    if (last_region)
        g_free(last_region);
    last_region = g_strdup(selected_region);

    char* old_locale = g_strdup(setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, selected_region);

    char *decimalsep = nl_langinfo(RADIXCHAR);
    char *thousandsep = nl_langinfo(THOUSEP);

    gtk_label_set_text(GTK_LABEL(decimal_label), decimalsep);
    gtk_label_set_text(GTK_LABEL(thousands_label), thousandsep);

    setlocale(LC_NUMERIC, old_locale);
    g_free(old_locale);

    gtk_tree_model_get(model, &iter, COLUMN_REGION_TRANS, &selected_region_trans, -1);
    hildon_button_set_value(HILDON_BUTTON(region_popup_button), selected_region_trans);

    /* XXX: todo: if last_region */

    g_free(selected_region);

    return;
}

gchar* get_local_region_name(void) {
    gchar* local_country_name = g_strdup(nl_langinfo(_NL_ADDRESS_COUNTRY_NAME));
    return local_country_name;
}

gchar* get_local_language_name(void) {
    gchar* local_language_name = g_strdup(nl_langinfo(_NL_ADDRESS_LANG_NAME));
    return local_language_name;
}

int show_error_note(GtkDialog* dialog) {
    const gchar* errormsg = dcgettext("ke-recv", "cerm_device_memory_full", LC_MESSAGES);
    const gchar* title = g_dgettext("osso-clock", "cpal_ti_language_and_regional_title");
    char* infotext = g_strdup_printf(errormsg, title);
    GtkWidget* widget = hildon_note_new_information(GTK_WINDOW(dialog), infotext);
    gtk_widget_show_all(widget);
    int response = gtk_dialog_run(GTK_DIALOG(widget));

    gtk_widget_destroy(widget);
    g_free(infotext);

    return response;
}

gboolean save_data(GtkDialog* dialog) {
    gchar* command = NULL;
    gchar* stdoutput = NULL;
    gchar* stderror = NULL;
    gint exitcode = 0;

    gboolean ok;

    command = g_strconcat("/usr/bin/setlocale", " ", last_language, " ", last_region, NULL);
    ok = g_spawn_command_line_sync(command, &stdoutput, &stderror, &exitcode, NULL);
    if (ok) {
        if (WIFSIGNALED(exitcode) || WIFEXITED(exitcode)) {
            g_spawn_command_line_sync("/bin/rm -rf /home/user/.cache/launch", NULL, NULL, NULL, NULL);
            g_mkdir_with_parents("/home/user/.cache/launch", 511);
            DBusConnection *system_bus = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
            if (system_bus) {
                DBusMessage *message = dbus_message_new_method_call(
                   "com.nokia.dsme",
                   "/com/nokia/dsme/request",
                   "com.nokia.dsme.request",
                   "req_reboot");
                if (message) {
                    dbus_connection_send(system_bus, message, NULL);
                }
                dbus_connection_flush(system_bus);
            }
        }

        return TRUE;
    } else {
        show_error_note(dialog);
        return FALSE;
    }


}

osso_return_t execute(osso_context_t * osso, gpointer user_data, gboolean user_activated)
{
    (void)user_activated;

    setlocale(LC_MESSAGES, NULL);
    setlocale(LC_NUMERIC, NULL);
    setlocale(LC_TIME, NULL);

    GtkDialog * dialog = create_main_dialog(user_data, osso);

    if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
        gint reboot_response = ask_confirm_reboot(dialog);
        if (reboot_response == GTK_RESPONSE_OK) {
            save_data(dialog);
            // save data
            // if not save, reverse (?)
        }
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
    free_static_vars();
    return OSSO_OK;
}

