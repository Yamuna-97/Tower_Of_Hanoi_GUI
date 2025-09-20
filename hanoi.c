#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define MAX_DISKS 10
#define RODS 3
#define MAX_PLAYERS 4

typedef struct {
    char name[50];
    int total_score;
    int moves_used;
    int finished;
} Player;

typedef struct {
    int rods[RODS][MAX_DISKS];
    int top[RODS];
    int num_disks;
    int moves;
    int selected;
    GtkWidget *drawing_area;
    GtkWidget *label_moves;
    GtkWidget *label_min_moves;
    GtkWidget *window;
    GQueue *solve_moves;
    guint timer_id;
    Player *current_player;
    int current_player_index;
    int total_players;
    Player players[MAX_PLAYERS];
    GtkWidget *spin_disks; // Store reference to spin button
} HanoiGame;

HanoiGame game = {0};
static void restart_game(GtkButton *button, gpointer user_data);

// Ask number of players with radio buttons
static int ask_num_players(GtkWidget *parent) {
    GtkWidget *dialog, *content, *vbox, *label, *radio1, *radio2, *radio3, *radio4;
    gint result;
    int num = 1;

    dialog = gtk_dialog_new_with_buttons("Select Number of Players",
        GTK_WINDOW(parent),
        GTK_DIALOG_MODAL,
        "Next", GTK_RESPONSE_OK,
        NULL);

    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_box_pack_start(GTK_BOX(content), vbox, TRUE, TRUE, 0);

    label = gtk_label_new("How many players? (1-4)");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    radio1 = gtk_radio_button_new_with_label(NULL, "1 Player");
    gtk_box_pack_start(GTK_BOX(vbox), radio1, FALSE, FALSE, 0);

    radio2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio1), "2 Players");
    gtk_box_pack_start(GTK_BOX(vbox), radio2, FALSE, FALSE, 0);

    radio3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio1), "3 Players");
    gtk_box_pack_start(GTK_BOX(vbox), radio3, FALSE, FALSE, 0);

    radio4 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio1), "4 Players");
    gtk_box_pack_start(GTK_BOX(vbox), radio4, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_OK) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio1))) num = 1;
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio2))) num = 2;
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio3))) num = 3;
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio4))) num = 4;
    }

    gtk_widget_destroy(dialog);
    return (result == GTK_RESPONSE_OK) ? num : 0;
}

// Ask player names one by one — with fallback to "Player X"
static gboolean ask_player_names(GtkWidget *parent, int num_players) {
    for (int i = 0; i < num_players; i++) {
        GtkWidget *dialog, *content, *vbox, *label, *entry;
        gint result;

        dialog = gtk_dialog_new_with_buttons(g_strdup_printf("Enter Player %d Name", i+1),
            GTK_WINDOW(parent),
            GTK_DIALOG_MODAL,
            "OK", GTK_RESPONSE_OK,
            NULL);

        content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
        gtk_box_pack_start(GTK_BOX(content), vbox, TRUE, TRUE, 0);

        label = gtk_label_new("Name:");
        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

        entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter player name");
        gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

        gtk_widget_show_all(dialog);

        result = gtk_dialog_run(GTK_DIALOG(dialog));

        if (result == GTK_RESPONSE_OK) {
            const char *name = gtk_entry_get_text(GTK_ENTRY(entry));
            if (strlen(name) == 0) {
                // Use default name
                snprintf(game.players[i].name, sizeof(game.players[i].name), "Player %d", i+1);
            } else {
                strncpy(game.players[i].name, name, 49);
                game.players[i].name[49] = '\0';
            }
            game.players[i].total_score = 0;
            game.players[i].moves_used = 0;
            game.players[i].finished = 0;
        } else {
            gtk_widget_destroy(dialog);
            return FALSE;
        }

        gtk_widget_destroy(dialog);
    }

    game.total_players = num_players;
    game.current_player_index = 0;
    game.current_player = &game.players[0];

    return TRUE;
}

