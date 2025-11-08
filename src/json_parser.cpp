#include <string>
#include <cstring>
#include <vector>
#include <mutex>
#include <algorithm>
#include <emscripten.h>

#include "../include/nlohmann/json.hpp"
#include "../include/miniz/miniz.h"

using json = nlohmann::json;

struct JudgeLineStats {
    int event_count;  // 该判定线的事件总数
    int special_event_count;    // 该判定线的特殊事件总数
    int note_count;   // 该判定线的音符总数
};

struct TimeSignature {
    int measure;
    int numerator;
    int denominator;
};

struct JudgeLineConfig {
    TimeSignature startTime;
    TimeSignature endTime;
    bool copyEvents;
    bool copyNotes;
};

struct ParseResult {
    // std::string raw_json;
    // 我去，直接传输一个 40MB 的源文件吗，小心内存爆炸

    int bpm_count;
    double min_bpm;
    double max_bpm;
    int rpe_version;
    std::string charter;
    std::string composer;
    std::string id;
    std::string level;
    std::string name;

    int judge_line_count;
    std::vector<JudgeLineStats> judge_line_stats;

    int error_code;
};

bool has_start_end_time(const json& obj) {
    return obj.contains("startTime") && obj["startTime"].is_array() 
        && obj.contains("endTime") && obj["endTime"].is_array();
}

ParseResult parse_single_json(const char* json_str, size_t json_len) {
    ParseResult result = {
        0, 0.0, 0.0, 0, "", "", "", "", "", 
        0, {},
        -1};
    if (!json_str || json_len == 0) {
        result.error_code = -2;
        return result;
    }

    // result.raw_json = std::string(json_str, json_len);
    json j = json::parse(json_str, json_str + json_len);
    
    // 处理 BPMList
    if (j.contains("BPMList") && j["BPMList"].is_array()) {
        auto& bpm_list = j["BPMList"];
        result.bpm_count = bpm_list.size();
           
        if (result.bpm_count > 0) {
            std::vector<double> bpms;
            for (auto& item : bpm_list) {
                if (item.contains("bpm") && item["bpm"].is_number()) {
                    bpms.push_back(item["bpm"].get<double>());
                }
            }
            if (!bpms.empty()) {
                result.min_bpm = *std::min_element(bpms.begin(), bpms.end());
                result.max_bpm = *std::max_element(bpms.begin(), bpms.end());
            }
        }
    }
        
    // 处理 META
    if (j.contains("META") && j["META"].is_object()) {
        auto& meta = j["META"];
        if (meta.contains("RPEVersion") && meta["RPEVersion"].is_number()) {
            result.rpe_version = meta["RPEVersion"].get<int>();
        }
        if (meta.contains("charter") && meta["charter"].is_string()) {
            result.charter = meta["charter"].get<std::string>();
        }
        if (meta.contains("composer") && meta["composer"].is_string()) {
            result.composer = meta["composer"].get<std::string>();
        }
        if (meta.contains("id") && meta["id"].is_string()) {
            result.id = meta["id"].get<std::string>();
        }
        if (meta.contains("level") && meta["level"].is_string()) {
            result.level = meta["level"].get<std::string>();
        }
        if (meta.contains("name") && meta["name"].is_string()) {
            result.name = meta["name"].get<std::string>();
        }
    }

    // 处理判定线数量（judgeLineList 数组长度）
    if (j.contains("judgeLineList") && j["judgeLineList"].is_array()) {
        auto& judge_lines = j["judgeLineList"];
        result.judge_line_count = judge_lines.size();
        result.judge_line_stats.reserve(result.judge_line_count);

        // 遍历每个判定线，单独统计
        for (auto& line : judge_lines) {
            JudgeLineStats stats = {0, 0};  // 初始化当前判定线的计数为 0

            // 处理当前判定线的 eventLayers 事件
            if (line.contains("eventLayers") && line["eventLayers"].is_array()) {
                for (auto& layer : line["eventLayers"]) {
                    // 遍历 eventLayers 中的集合
                    const std::vector<std::string> event_arrays = {
                        "alphaEvents",
                        "moveXEvents",
                        "moveYEvents",
                        "rotateEvents",
                        "speedEvents"
                    };
                    for (const auto& arr_name : event_arrays) {
                        if (layer.contains(arr_name) && layer[arr_name].is_array()) {
                            for (auto& event : layer[arr_name]) {
                                // 遍历数组中的事件
                                if (has_start_end_time(event)) {
                                    stats.event_count++;
                                }
                            }
                        }
                    }
                }
            }

            // 处理当前判定线的 extended 事件
            if (line.contains("extended") && line["extended"].is_object()) {
                auto& extended = line["extended"];
                // 遍历 extended 中所有数组（不限制键名）
                for (auto& [key, arr] : extended.items()) {
                    if (arr.is_array()) {
                        for (auto& event : arr) {
                            // 遍历数组中的事件
                            if (has_start_end_time(event)) {
                                stats.special_event_count++;
    /* 并不知道这个东西是什么，是不是故事板？
     * 但是它毕竟是事件，所以我也检测了，如果要用也可以用。
     */
                            }
                        }
                    }
                }
            }

            // 处理当前判定线的 notes
            if (line.contains("notes") && line["notes"].is_array()) {
                for (auto& note : line["notes"]) {
                    // 遍历每个音符
                    if (has_start_end_time(note)) {
                        stats.note_count++;
                    }
                }
            }

            // 将当前判定线的统计结果存入 vector
            result.judge_line_stats.push_back(stats);
        }
    } else {
        result.judge_line_count = 0;
    }

    result.error_code = 0;
    return result;
}

