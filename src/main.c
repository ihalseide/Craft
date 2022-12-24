#include "auth.h"
#include "client.h"
#include "config.h"
#include "db.h"
#include "game.h"
#include "player.h"
#include "texturedBox.h"
#include "tinycthread.h"
#include "util.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


static Model game_model = {0};
static Model *game = &game_model;


// Handle key press callback
void
on_key(
        GLFWwindow *window,
        int key,
        int /*scancode*/,
        int action,
        int mods) 
{
    int control = mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER);
    int exclusive =
        glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
    if (action == GLFW_RELEASE) {
        return;
    }
    if (key == GLFW_KEY_BACKSPACE) {
        if (game->typing) {
            int n = strlen(game->typing_buffer);
            if (n > 0) {
                game->typing_buffer[n - 1] = '\0';
            }
        }
    }
    if (action != GLFW_PRESS) {
        return;
    }
    if (key == GLFW_KEY_ESCAPE) {
        if (game->typing) {
            game->typing = 0;
        }
        else if (exclusive) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    if (key == GLFW_KEY_ENTER) {
        if (game->typing) {
            if (mods & GLFW_MOD_SHIFT) {
                int n = strlen(game->typing_buffer);
                if (n < MAX_TEXT_LENGTH - 1) {
                    game->typing_buffer[n] = '\r';
                    game->typing_buffer[n + 1] = '\0';
                }
            }
            else {
                game->typing = 0;
                if (game->typing_buffer[0] == CRAFT_KEY_SIGN) {
                    Player *player = game->players;
                    int x, y, z, face;
                    if (hit_test_face(game, player, &x, &y, &z, &face)) {
                        set_sign(game, x, y, z, face, game->typing_buffer + 1);
                    }
                }
                else if (game->typing_buffer[0] == '/') {
                    parse_command(game, game->typing_buffer, 1);
                }
                else {
                    client_talk(game->typing_buffer);
                }
            }
        }
        else {
            if (control) {
                on_right_click(game);
            }
            else {
                on_left_click(game);
            }
        }
    }
    if (control && key == 'V') {
        const char *buffer = glfwGetClipboardString(window);
        if (game->typing) {
            game->suppress_char = 1;
            strncat(game->typing_buffer, buffer,
                    MAX_TEXT_LENGTH - strlen(game->typing_buffer) - 1);
        }
        else {
            parse_command(game, buffer, 0);
        }
    }
    if (!game->typing) {
        if (key == CRAFT_KEY_FLY) {
            PlayerAttributes *s = &game->players[0].attrs;
            s->flying = !s->flying;
        }
        if (key >= '1' && key <= '9') {
            game->item_index = key - '1';
        }
        if (key == '0') {
            game->item_index = 9;
        }
        if (key == CRAFT_KEY_ITEM_NEXT) {
            game->item_index = (game->item_index + 1) % item_count;
        }
        if (key == CRAFT_KEY_ITEM_PREV) {
            game->item_index--;
            if (game->item_index < 0) {
                game->item_index = item_count - 1;
            }
        }
        if (key == CRAFT_KEY_OBSERVE) {
            game->observe1 = (game->observe1 + 1) % game->player_count;
        }
        if (key == CRAFT_KEY_OBSERVE_INSET) {
            game->observe2 = (game->observe2 + 1) % game->player_count;
        }
    }
}


// Handle character callback
void
on_char(
        GLFWwindow * /*window*/,
        unsigned int u) 
{
    if (game->suppress_char) {
        game->suppress_char = 0;
        return;
    }
    if (game->typing) {
        if (u >= 32 && u < 128) {
            char c = (char)u;
            int n = strlen(game->typing_buffer);
            if (n < MAX_TEXT_LENGTH - 1) {
                game->typing_buffer[n] = c;
                game->typing_buffer[n + 1] = '\0';
            }
        }
    }
    else {
        if (u == CRAFT_KEY_CHAT) {
            game->typing = 1;
            game->typing_buffer[0] = '\0';
        }
        if (u == CRAFT_KEY_COMMAND) {
            game->typing = 1;
            game->typing_buffer[0] = '/';
            game->typing_buffer[1] = '\0';
        }
        if (u == CRAFT_KEY_SIGN) {
            game->typing = 1;
            game->typing_buffer[0] = CRAFT_KEY_SIGN;
            game->typing_buffer[1] = '\0';
        }
    }
}

