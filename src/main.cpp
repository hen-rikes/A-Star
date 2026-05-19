#include "../include/raylib.h"
#include <fstream>
#include <map>
#include <iostream>
#include <vector>
#include <algorithm>

typedef unsigned int UInt;

template<typename T>
inline void clamp(T& value, T min, T max) {
    value = value <= min ? min : value >= max ? max : value;
}

template<typename T>
inline T normalize(T value, T min, T max) {
    return (value - min) / (max - min);
}

inline void UpdateWindowSize(int &width, int &height) {
    width = GetScreenWidth();
    height = GetScreenHeight();
    SetWindowSize(width, height);
}

struct Eclipse_Level_Collsion_Entity {
    Rectangle boundary;
    UInt eclipse_level;
};

#define FILE_OPENED_SUCCESSFULLY 0 
#define COULD_NOT_OPEN_FILE      1 

#define IS_A_VALID_XML           2 
#define IS_NOT_A_VALID_XML       3 

#define LOADING_STATE            4
#define NO_FILE_STATE            5
#define MAIN_STATE               6

UInt is_xml_file_valid(const std::string& file_path) {
    std::string identifier1 = "<UserProfile>";
    std::string identifier2 = "</unlock>";

    std::ifstream file(file_path);
    if (!file) return COULD_NOT_OPEN_FILE;

    char* buffer{};
    size_t file_length{};

    file.seekg(0, file.end);
    file_length = file.tellg();
    file.seekg(0, file.beg);

    buffer = new char[file_length];
    file.read(buffer, file_length);

    file.close();

    std::string file_content = buffer;

    size_t index_of_identifier1 = file_content.find(identifier1);
    size_t index_of_identifier2 = file_content.find(identifier2);

    if (index_of_identifier1 != std::string::npos && index_of_identifier2 != std::string::npos) return IS_A_VALID_XML;
    else return IS_NOT_A_VALID_XML;
}

UInt get_eclipse_info_from_xml(const char* file_path, std::map<std::string, UInt>& survivor_map) {
    std::ifstream eclipse_file(file_path);
    if (!eclipse_file) return COULD_NOT_OPEN_FILE;

    char* buffer{};
    size_t file_length{};

    eclipse_file.seekg(0, eclipse_file.end);
    file_length = eclipse_file.tellg();
    eclipse_file.seekg(0, eclipse_file.beg);

    buffer = new char[file_length];
    eclipse_file.read(buffer, file_length);

    eclipse_file.close();

    std::string file_content = buffer;

    std::string identifier = "Eclipse.";
    size_t identifier_pos = 0;
    size_t end_of_survivor_name = 0;
    size_t identifier_pos_plus_identifier_length = identifier_pos + identifier.length();

    for (size_t i = 0; i < file_length; i++) {
        if (end_of_survivor_name+2 >= file_content.find("</unlock></stats>")) break;

        identifier_pos = file_content.find(identifier, identifier_pos_plus_identifier_length);
        identifier_pos_plus_identifier_length = identifier_pos + identifier.length();

        end_of_survivor_name = file_content.find(".", identifier_pos_plus_identifier_length);

        survivor_map[file_content.substr(identifier_pos_plus_identifier_length, end_of_survivor_name - identifier_pos_plus_identifier_length)] = std::stoll(file_content.substr(end_of_survivor_name+1, 1));
    }

    delete[] buffer;

    return FILE_OPENED_SUCCESSFULLY;
}

void order_and_correct_survivor_list(const std::map<std::string, UInt>& survivors, std::vector<std::pair<std::string, UInt>>& survivors_ordered_from_image_file, UInt& eclipse_level_sum, UInt& mastery_level) {
    UInt survivor_order[18] = { 15, 18, 8, 4, 17, 9, 2, 5, 14, 6, 13, 7, 1, 12, 10, 3, 11, 16 }; // NOTE: This order is based on the sorted survivors plus their position on the survivors.png

    std::map<std::string, std::string> correct_names = {
        { "Toolbot", "MUL-T" },
        { "Engi", "Engineer" },
        { "Mage", "Artificer" },
        { "Merc", "Mercenary" },
        { "Treebot", "REX" },
        { "Croco", "Acrid" },
        { "VoidSurvivor", "Void Fiend" },
        { "DroneTech", "Operator" },
        { "Bandit2", "Bandit" },
    };

    int index = 0;
    for (const auto& [name, e_level] : survivors) {
        std::string temp_name = correct_names[name] != "" ? correct_names[name] : name;
        survivors_ordered_from_image_file[survivor_order[index]-1] = std::make_pair(temp_name, e_level > 0 ? e_level - 1 : 0);
        index++;
    }

    eclipse_level_sum = 0;
    mastery_level = 8;
    for (const auto& [name, e_level] : survivors_ordered_from_image_file) {
        if (e_level < mastery_level) mastery_level = e_level;
        eclipse_level_sum += e_level;
    }
}