// Login Dialog
static gboolean login_dialog(GtkWidget *parent) {
    GtkWidget *dialog, *content, *grid, *label, *entry_user, *entry_pass;
    gint result;

    dialog = gtk_dialog_new_with_buttons("Login",
        GTK_WINDOW(parent),
        GTK_DIALOG_MODAL,
        "Sign Up", 1,
        "Login", GTK_RESPONSE_OK,
        NULL);

    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_box_pack_start(GTK_BOX(content), grid, TRUE, TRUE, 0);

    label = gtk_label_new("Username:");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    entry_user = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry_user, 1, 0, 1, 1);

    label = gtk_label_new("Password:");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    entry_pass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry_pass), FALSE);
    gtk_grid_attach(GTK_GRID(grid), entry_pass, 1, 1, 1, 1);

    gtk_widget_show_all(dialog);

    result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_OK || result == 1) {
        const char *user = gtk_entry_get_text(GTK_ENTRY(entry_user));
        const char *pass = gtk_entry_get_text(GTK_ENTRY(entry_pass));

        if (strlen(user) == 0 || strlen(pass) == 0) {
            GtkWidget *err = gtk_message_dialog_new(GTK_WINDOW(dialog),
                GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                "Username and password cannot be empty.");
            gtk_dialog_run(GTK_DIALOG(err));
            gtk_widget_destroy(err);
            gtk_widget_destroy(dialog);
            return FALSE;
        }
    }

    gtk_widget_destroy(dialog);
    return TRUE;
}

// 🎨 Draw rods + disks
static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer data) {
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    int width = alloc.width, height = alloc.height;

    cairo_set_source_rgb(cr, 0, 0, 0); // Background
    cairo_paint(cr);

    for (int r = 0; r < RODS; r++) {
        int x = (r + 1) * width / (RODS + 1);

        if (r == game.selected)
            cairo_set_source_rgb(cr, 1, 1, 0); // Yellow for selected rod
        else
            cairo_set_source_rgb(cr, 1, 0, 0); // Red for others

        cairo_set_line_width(cr, 5);
        cairo_move_to(cr, x, height - 50);
        cairo_line_to(cr, x, height / 4);
        cairo_stroke(cr);

        // Base line
        cairo_move_to(cr, x - 80, height - 50);
        cairo_line_to(cr, x + 80, height - 50);
        cairo_stroke(cr);
    }

    // Draw disks
    for (int r = 0; r < RODS; r++) {
        for (int i = 0; i < game.top[r]; i++) {
            int disk = game.rods[r][i];
            int x = (r + 1) * width / (RODS + 1);
            int y = height - 70 - i * 25;
            int w = disk * 20;

            cairo_set_source_rgb(cr, 0.3, 0.5 + 0.05 * disk, 0.3 + 0.1 * disk);
            cairo_rectangle(cr, x - w/2, y, w, 20);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_stroke(cr);
        }
    }
    return FALSE;
}

// 🏆 Check success + Calculate Score
static void check_success() {
    if (game.top[2] == game.num_disks && game.num_disks > 0) {
        int min_moves = (1 << game.num_disks) - 1;
        int score = 100 - (game.moves - min_moves);
        if (score < 0) score = 0;

        game.current_player->total_score += score;
        game.current_player->moves_used = game.moves;
        game.current_player->finished = 1;

        char buf[300];
        sprintf(buf,
            "🎉 Congratulations %s!\n\n"
            "Moves used: %d\nMinimum possible: %d\nYour Score: %d\n\n"
            "Total Score: %d",
            game.current_player->name,
            game.moves, min_moves, score,
            game.current_player->total_score);

        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(game.window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", buf);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        // Auto-switch to next unfinished player
        int next = (game.current_player_index + 1) % game.total_players;
        int count = 0;
        while (game.players[next].finished && count < game.total_players) {
            next = (next + 1) % game.total_players;
            count++;
        }
        if (count < game.total_players) {
            game.current_player_index = next;
            game.current_player = &game.players[next];
            restart_game(NULL, NULL);
        }
    }
}

// 🖱 Mouse click handler
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    int width = alloc.width;
    int rod_width = width / (RODS + 1);

    int clicked = -1;
    for (int r = 0; r < RODS; r++) {
        int x = (r + 1) * width / (RODS + 1);
        if (fabs(event->x - x) < rod_width / 2) {
            clicked = r;
            break;
        }
    }
    if (clicked == -1) return TRUE;

    if (game.selected == -1) {
        if (game.top[clicked] > 0)
            game.selected = clicked;
    } else {
        if (clicked != game.selected) {
            int from = game.selected, to = clicked;
            int disk = game.rods[from][game.top[from] - 1];
            if (game.top[to] == 0 || game.rods[to][game.top[to] - 1] > disk) {
                game.rods[to][game.top[to]++] = disk;
                game.top[from]--;
                game.moves++;

                char buf[100];
                sprintf(buf, "Moves: %d | Player: %s", game.moves, game.current_player->name);
                gtk_label_set_text(GTK_LABEL(game.label_moves), buf);

                check_success();
            }
        }
        game.selected = -1;
    }

    gtk_widget_queue_draw(widget);
    return TRUE;
}