// Handle mouse scroll callback, scroll through block items
void
on_scroll(
        GLFWwindow * /*window*/,  // window the thing happens in
        double /*xdelta*/,        // change in x scroll
        double ydelta)            // change in y scroll
{
    static double ypos = 0;
    ypos += ydelta;
    if (ypos < -SCROLL_THRESHOLD) {
        game->item_index = (game->item_index + 1) % item_count;
        ypos = 0;
    }
    if (ypos > SCROLL_THRESHOLD) {
        game->item_index--;
        if (game->item_index < 0) {
            game->item_index = item_count - 1;
        }
        ypos = 0;
    }
}


// Handle mouse button press callback
void
on_mouse_button(
        GLFWwindow *window,
        int button,
        int action,
        int mods) 
{
    int control = mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER);
    int exclusive =
        glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
    if (action != GLFW_PRESS) {
        return;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (exclusive) {
            // Control + left click simulates a right click
            if (control) {
                on_right_click(game);
            }
            else {
                on_left_click(game);
            }
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (exclusive) {
            if (control) {
                on_light(game);
            }
            else {
                on_right_click(game);
            }
        }
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (exclusive) {
            on_middle_click(game);
        }
    }
}


// Arguments: none
// Returns: none
void
create_window(
        Model *g) 
{
    static const char *title = "Miscraft";
    int window_width = WINDOW_WIDTH;
    int window_height = WINDOW_HEIGHT;
    GLFWmonitor *monitor = NULL;
    if (FULLSCREEN) {
        int mode_count;
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *modes = glfwGetVideoModes(monitor, &mode_count);
        window_width = modes[mode_count - 1].width;
        window_height = modes[mode_count - 1].height;
    }
    g->window = glfwCreateWindow(
            window_width, window_height, title, monitor, NULL);
}