std::string result_to_json(const ParseResult& res) {
    json j;
    // j["raw_json"] = res.raw_json;

    j["bpm_count"] = res.bpm_count;
    j["min_bpm"] = res.min_bpm;
    j["max_bpm"] = res.max_bpm;
    j["rpe_version"] = res.rpe_version;
    j["charter"] = res.charter;
    j["composer"] = res.composer;
    j["id"] = res.id;
    j["level"] = res.level;
    j["name"] = res.name;

    j["judge_line_count"] = res.judge_line_count;
    // 转换每个判定线的统计数据为 JSON 数组
    json judge_line_stats_json = json::array();
    for (const auto& stats : res.judge_line_stats) {
        judge_line_stats_json.push_back({
            {"event_count", stats.event_count},
            {"special_event_count", stats.special_event_count},
            {"note_count", stats.note_count}
        });
    }
    j["judge_line_stats"] = judge_line_stats_json;

    j["error"] = res.error_code;

    return j.dump();
}

extern "C" const char* parse_json(const char* json_str, size_t json_len) {
    static std::string result_str;
    ParseResult res = parse_single_json(json_str, json_len);
    result_str = result_to_json(res);
    return result_str.c_str();
}

extern "C" const char* extract_pez(const unsigned char* pez_data, size_t data_size) {
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    // 初始化 ZIP 读取器
    if (!mz_zip_reader_init_mem(&zip_archive, pez_data, data_size, 0)) {
        // result_str = result_to_json(res);
        // return result_str.c_str();
        return "(failed)";
    }
    
    char* json_str;
    for (int i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++) {
        mz_zip_archive_file_stat file_info;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_info)) continue;
        
        char filename[256];
        if (!mz_zip_reader_get_filename(&zip_archive, i, filename, sizeof(filename))) continue;

        if (strstr(filename, ".json") == nullptr) continue;
        
        void* json_data = malloc(file_info.m_uncomp_size + 1);
        if (!json_data) {
            mz_zip_reader_end(&zip_archive);
            return "(failed due to memory)";
        }

        if (mz_zip_reader_extract_to_mem(&zip_archive, i, json_data, 
            file_info.m_uncomp_size, 0)) {
            ((char*)json_data)[file_info.m_uncomp_size] = '\0';
            mz_zip_reader_end(&zip_archive);
            return (const char*)json_data;
        } else {
            free(json_data);
            mz_zip_reader_end(&zip_archive);
            return "(failed while extracting)";
        }
    }

    mz_zip_reader_end(&zip_archive);
    // result_str = result_to_json(res);
    // return result_str.c_str();
    return "(failed not found)";
}

static std::vector<std::string> merge_chunks;
static int total_chunks = 0;
static std::mutex chunks_mutex;

extern "C" void init_merge(int total) {
    std::lock_guard<std::mutex> lock(chunks_mutex);
    merge_chunks.clear();
    total_chunks = total;
    merge_chunks.reserve(total); // 预分配空间
}