// 🔄 Restart game — NOW READS SPIN BUTTON VALUE
static void restart_game(GtkButton *button, gpointer user_data) {
    // Read value from spin button if provided
    if (user_data != NULL && GTK_IS_SPIN_BUTTON(user_data)) {
        game.num_disks = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(user_data));
    } else {
        // Fallback: if called without spin button, keep current or default to 3
        if (game.num_disks < 3 || game.num_disks > MAX_DISKS) {
            game.num_disks = 3;
        }
    }

    for (int r = 0; r < RODS; r++)
        game.top[r] = 0;

    for (int i = game.num_disks; i >= 1; i--)
        game.rods[0][game.top[0]++] = i;

    game.moves = 0;
    game.selected = -1;

    if (game.timer_id) {
        g_source_remove(game.timer_id);
        game.timer_id = 0;
    }

    char buf[100];
    sprintf(buf, "Moves: %d | Player: %s", game.moves, game.current_player->name);
    gtk_label_set_text(GTK_LABEL(game.label_moves), buf);

    sprintf(buf, "Minimum Moves: %d", (1 << game.num_disks) - 1);
    gtk_label_set_text(GTK_LABEL(game.label_min_moves), buf);

    gtk_widget_queue_draw(game.drawing_area);
}

// 🔄 Switch Player button
static void switch_player(GtkButton *button, gpointer user_data) {
    int next = (game.current_player_index + 1) % game.total_players;
    int count = 0;
    while (game.players[next].finished && count < game.total_players) {
        next = (next + 1) % game.total_players;
        count++;
    }
    if (count < game.total_players) {
        game.current_player_index = next;
        game.current_player = &game.players[next];
        restart_game(NULL, NULL);
    } else {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(game.window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            "All players have finished!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

// 📜 Log button — Show player stats
static void log_moves(GtkButton *button, gpointer user_data) {
    char buf[500] = "📊 Player Stats:\n\n";
    for (int i = 0; i < game.total_players; i++) {
        char tmp[100];
        sprintf(tmp, "%s: %d pts (moves: %d)\n",
                game.players[i].name,
                game.players[i].total_score,
                game.players[i].moves_used);
        strcat(buf, tmp);
    }

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(user_data),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", buf);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// 🔧 Recursive solve builder
static void build_moves(int n, int from, int to, int aux) {
    if (n == 0) return;
    build_moves(n - 1, from, aux, to);
    int *move = g_new(int, 2);
    move[0] = from; move[1] = to;
    g_queue_push_tail(game.solve_moves, move);
    build_moves(n - 1, aux, to, from);
}

// ⏱ Solver step
static gboolean solver_step(gpointer data) {
    if (g_queue_is_empty(game.solve_moves)) {
        game.timer_id = 0;
        check_success();
        return FALSE;
    }

    int *move = g_queue_pop_head(game.solve_moves);
    int from = move[0], to = move[1];
    g_free(move);

    if (game.top[from] <= 0) return TRUE;

    int disk = game.rods[from][game.top[from] - 1];
    game.rods[to][game.top[to]++] = disk;
    game.top[from]--;
    game.moves++;

    char buf[100];
    sprintf(buf, "Moves: %d | Player: %s", game.moves, game.current_player->name);
    gtk_label_set_text(GTK_LABEL(game.label_moves), buf);

    gtk_widget_queue_draw(game.drawing_area);
    return TRUE;
}

// ▶️ Solve button
static void solve_game(GtkButton *button, gpointer user_data) {
    if (game.timer_id) return;

    if (game.solve_moves) g_queue_free_full(game.solve_moves, g_free);
    game.solve_moves = g_queue_new();

    build_moves(game.num_disks, 0, 2, 1);
    game.timer_id = g_timeout_add(500, solver_step, NULL);
}

// 🏁 Finish button — Show final rankings
static void finish_game(GtkButton *button, gpointer user_data) {
    // Sort players by score (descending)
    for (int i = 0; i < game.total_players - 1; i++) {
        for (int j = i + 1; j < game.total_players; j++) {
            if (game.players[i].total_score < game.players[j].total_score) {
                Player temp = game.players[i];
                game.players[i] = game.players[j];
                game.players[j] = temp;
            }
        }
    }

    char buf[1000] = "🏆 FINAL RANKINGS\n\n";
    for (int i = 0; i < game.total_players; i++) {
        char tmp[150];
        sprintf(tmp, "%d. %s — Score: %d (Moves: %d)\n",
                i+1,
                game.players[i].name,
                game.players[i].total_score,
                game.players[i].moves_used);
        strcat(buf, tmp);
    }

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(game.window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", buf);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// UI setup
static void activate(GtkApplication *app, gpointer user_data) {
    // Step 1: Login
    if (!login_dialog(NULL)) {
        g_application_quit(G_APPLICATION(app));
        return;
    }

    // Step 2: Ask number of players
    int num = ask_num_players(NULL);
    if (num == 0) {
        g_application_quit(G_APPLICATION(app));
        return;
    }

    // Step 3: Ask names
    if (!ask_player_names(NULL, num)) {
        g_application_quit(G_APPLICATION(app));
        return;
    }

    // Build UI
    GtkWidget *vbox, *hbox, *spin, *restart, *solve, *logbtn, *switchbtn, *finishbtn;

    game.window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(game.window), "Tower of Hanoi");
    gtk_window_set_default_size(GTK_WINDOW(game.window), 600, 450);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(game.window), vbox);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Disks:"), FALSE, FALSE, 0);

    // Spin button to choose number of disks
    spin = gtk_spin_button_new_with_range(3, MAX_DISKS, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), 3);
    gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 0);
    game.spin_disks = spin; // Save reference

    game.label_moves = gtk_label_new("Moves: 0");
    gtk_box_pack_start(GTK_BOX(hbox), game.label_moves, FALSE, FALSE, 0);

    restart = gtk_button_new_with_label("Restart");
    gtk_box_pack_start(GTK_BOX(hbox), restart, FALSE, FALSE, 0);

    logbtn = gtk_button_new_with_label("Log");
    gtk_box_pack_start(GTK_BOX(hbox), logbtn, FALSE, FALSE, 0);

    solve = gtk_button_new_with_label("Solve!");
    gtk_box_pack_start(GTK_BOX(hbox), solve, FALSE, FALSE, 0);

    switchbtn = gtk_button_new_with_label("Switch Player");
    gtk_box_pack_start(GTK_BOX(hbox), switchbtn, FALSE, FALSE, 0);

    game.drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(game.drawing_area, -1, 300);
    gtk_box_pack_start(GTK_BOX(vbox), game.drawing_area, TRUE, TRUE, 0);

    g_signal_connect(G_OBJECT(game.drawing_area), "draw",
                     G_CALLBACK(on_draw_event), NULL);

    gtk_widget_add_events(game.drawing_area, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(game.drawing_area), "button-press-event",
                     G_CALLBACK(on_button_press), NULL);

    game.label_min_moves = gtk_label_new("Minimum Moves: 7");
    gtk_widget_set_halign(game.label_min_moves, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), game.label_min_moves, FALSE, FALSE, 0);

    finishbtn = gtk_button_new_with_label("Finish Game & Show Results");
    gtk_widget_set_halign(finishbtn, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), finishbtn, FALSE, FALSE, 10);

    // Connect signals — pass spin button to restart
    g_signal_connect(restart, "clicked", G_CALLBACK(restart_game), spin);
    g_signal_connect(logbtn, "clicked", G_CALLBACK(log_moves), game.window);
    g_signal_connect(solve, "clicked", G_CALLBACK(solve_game), NULL);
    g_signal_connect(switchbtn, "clicked", G_CALLBACK(switch_player), NULL);
    g_signal_connect(finishbtn, "clicked", G_CALLBACK(finish_game), NULL);

    // Initial setup
    restart_game(NULL, spin); // Initialize with spin value

    gtk_widget_show_all(game.window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.example.hanoi", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    if (game.solve_moves) {
        g_queue_free_full(game.solve_moves, g_free);
    }

    return status;
}