std::string save_to_file(const std::string& file_path) {
    if (!IsFileExtension(file_path.c_str(), ".xml")) return "";

    UInt xml_file_state = is_xml_file_valid(file_path);
    if (xml_file_state == IS_NOT_A_VALID_XML) return "";

    if (xml_file_state == IS_A_VALID_XML) {
        std::fstream xml_file("./xml_path.txt", std::fstream::out);
        if (!xml_file) return "";

        xml_file.write(file_path.c_str(), file_path.size());

        xml_file.close();
        return file_path;
    }

    return "";
}

UInt read_file(const std::string& file_path, std::string& xml_file) {
    if (!FileExists(file_path.c_str())) return COULD_NOT_OPEN_FILE;

    std::ifstream file(file_path);
    if (!file) return COULD_NOT_OPEN_FILE;

    char* buffer{};
    size_t file_length{};

    file.seekg(0, file.end);
    file_length = file.tellg();
    file.seekg(0, file.beg);

    buffer = new char[file_length];
    file.read(buffer, file_length);

    file.close();

    xml_file = buffer;

    delete[] buffer;

    return FILE_OPENED_SUCCESSFULLY;
}

struct Entity_Index {
    UInt index;
    UInt e_level;
};

enum Survivor_Order : UInt {
    Ascending = 0,
    Descending,
};

std::vector<Entity_Index> survivors_ordered_by_eclipse_level(std::vector<std::pair<std::string, UInt>>& survivors_ordered_from_image_file, std::vector<std::pair<std::string, UInt>>& survivors_ordered, Survivor_Order survivor_order) {
    survivors_ordered.clear();

    std::vector<Entity_Index> entity_index{};
    for (size_t i = 0; i < survivors_ordered_from_image_file.size(); i++) {
        entity_index.emplace_back((Entity_Index){(UInt)i, survivors_ordered_from_image_file[i].second});
    }

    size_t index = 0;
    size_t is_list_sorted = 0;
    while (1) {
        if (is_list_sorted == entity_index.size()-1) break;

        if (index >= entity_index.size()-1) {
            is_list_sorted = 0;
            index = 0;
        }

        switch (survivor_order) {
            break; case Ascending:
                if (entity_index[index].e_level < entity_index[index+1].e_level) {
                    auto temp_entity = entity_index[index+1];
                    entity_index[index+1] = entity_index[index];
                    entity_index[index] = temp_entity;
                } else is_list_sorted++;
            break; case Descending:
                if (entity_index[index].e_level > entity_index[index+1].e_level) {
                    auto temp_entity = entity_index[index+1];
                    entity_index[index+1] = entity_index[index];
                    entity_index[index] = temp_entity;
                } else is_list_sorted++;
        }

        index++;
    }

    for (size_t i = 0; i < survivors_ordered_from_image_file.size(); i++) survivors_ordered.emplace_back(survivors_ordered_from_image_file[entity_index[i].index]);

    return entity_index;
}