extern "C" int process_merge_chunk(int chunk_index, int total, const char* data, size_t len) {
    std::lock_guard<std::mutex> lock(chunks_mutex);
    if (chunk_index < 0 || chunk_index >= total || total != total_chunks) {
        return -1; // 块序号或总块数不匹配
    }
    if (!data || len == 0) {
        return -2; // 空数据
    }
    // 存储当前块
    if (chunk_index >= merge_chunks.size()) {
        merge_chunks.resize(chunk_index + 1);
    }
    merge_chunks[chunk_index] = std::string(data, len);
    return 0; // 成功
}

json merge_json(json form_json) {
    static std::string result_str;
    result_str = "";

    if (!form_json.contains("firstCardId") || !form_json["firstCardId"].is_number() ||
        !form_json.contains("truncateStart") || !form_json["truncateStart"].is_boolean() ||
        !form_json.contains("truncateEnd") || !form_json["truncateEnd"].is_boolean() ||
        !form_json.contains("cards") || !form_json["cards"].is_array()) {
        result_str = "{error: -2, message: Missing required fields in form}";
        return json::parse(result_str);
    }

    int first_card_id = form_json["firstCardId"].get<int>();
    bool truncate_start = form_json["truncateStart"].get<bool>();
    bool truncate_end = form_json["truncateEnd"].get<bool>();
    auto& cards_array = form_json["cards"];

    json base_chart;
    for (auto& card : cards_array) {
        if (card.contains("id") && card["id"].is_number() && 
            card["id"].get<int>() == first_card_id &&
            card.contains("chartJson") && card["chartJson"].is_string()) {
            base_chart = json::parse(card["chartJson"].get<std::string>());
            break;
        }
    }

    json merged = json::object();
    for (auto& [key, value] : base_chart.items()) {
        if (key != "judgeLineList") {
            merged[key] = value;
        }
    }
    int judge_line_count = 0;

    json& merged_judge_lines = merged["judgeLineList"] = json::array();
    if (base_chart.contains("judgeLineList") && base_chart["judgeLineList"].is_array()) {
        judge_line_count = base_chart["judgeLineList"].size();
        for (auto& line : base_chart["judgeLineList"]) {
            json line_frame = json::object();
            for (auto& [key, value] : line.items()) {
                if (key != "eventLayers" && key != "notes") {
                    line_frame[key] = value;
                }
            }
            
            line_frame["eventLayers"] = json::array();
            for (int i = 0; i < 4; ++i) { // 四层事件
                json layer_events = {
                    {"alphaEvents", json::array()},
                    {"moveXEvents", json::array()},
                    {"moveYEvents", json::array()},
                    {"rotateEvents", json::array()},
                    {"speedEvents", json::array()}
                };
                line_frame["eventLayers"].push_back(layer_events);
            }

            line_frame["notes"] = json::array();
            merged_judge_lines.push_back(line_frame);
        }
    }

    // 复用一下 base_chart
    for (auto& card : cards_array) {
        std::vector<JudgeLineConfig> judge_line_configs(judge_line_count);

        TimeSignature default_start = {0, 0, 1};
        TimeSignature default_end = {0, 0, 1};
        bool default_copy_events = false;
        bool default_copy_notes = false;

        if (card.contains("timeControls") && card["timeControls"].is_object()) {
            auto& time_controls = card["timeControls"];
            // 解析通用复选框参数
            if (time_controls.contains("checkboxes") && time_controls["checkboxes"].is_array() && 
                time_controls["checkboxes"].size() >= 2) {
                default_copy_events = time_controls["checkboxes"][0].get<bool>();
                default_copy_notes = time_controls["checkboxes"][1].get<bool>();
            }
            // 解析通用时间参数
            if (time_controls.contains("inputs") && time_controls["inputs"].is_array() && 
                time_controls["inputs"].size() >= 6) {
                auto& inputs = time_controls["inputs"];
                default_start = {
                    inputs[0].get<int>(),
                    inputs[1].get<int>(),
                    inputs[2].get<int>()
                };
                default_end = {
                    inputs[3].get<int>(),
                    inputs[4].get<int>(),
                    inputs[5].get<int>()
                };
            }
        }
        std::vector<json> independent_lines;
        if (card.contains("independentJudgeLines") && card["independentJudgeLines"].is_array()) {
            independent_lines = card["independentJudgeLines"].get<std::vector<json>>();
        }

        for (int i = 0; i < judge_line_count; ++i) {
            // 默认使用通用配置
            JudgeLineConfig config;
            config.startTime = default_start;
            config.endTime = default_end;
            config.copyEvents = default_copy_events;
            config.copyNotes = default_copy_notes;

            // 查找当前判定线是否有独立配置
            auto it = std::find_if(independent_lines.begin(), independent_lines.end(),
                [i](const json& line) {
                    return line.contains("id") && line["id"].is_number() && line["id"].get<int>() == i;
                });

            if (it != independent_lines.end()) {
                // 存在独立配置，处理参数覆盖
                auto& independent_config = *it;
                if (independent_config.contains("timeControls") && independent_config["timeControls"].is_object()) {
                    auto& ind_time_controls = independent_config["timeControls"];
                    // 处理复选框参数
                    if (ind_time_controls.contains("checkboxes") && ind_time_controls["checkboxes"].is_array() &&
                        ind_time_controls["checkboxes"].size() >= 2) {
                        config.copyEvents = ind_time_controls["checkboxes"][0].get<bool>();
                        config.copyNotes = ind_time_controls["checkboxes"][1].get<bool>();
                    }
                    // 处理时间参数（-1 表示使用通用配置对应值）
                    if (ind_time_controls.contains("inputs") && ind_time_controls["inputs"].is_array() &&
                        ind_time_controls["inputs"].size() >= 6) {
                        auto& ind_inputs = ind_time_controls["inputs"];
                        // 处理开始时间
                        config.startTime.measure = (ind_inputs[0].get<int>() == -1) ? default_start.measure : ind_inputs[0].get<int>();
                        config.startTime.numerator = (ind_inputs[1].get<int>() == -1) ? default_start.numerator : ind_inputs[1].get<int>();
                        config.startTime.denominator = (ind_inputs[2].get<int>() == -1) ? default_start.denominator : ind_inputs[2].get<int>();
                        // 处理结束时间
                        config.endTime.measure = (ind_inputs[3].get<int>() == -1) ? default_end.measure : ind_inputs[3].get<int>();
                        config.endTime.numerator = (ind_inputs[4].get<int>() == -1) ? default_end.numerator : ind_inputs[4].get<int>();
                        config.endTime.denominator = (ind_inputs[5].get<int>() == -1) ? default_end.denominator : ind_inputs[5].get<int>();
                    }
                }
            }

            // 存入配置向量
            judge_line_configs[i] = config;

            /*
            emscripten_log(EM_LOG_DEBUG, 
                "JudgeLine %d config: start=(%d, %d, %d), end=(%d, %d, %d), copyEvents=%d, copyNotes=%d",
                i,  // 判定线编号
                config.startTime.measure,    // 开始时间-小节
                config.startTime.numerator,  // 开始时间-分子
                config.startTime.denominator,// 开始时间-分母
                config.endTime.measure,      // 结束时间-小节
                config.endTime.numerator,    // 结束时间-分子
                config.endTime.denominator,  // 结束时间-分母
                config.copyEvents ? 1 : 0,   // 复制事件（0/1表示false/true）
                config.copyNotes ? 1 : 0     // 复制音符（0/1表示false/true）
            );
            */
        }

        if (card.contains("chartJson") && card["chartJson"].is_string()) {
            base_chart = json::parse(card["chartJson"].get<std::string>());
            if (base_chart.contains("judgeLineList") && base_chart["judgeLineList"].is_array()) {
                size_t idx = 0;
                auto to_total_beats = [](const TimeSignature& ts) -> double {
                    int denom = ts.denominator;
                    if (denom == 0) denom = 1; // 避免除零错误
                    return ts.measure + 
                        static_cast<double>(ts.numerator) / denom;
                };
                
                auto parse_time_array = [](const json& arr) -> TimeSignature {
                    TimeSignature ts = {0, 0, 1};
                    if (arr.size() >= 3) {
                        ts.measure = arr[0].get<int>();
                        ts.numerator = arr[1].get<int>();
                        ts.denominator = arr[2].get<int>() > 0 ? arr[2].get<int>() : 1;
                    }
                    return ts;
                };

                for (auto& line : base_chart["judgeLineList"]) {
                    JudgeLineConfig& config = judge_line_configs[idx];
                    double truncate_start_beats = to_total_beats(config.startTime);
                    double truncate_end_beats = to_total_beats(config.endTime);
                    if (truncate_start_beats > truncate_end_beats) {
                        idx++;
                        continue;
                    }

                    if (config.copyEvents) {
                        // 复制事件
                        if (line.contains("eventLayers") && line["eventLayers"].is_array()) {
                            size_t layer_idx = 0;
                            for (auto& layer : line["eventLayers"]) {
                                // 只处理前 4 层事件
                                if (layer_idx >= 4) break;
                                
                                // 遍历事件类型
                                const std::vector<std::string> event_types = {
                                    "alphaEvents", "moveXEvents", "moveYEvents", 
                                    "rotateEvents", "speedEvents"
                                };
                                
                                for (const auto& event_type : event_types) {
                                    if (layer.contains(event_type) && layer[event_type].is_array()) {
                                        for (auto& event : layer[event_type]) {
                                            // 检查事件是否包含有效时间信息
                                            if (!has_start_end_time(event)) continue;
                                            
                                            TimeSignature event_start = parse_time_array(event["startTime"]);
                                            TimeSignature event_end = parse_time_array(event["endTime"]);
                                            
                                            double event_start_beats = to_total_beats(event_start);
                                            double event_end_beats = to_total_beats(event_end);
                                            
                                            bool pass_start = false;
                                            bool pass_end = false;
                                            
                                            if (truncate_start) {
                                                pass_start = (event_start_beats >= truncate_start_beats);
                                            } else {
                                                pass_start = (event_end_beats >= truncate_start_beats);
                                            }
                                            
                                            if (truncate_end) {
                                                pass_end = (event_end_beats <= truncate_end_beats);
                                            } else {
                                                pass_end = (event_start_beats <= truncate_end_beats);
                                            }
                                            
                                            // 同时满足条件则复制事件
                                            if (pass_start && pass_end) {
                                                merged_judge_lines[idx]["eventLayers"][layer_idx][event_type].push_back(event);
                                            }
                                        }
                                    }
                                }
                                layer_idx++;
                            }
                        }
                    }
                    if (config.copyNotes) {
                        // 复制音符
                        if (line.contains("notes") && line["notes"].is_array()) {
                            for (auto& note : line["notes"]) {
                                // 检查音符是否包含有效时间信息
                                if (!has_start_end_time(note)) continue;
                                
                                TimeSignature note_start = parse_time_array(note["startTime"]);
                                TimeSignature note_end = parse_time_array(note["endTime"]);
                                
                                double note_start_beats = to_total_beats(note_start);
                                double note_end_beats = to_total_beats(note_end);
                                
                                // 筛选条件判断
                                bool pass_start = false;
                                bool pass_end = false;
                                
                                if (truncate_start) {
                                    pass_start = (note_start_beats >= truncate_start_beats);
                                } else {
                                    pass_start = (note_end_beats >= truncate_start_beats);
                                }

                                if (truncate_end) {
                                    pass_end = (note_end_beats <= truncate_end_beats);
                                } else {
                                    pass_end = (note_start_beats <= truncate_end_beats);
                                }
                                
                                // 同时满足条件则复制音符到合并结果的对应判定线
                                if (pass_start && pass_end) {
                                    merged_judge_lines[idx]["notes"].push_back(note);
                                }
                            }
                        }
                    }
                    
                    idx++;
                }
            }
        }
    }
    
    return merged;
}

extern "C" const char* finalize_merge() {
    std::lock_guard<std::mutex> lock(chunks_mutex);
    // 拼接所有分块为完整 JSON 字符串
    std::string full_json;
    for (const auto& chunk : merge_chunks) {
        full_json += chunk;
    }

    /*
    emscripten_log(EM_LOG_DEBUG, "JSON length: %zu bytes", full_json.size());

    // 只打印开头和结尾部分（调试用）
    if (full_json.size() > 1024) {
        emscripten_log(EM_LOG_DEBUG, "JSON start: %s", full_json.substr(0, 512).c_str());
        emscripten_log(EM_LOG_DEBUG, "JSON end: %s", full_json.substr(full_json.size()-512).c_str());
    } else {
        emscripten_log(EM_LOG_DEBUG, "full_json = %s", full_json.c_str());
    }
    */

    // 执行合并逻辑
    json mergeForm = json::parse(full_json);
    json result = merge_json(mergeForm);

    std::string result_str = result.dump(3);
    char* output = static_cast<char*>(malloc(result_str.size() + 1));
    strcpy(output, result_str.c_str());
    return output;
}