int
main(
        int argc,
        char **argv) 
{

    // INITIALIZATION //
    curl_global_init(CURL_GLOBAL_DEFAULT);
    srand(time(NULL));
    rand();

    // WINDOW INITIALIZATION //
    if (!glfwInit()) { return -1; }
    create_window(game);
    if (!game->window) {
        glfwTerminate();
        return -1;
    }

    // Initialize the graphical user interface with GLFW
    glfwMakeContextCurrent(game->window);
    glfwSwapInterval(VSYNC);
    glfwSetInputMode(game->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(game->window, on_key);
    glfwSetCharCallback(game->window, on_char);
    glfwSetMouseButtonCallback(game->window, on_mouse_button);
    glfwSetScrollCallback(game->window, on_scroll);

    if (glewInit() != GLEW_OK) { return -1; }

    // Initialize some OpenGL settings
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glLogicOp(GL_INVERT);
    glClearColor(0, 0, 0, 1);

    // LOAD TEXTURES //
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    load_png_texture("textures/texture.png");

    GLuint font;
    glGenTextures(1, &font);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, font);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    load_png_texture("textures/font.png");

    GLuint sky;
    glGenTextures(1, &sky);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, sky);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    load_png_texture("textures/sky.png");

    GLuint sign;
    glGenTextures(1, &sign);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, sign);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    load_png_texture("textures/sign.png");

    // LOAD SHADERS //
    Attrib block_attrib = {0};
    Attrib line_attrib = {0};
    Attrib text_attrib = {0};
    Attrib sky_attrib = {0};
    GLuint program;

    program = load_program(
        "shaders/block_vertex.glsl", "shaders/block_fragment.glsl");
    block_attrib.program  = program;
    block_attrib.position = glGetAttribLocation(program, "position");
    block_attrib.normal   = glGetAttribLocation(program, "normal");
    block_attrib.uv       = glGetAttribLocation(program, "uv");
    block_attrib.matrix   = glGetUniformLocation(program, "matrix");
    block_attrib.sampler  = glGetUniformLocation(program, "sampler");
    block_attrib.extra1   = glGetUniformLocation(program, "sky_sampler");
    block_attrib.extra2   = glGetUniformLocation(program, "daylight");
    block_attrib.extra3   = glGetUniformLocation(program, "fog_distance");
    block_attrib.extra4   = glGetUniformLocation(program, "ortho");
    block_attrib.camera   = glGetUniformLocation(program, "camera");
    block_attrib.timer    = glGetUniformLocation(program, "timer");

    program = load_program(
        "shaders/line_vertex.glsl", "shaders/line_fragment.glsl");
    line_attrib.program  = program;
    line_attrib.position = glGetAttribLocation(program, "position");
    line_attrib.matrix   = glGetUniformLocation(program, "matrix");

    program = load_program(
        "shaders/text_vertex.glsl", "shaders/text_fragment.glsl");
    text_attrib.program  = program;
    text_attrib.position = glGetAttribLocation(program, "position");
    text_attrib.uv       = glGetAttribLocation(program, "uv");
    text_attrib.matrix   = glGetUniformLocation(program, "matrix");
    text_attrib.sampler  = glGetUniformLocation(program, "sampler");
    text_attrib.extra1   = glGetUniformLocation(program, "is_sign");

    program = load_program(
        "shaders/sky_vertex.glsl", "shaders/sky_fragment.glsl");
    sky_attrib.program  = program;
    sky_attrib.position = glGetAttribLocation(program, "position");
    sky_attrib.normal   = glGetAttribLocation(program, "normal");
    sky_attrib.uv       = glGetAttribLocation(program, "uv");
    sky_attrib.matrix   = glGetUniformLocation(program, "matrix");
    sky_attrib.sampler  = glGetUniformLocation(program, "sampler");
    sky_attrib.timer    = glGetUniformLocation(program, "timer");

    // CHECK COMMAND LINE ARGUMENTS //
    if (argc == 2 || argc == 3) {
        game->mode = MODE_ONLINE;
        strncpy(game->server_addr, argv[1], MAX_ADDR_LENGTH-1);
        game->server_port = argc == 3 ? atoi(argv[2]) : DEFAULT_PORT;
        snprintf(game->db_path, MAX_PATH_LENGTH,
            "cache.%s.%d.db", game->server_addr, game->server_port);
    }
    else {
        game->mode = MODE_OFFLINE;
        snprintf(game->db_path, MAX_PATH_LENGTH, "%s", DB_PATH);
    }

    // Configure game radius settings
    game->create_radius = CREATE_CHUNK_RADIUS;
    game->render_radius = RENDER_CHUNK_RADIUS;
    game->delete_radius = DELETE_CHUNK_RADIUS;
    game->sign_radius = RENDER_SIGN_RADIUS;

    // INITIALIZE WORKER THREADS
    for (int i = 0; i < WORKERS; i++) {
        Worker *worker = game->workers + i;
        worker->index = i;
        worker->state = WORKER_IDLE;
        mtx_init(&worker->mtx, mtx_plain);
        cnd_init(&worker->cnd);
        thrd_create(&worker->thrd, worker_run, worker);
    }


    // OUTER LOOP //
    // This outer loop is necessary because the game can switch between online
    // and offline mode any time and needs to shutdown and re-init the db and
    // other resources.
    int running = 1;
    while (running) {
        // DATABASE INITIALIZATION //
        if (game->mode == MODE_OFFLINE || USE_CACHE) {
            db_enable();
            int rc = db_init(game->db_path);
            if (rc) {
                // db initialization failed
                return -1;
            }
            if (game->mode == MODE_ONLINE) {
                // TODO: support proper caching of signs (handle deletions)
                db_delete_all_signs();
            }
        }

        // CLIENT INITIALIZATION //
        if (game->mode == MODE_ONLINE) {
            client_enable();
            client_connect(game->server_addr, game->server_port);
            client_start();
            client_version(1);
            login();
        }

        // LOCAL VARIABLES //
        reset_model(game);
        FPS fps = {0, 0, 0};
        double last_commit = glfwGetTime();
        double last_update = glfwGetTime();
        GLuint sky_buffer = gen_sky_buffer();

        // Init local player
        Player *me = game->players;
        State *s = &game->players->state;
        memset(me, 0, sizeof(*me));
        me->id = 0;
        me->name[0] = '\0';
        me->buffer = 0;
        me->attrs.attack_damage = 1;
        me->attrs.reach = 8;
        game->player_count = 1;
        //printf("player damage is %d\n", me->attrs.attack_damage);

        // LOAD STATE FROM DATABASE //
        int loaded = db_load_state(&s->x, &s->y, &s->z, &s->rx, &s->ry, &me->attrs.flying);
        s->brx = s->rx;
        force_chunks(game, me);
        if (!loaded) {
            s->y = highest_block(game, s->x, s->z) + 2;
        }

        // BEGIN MAIN LOOP //
        double previous = glfwGetTime();
        while (1) {
            // WINDOW SIZE AND SCALE //
            game->scale = get_scale_factor(game);
            glfwGetFramebufferSize(game->window, &game->width, &game->height);
            glViewport(0, 0, game->width, game->height);

            // FRAME RATE //
            if (game->time_changed) {
                game->time_changed = 0;
                last_commit = glfwGetTime();
                last_update = glfwGetTime();
                memset(&fps, 0, sizeof(fps));
            }
            update_fps(&fps);
            double now = glfwGetTime();
            double dt = now - previous;
            dt = MIN(dt, 0.2);
            dt = MAX(dt, 0.0);
            previous = now;

            // HANDLE MOUSE INPUT //
            handle_mouse_input(game);

            // HANDLE MOVEMENT //
            handle_movement(game, dt);

            // HANDLE DATA FROM SERVER //
            char *buffer = client_recv();
            if (buffer) {
                parse_buffer(game, buffer);
                free(buffer);
            }

            // FLUSH DATABASE //
            if (now - last_commit > COMMIT_INTERVAL) {
                last_commit = now;
                db_commit();
            }

            // SEND POSITION TO SERVER //
            if (now - last_update > 0.1) {
                last_update = now;
                client_position(s->x, s->y, s->z, s->rx, s->ry);
            }

            // PREPARE TO RENDER //
            game->observe1 = game->observe1 % game->player_count;
            game->observe2 = game->observe2 % game->player_count;
            delete_chunks(game);
            del_buffer(me->buffer);
            me->buffer = gen_player_buffer(
                    s->x, s->y, s->z, s->rx, s->ry, s->brx);
            for (int i = 1; i < game->player_count; i++) {
                interpolate_player(game->players + i);
            }
            Player *player = game->players + game->observe1;

            // RENDER 3-D SCENE //
            glClear(GL_COLOR_BUFFER_BIT);
            glClear(GL_DEPTH_BUFFER_BIT);
            render_sky(game, &sky_attrib, player, sky_buffer);
            glClear(GL_DEPTH_BUFFER_BIT);
            // Get the face count while rendering for displaying the number on screen
            int face_count = render_chunks(game, &block_attrib, player);
            render_signs(game, &text_attrib, player);
            render_sign(game, &text_attrib, player);
            render_players(game, &block_attrib, player);
            if (SHOW_WIREFRAME) {
                render_wireframe(game, &line_attrib, player);
                render_players_hitboxes(game, &line_attrib, player);
            }

            // RENDER HUD //
            glClear(GL_DEPTH_BUFFER_BIT);
            if (SHOW_CROSSHAIRS) {
                render_crosshairs(game, &line_attrib);
            }
            if (SHOW_ITEM) {
                render_item(game, &block_attrib);
            }

            // RENDER TEXT //
            char text_buffer[1024];
            float ts = 12 * game->scale;
            float tx = ts / 2;
            float ty = game->height - ts;

            // Technical info text
            if (SHOW_INFO_TEXT) {
                int hour = time_of_day(game) * 24;
                char am_pm = hour < 12 ? 'a' : 'p';
                hour = hour % 12;
                hour = hour ? hour : 12;
                snprintf(
                    text_buffer, 1024,
                    "(%d, %d) (%.2f, %.2f, %.2f) [%d, %d, %d] %d%cm %dfps v:<%.2f, %.2f, %.2f>",
                    chunked(s->x), chunked(s->z), s->x, s->y, s->z,
                    game->player_count, game->chunk_count,
                    face_count * 2, hour, am_pm, fps.fps,
                    s->vx, s->vy, s->vz);
                render_text(game, &text_attrib, ALIGN_LEFT, tx, ty, ts, text_buffer);
                ty -= ts * 2;
            }

            // Health debug text
            {
                snprintf(text_buffer, sizeof(text_buffer),
                        "damage: %d",
                        me->attrs.taken_damage);
                render_text(game, &text_attrib, ALIGN_LEFT, tx, ty, ts, text_buffer);
                ty -= ts * 2;
            }

            // Chat text
            if (SHOW_CHAT_TEXT) {
                for (int i = 0; i < MAX_MESSAGES; i++) {
                    int index = (game->message_index + i) % MAX_MESSAGES;
                    if (strlen(game->messages[index])) {
                        render_text(game, &text_attrib, ALIGN_LEFT, tx, ty, ts,
                                game->messages[index]);
                        ty -= ts * 2;
                    }
                }
            }
            // Current typing text
            if (game->typing) {
                snprintf(text_buffer, 1024, "> %s", game->typing_buffer);
                render_text(game, &text_attrib, ALIGN_LEFT, tx, ty, ts, text_buffer);
                ty -= ts * 2;
            }

            if (SHOW_PLAYER_NAMES) {
                if (player != me) {
                    render_text(game, &text_attrib, ALIGN_CENTER,
                        game->width / 2, ts, ts, player->name);
                }
                Player *other = player_crosshair(game, player);
                if (other) {
                    render_text(game, &text_attrib, ALIGN_CENTER,
                            game->width / 2, game->height / 2 - ts - 24, ts,
                            other->name);
                }
            }

            // show damage info for current block
            {
                State *s = &me->state;
                int hx, hy, hz;
                int hw = hit_test(game, 0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
                if (hw) {
                    int damage = get_block_damage(game, hx, hy, hz);
                    if (damage) {
                        snprintf(text_buffer, 1024, "block: %d, damage: %d", hw, damage);
                        render_text(game, &text_attrib, ALIGN_LEFT, tx, ty, ts, text_buffer);
                    }
                }
            }

            // SWAP AND POLL //
            glfwSwapBuffers(game->window);
            glfwPollEvents();
            // When closing the window, break this inner loop to shutdown and do
            // not re-init.
            if (glfwWindowShouldClose(game->window)) {
                running = 0;
                break;
            }
            // If online/offline mode changed, break this inner loop to shutdown
            // and re-init.
            if (game->mode_changed) {
                game->mode_changed = 0;
                break;
            }
        }

        // SHUTDOWN //
        // Shutdown of current game mode
        // (The outer game loop may or may not continue after this)
        db_save_state(s->x, s->y, s->z, s->rx, s->ry, me->attrs.flying);
        db_close();
        db_disable();
        client_stop();
        client_disable();
        del_buffer(sky_buffer);
        delete_all_chunks(game);
        delete_all_players(game);
    }

    // Final program closing
    glfwTerminate();
    curl_global_cleanup();
    return 0;
}

