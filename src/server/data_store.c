#define _POSIX_C_SOURCE 200809L

#include "isabela/server.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *k_default_fiware_url =
    "http://socialiteorion2.dei.uc.pt:9014/v2/entities?options=keyValues&type=student&"
    "attrs=activity,calls_duration,calls_made,calls_missed,calls_received,department,location,sms_received,sms_sent&limit=1000";

struct payload_buffer {
    char *data;
    size_t len;
};

static void payload_init(struct payload_buffer *buf) {
    buf->data = NULL;
    buf->len = 0;
}

static bool payload_append(struct payload_buffer *buf, const char *chunk, size_t chunk_len) {
    char *new_data = realloc(buf->data, buf->len + chunk_len + 1);
    if (!new_data) {
        return false;
    }
    buf->data = new_data;
    memcpy(buf->data + buf->len, chunk, chunk_len);
    buf->len += chunk_len;
    buf->data[buf->len] = '\0';
    return true;
}

static void payload_free(struct payload_buffer *buf) {
    free(buf->data);
    buf->data = NULL;
    buf->len = 0;
}

static bool load_from_fixture(struct payload_buffer *buf, const char *path, char *err, size_t err_len) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        if (err && err_len > 0) {
            snprintf(err, err_len, "Unable to open fixture %s: %s", path, strerror(errno));
        }
        return false;
    }

    payload_init(buf);
    char tmp[1024];
    while (!feof(fp)) {
        size_t read_bytes = fread(tmp, 1, sizeof(tmp), fp);
        if (read_bytes == 0) {
            break;
        }
        if (!payload_append(buf, tmp, read_bytes)) {
            fclose(fp);
            return false;
        }
    }
    fclose(fp);
    return true;
}

static bool load_from_remote(struct payload_buffer *buf, char *err, size_t err_len) {
    char command[512];
    snprintf(command, sizeof(command), "curl -s '%s'", k_default_fiware_url);
    FILE *pipe = popen(command, "r");
    if (!pipe) {
        if (err && err_len > 0) {
            snprintf(err, err_len, "Failed to execute curl command");
        }
        return false;
    }
    payload_init(buf);
    char tmp[1024];
    while (!feof(pipe)) {
        size_t read_bytes = fread(tmp, 1, sizeof(tmp), pipe);
        if (read_bytes == 0) {
            break;
        }
        if (!payload_append(buf, tmp, read_bytes)) {
            pclose(pipe);
            return false;
        }
    }
    pclose(pipe);
    return true;
}

static const char *skip_ws(const char *p) {
    while (*p && isspace((unsigned char)*p)) {
        ++p;
    }
    return p;
}

static bool parse_string(const char **p, char *out, size_t out_len) {
    const char *s = *p;
    if (*s != '"') {
        return false;
    }
    ++s;
    size_t idx = 0;
    while (*s && *s != '"') {
        if (*s == '\\' && s[1]) {
            ++s;
        }
        if (idx + 1 < out_len) {
            out[idx++] = *s;
        }
        ++s;
    }
    if (*s != '"') {
        return false;
    }
    out[idx] = '\0';
    *p = s + 1;
    return true;
}

static bool parse_number_token(const char **p, int *value) {
    const char *s = *p;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (end == s) {
        return false;
    }
    *value = (int)v;
    *p = end;
    return true;
}

static bool parse_int_value(const char **p, int *value) {
    const char *s = skip_ws(*p);
    if (*s == '"') {
        char tmp[64];
        if (!parse_string(&s, tmp, sizeof(tmp))) {
            return false;
        }
        *value = atoi(tmp);
        *p = s;
        return true;
    }
    if (!parse_number_token(&s, value)) {
        return false;
    }
    *p = s;
    return true;
}

static bool parse_string_value(const char **p, char *out, size_t out_len) {
    const char *s = skip_ws(*p);
    if (*s == '"') {
        if (!parse_string(&s, out, out_len)) {
            return false;
        }
        *p = s;
        return true;
    }
    int value = 0;
    if (!parse_number_token(&s, &value)) {
        return false;
    }
    snprintf(out, out_len, "%d", value);
    *p = s;
    return true;
}

static bool parse_clients(const char *payload, IsabelaServerContext *ctx, char *err, size_t err_len) {
    const char *p = skip_ws(payload);
    if (*p != '[') {
        if (err && err_len > 0) {
            snprintf(err, err_len, "Payload is not an array");
        }
        return false;
    }
    ++p;
    size_t idx = 0;
    while (1) {
        p = skip_ws(p);
        if (*p == ']') {
            ++p;
            break;
        }
        if (*p != '{') {
            return false;
        }
        ++p;
        IsabelaClientProfile profile = {0};
        profile.id = (int)(idx + 1);
        while (1) {
            p = skip_ws(p);
            if (*p == '}') {
                ++p;
                break;
            }
            char key[64];
            if (!parse_string(&p, key, sizeof(key))) {
                return false;
            }
            p = skip_ws(p);
            if (*p != ':') {
                return false;
            }
            ++p;
            if (strcmp(key, "activity") == 0) {
                parse_string_value(&p, profile.activity, sizeof(profile.activity));
            } else if (strcmp(key, "location") == 0) {
                parse_string_value(&p, profile.location, sizeof(profile.location));
            } else if (strcmp(key, "department") == 0) {
                parse_string_value(&p, profile.department, sizeof(profile.department));
            } else if (strcmp(key, "calls_duration") == 0) {
                parse_int_value(&p, &profile.calls_duration);
            } else if (strcmp(key, "calls_made") == 0) {
                parse_int_value(&p, &profile.calls_made);
            } else if (strcmp(key, "calls_missed") == 0) {
                parse_int_value(&p, &profile.calls_missed);
            } else if (strcmp(key, "calls_received") == 0) {
                parse_int_value(&p, &profile.calls_received);
            } else if (strcmp(key, "sms_received") == 0) {
                parse_int_value(&p, &profile.sms_received);
            } else if (strcmp(key, "sms_sent") == 0) {
                parse_int_value(&p, &profile.sms_sent);
            } else {
                char tmp[128];
                parse_string_value(&p, tmp, sizeof(tmp));
            }
            p = skip_ws(p);
            if (*p == ',') {
                ++p;
                continue;
            }
            if (*p == '}') {
                ++p;
                break;
            }
        }
        if (idx < ISABELA_MAX_CLIENTS) {
            ctx->clients[idx++] = profile;
        }
        p = skip_ws(p);
        if (*p == ',') {
            ++p;
            continue;
        }
        if (*p == ']') {
            ++p;
            break;
        }
    }
    ctx->client_count = idx;
    return true;
}

bool isabela_refresh_registry(IsabelaServerContext *ctx, char *err_buf, size_t err_buf_len) {
    if (!ctx) {
        return false;
    }

    struct payload_buffer payload;
    payload_init(&payload);
    const char *fixture = getenv("ISABELA_DATA_FILE");
    bool ok = false;
    if (fixture && *fixture) {
        ok = load_from_fixture(&payload, fixture, err_buf, err_buf_len);
    } else {
        ok = load_from_remote(&payload, err_buf, err_buf_len);
    }

    if (!ok) {
        payload_free(&payload);
        return false;
    }

    bool parsed = parse_clients(payload.data, ctx, err_buf, err_buf_len);
    payload_free(&payload);
    if (!parsed) {
        return false;
    }

    isabela_recalculate_averages(ctx);
    return true;
}