int main() {
    int width = 980, height = 900;
    InitWindow(width, height, "A-Star");
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(width, height);

    std::string textures_paths[] = {
        "./images/Icon/MiniIcon.png",
        "./images/Survivors/Survivors.png",
        "./images/Eclipse/EclipseLevels.png",
        "./images/Template/Template.png",
        "./images/Completion/CompletionHUD.png",
        "./images/MasteryNumbers/MasteryNumbers.png",
        "./images/Modifiers/EclipseModifiers.png",
        "./images/Icons/Reload.png",
        "./images/Icons/SortUP.png",
        "./images/Icons/SortDOWN.png",
    };

    size_t texture_count = sizeof(textures_paths)/sizeof(std::string);
    UInt texture_index = 0;
    bool are_all_textures_loaded = false;

    Texture2D textures[texture_count];
    Font bombardier;

    const Texture2D& icon_texture       = textures[0];
    const Texture2D& survivors_texture  = textures[1];
    const Texture2D& eclipse_texture    = textures[2];
    const Texture2D& template_texture   = textures[3];
    const Texture2D& completion_texture = textures[4];
    const Texture2D& numbers_texture    = textures[5];
    const Texture2D& modifiers_texture  = textures[6];
    const Texture2D& reload_texture     = textures[7];
    // NOTE: Comment to keep track of the indices.
    /* const Texture2D& sort_up_texture    = textures[8]; */
    /* const Texture2D& sort_down_texture  = textures[9]; */

    enum Sort_Texture : UInt {
        UP = 8,
        DOWN,
    };

    UInt sort_texture_index = Sort_Texture::UP;
    const Texture2D& sort_texture = textures[sort_texture_index];
    bool change_sort_texture = false;
    Survivor_Order survivor_order = Ascending;

    UInt state = LOADING_STATE;
    UInt eclipse_info_xml_file_state{};
    bool load_eclipse_info_from_xml_file_once = true;

    std::map<std::string, UInt> survivors {
        {"Commando", 0},
        {"Huntress", 0},
        {"Bandit2", 0},
        {"Toolbot", 0},
        {"Engi", 0},
        {"Mage", 0},
        {"Merc", 0},
        {"Treebot", 0},
        {"Loader", 0},
        {"Croco", 0},
        {"Captain", 0},
        {"Railgunner", 0},
        {"VoidSurvivor", 0},
        {"Seeker", 0},
        {"FalseSon", 0},
        {"Chef", 0},
        {"DroneTech", 0},
        {"Drifter", 0},
    };

    std::string xml_file = ""; 
    std::string file_path = "./xml_path.txt";
    UInt read_file_state = read_file(file_path, xml_file);
    if (read_file_state == COULD_NOT_OPEN_FILE) {
            std::cout << "Could not open file!\n";
            state = LOADING_STATE;
            xml_file = "";
    } else if (read_file_state == FILE_OPENED_SUCCESSFULLY) state = LOADING_STATE;

    // MainState Setup -------------------------------- //
    std::vector<std::pair<std::string, UInt>> survivors_ordered_from_image_file{ {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, };
    survivors_ordered_from_image_file.reserve(18);

    float cooldown_time_for_reloading = 80.f;
    float time_until_next_update = cooldown_time_for_reloading;
    UInt eclipse_level_sum = 0;
    UInt mastery_level = 0;

    bool reloaded = 0;
    float cooldown_time_for_reloaded = 2.6f;
    float reloaded_time = cooldown_time_for_reloaded;

    Camera2D camera {
        .offset = { 0.f, 0.f },
        .target = { 0.f, 0.f },
        .rotation = 0.f,
        .zoom = 1.f,
    };
    float max_camera_y{};

    Rectangle template_complete_boundary{};
    Vector2 mouse_wheel = GetMouseWheelMoveV();
    Vector2 mouse_pos = GetMousePosition();
    float acceleration = 800.f;
    float velocity = 0.f;
    float max_velocity = 70.f;
    float friction = 220.f;
    int direction = 0.f;
    // MainState Setup -------------------------------- //

    std::vector<std::pair<std::string, UInt>> survivors_ordered;
    std::vector<Entity_Index> entity_index{};
    Vector2 survivors_image_pos[] = {
        { 0.f, 0.f }, { 256.f, 0.f, }, { 512.f, 0.f, }, { 768.f, 0.f, }, { 1024.f, 0.f, }, { 1280.f, 0.f, }, { 1536.f, 0.f, },
        { 0.f, 256.f }, { 128.f, 256.f }, { 256.f, 256.f }, { 384.f, 256.f }, { 512.f, 256.f }, { 640.f, 256.f }, { 768.f, 256.f }, { 896.f, 256.f }, { 1024.f, 256.f }, { 1152.f, 256.f }, { 1280.f, 256.f }, 
    };

    // Reload Animation Setup ---------------------------- //
    int animation_step        = 0;
    int animation_total_steps = 360;
    float animation_speed     = 0.0001f;
    float animation_duration  = animation_speed;
    bool is_animation_done    = true;
    // Reload Animation Setup ---------------------------- //

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        UpdateWindowSize(width, height);

        // Drawing
        BeginDrawing();
        ClearBackground(BLACK);

        Color color1 = { 62, 49, 115, 255 };
        Color color2 = { 29, 24, 47, 255 };
        DrawRectangleGradientV(0, 0, width, height, color1, color2);

        if (state == LOADING_STATE) {
            if (!are_all_textures_loaded) {
                if (texture_index == 0) {
                    textures[texture_index] = LoadTexture(textures_paths[texture_index].c_str());
                    SetTextureFilter(textures[texture_index], TEXTURE_FILTER_TRILINEAR);
                    SetWindowIcon(LoadImageFromTexture(icon_texture));

                    texture_index++;
                }
                else if (texture_index < texture_count) {
                    textures[texture_index] = LoadTexture(textures_paths[texture_index].c_str());
                    SetTextureFilter(textures[texture_index], TEXTURE_FILTER_TRILINEAR);

                    texture_index++;
                } else {
                    if (texture_index == texture_count) {
                        bombardier = LoadFontEx("./font/BOMBARD_.ttf", 128, 0, 0);
                        SetTextureFilter(bombardier.texture, TEXTURE_FILTER_TRILINEAR);
                    }

                    are_all_textures_loaded = true;
                    state = NO_FILE_STATE;
                    if (read_file_state == COULD_NOT_OPEN_FILE) {
                        std::cout << "Could not open file!\n";

                        state = NO_FILE_STATE;
                        xml_file = "";
                    } else if (read_file_state == FILE_OPENED_SUCCESSFULLY) {
                        state = MAIN_STATE;
                    }
                }
            } 
        } else if (state == NO_FILE_STATE) {
            if (IsFileDropped()) {
                FilePathList files = LoadDroppedFiles();
                std::string xml_file_path = files.paths[0];

                std::string temp_file_path = save_to_file(xml_file_path);
                if (temp_file_path == "") std::cout << "Could not open file!";
                else {
                    state = MAIN_STATE;
                    xml_file = temp_file_path;

                    eclipse_info_xml_file_state = get_eclipse_info_from_xml(xml_file.c_str(), survivors);
                }

                UnloadDroppedFiles(files);
            }

            std::string drop_file_here_text = "Drop Your File Here!";
            float drop_file_here_fontsize = width*0.1f;
            Vector2 drop_file_here_text_size = MeasureTextEx(bombardier, drop_file_here_text.c_str(), drop_file_here_fontsize, 0.f);

            Vector2 drop_file_here_pos = {
                .x = width/2.f - drop_file_here_text_size.x/2.f,
                .y = height/2.f - drop_file_here_text_size.y/2.f,
            };

            for (size_t i = 2; i > 0; i--) 
                DrawTextPro(bombardier, 
                        drop_file_here_text.c_str(),
                        { drop_file_here_pos.x+(i*4.f), drop_file_here_pos.y+(i*4.f) }, 
                        {0.f, 0.f},
                        0.f,
                        drop_file_here_fontsize, 
                        0.f, 
                        i < 2 ? WHITE : (Color){0,0,0,200});

            camera = {
                .offset = { 0.f, 0.f },
                .target = { 0.f, 0.f },
                .rotation = 0.f,
                .zoom = 1.f,
            };
        } else if (state == MAIN_STATE) {
            // NOTE: We load the info from the file once at startup.
            if (load_eclipse_info_from_xml_file_once) {
                eclipse_info_xml_file_state = get_eclipse_info_from_xml(xml_file.c_str(), survivors);
                load_eclipse_info_from_xml_file_once = false;

                if (eclipse_info_xml_file_state == COULD_NOT_OPEN_FILE) {
                    state = NO_FILE_STATE;
                    time_until_next_update = cooldown_time_for_reloading;
                    continue;
                } else if (eclipse_info_xml_file_state == FILE_OPENED_SUCCESSFULLY) {
                    order_and_correct_survivor_list(survivors, survivors_ordered_from_image_file, eclipse_level_sum, mastery_level);
                    entity_index = survivors_ordered_by_eclipse_level(survivors_ordered_from_image_file, survivors_ordered, survivor_order);
                }
            }

            if (mouse_wheel.y != 0) {
                velocity += acceleration * GetFrameTime();
                direction = mouse_wheel.y;
            }

            camera.target.y -= velocity * direction;

            velocity -= friction * GetFrameTime();
            clamp(velocity, 0.f, max_velocity);

            max_camera_y = template_complete_boundary.height-height+(height*0.21f);
            clamp(camera.target.y, 0.f, max_camera_y);
            mouse_wheel = GetMouseWheelMoveV();

            time_until_next_update -= GetFrameTime();
            if (time_until_next_update <= 0.f) {
                time_until_next_update = cooldown_time_for_reloading;

                eclipse_info_xml_file_state = get_eclipse_info_from_xml(xml_file.c_str(), survivors);
                if (eclipse_info_xml_file_state == COULD_NOT_OPEN_FILE) {
                    state = NO_FILE_STATE;
                    time_until_next_update = cooldown_time_for_reloading;
                    continue;
                } else if (eclipse_info_xml_file_state == FILE_OPENED_SUCCESSFULLY) {
                    eclipse_level_sum = 0;
                    mastery_level = 0;

                    order_and_correct_survivor_list(survivors, survivors_ordered_from_image_file, eclipse_level_sum, mastery_level);
                    entity_index = survivors_ordered_by_eclipse_level(survivors_ordered_from_image_file, survivors_ordered, Survivor_Order::Ascending);
                }
            } 

            std::vector<Eclipse_Level_Collsion_Entity> eclipse_level_collision_entities = {};

            BeginMode2D(camera);
            for (size_t index = 0; index < survivors_ordered.size(); index++) {
                // Template ---------------------------------------- //
                Rectangle template_source = {
                    .x = 0.f,
                    .y = 0.f,
                    .width  = (float)template_texture.width,
                    .height = (float)template_texture.height,
                };

                Vector2 template_size = {
                    .x = width * 0.85f,
                    .y = template_size.x*0.2f,
                };

                float padding_between_templates = template_size.y * 0.12f;
                float template_centered_x = width/2.f - template_size.x/2.f;

                Rectangle template_dest = {
                    .x = template_centered_x,
                    .y = (index * (template_size.y + padding_between_templates)) + padding_between_templates,
                    .width  = template_size.x,
                    .height = template_size.y,
                };

                DrawTexturePro(template_texture, 
                        template_source, 
                        template_dest, 
                        { 0.f, 0.f}, 
                        0.f, 
                        WHITE);

                // NOTE: Collecting information about the size of every survivor template.
                template_complete_boundary = {
                    .x = template_dest.x,
                    .y = 0.f,
                    .width = template_dest.width,
                    .height = template_dest.height + padding_between_templates,
                };
                // Template ---------------------------------------- //

                // NOTE: This is the scale between the final template image size and the original template image size.
                Vector2 scale = {
                    template_texture.width  / template_dest.width,
                    template_texture.height / template_dest.height,
                };
                   
                // Survivor ---------------------------------------- //
                float tile_size = entity_index[index].index < 7 ? 256.f : 128.f;

                Vector2 survivor_pos = survivors_image_pos[entity_index[index].index];
                
                Rectangle survivor_source = {
                    .x = survivor_pos.x,
                    .y = survivor_pos.y,
                    .width  = tile_size,
                    .height = tile_size,
                };

                Vector2 final_survivor_image_size = { 
                    451.f / scale.x,
                    437.f / scale.y
                };

                Rectangle survivor_dest = { 
                    template_dest.x+(91.f / scale.x),
                    template_dest.y+(77.f / scale.y),
                    final_survivor_image_size.x,
                    final_survivor_image_size.y 
                };

                DrawTexturePro(survivors_texture, 
                        survivor_source,
                        survivor_dest,
                        { 0.f, 0.f}, 
                        0.f, 
                        WHITE);

                // Survivor ---------------------------------------- //

                // Survivor name ----------------------------------- //
                std::string survivor_name = survivors_ordered[index].first;

                Rectangle final_text = {
                    600.f / scale.x + template_dest.x,
                    100.f / scale.y + template_dest.y,
                    (float)survivor_dest.height*0.3f,
                    0.f,
                };

                // NOTE: Text plus Long Shadows
                for (size_t i = 2; i > 0; i--) {
                    DrawTextEx(bombardier, 
                            survivor_name.c_str(),
                            { final_text.x+(i*3.f), final_text.y+(i*3.f) }, 
                            final_text.width, 
                            1.f, 
                            i < 2 ? WHITE : (Color){0,0,0,200});
                }
                // Survivor name ----------------------------------- //

                // Eclipse level ----------------------------------- //
                UInt eclipse_level = survivors_ordered[index].second;
                for (size_t i = 0; i < eclipse_level; i++) {
                    float eclipse_image_size = 128.f;

                    Rectangle eclipse_source = {
                        .x = i * eclipse_image_size,
                        .y = i < mastery_level ? eclipse_image_size : 0.f,
                        .width  = eclipse_image_size,
                        .height = eclipse_image_size,
                    };

                    Vector2 final_eclipse_image_size = {
                        200 / scale.x,
                        211 / scale.y,                    
                    };

                    float eclipse_padding = 61.f / scale.x;
                    Rectangle eclipse_dest = {
                        .x = (616.f / scale.x) + ((final_eclipse_image_size.x + eclipse_padding) * i) + template_dest.x,
                        .y = 295.f / scale.y + template_dest.y,
                        .width  = final_eclipse_image_size.x,
                        .height = final_eclipse_image_size.y 
                    };

                    DrawTexturePro(eclipse_texture, 
                            eclipse_source,
                            eclipse_dest,
                            { 0.f, 0.f}, 
                            0.f, 
                            WHITE);

                    eclipse_level_collision_entities.emplace_back((Eclipse_Level_Collsion_Entity){eclipse_dest, (UInt)i});
                }
                // Eclipse level ----------------------------------- //
            }

            // Modifers ------------------------------------ //
            mouse_pos = GetMousePosition();
            for (const auto& entity : eclipse_level_collision_entities) {
                if (CheckCollisionPointRec({mouse_pos.x, mouse_pos.y+camera.target.y}, entity.boundary)) {
                        UInt modifiers_width[] = { 2030, 1975, 2314, 1636, 1407, 1797, 1766, 2311 };
                        UInt modifiers_height = 201;

                        Rectangle modifiers_source = {
                            0.f, 
                            (float)modifiers_height*entity.eclipse_level,
                            (float)modifiers_width[entity.eclipse_level],
                            (float)modifiers_height
                        };

                        Vector2 modifiers_dest_size = {
                            template_complete_boundary.width*0.11f + modifiers_width[entity.eclipse_level]*0.1f,
                            template_complete_boundary.height*0.06f + modifiers_height*0.1f,
                        };

                        float result_to_align = 0;
                        if (mouse_pos.x+modifiers_dest_size.x > width) result_to_align = mouse_pos.x+modifiers_dest_size.x - width + 10.f;

                        Vector2 modifiers_dest_pos = {
                            mouse_pos.x - result_to_align, 
                            camera.target.y+mouse_pos.y,
                        };

                        DrawTexturePro(modifiers_texture, modifiers_source, {modifiers_dest_pos.x, modifiers_dest_pos.y, modifiers_dest_size.x, modifiers_dest_size.y}, {0.f, 0.f}, 0.f, WHITE);
                }
            }
            // Modifers ------------------------------------ //

            template_complete_boundary.height *= survivors_ordered_from_image_file.size();
            EndMode2D();

            // Completion Image -------------------------------------------- //
            Vector2 completion_size = {
                .x = width * 0.5f,
                .y = height * 0.17f,
            };

            Rectangle completion_dest = {
                .x = width/2.f - completion_size.x/2.f,
                .y = height - completion_size.y - 10.f,
                .width  = completion_size.x,
                .height = completion_size.y,
            };

            Vector2 background_rec_size = {
                completion_size.x+(completion_size.x*0.03f),
                completion_size.y+(completion_size.y*0.1f),
            };

            Vector2 background_rec_pos = {
                completion_dest.x+(completion_size.x/2.f)-(background_rec_size.x/2.f),
                completion_dest.y+(completion_size.y/2.f)-(background_rec_size.y/2.f)
            };

            DrawRectangleRounded({
                    background_rec_pos.x,
                    background_rec_pos.y,
                    background_rec_size.x,
                    background_rec_size.y},
                    0.2f, 
                    11, 
                    {color2.r, color2.g, color2.b, 240});

            DrawRectangleRoundedLines({
                    background_rec_pos.x+1.f,
                    background_rec_pos.y+1.f,
                    background_rec_size.x-2.f,
                    background_rec_size.y-2.f},
                    0.2f,
                    100.f,
                    4.0f,
                    {color1.r, color1.g, color1.b, 185});

            DrawTexturePro(completion_texture, 
                    { 0.f, 0.f, (float)completion_texture.width, (float)completion_texture.height}, 
                    completion_dest,
                    { 0.f, 0.f}, 
                    0.f, 
                    WHITE);
            // Completion Image -------------------------------------------- //

            // Completion Text --------------------------------------------- //
            Vector2 completion_scale = {
                completion_texture.width / completion_dest.width,
                completion_texture.height / completion_dest.height,
            };

            const char* completion_text = TextFormat("%d/%d", eclipse_level_sum, survivors_ordered_from_image_file.size()*8);

            float completion_text_fontsize = completion_dest.height*0.30f + completion_dest.width*0.034f;
            Vector2 completion_text_size = MeasureTextEx(bombardier, completion_text, completion_text_fontsize, 0.f);

            Vector2 completion_text_pos = {
                .x = completion_dest.x + (550 / completion_scale.x) - (completion_text_size.x / 2.f),
                .y = completion_dest.y + (320 / completion_scale.y) - (completion_text_size.y / 2.f),
            };

            // NOTE: Text plus Long Shadows
            for (size_t i = 2; i > 0; i--) 
                DrawTextEx(bombardier, 
                        completion_text,
                        { completion_text_pos.x+(i*3.f), completion_text_pos.y+(i*3.f) }, 
                        completion_text_fontsize, 
                        1.f, 
                        i < 2 ? WHITE : (Color){0,0,0,200});
            // Completion Text --------------------------------------------- //

            // Completion Percent ------------------------------------------ //
            UInt completion_percent = normalize<float>((float)eclipse_level_sum, 0, survivors_ordered_from_image_file.size()*8) * 100.f;
            const char* completion_percent_text = TextFormat("%d\%%", completion_percent); 

            float completion_percent_fontsize = completion_dest.height*0.55f;
            Vector2 completion_percent_size = MeasureTextEx(bombardier, completion_percent_text, completion_percent_fontsize, 1.f);

            Vector2 completion_percent_pos = {
                .x = (1030.f / completion_scale.x + completion_dest.x) + ((690.f / completion_scale.x)/2.f) - (completion_percent_size.x/2.f),
                .y = (155.f / completion_scale.y + completion_dest.y) + ((309.f / completion_scale.y)/2.f) - (completion_percent_size.y/2.f) + 3.f,
            };

            // NOTE: Text plus Long Shadows
            for (size_t i = 2; i > 0; i--) 
                DrawTextEx(bombardier, 
                        completion_percent_text,
                        { completion_percent_pos.x+(i*3.f), completion_percent_pos.y+(i*3.f) }, 
                        completion_percent_fontsize, 
                        1.f, 
                        i < 2 ? WHITE : (Color){0,0,0,200});
            // Completion Percent ------------------------------------------ //

            // Completion Mastery ------------------------------------------ //
            Vector2 numbers_image_size = {
                .x = 256.f,//43.f,
                .y = (float)numbers_texture.height,
            };

            Rectangle numbers_source = {
                .x = mastery_level * numbers_image_size.x,
                .y = 0.f,
                .width  = numbers_image_size.x,
                .height = numbers_image_size.y,
            };

            Vector2 numbers_size = {
                completion_dest.width*0.099f,
                completion_dest.height*0.39f,
            };

            Rectangle numbers_dest = {
                .x = (1729.f / completion_scale.x + completion_dest.x) + ((330.f / completion_scale.x)/2.f) - (numbers_size.x/2.f),
                .y = (179.f / completion_scale.y + completion_dest.y) + ((323.f / completion_scale.y)/2.f) - (numbers_size.y/2.f),
                .width  = numbers_size.x,
                .height = numbers_size.y,
            };

            DrawTexturePro(numbers_texture, 
                    numbers_source,
                    numbers_dest,
                    { 0.f, 0.f}, 
                    0.f, 
                    WHITE);
            // Completion Mastery ------------------------------------------ //

            // Scroll Bar -------------------------------------------------- //
            float camera_target_y_normalized = normalize(camera.target.y, template_complete_boundary.y, max_camera_y);

            Vector2 scroll_bar_size = {
                .x = template_complete_boundary.width*0.009f,
                .y = template_complete_boundary.height*0.03f,
            };

            Vector2 scroll_bar_pos = {
                .x = width - scroll_bar_size.x,
                .y = (camera_target_y_normalized*(height-scroll_bar_size.y)),
            };

            DrawRectangleRounded({
                    scroll_bar_pos.x,
                    scroll_bar_pos.y,
                    scroll_bar_size.x,
                    scroll_bar_size.y}, 
                    1.f, 
                    20, {170, 170, 170, 180});
            // Scroll Bar -------------------------------------------------- //

            // Reload ------------------------------------------------------ //
            if (!is_animation_done) animation_duration -= GetFrameTime();
            if (animation_duration < 0.f) {
                animation_step += 12.6;
                animation_duration = animation_speed;

                if (animation_step > animation_total_steps-1) {
                    animation_step = 0;
                    is_animation_done = true;
                }
            }

            reloaded_time -= GetFrameTime();
            if (reloaded_time <= 0.f) {
                reloaded = 0;
                reloaded_time = cooldown_time_for_reloaded;
            }

            Rectangle reload_source = {
                .x = 0.f,
                .y = 0.f,
                .width  = (float)reload_texture.width,
                .height = (float)reload_texture.height,
            };

            float reload_size = template_complete_boundary.x*0.8f;
            const char* reloaded_text = TextFormat("%s", reloaded ? "Reloaded!" : "Reload!");
            float reloaded_text_fontsize = reload_size*0.32f;

            Vector2 reload_pos = {
                .x = template_complete_boundary.x/2.f - reload_size/2.f,
                .y = height-reload_size-reloaded_text_fontsize,
            };

            Vector2 reloaded_text_size = MeasureTextEx(bombardier, reloaded_text, reloaded_text_fontsize, 0.f);
            Vector2 reloaded_text_pos = { 
                .x = reload_pos.x + reload_size/2.f - reloaded_text_size.x/2.f,
                .y = height-reloaded_text_fontsize,
            };

            float time_fontsize = reload_size*0.48f;
            const char* time_text = TextFormat("%d", (int)time_until_next_update);
            Vector2 time_size = MeasureTextEx(bombardier, time_text, time_fontsize, 0.f);

            Vector2 time_pos = {
                template_complete_boundary.x/2.f - time_size.x/2.f,
                reload_pos.y + reload_size/2.f - time_size.y/2.f
            };

            if (CheckCollisionPointRec(mouse_pos, {reload_pos.x, reload_pos.y, reload_size, reload_size})) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    eclipse_info_xml_file_state = get_eclipse_info_from_xml(xml_file.c_str(), survivors);

                    is_animation_done = false;

                    if (eclipse_info_xml_file_state == COULD_NOT_OPEN_FILE) {
                        time_until_next_update = cooldown_time_for_reloading;
                        state = NO_FILE_STATE;
                        continue;
                    } else if (eclipse_info_xml_file_state == FILE_OPENED_SUCCESSFULLY) {
                        eclipse_level_sum = 0;
                        mastery_level = 0;
                        time_until_next_update = cooldown_time_for_reloading;
                        order_and_correct_survivor_list(survivors, survivors_ordered_from_image_file, eclipse_level_sum, mastery_level);
                        entity_index = survivors_ordered_by_eclipse_level(survivors_ordered_from_image_file, survivors_ordered, survivor_order);

                        reloaded = 1;
                    }
                }

                float new_size = template_complete_boundary.x*0.85f;

                Vector2 new_pos = {
                    .x = template_complete_boundary.x/2.f - new_size/2.f,
                    .y = reload_pos.y + reload_size/2.f - new_size/2.f,
                };

                reload_size = new_size;
                reload_pos = new_pos;

                time_fontsize += new_size*0.05f;
                time_size = MeasureTextEx(bombardier, time_text, time_fontsize, 0.f);

                time_pos = {
                    template_complete_boundary.x/2.f - time_size.x/2.f,
                    reload_pos.y + reload_size/2.f - time_size.y/2.f
                };
            }

            DrawTexturePro(reload_texture, 
                    reload_source,
                    { reload_pos.x + reload_size/2.f, reload_pos.y + reload_size/2.f, reload_size, reload_size },
                    { reload_size/2.f, reload_size/2.f }, 
                    animation_step, 
                    WHITE);

            DrawTextPro(bombardier, 
                    time_text,
                    time_pos, 
                    {0.f, 0.f},
                    0.f,
                    time_fontsize, 
                    0.f, 
                    WHITE);

            DrawTextPro(bombardier, 
                    reloaded_text,
                    reloaded_text_pos, 
                    {0.f, 0.f},
                    0.f,
                    reloaded_text_fontsize, 
                    0.f, 
                    WHITE);
            // Reload ------------------------------------------------------ //

            // Sort ------------------------------------------------------ //
            Rectangle sort_up_source = {
                .x = 0.f,
                .y = 0.f,
                .width  = (float)sort_texture.width,
                .height = (float)sort_texture.height,
            };

            float sort_up_size = template_complete_boundary.x*0.6f;
            Rectangle sort_up_dest = {
                .x = width * 0.003f,
                .y = sort_up_dest.x,
                .width  = sort_up_size,
                .height = sort_up_size,
            };

            if (CheckCollisionPointRec(mouse_pos, sort_up_dest)) {
                float new_sort_up_size = template_complete_boundary.x*0.65f;
                float difference_in_size = new_sort_up_size - sort_up_size;

                sort_up_dest = {
                    .x = width * 0.003f - difference_in_size/2.f,
                    .y = sort_up_dest.x - difference_in_size/2.f,
                    .width  = new_sort_up_size,
                    .height = new_sort_up_size,
                };

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    change_sort_texture = !change_sort_texture;
                }
            }

            sort_texture_index = change_sort_texture ? Sort_Texture::DOWN : Sort_Texture::UP;
            switch (sort_texture_index) {
                break; case Sort_Texture::UP:
                    survivor_order = Ascending;
                break; case Sort_Texture::DOWN:
                    survivor_order = Descending;
            }
            entity_index = survivors_ordered_by_eclipse_level(survivors_ordered_from_image_file, survivors_ordered, survivor_order);

            DrawTexturePro(textures[sort_texture_index], sort_up_source, sort_up_dest, { 0.f, 0.f }, 0.f, WHITE);
            // Sort ------------------------------------------------------ //

            }

        EndDrawing();
    }

    for (size_t i = 0; i < texture_count; i++) UnloadTexture(textures[i]);
    UnloadFont(bombardier);

    CloseWindow();

    return 0;
}
