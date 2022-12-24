#include "auth.h"
#include "client.h"
#include "config.h"
#include "db.h"
#include "game.h"
#include "player.h"
#include "tinycthread.h"
#include "util.h"
#include "texturedBox.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


extern Model *g;


int main(int argc, char **argv) 
{

    // INITIALIZATION //
    curl_global_init(CURL_GLOBAL_DEFAULT);
    srand(time(NULL));
    rand();

    // WINDOW INITIALIZATION //
    if (!glfwInit()) { return -1; }
    create_window();
    if (!g->window) {
        glfwTerminate();
        return -1;
    }

    // Initialize the graphical user interface with GLFW
    glfwMakeContextCurrent(g->window);
    glfwSwapInterval(VSYNC);
    glfwSetInputMode(g->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(g->window, on_key);
    glfwSetCharCallback(g->window, on_char);
    glfwSetMouseButtonCallback(g->window, on_mouse_button);
    glfwSetScrollCallback(g->window, on_scroll);

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
        g->mode = MODE_ONLINE;
        strncpy(g->server_addr, argv[1], MAX_ADDR_LENGTH-1);
        g->server_port = argc == 3 ? atoi(argv[2]) : DEFAULT_PORT;
        snprintf(g->db_path, MAX_PATH_LENGTH,
            "cache.%s.%d.db", g->server_addr, g->server_port);
    }
    else {
        g->mode = MODE_OFFLINE;
        snprintf(g->db_path, MAX_PATH_LENGTH, "%s", DB_PATH);
    }

    // Configure game radius settings
    g->create_radius = CREATE_CHUNK_RADIUS;
    g->render_radius = RENDER_CHUNK_RADIUS;
    g->delete_radius = DELETE_CHUNK_RADIUS;
    g->sign_radius = RENDER_SIGN_RADIUS;

    // INITIALIZE WORKER THREADS
    for (int i = 0; i < WORKERS; i++) {
        Worker *worker = g->workers + i;
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
        if (g->mode == MODE_OFFLINE || USE_CACHE) {
            db_enable();
            int rc = db_init(g->db_path);
            if (rc) {
                // db initialization failed
                return -1;
            }
            if (g->mode == MODE_ONLINE) {
                // TODO: support proper caching of signs (handle deletions)
                db_delete_all_signs();
            }
        }

        // CLIENT INITIALIZATION //
        if (g->mode == MODE_ONLINE) {
            client_enable();
            client_connect(g->server_addr, g->server_port);
            client_start();
            client_version(1);
            login();
        }

        // LOCAL VARIABLES //
        reset_model();
        FPS fps = {0, 0, 0};
        double last_commit = glfwGetTime();
        double last_update = glfwGetTime();
        GLuint sky_buffer = gen_sky_buffer();

        // Init local player
        Player *me = g->players;
        State *s = &g->players->state;
        memset(me, 0, sizeof(*me));
        me->id = 0;
        me->name[0] = '\0';
        me->buffer = 0;
        me->attrs.attack_damage = 1;
        me->attrs.reach = 8;
        g->player_count = 1;
        //printf("player damage is %d\n", me->attrs.attack_damage);

        // LOAD STATE FROM DATABASE //
        int loaded = db_load_state(&s->x, &s->y, &s->z, &s->rx, &s->ry, &me->attrs.flying);
        s->brx = s->rx;
        force_chunks(me);
        if (!loaded) {
            s->y = highest_block(s->x, s->z) + 2;
        }

        // BEGIN MAIN LOOP //
        double previous = glfwGetTime();
        while (1) {
            // WINDOW SIZE AND SCALE //
            g->scale = get_scale_factor();
            glfwGetFramebufferSize(g->window, &g->width, &g->height);
            glViewport(0, 0, g->width, g->height);

            // FRAME RATE //
            if (g->time_changed) {
                g->time_changed = 0;
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
            handle_mouse_input();

            // HANDLE MOVEMENT //
            handle_movement(dt);

            // HANDLE DATA FROM SERVER //
            char *buffer = client_recv();
            if (buffer) {
                parse_buffer(buffer);
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
            g->observe1 = g->observe1 % g->player_count;
            g->observe2 = g->observe2 % g->player_count;
            delete_chunks();
            del_buffer(me->buffer);
            me->buffer = gen_player_buffer(
                    s->x, s->y, s->z, s->rx, s->ry, s->brx);
            for (int i = 1; i < g->player_count; i++) {
                interpolate_player(g->players + i);
            }
            Player *player = g->players + g->observe1;

            // RENDER 3-D SCENE //
            glClear(GL_COLOR_BUFFER_BIT);
            glClear(GL_DEPTH_BUFFER_BIT);
            render_sky(&sky_attrib, player, sky_buffer);
            glClear(GL_DEPTH_BUFFER_BIT);
            // Get the face count while rendering for displaying the number on screen
            int face_count = render_chunks(&block_attrib, player);
            render_signs(&text_attrib, player);
            render_sign(&text_attrib, player);
            render_players(&block_attrib, player);
            if (SHOW_WIREFRAME) {
                render_wireframe(&line_attrib, player);
                render_players_hitboxes(&line_attrib, player);
            }

            // RENDER HUD //
            glClear(GL_DEPTH_BUFFER_BIT);
            if (SHOW_CROSSHAIRS) {
                render_crosshairs(&line_attrib);
            }
            if (SHOW_ITEM) {
                render_item(&block_attrib);
            }

            // RENDER TEXT //
            char text_buffer[1024];
            float ts = 12 * g->scale;
            float tx = ts / 2;
            float ty = g->height - ts;

            // Technical info text
            if (SHOW_INFO_TEXT) {
                int hour = time_of_day() * 24;
                char am_pm = hour < 12 ? 'a' : 'p';
                hour = hour % 12;
                hour = hour ? hour : 12;
                snprintf(
                    text_buffer, 1024,
                    "(%d, %d) (%.2f, %.2f, %.2f) [%d, %d, %d] %d%cm %dfps v:<%.2f, %.2f, %.2f>",
                    chunked(s->x), chunked(s->z), s->x, s->y, s->z,
                    g->player_count, g->chunk_count,
                    face_count * 2, hour, am_pm, fps.fps,
                    s->vx, s->vy, s->vz);
                render_text(&text_attrib, ALIGN_LEFT, tx, ty, ts, text_buffer);
                ty -= ts * 2;
            }

            // Health debug text
            {
                snprintf(text_buffer, sizeof(text_buffer),
                        "damage: %d",
                        me->attrs.taken_damage);
                render_text(&text_attrib, ALIGN_LEFT, tx, ty, ts, text_buffer);
                ty -= ts * 2;
            }

            // Chat text
            if (SHOW_CHAT_TEXT) {
                for (int i = 0; i < MAX_MESSAGES; i++) {
                    int index = (g->message_index + i) % MAX_MESSAGES;
                    if (strlen(g->messages[index])) {
                        render_text(&text_attrib, ALIGN_LEFT, tx, ty, ts,
                            g->messages[index]);
                        ty -= ts * 2;
                    }
                }
            }
            // Current typing text
            if (g->typing) {
                snprintf(text_buffer, 1024, "> %s", g->typing_buffer);
                render_text(&text_attrib, ALIGN_LEFT, tx, ty, ts, text_buffer);
                ty -= ts * 2;
            }

            if (SHOW_PLAYER_NAMES) {
                if (player != me) {
                    render_text(&text_attrib, ALIGN_CENTER,
                        g->width / 2, ts, ts, player->name);
                }
                Player *other = player_crosshair(player);
                if (other) {
                    render_text(&text_attrib, ALIGN_CENTER,
                        g->width / 2, g->height / 2 - ts - 24, ts,
                        other->name);
                }
            }

            // show damage info for current block
            {
                State *s = &me->state;
                int hx, hy, hz;
                int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
                if (hw) {
                    int damage = get_block_damage(hx, hy, hz);
                    if (damage) {
                        snprintf(text_buffer, 1024, "block: %d, damage: %d", hw, damage);
                        render_text(&text_attrib, ALIGN_LEFT, tx, ty, ts, text_buffer);
                    }
                }
            }

            // SWAP AND POLL //
            glfwSwapBuffers(g->window);
            glfwPollEvents();
            // When closing the window, break this inner loop to shutdown and do
            // not re-init.
            if (glfwWindowShouldClose(g->window)) {
                running = 0;
                break;
            }
            // If online/offline mode changed, break this inner loop to shutdown
            // and re-init.
            if (g->mode_changed) {
                g->mode_changed = 0;
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
        delete_all_chunks();
        delete_all_players();
    }

    // Final program closing
    glfwTerminate();
    curl_global_cleanup();
    return 0;
}

